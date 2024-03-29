#include "points.h"
#include "ncepgrids.h"

// do harmonic analysis at doodson-like-specified periods
// ignore non-orthogonality for now.
// 13 July 2012

#define PERIOD 365.259636
//#define NPER 38
#define NPER 6

#define TROPICAL    365.24219
#define SIDEREAL    365.256363
#define ANOMALISTIC 365.259635

#define lunar_sidereal 27.32166
#define lunar_perigee (8.85*SIDEREAL)
#define lunar_node    (18.6*SIDEREAL)

////////////////////////////////////////////////////////////////////////
template <class T>
void  read_first_pass(FILE *fin, global_quarter<T> &slope, global_quarter<T> &intercept) ;

void compute_harmonics(mvector<global_quarter<double> > &sum_sin, mvector<global_quarter<double> > &sum_cos, mvector<global_quarter<float> > &ampl, mvector<global_quarter<float> > &phase, int npts, mvector<double> &period) ;

template <class T>
void compute_trends(grid2<T> &sx, grid2<T> &sx2, grid2<T> &sxy, grid2<T> &sy, grid2<T> &sy2, grid2<float> &slope, grid2<float> &intercept, grid2<float> &correl, grid2<float> &tstatistic, int npts) ;

template <class T>
void assemble2(global_quarter<T> &expectation, global_quarter<T> &slope, global_quarter<T> &intercept, mvector<global_quarter<T> > &ampl, mvector<global_quarter<T> > &phase, float &time, int &nharm, mvector<double> &omega) ;

template <class T>
void assemble1(grid2<T> &expectation, grid2<T> &slope, grid2<T> &intercept, float time) ;
////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
// For local reading and accumulating
  global_quarter<short int> tmp;
  global_quarter<long long int> stmp, stmp2, sumy, sumy2;
// For local trend
  global_quarter<long long int> sumx, sumx2, sumxy;  
// For harmonics
  mvector<global_quarter<double> > sum_sin(NPER), sum_cos(NPER);
  double fsin[NPER], fcos[NPER];
  mvector<double> omega(NPER), period(NPER);
// From first pass
  global_quarter<float> slope, intercept;

// utility:
  FILE *fin;
  int i, j, npts, nharm = 6;
  ijpt loc;
  float time;
  global_quarter<float> expect;

///////////////////////////////////////////////////////////
// Start working
  npts = argc - 2;
  printf("npts = %d\n",npts); fflush(stdout);

  fin = fopen(argv[1],"r");
  read_first_pass(fin, slope, intercept);
  fclose(fin);

// Initialize:
  sumy.set((long long int) 0);
  sumy2.set((long long int) 0);
  sumx.set((long long int) 0);
  sumxy.set((long long int) 0);
  sumx2.set((long long int) 0);
  for (i = 0; i < NPER; i++) {
    sum_sin[i].set((double) 0);
    sum_cos[i].set((double) 0);
  }

  fin = fopen("doodson","r");
  int d1, d2, d3, d4, d5, d6;
  for (j = 0; j < NPER; j++) {
    fscanf(fin, "%d %d %d %d %d %d\n",&d1, &d2, &d3, &d4, &d5, &d6);
    printf("%d %d %d %d %d %d  ",d1, d2, d3, d4, d5, d6);
    omega[j] = 
        1/(TROPICAL)*d1        +
        1/(SIDEREAL)*d2        + 
        1/(ANOMALISTIC)*d3     + 
        1/(lunar_sidereal)*d4  + 
        1/(lunar_node)*d5      + 
        1/(lunar_perigee)*d6 ;
     period[j] = 1./omega[j];
     omega[j] *= 2.*M_PI;
     printf(" %e %e\n",period[j],omega[j]);
  }
  fflush(stdout);

