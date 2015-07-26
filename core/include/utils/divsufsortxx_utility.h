/*
 * divsufsortxx_utility.h
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// r3
#ifndef _DIVSUFSORTXX_UTILITY_H_
#define _DIVSUFSORTXX_UTILITY_H_

#include <iostream>
#include <assert.h>

namespace divsufsortxx {

/* Checks the suffix array SA of the string T. */
template<typename StringIterator_type, typename SAIterator_type,
    typename alphabetsize_type>
int check(const StringIterator_type T, const StringIterator_type T_last,
          SAIterator_type SA, SAIterator_type SA_last,
          alphabetsize_type alphabetsize, int verbose) {
  typedef typename std::iterator_traits<SAIterator_type>::value_type pos_type;
  typedef typename std::iterator_traits<StringIterator_type>::value_type value_type;

  pos_type *C;
  SAIterator_type i;
  pos_type j, p, q, t, n;
  value_type c;

  if (1 <= verbose) {
    std::cerr << "sufchecker: ";
  }

  /* Check arguments. */
  n = T_last - T;
  if (n == 0) {
    if (1 <= verbose) {
      std::cerr << "Done." << std::endl;
    }
    return 0;
  }
  if ((n < 0) || ((SA_last - SA) != n)) {
    if (1 <= verbose) {
      std::cerr << "Invalid arguments." << std::endl;
    }
    return -1;
  }

  /* ranges. */
  for (i = SA; i < SA_last; ++i) {
    if ((*i < 0) || (n <= *i)) {
      if (1 <= verbose) {
        std::cerr << "Out of the range [0, " << (n - 1) << "]." << std::endl;
        std::cerr << "SA[" << i - SA << "]=" << *i << std::endl;
      }
      return -2;
    }
  }

  /* first characters. */
  for (i = SA + 1; i < SA_last; ++i) {
    if (T[*(i - 1)] > T[*i]) {
      if (1 <= verbose) {
        std::cerr << "Suffixes in wrong order." << std::endl;
        std::cerr << "  T[SA[" << i - SA << "] = " << *i << "] = " << T[*i]
            << " > ";
        std::cerr << "T[SA[" << i + 1 - SA << "] = " << *(i + 1) << "] = "
            << T[*(i + 1)] << "," << std::endl;
      }
      return -3;
    }
  }

  /* suffixes. */
  C = new pos_type[alphabetsize];
  for (j = 0; j < alphabetsize; ++j) {
    C[j] = 0;
  }
  for (j = 0; j < n; ++j) {
    ++C[T[j]];
  }
  for (j = 0, p = 0; j < alphabetsize; ++j) {
    t = C[j];
    C[j] = p;
    p += t;
  }
  q = C[T[n - 1]];
  C[T[n - 1]] += 1;

  for (i = SA; i < SA_last; ++i) {
    p = *i;
    if (0 < p) {
      c = T[--p];
      t = C[c];
    } else {
      c = T[p = n - 1];
      t = q;
    }
    if (p != SA[t]) {
      if (1 <= verbose) {
        std::cerr << "Suffix in wrong position." << std::endl;
        if (0 <= t) {
          std::cerr << "  SA[" << t << "] = " << SA[t] << " or" << std::endl;
        }
        std::cerr << "  SA[" << i - SA << "] = " << *i << std::endl;
      }
      delete[] C;
      return -4;
    }
    if (t != q) {
      C[c] += 1;
      if ((n <= C[c]) || (T[SA[C[c]]] != c)) {
        C[c] = -1;
      }
    }
  }

  if (1 <= verbose) {
    std::cerr << "Done." << std::endl;
  }

  delete[] C;
  return 0;
}

template<typename StringIterator1_type, typename StringIterator2_type,
    typename pos_type>
