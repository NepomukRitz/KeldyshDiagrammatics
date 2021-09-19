#ifndef FPP_MFRG_TEST_ODE_SOLVER_H
#define FPP_MFRG_TEST_ODE_SOLVER_H

double max_rel_err(double x, double scale, double tiny) {
    return std::abs(x/scale);
}


#include "../ODE_solvers.h"

template<> void postRKstep_stuff<double>(double y, vec<double> x_vals, int iteration, std::string filename) {
    std::cout <<"Intermediate result of ODE solver: " << y << std::endl;
}

namespace {
    double rhs_lin(double y, double x) {
        return x*2.;
    }
    double rhs_quartic(double y, double x) {
        return x*x*x*x*5.;
    }
    double rhs_exp(double y, double x) {
        return y;
    }
}

TEST_CASE( "Does the ODE solver work for a simple ODE?", "[ODEsolver]" ) {

    double Lambda_i = 1.;
    double Lambda_f = 1e-12;
    std::vector<double> lambda_checkpoints = {0.5};

    double y_ini = 1.;
    double result;
    ode_solver<double>(result, Lambda_f, y_ini, Lambda_i, lambda_checkpoints, rhs_lin);


    double result_exact = 0.;
    SECTION( "Is the correct value retrieved from ODE solver?" ) {
        REQUIRE( std::abs(result - result_exact) < 1e-5 );
    }

}


TEST_CASE( "Does the ODE solver work for a medium ODE?", "[ODEsolver]" ) {

    double Lambda_i = 1.;
    double Lambda_f = 1e-12;
    std::vector<double> lambda_checkpoints = {0.5};

    double y_ini = exp(1.);
    double result;
    ode_solver<double>(result, Lambda_f, y_ini, Lambda_i, lambda_checkpoints, rhs_exp);


    double result_exact = 1.;
    SECTION( "Is the correct value retrieved from ODE solver?" ) {
        REQUIRE( std::abs(result - result_exact) < 1e-1 );
    }

}



#endif //FPP_MFRG_TEST_ODE_SOLVER_H