////////////////////////////////////////////////////////////////////////////
// Start scan through observations:
  for (i = 2; i < argc; i++) {
    time = (float) (i - 2);
    fin = fopen(argv[i], "r");
    tmp.binin(fin);
    fclose(fin);
    printf("read in file %d max = %d\n",i, tmp.gridmax() ); fflush(stdout);
    conv(tmp, stmp);

    assemble1(expect, slope, intercept, time);
    conv(expect, stmp2);
    stmp -= stmp2;

    // for harmonics
    for (j = 0; j < NPER; j++) {
      fsin[j] = sin(omega[j]*(time) ) * 2./period[j];
      fcos[j] = cos(omega[j]*(time) ) * 2./period[j];
    }

    for (int index = 0; index < sumy.xpoints()*sumy.ypoints(); index++) {
      // for trends:
      sumy[index]  += stmp[index];
      sumy2[index] += stmp[index]*stmp[index];
      sumx[index]  += (long long int) (time);
      sumx2[index] += (long long int) (time*time);
      sumxy[index] += stmp[index]*(long long int) (time);

      // for harmonics
      for (j = 0; j < NPER; j++) {
        sum_sin[j][index] += stmp[index]*fsin[j];
        sum_cos[j][index] += stmp[index]*fcos[j];
      }
    }

  }
// Done doing accumulations
////////////////////////////////////////////////////////////////////////////

// Find things of interest and write them out:
  FILE *fout;
  global_quarter<float> lslope, lintercept, correl, tstatistic;
  compute_trends(sumx, sumx2, sumxy, sumy, sumy2, lslope, lintercept, correl, tstatistic, npts);
  printf("since this is second pass, local slope and intercept should be ~zero\n");
  printf(" since the arithmetic is bounded, it won't be exactly so, so prior to output, update slope with lslope\n");
  printf("slope max min avg rms %f %f %f %f\n",slope.gridmax(), slope.gridmin(), slope.average(), slope.rms() );
  printf("intercept max min avg rms %f %f %f %f\n",intercept.gridmax(), intercept.gridmin(), intercept.average(), intercept.rms() );
  printf("lslope max min avg rms %f %f %f %f\n",lslope.gridmax(), lslope.gridmin(), lslope.average(), lslope.rms() );
  printf("lintercept max min avg rms %f %f %f %f\n",lintercept.gridmax(), lintercept.gridmin(), lintercept.average(), lintercept.rms() );
  fflush(stdout);

  slope += lslope;
  intercept += lintercept;


// Defer this declaration for memory considerations
  fout = fopen("dood_harmonics_out", "w");
  mvector<global_quarter<float> > ampl(NPER), phase(NPER);
  compute_harmonics(sum_sin, sum_cos, ampl, phase, npts, period);
  global_quarter<long long int> tvar;
  global_quarter<float> var;

  // var is variance after removal of trend but before removal of harmonics
  for (int index = 0; index < var.xpoints()*var.ypoints(); index++) {
    tvar[index] = (sumy2[index] - sumy[index]*sumy[index] / npts );
    var[index] = (float) ((double) tvar[index] / (double) (npts*100*100));
  }
  var.binout(fout);
  printf("var stats: %f %f %f %f\n",var.gridmax(), var.gridmin(), var.average(), var.rms() );
  fflush(stdout);

  for (j = 0; j < NPER; j++) {
    printf("period %7.2f  ampl stats %6.3f %5.3f %5.3f\n",period[j], ampl[j].gridmax()/100, ampl[j].average()/100, ampl[j].rms()/100 );
    fflush(stdout);
    ampl[j].binout(fout);  fflush(fout);
    phase[j].binout(fout); fflush(fout);
  }
  
  fclose(fout);


  fout = fopen("reference_fields","w");
  intercept.binout(fout);
  slope.binout(fout);
  for (j = 0; j < nharm; j++) {
    ampl[j].binout(fout);
    phase[j].binout(fout);
  }
  fclose(fout);
//////////////////////////////////////////////////////////////////////


