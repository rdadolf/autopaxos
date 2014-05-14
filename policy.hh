#ifndef _POLICY_HH_
#define _POLICY_HH_

#include <iostream>
#include <cmath>
#include "float.h"
#include <math.h>

// variation on http://www.johndcook.com/blog/2009/01/19/stand-alone-error-function-erf/
static inline double approx_erf(double x) {
    // constants
    double a1 =  0.254829592;
    double a2 = -0.284496736;
    double a3 =  1.421413741;
    double a4 = -1.453152027;
    double a5 =  1.061405429;
    double p  =  0.3275911;

    // remember sign on x
    int sign = (x < 0) ? -1 : 1;
    x = fabs(x);

    double t = 1./(1. + p * x);
    double y = 1. - (((((a5 * t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

    return sign * y;
}

static inline double cdf(double x, double mean, double stdev) {
    return 0.5 * (1 + approx_erf((x - mean)/(stdev * sqrt(2))));
}

static inline double pmetric( double T_hb, // Heartbeat interval
                              double T_bf, // MTBF
                              double T_to, // Timeout interval
                              double T_l,  // Latency
                              double C_r,  // Cost of recovery messages
                              double C_hb  // Cost of 1 heartbeat message
                            ) {
  double uptime;
  // True failure detection & recovery
  //   (MTBF - blindWin+electionWin)
  // False failure recovery
  //   (1-P_ff) * electionWin
  double P_ff;
  // P_ff Fixed latency model
  //P_ff = (T_hb+T_l) > T_to
  // P_ff Gaussian latency model (std=100ms)
  P_ff = cdf( T_hb+T_l-T_to, 0, 100 );
  double P_tf;
  P_tf = exp(-1000./T_bf);
  uptime = 1 - P_tf - P_ff;

  double traffic;
  // Heartbeat overhead
  //   C_hb/T_hb
  // True failure recovery
  //   C_r/T_bf
  // False failure recovery
  //   P_ff * C_r
  traffic= C_hb/T_hb + C_r/T_bf + P_ff*C_r;

  //std::cerr << "T_hb: " << T_hb << "\n";
  //std::cerr << "T_bf: " << T_bf << "\n";
  //std::cerr << "T_to: " << T_to << "\n";
  //std::cerr << "T_l:  " << T_l << "\n";
  //std::cerr << "C_r:  " << C_r << "\n";
  //std::cerr << "C_hb: " << C_hb << "\n";
  
  return uptime/traffic;

  //double false_fail = cdf(0,100,1./F_hb + T_l - T_to);
  //return (F_mf * T_to) / (2*(F_hb*C_hb + false_fail*C_r));
}

// find max goodness
static inline std::pair<double,double>
get_best_params(double min_T_hb, double max_T_hb, double step_T_hb, // sweep T_hb
                double min_T_to, double max_T_to, double step_T_to, // sweep T_to
                double T_bf, double T_l, double C_r, double C_hb) { // fix others
    double best_T_hb(0), best_T_to(0);
    double best_p = -DBL_MAX;
    for (double thb = min_T_hb; thb < max_T_hb; thb += step_T_hb) {
        for (double tto = min_T_to; tto < max_T_to; tto += step_T_to) {
            double p = pmetric(thb, T_bf, tto, T_l, C_r, C_hb);
            if (p > best_p) {
                best_p = p;
                best_T_hb = thb;
                best_T_to = tto;
            }
        }
    }
    std::pair<double,double> ret (best_T_hb, best_T_to);
    return ret;
}

#endif // _POLICY_HH_