int compare(StringIterator1_type T, StringIterator1_type T_last,
            StringIterator2_type P, StringIterator2_type P_last,
            pos_type &match) {
  StringIterator1_type Ti;
  StringIterator2_type Pi;

  for (Ti = T + match, Pi = P + match;
      (Ti < T_last) && (Pi < P_last) && (*Ti == *Pi); ++Ti, ++Pi) {
  }
  match = Pi - P;

  return
      ((Ti < T_last) && (Pi < P_last)) ? (*Pi < *Ti) * 2 - 1 : -(Pi != P_last);
}

/* Search for the pattern P in the string T. */
template<typename StringIterator1_type, typename StringIterator2_type,
    typename SAIterator_type>
typename std::iterator_traits<SAIterator_type>::value_type search(
    StringIterator1_type T, StringIterator1_type T_last, StringIterator2_type P,
    StringIterator2_type P_last, SAIterator_type SA, SAIterator_type SA_last,
    typename std::iterator_traits<SAIterator_type>::value_type &outidx) {
  typedef typename std::iterator_traits<SAIterator_type>::value_type pos_type;
  pos_type size, lsize, rsize, half;
  pos_type match, lmatch, rmatch;
  pos_type llmatch, lrmatch, rlmatch, rrmatch;
  pos_type i, j, k;
  int r;

  outidx = -1;
  if ((T_last < T) || (P_last < P) || (SA_last < SA)) {
    return -1;
  }
  if ((T == T_last) || (SA == SA_last)) {
    return 0;
  }
  if (P == P_last) {
    outidx = 0;
    return SA_last - SA;
  }

  for (i = j = k = 0, lmatch = rmatch = 0, size = SA_last - SA, half = size >> 1;
      0 < size; size = half, half >>= 1) {
    match = std::min(lmatch, rmatch);
    r = compare(T + SA[i + half], T_last, P, P_last, match);
    if (r < 0) {
      i += half + 1;
      half -= (size & 1) ^ 1;
      lmatch = match;
    } else if (r > 0) {
      rmatch = match;
    } else {
      lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

      /* left part */
      for (llmatch = lmatch, lrmatch = match, half = lsize >> 1; 0 < lsize;
          lsize = half, half >>= 1) {
        lmatch = std::min(llmatch, lrmatch);
        r = compare(T + SA[j + half], T_last, P, P_last, lmatch);
        if (r < 0) {
          j += half + 1;
          half -= (lsize & 1) ^ 1;
          llmatch = lmatch;
        } else {
          lrmatch = lmatch;
        }
      }

      /* right part */
      for (rlmatch = match, rrmatch = rmatch, half = rsize >> 1; 0 < rsize;
          rsize = half, half >>= 1) {
        rmatch = std::min(rlmatch, rrmatch);
        r = compare(T + SA[k + half], T_last, P, P_last, rmatch);
        if (r <= 0) {
          k += half + 1;
          half -= (rsize & 1) ^ 1;
          rlmatch = rmatch;
        } else {
          rrmatch = rmatch;
        }
      }

      break;
    }
  }

  outidx = (0 < (k - j)) ? j : i;
  return k - j;
}

/**
 * Reconstructs the original string of a given BWTed string.
 * PSI version
 *
 * @param T An input string iterator.
 * @param T_last An input string iterator.
 * @param U An output string iterator.
 * @param U_last An output string iterator.
 * @param pidx The primary index.
 * @param alphabetsize
 * @return 0 if no error occurred, -1 or -2 otherwise.
 *
 * @space
 *   if sizeof(pos_type) == 4 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     5n + 8*256 bytes
 *   elif sizeof(pos_type) == 8 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     9n + 16*256 bytes
 */
template<typename StringIterator1_type, typename StringIterator2_type,
    typename pos_type, typename alphabetsize_type>
