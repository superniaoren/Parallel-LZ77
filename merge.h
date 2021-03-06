// This code is part of the Problem Based Benchmark Suite (PBBS), 15418 course
// project: Implementation and Comparison of Parallel LZ77 and LZ78 Algorithms
// and DCC 2013 paper: Practical Parallel Lempel-Ziv Factorization
// Copyright (c) 2012 Julian Shun, Fuyao Zhao and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef _MERGE_H
#define _MERGE_H

#include "parallel.h"

#define _MERGE_BSIZE 8192

template <class ET, class F>
intT binSearchOld(ET *S, intT n, ET v) {
  if (n == 0) return 0;
  else {
    intT mid = n / 2;
    // if (f(v,S[mid])) return binSearch(S, mid, v, f);
    if (v < S[mid]) return binSearch(S, mid, v);
    else return mid + 1 + binSearch(S + mid + 1, (n - mid) - 1, v);
  }
}

template <class ET, class F>
intT binSearch(ET *S, intT n, ET v, F f) {
  ET *T = S;
  while (n > 0) {
    intT mid = n / 2;
    if (f(v, T[mid])) n = mid;
    else {
      n = (n - mid) - 1;
      T = T + mid + 1;
    }
  }
  return T - S;
}

#ifdef OPENMP

#define SPLIT_BSIZE (_MERGE_BSIZE >> 2)

template <class ET, class F>
void merge(ET *S1, intT l1, ET *S2, intT l2, ET *R, F f) {
  intT lr = l1 + l2;
  if (lr > _MERGE_BSIZE) {
    if (l2 > l1)  merge(S2, l2, S1, l1, R, f);
    else {
      intT ll1 = (l1 + SPLIT_BSIZE - 1) / SPLIT_BSIZE;

      intT *pos1 = new intT[ll1 + 1]; // pos1[i] means the position of s1[i*Splite] in s2
      *(pos1++) = 0;

      parallel_for (intT i = 0; i < ll1; i++) {
        pos1[i] = binSearch(S2, l2, S1[std::min((i + 1) * SPLIT_BSIZE, l1) - 1], f);
      }

      pos1[ll1 - 1] = l2;

      {
        parallel_for (intT i = 0; i < ll1; i++) {
          intT start1 = i * SPLIT_BSIZE;
          intT n1 = std::min(SPLIT_BSIZE, l1 - start1);
          intT start2 = pos1[i - 1];
          intT n2 = pos1[i] - start2;

          std::merge(S1 + start1, S1 + start1 + n1, S2 + start2, S2 + start2 + n2, R + start1 + start2, f);
        }
      }

      delete --pos1;
    }
  } else {
    std::merge(S1, S1 + l1, S2, S2 + l2, R + l1 + l2, f);
  }
}

#else
template <class ET, class F>
void merge(ET *S1, intT l1, ET *S2, intT l2, ET *R, F f) {
  intT lr = l1 + l2; //cout<<"merge "<<lr<<endl;
  if (lr > _MERGE_BSIZE) {
    if (l2 > l1)  merge(S2, l2, S1, l1, R, f);
    else {
      intT m1 = l1 / 2;
      intT m2 = binSearch(S2, l2, S1[m1], f);
      parallel_spawn
      merge(S1, m1, S2, m2, R, f);
      merge(S1 + m1, l1 - m1, S2 + m2, l2 - m2, R + m1 + m2, f);
      parallel_sync;
    }
  } else {
    ET *pR = R;
    ET *pS1 = S1;
    ET *pS2 = S2;
    ET *eS1 = S1 + l1;
    ET *eS2 = S2 + l2;
    while (true) {
      *pR++ = f(*pS2, *pS1) ? *pS2++ : *pS1++;
      if (pS1 == eS1) {
        //while(pS2 != eS2) *pR++ = *pS2++;
        std::copy(pS2, eS2, pR);
        break;
      }
      if (pS2 == eS2) {
        //while(pS1 != eS1) *pR++ = *pS1++;
        std::copy(pS1, eS1, pR);
        break;
      }
    }
  }
}
#endif

#endif

