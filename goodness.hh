#ifndef _GOODNESS_HH_
#define _GOODNESS_HH_

#include <iostream>
#include <cmath>
#include "float.h"

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

static inline double cdf(double mean, double stdev, double x) {
    return 0.5 * (1 + approx_erf((x - mean)/(stdev * sqrt(2))));
}

static inline double goodness(double F_hb, double F_mf, double T_to, 
                double T_l, double C_r, double C_hb) {
    double false_fail = cdf(0,100,1./F_hb + T_l - T_to);
    //std::cerr << "F_hb: " << F_hb << "\n";
    //std::cerr << "F_mf: " << F_mf << "\n";
    //std::cerr << "T_to: " << T_to << "\n";
    //std::cerr << "T_l:  " << T_l << "\n";
    //std::cerr << "C_r:  " << C_r << "\n";
    //std::cerr << "C_hb: " << C_hb << "\n";
    return (F_mf * T_to) / (2*(F_hb*C_hb + false_fail*C_r));
}

// find max goodness
static inline std::pair<double,double> get_best_params(double min_F_hb, double max_F_hb, double step_F_hb,
                                         double min_T_to, double max_T_to, double step_T_to,
                                         double F_mf, double T_l, double C_r, double C_hb) {
    double best_F_hb = 200, best_T_to = 500;
    double best_g = -DBL_MAX;
    for (double fhb = min_F_hb; fhb < max_F_hb; fhb += step_F_hb) {
        for (double tto = min_T_to; tto < max_T_to; tto += step_T_to) {
            double g = goodness(fhb,F_mf,tto,T_l,C_r,C_hb);
            if (g > best_g) {
                best_g = g;
                best_F_hb = fhb;
                best_T_to = tto;
            }
        }
    }
    std::pair<double,double> ret (best_F_hb,best_T_to);
    return ret;
}

#endif // _GOODNESS_HH_