int reverseBWT_psi(const StringIterator1_type T,
                   const StringIterator1_type T_last, StringIterator2_type U,
                   StringIterator2_type U_last, pos_type pidx,
                   alphabetsize_type alphabetsize = 256) {
  typedef typename std::iterator_traits<StringIterator2_type>::value_type outvalue_type;
  typedef typename std::iterator_traits<StringIterator1_type>::difference_type difference1_type;
  typedef typename std::iterator_traits<StringIterator2_type>::difference_type difference2_type;
  pos_type *alphabets = NULL, *counts = NULL, *psi = NULL;
  StringIterator1_type i;
  StringIterator2_type j;
  difference1_type size;
  difference2_type Usize;
  pos_type p, t, sum;
  alphabetsize_type c, d;
  int err = 0;

  size = std::distance(T, T_last);
  Usize = std::distance(U, U_last);
  if ((size < 0) || (pidx < 0) || (size < pidx) || (alphabetsize <= 0)
      || (Usize < 0)) {
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  if (pidx == 0) {
    return -1;
  }
  if (Usize == 0) {
    return 0;
  }

  try {
    alphabets = new pos_type[alphabetsize];
    if (alphabets == NULL) {
      throw;
    }
    counts = new pos_type[alphabetsize];
    if (counts == NULL) {
      throw;
    }

    for (c = 0; c < alphabetsize; ++c) {
      counts[c] = 0;
    }
    for (i = T; i != T_last; ++i) {
      ++counts[*i];
    }
    for (c = 0, d = 0, sum = 1; c < alphabetsize; ++c) {
      t = counts[c];
      if (0 < t) {
        counts[c] = sum;
        alphabets[d++] = c;
        sum += t;
      }
    }

    psi = new pos_type[size + 1];
    if (psi == NULL) {
      throw;
    }

    psi[0] = pidx;
    for (i = T, p = 0; p < pidx; ++i, ++p) {
      psi[counts[*i]++] = p;
    }
    for (p += 1; i != T_last; ++i, ++p) {
      psi[counts[*i]++] = p;
    }
    for (c = 0; c < d; ++c) {
      counts[c] = counts[alphabets[c]];
    }
    for (j = U, t = 0; j != U_last; ++j) {
      *j = outvalue_type(alphabets[std::upper_bound(counts, counts + d, t =
                                                        psi[t]) - counts]);
    }
  } catch (...) {
    err = -2;
  }

  delete[] psi;
  delete[] counts;
  delete[] alphabets;

  return err;
}

template<typename StringIterator_type, typename pos_type,
    typename alphabetsize_type>
int reverseBWT_psi(StringIterator_type T, StringIterator_type T_last,
                   pos_type pidx, alphabetsize_type alphabetsize = 256) {
  return reverseBWT_psi(T, T_last, T, T_last, pidx, alphabetsize);
}

/**
 * Reconstructs the original string of a given BWTed string.
 * LF version
 *
 * @param T An input string iterator.
 * @param T_last An input string iterator.
 * @param U An output string iterator.
 * @param U_last An output string iterator.
 * @param pidx The primary index.
 * @param alphabetsize
 * @return 0 if no error occurred, -1 or -2 otherwise.
 *
 * @space
 *   if sizeof(pos_type) == 4 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     6n + 8*256 bytes
 *   elif sizeof(pos_type) == 8 and sizeof(value_type) == 1 and alphabetsize == 256:
 *    10n + 16*256 bytes
 */
template<typename StringIterator1_type, typename StringIterator2_type,
    typename pos_type, typename alphabetsize_type>
int reverseBWT_lf(const StringIterator1_type T,
                  const StringIterator1_type T_last, StringIterator2_type U,
                  StringIterator2_type U_last, pos_type pidx,
                  alphabetsize_type alphabetsize = 256) {
  typedef typename std::iterator_traits<StringIterator1_type>::difference_type difference1_type;
  typedef typename std::iterator_traits<StringIterator2_type>::difference_type difference2_type;
  pos_type *counts = NULL, *lf = NULL;
  StringIterator1_type i;
  StringIterator2_type j;
  difference1_type size;
  difference2_type Usize;
  pos_type p, t, sum;
  alphabetsize_type c;
  int err = 0;

  size = std::distance(T, T_last);
  Usize = std::distance(U, U_last);
  if ((size < 0) || (pidx < 0) || (size < pidx) || (alphabetsize <= 0)
      || (Usize < 0)) {
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  if (pidx == 0) {
    return -1;
  }
  if (Usize == 0) {
    return 0;
  }

  try {
    counts = new pos_type[alphabetsize];
    if (counts == NULL) {
      throw;
    }
    lf = new pos_type[size];
    if (lf == NULL) {
      throw;
    }

    for (c = 0; c < alphabetsize; ++c) {
      counts[c] = 0;
    }
    for (i = T, p = 0; i != T_last; ++i, ++p) {
      lf[p] = counts[*i]++;
    }
    for (c = 0, sum = 1; c < alphabetsize; ++c) {
      t = counts[c];
      counts[c] = sum;
      sum += t;
    }
    for (j = U_last, t = 0; j != U;) {
      t = lf[t] + counts[*--j = T[t]];
      if (pidx <= t) {
        --t;
      }
    }
  } catch (...) {
    err = -2;
  }

  delete[] lf;
  delete[] counts;

  return err;
}

template<typename StringIterator_type, typename pos_type,
    typename alphabetsize_type>
int reverseBWT_lf(StringIterator_type T, StringIterator_type T_last,
                  pos_type pidx, alphabetsize_type alphabetsize = 256) {
  typedef typename std::iterator_traits<StringIterator_type>::value_type value_type;
  typedef typename std::iterator_traits<StringIterator_type>::difference_type difference_type;
  value_type *string = NULL;
  pos_type *counts = NULL, *lf = NULL;
  StringIterator_type i;
  difference_type size;
  pos_type p, t, sum;
  alphabetsize_type c;
  int err = 0;

  size = std::distance(T, T_last);
  if ((size < 0) || (pidx < 0) || (size < pidx) || (alphabetsize <= 0)) {
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  if (pidx == 0) {
    return -1;
  }

  try {
    string = new value_type[size + 1];
    if (string == NULL) {
      throw;
    }
    counts = new pos_type[alphabetsize];
    if (counts == NULL) {
      throw;
    }
    lf = new pos_type[size + 1];
    if (lf == NULL) {
      throw;
    }

    for (c = 0; c < alphabetsize; ++c) {
      counts[c] = 0;
    }
    for (i = T, p = 0; p < pidx; ++i, ++p) {
      lf[p] = counts[string[p] = *i]++;
    }
    lf[p] = 0;
    for (p += 1; i != T_last; ++i, ++p) {
      lf[p] = counts[string[p] = *i]++;
    }
    for (c = 0, sum = 1; c < alphabetsize; ++c) {
      t = counts[c];
      counts[c] = sum;
      sum += t;
    }
    for (i = T_last, t = 0; i != T;) {
      t = lf[t] + counts[*--i = string[t]];
    }
  } catch (...) {
    err = -2;
  }

  delete[] lf;
  delete[] string;
  delete[] counts;

  return err;
}

/**
 * Reconstructs the original string of a given BWTed string.
 * MergedTLF version
 *
 * @param T An input string iterator.
 * @param T_last An input string iterator.
 * @param U An output string iterator.
 * @param U_last An output string iterator.
 * @param pidx The primary index.
 * @param alphabetsize
 * @return 0 if no error occurred, -1 or -2 otherwise.
 *
 * @space
 *   if sizeof(pos_type) == 4 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     5n + 4*256 bytes
 *   elif sizeof(pos_type) == 8 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     9n + 8*256 bytes
 */
template<typename StringIterator1_type, typename StringIterator2_type,
    typename pos_type, typename alphabetsize_type>
int reverseBWT_tlf(const StringIterator1_type T,
                   const StringIterator1_type T_last, StringIterator2_type U,
                   StringIterator2_type U_last, pos_type pidx,
                   alphabetsize_type alphabetsize = 256) {
  typedef typename std::iterator_traits<StringIterator1_type>::value_type value_type;
  typedef typename std::iterator_traits<StringIterator2_type>::value_type outvalue_type;
  typedef typename std::iterator_traits<StringIterator1_type>::difference_type difference1_type;
  typedef typename std::iterator_traits<StringIterator2_type>::difference_type difference2_type;
  pos_type *counts = NULL, *lf = NULL;
  StringIterator1_type i;
  StringIterator2_type j;
  pos_type p, t, sum;
  difference1_type size;
  difference2_type Usize;
  alphabetsize_type c;
  value_type d;
  outvalue_type e;
  int err = 0;

  size = std::distance(T, T_last);
  Usize = std::distance(U, U_last);
  if ((size < 0) || (pidx < 0) || (size < pidx) || (alphabetsize <= 0)
      || (Usize < 0)) {
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  if (pidx == 0) {
    return -1;
  }
  if (Usize == 0) {
    return 0;
  }

  p = sizeof(pos_type) * 8 - (pos_type(-1) < 0) - 1;
  t = (pos_type(1) << p) - 1 + (pos_type(1) << p);
  t /= alphabetsize;
  if (t < size) {
    return reverseBWT_psi(T, T_last, U, U_last, pidx, alphabetsize);
  }

  if (alphabetsize == 256) {
    try {
      counts = new pos_type[256];
      if (counts == NULL) {
        throw;
      }
      lf = new pos_type[size + 1];
      if (lf == NULL) {
        throw;
      }

      for (c = 0; c < 256; ++c) {
        counts[c] = 0;
      }
      for (i = T, p = 0; p < pidx; ++i, ++p) {
        d = *i;
        lf[p] = (counts[d]++ << 8) + d;
      }
      lf[p] = 0;
      for (p += 1; i != T_last; ++i, ++p) {
        d = *i;
        lf[p] = (counts[d]++ << 8) + d;
      }

      for (c = 0, sum = 1; c < 256; ++c) {
        t = counts[c];
        counts[c] = sum;
        sum += t;
      }

      for (j = U_last, t = 0; j != U;) {
        --j;
        t = lf[t];
        e = outvalue_type(t & 255);
        t = t >> 8;
        *j = e;
        t = t + counts[e];
      }
    } catch (...) {
      err = -1;
    }

    delete[] lf;
    delete[] counts;
  } else {
    try {
      counts = new pos_type[alphabetsize];
      if (counts == NULL) {
        throw;
      }
      lf = new pos_type[size + 1];
      if (lf == NULL) {
        throw;
      }

      for (c = 0; c < alphabetsize; ++c) {
        counts[c] = 0;
      }
      for (i = T, p = 0; p < pidx; ++i, ++p) {
        d = *i;
        lf[p] = counts[d]++ * alphabetsize + d;
      }
      lf[p] = 0;
      for (p += 1; i != T_last; ++i, ++p) {
        d = *i;
        lf[p] = counts[d]++ * alphabetsize + d;
      }

      for (c = 0, sum = 1; c < alphabetsize; ++c) {
        t = counts[c];
        counts[c] = sum;
        sum += t;
      }

      for (j = U_last, t = 0; j != U;) {
        --j;
        t = lf[t];
        e = outvalue_type(t % alphabetsize);
        t = t / alphabetsize;
        *j = e;
        t = t + counts[e];
      }
    } catch (...) {
      err = -1;
    }

    delete[] lf;
    delete[] counts;
  }

  return err;
}

template<typename StringIterator_type, typename pos_type,
    typename alphabetsize_type>
int reverseBWT_tlf(StringIterator_type T, StringIterator_type T_last,
                   pos_type pidx, alphabetsize_type alphabetsize = 256) {
  return reverseBWT_tlf(T, T_last, T, T_last, pidx, alphabetsize);
}

/**
 * Reconstructs the original string of a given BWTed string.
 * Compact LF version
 *
 * @param T An input string iterator.
 * @param T_last An input string iterator.
 * @param U An output string iterator.
 * @param U_last An output string iterator.
 * @param pidx The primary index.
 * @param alphabetsize
 * @return 0 if no error occurred, -1 or -2 otherwise.
 *
 * @space
 *   if sizeof(pos_type) == 4 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     3.625n + 4*256 bytes
 *   elif sizeof(pos_type) == 8 and sizeof(value_type) == 1 and alphabetsize == 256:
 *     3.75n + 8*256 bytes
 */
template<typename StringIterator1_type, typename StringIterator2_type,
    typename pos_type, typename alphabetsize_type>
int reverseBWT_clf(const StringIterator1_type T,
                   const StringIterator1_type T_last, StringIterator2_type U,
                   StringIterator2_type U_last, pos_type pidx,
                   alphabetsize_type alphabetsize = 256) {
  typedef typename std::iterator_traits<StringIterator1_type>::value_type value_type;
  typedef typename std::iterator_traits<StringIterator2_type>::value_type outvalue_type;
  typedef typename std::iterator_traits<StringIterator1_type>::difference_type difference1_type;
  typedef typename std::iterator_traits<StringIterator2_type>::difference_type difference2_type;
  pos_type *counts = NULL, *tempcounts = NULL, *lf_upper = NULL;
  unsigned char *lf_lower = NULL;
  unsigned char *lf_middle = NULL;
  StringIterator1_type i, j, k;
  StringIterator2_type l;
  difference1_type size;
  difference2_type Usize;
  pos_type p, q, r, t, sum;
  unsigned int temp;
  alphabetsize_type c;
  value_type d;
  outvalue_type e;
  int err = 0;

  size = std::distance(T, T_last);
  Usize = std::distance(U, U_last);
  if ((size < 0) || (pidx < 0) || (size < pidx) || (alphabetsize <= 0)
      || (Usize < 0)) {
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  if (pidx == 0) {
    return -1;
  }
  if (Usize == 0) {
    return 0;
  }

  try {
    counts = new pos_type[alphabetsize];
    if (counts == NULL) {
      throw;
    }
    tempcounts = new pos_type[alphabetsize];
    if (tempcounts == NULL) {
      throw;
    }
    lf_upper = new pos_type[(((size + 8191) >> 13) + 1) * alphabetsize];
    if (lf_upper == NULL) {
      throw;
    }
    lf_middle = new unsigned char[size];
    if (lf_middle == NULL) {
      throw;
    }
    lf_lower = new unsigned char[(size + 1) >> 1];
    if (lf_lower == NULL) {
      throw;
    }

    for (c = 0; c < alphabetsize; ++c) {
      counts[c] = 0;
    }
    for (i = T, p = 0, q = 0; i != T_last; ++p) {
      t = p * alphabetsize;
      for (c = 0; c < alphabetsize; ++c) {
        lf_upper[t + c] = tempcounts[c] = counts[c];
      }
      for (j = i + std::min(4096, size - q); i != j; ++i, ++q) {
        d = *i;
        temp = counts[d] - tempcounts[d];
        lf_middle[q] = (unsigned char) (temp >> 4);
        if (q & 1) {
          lf_lower[q >> 1] |= (temp & 15) << 4;
        } else {
          lf_lower[q >> 1] = temp & 15;
        }
        counts[d]++;
      }
      for (c = 0; c < alphabetsize; ++c) {
        tempcounts[c] = counts[c];
      }
      for (k = i + std::min(4096, size - q), r = q; i != k; ++i, ++r) {
        tempcounts[*i]++;
      }
      for (; q < r; ++q, ++j) {
        d = *j;
        temp = tempcounts[d] - counts[d] - 1;
        lf_middle[q] = (unsigned char) (temp >> 4);
        if (q & 1) {
          lf_lower[q >> 1] |= (temp & 15) << 4;
        } else {
          lf_lower[q >> 1] = temp & 15;
        }
        counts[d]++;
      }
    }

    for (c = 0, sum = 1, p *= alphabetsize; c < alphabetsize; ++c) {
      t = counts[c];
      lf_upper[p + c] = t;
      counts[c] = sum;
      sum += t;
    }

    for (l = U_last, t = 0; l != U;) {
      *--l = e = outvalue_type(T[t]);
      temp = (t & 1) ? (lf_lower[t >> 1] >> 4) & 15 : (lf_lower[t >> 1] & 15);
      temp |= lf_middle[t] << 4;
      t = ((t & 8191) <= 4095) ?
          (lf_upper[(t >> 13) * alphabetsize + e] + temp + counts[e]) :
          (lf_upper[(t >> 13) * alphabetsize + e + alphabetsize] - temp
              + counts[e] - 1);
      if (pidx < t) {
        --t;
      }
    }
  } catch (...) {
    err = -1;
  }

  delete[] lf_lower;
  delete[] lf_middle;
  delete[] lf_upper;
  delete[] counts;
  delete[] tempcounts;

  return err;
}

template<typename StringIterator_type, typename pos_type,
    typename alphabetsize_type>
int reverseBWT_clf(StringIterator_type T, StringIterator_type T_last,
                   pos_type pidx, alphabetsize_type alphabetsize = 256) {
  typedef typename std::iterator_traits<StringIterator_type>::value_type value_type;
  typedef typename std::iterator_traits<StringIterator_type>::difference_type difference_type;
  pos_type *counts = NULL, *tempcounts = NULL, *lf_upper = NULL;
  unsigned char *lf_lower = NULL;
  StringIterator_type i, j, k;
  difference_type size;
  pos_type p, q, r, t, sum;
  unsigned int temp;
  alphabetsize_type c;
  value_type d;
  int err = 0;

  size = std::distance(T, T_last);
  if ((size < 0) || (pidx < 0) || (size < pidx) || (alphabetsize <= 0)) {
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  if (pidx == 0) {
    return -1;
  }

  if (alphabetsize == 256) {
    unsigned short *lf_middle = NULL;
    try {
      counts = new pos_type[256];
      if (counts == NULL) {
        throw;
      }
      tempcounts = new pos_type[256];
      if (tempcounts == NULL) {
        throw;
      }
      lf_upper = new pos_type[(((size + 8191) >> 13) + 1) << 8];
      if (lf_upper == NULL) {
        throw;
      }
      lf_middle = new unsigned short[size];
      if (lf_middle == NULL) {
        throw;
      }
      lf_lower = new unsigned char[(size + 1) >> 1];
      if (lf_lower == NULL) {
        throw;
      }

      for (c = 0; c < 256; ++c) {
        counts[c] = 0;
      }
      for (i = T, p = 0, q = 0; i != T_last; ++p) {
        t = p << 8;
        for (c = 0; c < 256; ++c) {
          lf_upper[t + c] = tempcounts[c] = counts[c];
        }
        for (j = i + std::min(4096, size - q); i != j; ++i, ++q) {
          d = *i;
          temp = counts[d] - tempcounts[d];
          lf_middle[q] = ((temp & 4080) << 4) | d;
          if (q & 1) {
            lf_lower[q >> 1] |= (temp & 15) << 4;
          } else {
            lf_lower[q >> 1] = temp & 15;
          }
          counts[d]++;
        }
        for (c = 0; c < 256; ++c) {
          tempcounts[c] = counts[c];
        }
        for (k = i + std::min(4096, size - q), r = q; i != k; ++i, ++r) {
          tempcounts[lf_middle[r] = *i]++;
        }
        for (; q < r; ++q) {
          d = value_type(lf_middle[q]);
          temp = tempcounts[d] - counts[d] - 1;
          lf_middle[q] |= (temp & 4080) << 4;
          if (q & 1) {
            lf_lower[q >> 1] |= (temp & 15) << 4;
          } else {
            lf_lower[q >> 1] = temp & 15;
          }
          counts[d]++;
        }
      }

      for (c = 0, sum = 1, p <<= 8; c < 256; ++c) {
        t = counts[c];
        lf_upper[p + c] = t;
        counts[c] = sum;
        sum += t;
      }

      for (i = T_last, t = 0; i != T;) {
        *--i = d = value_type(lf_middle[t] & 255);
        temp = (t & 1) ? (lf_lower[t >> 1] >> 4) & 15 : (lf_lower[t >> 1] & 15);
        temp |= (lf_middle[t] & 65280) >> 4;
        t = ((t & 8191) <= 4095) ?
            (lf_upper[((t & ~8191) >> 5) + d] + temp + counts[d]) :
            (lf_upper[((t & ~8191) >> 5) + d + 256] - temp + counts[d] - 1);
        if (pidx < t) {
          --t;
        }
      }
    } catch (...) {
      err = -1;
    }

    delete[] lf_lower;
    delete[] lf_middle;
    delete[] lf_upper;
    delete[] counts;
    delete[] tempcounts;
  } else {
    value_type *lf_string = NULL;
    unsigned char *lf_middle = NULL;
    try {
      counts = new pos_type[alphabetsize];
      if (counts == NULL) {
        throw;
      }
      tempcounts = new pos_type[alphabetsize];
      if (tempcounts == NULL) {
        throw;
      }
      lf_string = new value_type[size];
      if (lf_string == NULL) {
        throw;
      }
      lf_upper = new pos_type[(((size + 8191) >> 13) + 1) * alphabetsize];
      if (lf_upper == NULL) {
        throw;
      }
      lf_middle = new unsigned char[size];
      if (lf_middle == NULL) {
        throw;
      }
      lf_lower = new unsigned char[(size + 1) >> 1];
      if (lf_lower == NULL) {
        throw;
      }

      for (c = 0; c < alphabetsize; ++c) {
        counts[c] = 0;
      }
      for (i = T, p = 0, q = 0; i != T_last; ++p) {
        t = p * alphabetsize;
        for (c = 0; c < alphabetsize; ++c) {
          lf_upper[t + c] = tempcounts[c] = counts[c];
        }
        for (j = i + std::min(4096, size - q); i != j; ++i, ++q) {
          d = *i;
          temp = counts[d] - tempcounts[d];
          lf_string[q] = d;
          lf_middle[q] = (unsigned char) (temp >> 4);
          if (q & 1) {
            lf_lower[q >> 1] |= (temp & 15) << 4;
          } else {
            lf_lower[q >> 1] = temp & 15;
          }
          counts[d]++;
        }
        for (c = 0; c < alphabetsize; ++c) {
          tempcounts[c] = counts[c];
        }
        for (k = i + std::min(4096, size - q), r = q; i != k; ++i, ++r) {
          tempcounts[lf_string[r] = *i]++;
        }
        for (; q < r; ++q) {
          d = lf_string[q];
          temp = tempcounts[d] - counts[d] - 1;
          lf_middle[q] = (unsigned char) (temp >> 4);
          if (q & 1) {
            lf_lower[q >> 1] |= (temp & 15) << 4;
          } else {
            lf_lower[q >> 1] = temp & 15;
          }
          counts[d]++;
        }
      }

      for (c = 0, sum = 1, p *= alphabetsize; c < alphabetsize; ++c) {
        t = counts[c];
        lf_upper[p + c] = t;
        counts[c] = sum;
        sum += t;
      }

      for (i = T_last, t = 0; i != T;) {
        *--i = d = value_type(lf_string[t]);
        temp = (t & 1) ? (lf_lower[t >> 1] >> 4) & 15 : (lf_lower[t >> 1] & 15);
        temp |= lf_middle[t] << 4;
        t = ((t & 8191) <= 4095) ?
            (lf_upper[(t >> 13) * alphabetsize + d] + temp + counts[d]) :
            (lf_upper[(t >> 13) * alphabetsize + d + alphabetsize] - temp
                + counts[d] - 1);
        if (pidx < t) {
          --t;
        }
      }
    } catch (...) {
      err = -1;
    }

    delete[] lf_string;
    delete[] lf_lower;
    delete[] lf_middle;
    delete[] lf_upper;
    delete[] counts;
    delete[] tempcounts;
  }

  return err;
}

} /* namespace divsufsortxx */

#endif /* _DIVSUFSORT_UTILITY_H_ */
