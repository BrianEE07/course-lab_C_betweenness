#include "top.hpp"
#include <list>
#include <queue>
#include <stack>
#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#endif
// #include <hls_vector.h>

#define MaxNumVert 3600
#define MaxNumEdge 42000

extern "C" void dut(
        unsigned numVert, 
        unsigned numEdge, 
        unsigned *offset, 
        unsigned *column,
        float *btwn, 
        unsigned *tmp0, 
        unsigned *tmp1, 
        unsigned *tmp2,
        unsigned *tmp3) {
  // clang-format off

    const unsigned MEMSIZE=INTERFACE_MEMSIZE;
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 256 bundle = gmem0 port = offset depth = MEMSIZE

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 256 bundle = gmem1 port = column depth = MEMSIZE

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 256 max_read_burst_length = 256 bundle = gmem3 port = btwn depth = MEMSIZE

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 256 bundle = gmem6 port = tmp0 depth = MEMSIZE

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 256 bundle = gmem7 port = tmp1 depth = MEMSIZE

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 256 bundle = gmem8 port = tmp2 depth = MEMSIZE

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 256 bundle = gmem9 port = tmp3 depth = MEMSIZE

// clang-format on
// #ifndef __SYNTHESIS__
  // for (int i = 0; i < numVert; i++) {
  //   btwn[i] = 0;
  // }
  // for (int i = 0; i < numVert; i++) {
  //   std::stack<unsigned> s;
  //   std::vector<std::list<unsigned>> p(numVert);
  //   std::vector<float> sigma(numVert);
  //   std::vector<int> dist(numVert);
  //   std::queue<unsigned> q;
  //   unsigned source = i;

  //   for (int j = 0; j < numVert; j++) {
  //     sigma[j] = 0;
  //     dist[j] = -1;
  //   }
  //   sigma[source] = 1;
  //   dist[source] = 0;

  //   q.push(source);
  //   while (!q.empty()) {
  //     unsigned v = q.front();
  //     s.push(v);
  //     for (int j = offset[v]; j < offset[v + 1]; j++) {
  //       unsigned w = column[j];
  //       if (dist[w] < 0) {
  //         q.push(w);
  //         dist[w] = dist[v] + 1;
  //       }
  //       if (dist[w] == dist[v] + 1) {
  //         sigma[w] = sigma[w] + sigma[v];
  //         p[w].push_back(v);
  //       }
  //     }
  //     q.pop();
  //   }

  //   std::vector<float> delta(numVert);
  //   for (int j = 0; j < numVert; j++) {
  //     delta[j] = 0;
  //   }
  //   while (!s.empty()) {
  //     unsigned w = s.top();
  //     if (source != w) {
  //       btwn[w] = btwn[w] + delta[w];
  //     }
  //     for (std::list<unsigned>::iterator it = p[w].begin(); it != p[w].end(); it++) {
  //       unsigned v = *it;
  //       delta[v] = delta[v] + (sigma[v] / sigma[w]) * (1 + delta[w]);
  //     }
  //     s.pop();
  //   }
  // }
  // buffer offset & column
  unsigned off[MaxNumVert];
  unsigned col[MaxNumEdge];
  float btwn_buff[MaxNumVert];
  BUFF_REGION: {
#pragma HLS loop_merge force
    LOOP_BUFF_OFFSET:
    for (int i = 0;i < MaxNumVert;++i){
#pragma HLS PIPELINE II=1 rewind
      if (i < numVert+1) {
        off[i] = offset[i];
      }
      else break;
    }
    LOOP_BUFF_COLUMN:
    for (int i = 0;i < MaxNumEdge;++i){
#pragma HLS PIPELINE II=1 rewind
      if (i < numEdge) {
        col[i] = column[i];
      }
      else break;
    }
    // initialization
    LOOP_INIT:
    for (int i = 0;i < MaxNumVert;++i) {
#pragma HLS PIPELINE II=1 rewind
      if (i < numVert){
        btwn_buff[i] = 0;
      }
      else break;
    }
  }
  
  // main loop
  LOOP_MAIN:
  for (int i = 0;i < MaxNumVert;++i) {
    if (i < numVert){
      // initialization
      unsigned s[MaxNumVert], s_top;
      unsigned p[MaxNumVert][20], p_tail[MaxNumVert];
      float    sigma[MaxNumVert];
      int      dist[MaxNumVert];
      unsigned q[MaxNumVert], q_head, q_tail;
      float delta[MaxNumVert];   
#pragma HLS ARRAY_PARTITION variable=dist type=cyclic factor=8 dim=1
#pragma HLS ARRAY_PARTITION variable=sigma type=cyclic factor=8 dim=1
#pragma HLS ARRAY_PARTITION variable=p_tail type=cyclic factor=8 dim=1
#pragma HLS ARRAY_PARTITION variable=delta type=cyclic factor=8 dim=1
      unsigned source = i;
      LOOP_MAIN_INIT:
      for (int j = 0;j < MaxNumVert;++j){
#pragma HLS PIPELINE II=1 rewind
#pragma HLS UNROLL factor=16
        if (j < numVert){
          sigma[j] = 0;
          dist[j] = -1;
          p_tail[j] = 0;
          delta[j] = 0;
        }
        else break;
      }
      sigma[source] = 1;
      dist[source] = 0;
      s_top = 0;
      q_head = 0;
      q_tail = 0;
      q[q_tail++] = source;
      // graph traversal (BFS) for shortest path
      LOOP_MAIN_BFS:
      for (int j = 0;j < MaxNumVert;++j){
        if (q_tail != q_head) {
          unsigned v = q[q_head++];
          s[s_top++] = v;
          // for (int k = offset[v];k < offset[v + 1];++k) {
          for (int k = off[v];k < off[v + 1];++k) {
#pragma HLS PIPELINE II=1 rewind
            // unsigned w = column[k];
            unsigned w = col[k];
            if (dist[w] < 0) {
              dist[w] = dist[v] + 1;
              q[q_tail++] = w;
            }
            if (dist[w] == dist[v] + 1) {
              sigma[w] += sigma[v];
              p[w][p_tail[w]++] = v;
            }
          }
        }
        else break;
      }
      // dependency accumulation
      LOOP_MAIN_ACC:
      for (int j = 0;j < MaxNumVert;++j){
        if (s_top != 0) {
          unsigned w = s[--s_top];
          if (source != w) {
            btwn_buff[w] += delta[w];
          }
          for (int k = 0;k < p_tail[w];++k) {
#pragma HLS PIPELINE II=1 rewind
            unsigned v = p[w][k];
            delta[v] += (sigma[v] / sigma[w]) * (1 + delta[w]);
          }
        }
        else break;
      }
    }
    else break;
  }
  // write back
  LOOP_WRITE:
  for (int i = 0;i < MaxNumVert;++i) {
#pragma HLS PIPELINE II=1 rewind
    if (i < numVert){
      btwn[i] = btwn_buff[i];
    }
    else break;
  }

// #else
  
// #endif
}