//// Now write out residual fields -- subtract trend and first 6 annual harmonics.
// Note that the read in files are not scaled to degrees, still centi-degrees,
//  so we want mean and amplitude to remain unscaled at this point.
  global_quarter<float> mean, ftmp, ftmp2;
  char fname[90];

  for (i = 2; i < argc; i++) {
    time = (float) (i-2);
    fin = fopen(argv[i],"r"); 
    tmp.binin(fin);
    fclose(fin);

    conv(tmp, ftmp);

    assemble2(expect, slope, intercept, ampl, phase, time, nharm, omega);
    ftmp -= expect;
    
    sprintf(fname,"residual.%05d",i);
    fout = fopen(fname,"w");
    ftmp.binout(fout);
    fclose(fout);
  }

  return 0;
}


////////////////////
void compute_harmonics(mvector<global_quarter<double> > &sum_sin, mvector<global_quarter<double> > &sum_cos, mvector<global_quarter<float> > &ampl, mvector<global_quarter<float> > &phase, int npts, mvector<double> &period) {
  ijpt loc;
  int i;
// Compute fourier amplitude and phase:
  for (loc.j = 0; loc.j < ampl[0].ypoints(); loc.j++) {
  for (loc.i = 0; loc.i < ampl[0].xpoints(); loc.i++) {
    for (i = 0; i < NPER; i++) {
       ampl[i][loc] = sqrt(sum_sin[i][loc]*sum_sin[i][loc] +
                           sum_cos[i][loc]*sum_cos[i][loc]    ) / ((float) npts / (float) period[i]);
       phase[i][loc] = atan2(sum_sin[i][loc], sum_cos[i][loc] );
    }
  }
  }

  return;
}
////////////////////
template <class T>
void compute_trends(grid2<T> &sx, grid2<T> &sx2, grid2<T> &sxy, grid2<T> &sy, grid2<T> &sy2, grid2<float> &slope, grid2<float> &intercept, grid2<float> &correl, grid2<float> &tstatistic, int npts) {
  ijpt loc;

// x = time, y = sst.
  for (loc.j = 0; loc.j < sx.ypoints(); loc.j++) {
  for (loc.i = 0; loc.i < sx.xpoints(); loc.i++) {
    slope[loc] = (float) ((double) ((double) npts*sxy[loc] - (double) sx[loc]*sy[loc]) /
                          (double) ((double) npts*sx2[loc] - (double) sx[loc]*sx[loc])  ) ;
    intercept[loc] = ((double) sy[loc]/npts - slope[loc]*(double) sx[loc]/(double) npts);
    correl[loc] =    ((double) npts*sxy[loc] - (double) sx[loc]*sy[loc]) /
                 sqrt((double) npts*sx2[loc] - (double) sx[loc]*sx[loc]) /
                 sqrt((double) npts*sy2[loc] - (double) sy[loc]*sy[loc]) ;

    // test statistic goes here
    tstatistic[loc] = correl[loc]*sqrt(npts)/(1. - correl[loc]*correl[loc]);
  }
  }
  return;
}
////////////////////
template <class T>
void assemble1(grid2<T> &expectation, grid2<T> &slope, grid2<T> &intercept, float time) {
// time is days since 1 September 1981, with that day being 0.
  expectation = slope;
  expectation *= time;
  expectation += intercept;
  return;
}

template <class T>
void assemble2(global_quarter<T> &expectation, global_quarter<T> &slope, global_quarter<T> &intercept, mvector<global_quarter<T> > &ampl, mvector<global_quarter<T> > &phase, float &time, int &nharm, mvector<double> &omega) {
// time is days since 1 September 1981, with that day being 0.

  expectation = slope;
  expectation *= time;
  expectation += intercept;

  for (int index = 0; index < expectation.ypoints()*expectation.xpoints(); index++) {
    for (int j = 0; j < nharm; j++) {
      expectation[index] += ampl[j][index]*cos(omega[j]*time + phase[j][index] );
    }
  }
  return;
}
template <class T>
void  read_first_pass(FILE *fin, global_quarter<T> &slope, global_quarter<T> &intercept) {
  global_quarter<int> ix;
  global_quarter<float> fx;

  ix.binin(fin); 
  ix.binin(fin);
  ix.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  slope.binin(fin);
  intercept.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);
  fx.binin(fin);

  return;
}
