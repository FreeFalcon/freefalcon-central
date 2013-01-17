/* credits
   Rod Farlee - original version
   Ed Falk - getVar routine added
   Julian Onions - updated for year 2000+
   */

/* ANSI double precision math used: sin cos atan2 sqrt pow fmod M_PI */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_N 10        /* maximum spherical harmonic degree N */
#define R0 6371.2       /* mean radius of earth / km */
#define A  6378.16      /* equatorial radius of earth / km */
#define B  6356.78      /* polar radius of earth / km */
#define CORE 3480       /* radius of earth's core / km */
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

/*
 * a reference geomagnetic potential model includes:
 *   name, literature reference,
 *   epoch (base year),
 *   begin and end year of valid period,
 *   valid latitude and longitude range,
 *   maximum spherical harmonic degree=order,
 *   g[n][m] cosine coefficients, 0<n<=degree, 0<=m<=n,
 *   h[n][m] sine coefficients,
 *   and their secular (annual change) derivatives.
 */

struct model {
  char name[12],ref[32];
  double epoch, begin,end, S,N,W,E;  int degree;
  double g[MAX_N+1][MAX_N+1], h[MAX_N+1][MAX_N+1];
  double gs[MAX_N+1][MAX_N+1], hs[MAX_N+1][MAX_N+1];
};

/* internal procedure prototypes ("header file") */
double getVar(double lat,double lon,double elev,double year) ;
void compute(double *, double ,double ,double ,double ,struct model );
static void setup(double ,double ,double ,double ,struct model );
static void schmidt(int );
static double C1(int );
static double C2(int ,int );
static double C3(int ,int );
static double V(int );
static double Bp(int );
static double Br(int );
static double Bt(int );
static double dPdt(int ,int );
static double D5(int ,int );
static double D6(int ,int );

