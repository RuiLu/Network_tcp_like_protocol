/***************************************************************************
 *  Math library
 *  akiramath.c
 *
 *  by Akira Tsukamoto
 ***************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "akiramath.h"

#define IA   16807              /* ran1() */
#define IM   2147483647
#define AM   (1.0 / IM)
#define IQ   127773
#define IR   2836
#define NTAB 32
#define NDIV (1 + (IM -1) / NTAB)
#define EPS  1.2e-7
#define RNMX (1.0 - EPS)

#define PI   3.141592654        /* bnldev() */

/** stage for Gilbert packet loss. previous loss is 1 */
int stage = 0;

/*** gammln() ***/
float gammln(float xx)
{
  double x, y, tmp, ser;
  static double cof[6] = {
    76.18009172947146,
    -86.50532032941677,
    24.01409824083091,
    -1.231739572450155,
    0.1208650973866179e-2,
    -0.5395239384953e-5,
  };
  int j;

  y = x = xx;
  tmp = x + 5.5;
  tmp -= (x + 0.5) * log(tmp);
  ser = 1.000000000190015;
  for (j = 0; j <= 5; j++)
    ser += cof[j] / ++y;

  return -tmp + log(2.5066282746310005 * ser / x);
}                               /* gammln() */

/*** ran1() ***/
#if 0
float ran1(long *idum)
{
  int j;
  long k;
  static long iy = 0;
  static long iv[NTAB];
  float temp;

  if (*idum <= 0 || !iy) {
    if (-(*idum) < 1)
      *idum = 1;
    else
      *idum = -(*idum);
    for (j = NTAB + 7; j >= 0; j--) {
      k = (*idum) / IQ;
      *idum = IA * (*idum - k * IQ) - IR * k;
      if (*idum < 0)
        *idum += IM;
      if (j < NTAB)
        iv[j] = *idum;
    }
    iy = iv[0];
  }

  k = (*idum) / IQ;
  *idum = IA * (*idum - k * IQ) - IR * k;
  if (*idum < 0)
    *idum += IM;
  j = iy / NDIV;
  iy = iv[j];
  iv[j] = *idum;

  if ((temp = (float) (AM * iy)) > RNMX)
    return RNMX;
  else
    return temp;
}                               /* ran1() */
#else                           /* using ANSI C rand() to make it faster */
float ran1(long *idum)
{
  return rand() / (float) RAND_MAX;
}

#endif

/*** bnldev() ***/
int bnldev(float pp, int n, long *idum)
{
  int j;
  int bnl;
  static int nold = (-1);
  float am, em, g, angle, p, sq, t, y;
  static float pold = (-1.0), pc, plog, pclog, en, oldg;

  p = (pp <= 0.5 ? pp : 1.0 - pp);

  am = n * p;
  if (n < 0) {                /* original code was n < 25, but this is faster */
    bnl = 0;
    for (j = 1; j <= n; j++)
      if (ran1(idum) < p)
        bnl++;
  } else if (am < 1.0) {
    g = exp(-am);
    t = 1.0;
    for (j = 0; j <= n; j++) {
      t *= ran1(idum);
      if (t < g)
        break;
    }
    bnl = (j <= n ? j : n);
  } else {
    if (n != nold) {
      en = n;
      oldg = gammln(en + 1.0);
      nold = n;
    }
    if (p != pold) {
      pc = 1.0 - p;
      plog = log(p);
      pclog = log(pc);
      pold = p;
    }
    sq = sqrt(2.0 * am * pc);
    do {
      do {
        angle = PI * ran1(idum);
        y = tan(angle);
        em = sq * y + am;
      } while (em < 0.0 || em >= (en + 1.0));
      em = floor(em);
      t = 1.2 * sq * (1.0 + y * y) * exp(oldg - gammln(em + 1.0)
                                         - gammln(en - em + 1.0)
                                         + em * plog + (en - em) * pclog);
    } while (ran1(idum) > t);
    bnl = (int) em;
  }

  if (p != pp)
    bnl = n - bnl;
  return bnl;
}                               /* bnldev() */

int gilbRand(float pc, float pa)
{

  if (stage == 0) {
    if (rand() / (float) RAND_MAX < pa) {
      stage++;
      return 1;
    }
  }
  if (stage == 1) {
    if (rand() / (float) RAND_MAX < pc) {
      return 1;
    } else {
      stage--;
    }
  }

  return 0;       /* no error */
}