static struct model igrf2000 = {
  "IGRF2000", 
  "igrf000.html",
  2000.0, 2000.0, 2005.0, 
  -90,90,0,360, 10,
{
    { 0},
  { -29615, -1728,},
  { -2267, 3072, 1672,},
  { 1341, -2290, 1253, 715,},
  { 935, 787, 251, -405, 110,},
  { -217, 351, 222, -131, -169, -12,},
  { 72, 68, 74, -161, -5, 17, -91,},
  { 79, -74, 0, 33, 9, 7, 8, -2,},
  { 25, 6, -9, -8, -17, 9, 7, -8, -7,},
  { 5, 9, 3, -8, 6, -9, -2, 9, -4, -8,},
  { -2, -6, 2, -3, 0, 4, 1, 2, 4, 0, -1,},
},
{
  { 0,},
  { 0, 5186,},
  { 0, -2478, -458,},
  { 0, -227, 296, -492,},
  { 0, 272, -232, 119, -304,},
  { 0, 44, 172, -134, -40, 107,},
  { 0, -17, 64, 65, -61, 1, 44,},
  { 0, -65, -24, 6, 24, 15, -25, -6,},
  { 0, 12, -22, 8, -21, 15, 9, -16, -3,},
  { 0, -20, 13, 12, -6, -8, 9, 4, -8, 5,},
  { 0, 1, 0, 4, 5, -6, -1, -3, 0, -2, -8,},
},
{
  {0,},
  { 14.6, 10.7},
  { -12.4, 1.1, -1.1},
  { 0.7, -5.4, 0.9, -7.7},
  { -1.3, 1.6, -7.3, 2.9, -3.2},
  { 0.0, -0.7, -2.1, -2.8, -0.8, 2.5},
  { 1.0, -0.4, 0.9, 2.0, -0.6, -0.3, 1.2},
  { -0.4, -0.4, -0.3, 1.1, 1.1, -0.2, 0.6, -0.9},
  { -0.3, 0.2, -0.3, 0.4, -1.0, 0.3, -0.5, -0.7, -0.4},
  { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
  { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
},
{
  { 0,},
  { 0, -22.5,},
  { 0, -20.6, -9.6,},
  { 0, 6.0, -0.1, -14.2,},
  { 0, 2.1, 1.3, 5.0, 0.3,},
  { 0, -0.1, 0.6, 1.7, 1.9, 0.1,},
  { 0, -0.2, -1.4, 0.0, -0.8, 0.0, 0.9,},
  { 0, 1.1, 0.0, 0.3, -0.1, -0.6, -0.7, 0.2,},
  { 0, 0.1, 0.0, 0.0, 0.3, 0.6, -0.4, 0.3, 0.7,},
  { 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,},
  { 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,},
}
};


/* return variance for a point */
double
getVar(double lat,double lon,double elev,double year)
{
  double f[7];

  compute(f,lat,lon,elev,year,igrf2000);       /* compute field compenents */
  return f[5] ;
}

void computedef(double f[7],
  double lat,double lon,double elev,double year) {
    compute(f, lat, lon, elev, year, igrf2000);
}

/***********  beginning of geodetic computational section  ***********/
/*
 * convert from geodetic (geographic) to geocentric coordinates,
 * compute field components and return them in array f[7].
 * References: Peddie, program GEOMAG subroutine SHVAL3, based on:
 * D R Barraclough, S R C Malin, Report 71/1, Institude of Geological
 * Sciences, UK, subroutine IGRF.
 */
void compute(double f[7],
  double lat,double lon,double elev,double year,struct model x) {
  static double A2=A*A, B2=B*B;         /* earth semimajor,semiminor axes */
  double clat,slat,AA,BB,CC,DD,R,CD,SD, X,Y,Z;

  /* convert geodetic (geographic) to geocentric coordinates for the
   * oblate spheroidal earth (Langel, eqns 51, 53, 57, pp 266-7; note
   * that eqn 51 p 266 has a typographical error) */
  clat = cos(lat*M_PI/180.);
  slat = sin(lat*M_PI/180.);
  AA = A2 * clat * clat;
  BB = B2 * slat * slat;
  CC = AA + BB;
  DD = sqrt(CC);
  /* distance to center of earth (Langel, eqn 53, p 266) */
  R = sqrt(elev*(elev + 2.*DD) + (A2*AA + B2*BB)/CC);
  /* cos,sin of angle between geodetic and geocentric latitude */
  CD = (elev + DD)/R;
  SD = (A2 - B2)/DD * slat * clat / R;
  /* geocentric latitude (Langel, eqn 51, p 266) */
  lat = 180./M_PI* atan((slat*CD - clat*SD) / (clat*CD + slat*SD));
  /* printf("geocentric lat %.1f deg, elev %.3f km\n",lat,R-R0); */

  /* initialize variables and compute geocentric field components */
  setup(lat,lon, R, year, x);
  X = -Bt(x.degree);                    /* field to geocentric N */
  Y = Bp(x.degree);                     /* field to E */
  Z = -Br(x.degree);                    /* field geocentric down */

  /* convert geocentric to geodetic coordinates (Langel, eqns 56, p 267) */
  f[0] = X*CD + Z*SD;                   /* field to geodetic N */
  f[1] = Y;
  f[2] = Z*CD - X*SD;                   /* field geodetic down */
  X = f[0];
  Z = f[2];
  /* compute other components (Langel, eqns 46, 47, p 264) */
  f[3] = sqrt(X*X + Y*Y);               /* horizontal */
  f[4] = sqrt(X*X + Y*Y + Z*Z);         /* total field */
  f[5] = atan2(Y,X)*180.0/M_PI;         /* declination, E of N, degrees */
  f[6] = atan2(Z,f[3])*180.0/M_PI;      /* inclination, degrees */
}

/***********  beginning of geocentric computational section  ***********/
/*
 * global variables.
 * for speed, many variables (which are used repeatedly in computation
 * of the various gradients) are first computed once and stored.
 */
static double CT,ST;                    /* cos,sin of colatitude */
static double CP[MAX_N+2];              /* cos(longitude*order m) */
static double SP[MAX_N+2];              /* sin(        ""       ) */
  /* coefficients of the selected model, including secular change */
static double G[MAX_N+1][MAX_N+1], H[MAX_N+1][MAX_N+1];
static double P[MAX_N+2][MAX_N+2];      /* spherical harmonics */
static double A_R[MAX_N+3];             /* A_R[n]=(A/R)^n */

/* initialize all variables */
static void setup(double lat,double lon,double R,double year,struct model x) {
  double theta=(90.0-lat)*M_PI/180.0;   /* colatitude/radians */
  double phi=lon*M_PI/180.0;            /* longitude/radians */
  int n,m;
  double dy;

  CT = cos(theta);                      /* cos,sin(theta) */
  ST = sin(theta);
  for(m=0; m<=x.degree+1; m++) {
    CP[m] = cos((double)m*phi);         /* cos,sin(m*phi) */
    SP[m] = sin((double)m*phi);
  }
  dy = year - x.epoch;          /* update g,h for secular variation */
  for(n=1; n<=x.degree; n++) {
    for(m=0; m<=n; m++) {
      G[n][m] = x.g[n][m] + dy*x.gs[n][m];
      H[n][m] = x.h[n][m] + dy*x.hs[n][m];
    }
  }
  A_R[1] = R0/R;                        /* (A/R)^N */
  for(n=2; n<=x.degree+2; n++) {
    A_R[n] = pow(A_R[1],(double)n);
  }
  schmidt(x.degree);                    /* spherical harmonics */
}

/*  Schmidt quasi-normalized spherical harmonics,
 *  as used in geophysics, differ from the usual mathematical
 *  definition of spherical harmonics in two ways:
 *  1) the coefficients are multiplied by -1^m.  See Langel p 252
 *     ("Often a factor of (-1)^m is included") and Table 4 p 259
 *     (note that F and P are positive for all m).
 *  2) magnitude: quasi-normalized = fully normalized/sqrt(2n+1)
 *
 *  Note: dP/dt will require P[0][0] and P[degree+1][m]
 */
static void schmidt(int degree) {
  int n,m;
  P[0][0] = 1.0;
  /*  Pnn (Langel, eqn 26, p 256) */
  for(n=1; n<=degree+1; n++)
    P[n][n] = C1(n) * ST * P[n-1][n-1];
  /*  Pnm, m<n (Langel, eqn 27, p 256) */
  for(n=1; n<=degree+1; n++) {
    for(m=0; m<n; m++) {
      P[n][m] = C2(n,m) * CT * P[n-1][m];
      if(m<n-1) P[n][m] -= C3(n,m) * P[n-2][m];         /* C3(n,n-1)=0 */
    }
  }
}

/* coefficients for recursions for spherical harmonics
 * (Langel, Table 2, p 256).
 */
static double C1(int n) {
  return(sqrt((double)((n?2:1)*(2*n-1))/(double)((n-1?2:1)*2*n)));
}

static double C2(int n,int m) {
  return((double)(2*n-1)/sqrt((double)(n*n-m*m)));
}

static double C3(int n,int m) {
  return(sqrt((double)((n-1)*(n-1)-m*m)/((double)(n*n-m*m))));
}

/* compute geomagnetic potential (Langel, eqn 20, p 255).
 * note: field = gradient of potential.  This can be used as a check of
 * the more accurate direct computation of the gradient using Bt,Bp,Br.
 * note: V is in units nT/A, where A=radius of earth.
 */
static double V(int degree) {
  int n,m;
  double v=0.0;
  for(n=1; n<=degree; n++) {
    for(m=0; m<=n; m++) {
      v += (G[n][m]*CP[m] + H[n][m]*SP[m]) * A_R[n+1] * P[n][m];
    }
  }
  return(v);
}

/* compute the EW field (Langel, eqn 23, p 255) */
static double Bp(int degree) {
  int n,m;
  double b=0.0;
  for(n=1;n<=degree;n++) {
    for(m=0;m<=n;m++) {
      b += m * (G[n][m]*SP[m] - H[n][m]*CP[m]) * A_R[n+2] * P[n][m];
    }
  }
  return(b/ST);
}

/* compute vertical field (Langel, eqn 21, p 255) */
static double Br(int degree) {
  int n,m;
  double b=0.0;
  for(n=1;n<=degree;n++) {
    for(m=0;m<=n;m++) {
      b += (n+1) * (G[n][m]*CP[m] + H[n][m]*SP[m]) * A_R[n+2] * P[n][m];
    }
  }
  return(b);
}

/* compute NS field (Langel, eqn 22, p 255) */
static double Bt(int degree) {
  int n,m;
  double b=0.0;
  for(n=1; n<=degree; n++) {
    for(m=0; m<=n; m++) {
      b -= (G[n][m]*CP[m] + H[n][m]*SP[m]) * A_R[n+2] * dPdt(n,m);
    }
  }
  return(b);
}

/* compute d(Legendre)/d(theta), NS derivative of spherical harmonic
 * (Langel, eqn 34, p 257) */
static double dPdt(int n,int m) {
  double d;
  d = D5(n,m) * P[n+1][m];
  if(n>m) d -= D6(n,m) * P[n-1][m];     /* D6(n,n)=0 */
  d = d/ST;
  return(d);
}

/* coefficients for NS derivative (Langel, Table 3, p 258) */
static double D5(int n,int m) { return(n*sqrt((n+1)*(n+1)-m*m)/(double)(2*n+1)); }
static double D6(int n,int m) { return((n+1)*sqrt(n*n-m*m)/(double)(2*n+1)); }

/* end of GEOMAG */
