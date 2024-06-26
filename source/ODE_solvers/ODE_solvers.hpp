#ifndef KELDYSH_MFRG_SOLVERS_H
#define KELDYSH_MFRG_SOLVERS_H

#include <cmath>                                    // needed for exponential and sqrt function
#include "../grids/flow_grid.hpp"                        // flow grid
#include "../utilities/util.hpp"                         // text input/output
#include "../utilities/write_data2file.hpp"              // writing data into text or hdf5 files
#include "../parameters/master_parameters.hpp"                             // needed for the vector of grid values to add
#include "../postprocessing/causality_FDT_checks.hpp"    // check causality and FDTs at each step in the flow
#include "../utilities/hdf5_routines.hpp"
#include "../correlation_functions/state.hpp"
#include "old_solvers.hpp"
#include "ODE_solver_config.hpp"


/**
 * Implementation of things that can be done after a completed Runge-Kutta iteration of the ODE-solver,
 * such as consistency checks, updates of frequency grids, output of intermediate results, ...
 * @tparam Y Type of the data handled by the ODE solver.
 * @tparam System Type of the callable class that can return the RHS of the ODE.
 * @param y_run Object to be computed by the ODE solver
 * @param rhs Callable class that can return the RHS of the ODE.
 * @param x_run Flowing variable defining the ODE.
 * @param x_vals Values of the flowing variable.
 * @param iteration Iteration number.
 * @param filename Filename to be used for potential output.
 * @param config Set of parameters for the ODE solver.
 * @param verbose If true, additional information is printed into the log file.
 */
template <typename Y, typename System>
void postRKstep_stuff(Y& y_run, System& rhs, double x_run, const vec<double>& x_vals, int iteration, const std::string& filename, const ODE_solver_config& config, bool verbose) {
    if constexpr (std::is_same_v<Y, State<state_datatype>>) {
        y_run.Lambda = x_run;
        check_SE_causality(y_run); // check if the self-energy is causal at each step of the flow
        if constexpr(KELDYSH and (REG!=5)) check_FDTs(y_run, verbose); // check FDTs for Sigma and K1r at each step of the flow

        if (filename != "") {
            const bool is_converged = std::abs(x_run - config.Lambda_f) <= 1e-10 * (1 + std::abs(config.Lambda_f));
            add_state_to_hdf(filename, iteration + 1, y_run, is_converged); // save result to hdf5 file
        }
        #ifdef ADAPTIVE_GRID
                y_run.findBestFreqGrid(true);
            y_run.analyze_tails();
            y_run.vertex.half1().check_vertex_resolution();
            if (filename != "") {
                add_state_to_hdf(filename+"_postOpt", iteration + 1,  y_run); // save result to hdf5 file
            }
        #else
            y_run.update_grid(x_run); // rescales grid with Delta or U
        #endif


        rhs.stats.write_to_hdf(filename, y_run.config, iteration + 1);
        rhs.stats.number_of_SE_iterations = 0;
        rhs.stats.time = 0.;
        rhs.stats.RKattempts = 0;

    }
    else {
        std::cout << "current value: " << y_run << std::endl;
    }

}




/**
 * Explicit RK4 using non-constant step-width determined by substitution, allowing to save state at each Lambda step.
 * Allows for checkpointing: If last parameter it_start is given, ODE solver starts at this iteration (to continue
 * previously canceled computation). If it_start is not given (see overload below), full flow is computed.
 * @tparam T        : type of object to be integrated (usually State, will currently only work for state, since
 *                    result is saved using functions from hdf5_routines.h, which only support State)
 * @param y_fin     : reference to object into which result is stored (final State)
 * @param x_fin     : final value of the integration variable (final Lambda)
 * @param y_ini     : initial value of the integrated object (initial State)
 * @param x_ini     : initial value of the integration variable (initial Lambda)
 * @param rhs       : right hand side of the flow equation to be solved
 * @param subst     : substitution to generate non-equidistant flow grid
 * @param resubst   : resubstitution
 * @param N_ODE     : number of ODE steps (will be increased by adding points at interesting Lambda values)
 * @param filename  : output file name
 * @param it_start  : Lambda iteration at which to start solving the flow
 */
template <typename T, typename System>
void ODE_solver_RK4(T& y_fin, const T& y_ini,
                    const System& rhs,
                    const ODE_solver_config& config,
                    double subst(double x), double resubst(double x),
                    const std::vector<double> lambda_checkpoints = {}, std::string filename="", const int it_start=0, bool save_intermediate_states=false) {
    // construct non-linear flow grid via substitution
    rvec x_vals  = flowgrid::construct_flow_grid(config.Lambda_f, config.Lambda_i, subst, resubst, config.maximal_number_of_ODE_steps, lambda_checkpoints);
    rvec x_diffs = flowgrid::flow_grid_step_sizes(x_vals); // compute step sizes for flow grid

        // solve ODE using step sizes x_diffs
        T y_run = y_ini; // initial y value
        double x_run = x_vals[it_start]; // initial x value
        double dx;
        for (int i=it_start; i<x_diffs.size(); ++i) {
            dx = x_diffs[i];

        // perform RK4 step and write result into output file in
        old_ode_solvers::RK4_step(y_run, x_run, dx, rhs, x_vals, filename, i, save_intermediate_states);

        // update frequency grid, interpolate result to new grid
        postRKstep_stuff<T>(y_run, x_run, x_vals, i, filename, config, true);

    }
    y_fin = y_run; // final y value
}



namespace ode_solver_impl
{


    // Use this to define RK methods
    template <size_t stages>
    struct butcher_tableau
    {
        // Runge-Kutta Matrix (the 0th row and the last stage are always 0)
        const std::array<double, (stages - 1) * (stages - 1)> a;
        // weights for the two different orders
        const std::array<double, stages> b_high;
        const std::array<double, stages> b_low;
        // nodes (the 0th node is always 0)
        const std::array<double, stages - 1> c;
        const bool adaptive;
        const std::string name;

        double get_a(size_t row_index, size_t column_index) const
        {
            // 0th row and last column are 0.
            if (row_index == 0 || column_index == stages)
            {
                return 0.;
            }
            assert((row_index - 1) * (stages - 1) + column_index >= 0);
            return a[(row_index - 1) * (stages - 1) + column_index];
        }

        double get_node(size_t stage) const
        {
            if (stage == 0)
            {
                return 0.;
            }

            return c[stage - 1];
        }

        double get_error_b(size_t stage) const
        {
            return b_high[stage] - b_low[stage];
        }
    };

    // Cash, J. R., & Karp, A. H. (1990). A variable order Runge-Kutta method for initial value problems with rapidly varying right-hand sides. ACM Transactions on Mathematical Software, 16(3), 201–222. https://doi.org/10.1145/79505.79507
    const butcher_tableau<6> cash_carp{
            // a (Runge-Kutta Matrix)
            .a = {1. / 5.,           0.,             0.,             0.,                 0.,
                  3. / 40.,          9. / 40.,       0.,             0.,                 0.,
                  3. / 10.,          -9. / 10.,      6. / 5.,        0.,                 0.,
                  -11. / 54.,        5. / 2.,        -70. / 27.,     35. / 27.,          0.,
                  1631. / 55296.,    175. / 512.,    575. / 13824.,  44275. / 110592.,   253. / 4096.},
            // b (weights) for the 5th order solution
            .b_high = {37. / 378., 0., 250. / 621., 125. / 594., 0., 512. / 1771.},
            // b (weights) for the 4th order solution
            .b_low = {2825. / 27648., 0., 18575. / 48384., 13525. / 55296., 277. / 14336., 1. / 4.},
            // c (nodes)
            .c = {1. / 5., 3. / 10., 3. / 5., 1., 7. / 8.},
            .adaptive = true,
            .name = "Cash-Carp"
    };

    const butcher_tableau<4> RK4basic{
            // a (Runge-Kutta Matrix)
            .a = {1. / 2.,     0.,          0.,
                  0.,          1. / 2.,     0.,
                  0.,          0.,          1.},
            // b (weights) for the 4th order solution
            .b_high = {1./6., 1./3., 1./3., 1./6.},
            // b (weights) for the 4th order solution
            .b_low = {1./6., 1./3., 1./3., 1./6.},
            // c (nodes)
            .c = {1./2, 1./2., 1.},
            .adaptive = false,
            .name = "basic Runge-Kutta 4"
    };



    const butcher_tableau<4> BogaSha{
            // a (Runge-Kutta Matrix)
            .a = {1. / 2.,     0.,     0.,
                  0.,          3. / 4.,0.,
                  2./9.,       1./3.,  4./9.},
            // b (weights) for the 3rd order solution
            .b_high = {2./9., 1./3., 4./9., 0.},
            // b (weights) for the 2nd order solution
            .b_low = {7./24., 1./4., 1./3., 1./8.},
            // c (nodes)
            .c = {1./2., 3./4., 1.},
            .adaptive = true,
            .name = "Bogacki–Shampine"
    };



    /**
     * Runge Kutta solver for methods that can be represented by a butcher tableau
     * @tparam Y                datatype of y, in fRG: State<Q>
     * @tparam FlowGrid         used to determine Lambdas from t (where t is the x used by the (adaptive) ODE solver)
     * @tparam stages           number of stages for the Runge-Kutta method specified in tableau
     * @param tableau           butcher tableau
     * @param y_init            initial state
     * @param result            (returned) result of Runge-Kutte step
     * @param t_value           gives initial x via FlowGrid
     * @param t_step            step size in x
     * @param maxrel_error      (returned) maximal relative deviation between 5-point and 4-point rule
     * @param rhs               function that computes the right-hand side of the flow equation
     */
    template <typename Y, typename FlowGrid, size_t stages, typename System>
    void rk_step(const butcher_tableau<stages> &tableau, const Y &y_init, const Y &dydx,
                   Y &result, const double t_value, const double t_step, double &maxrel_error, const System& rhs, const ODE_solver_config& config)
    {
        Y state_stage=y_init; // temporary state
        //
        vec<Y> k; // stores the pure right-hand-sides ( without multiplication with an step size )

        k.reserve(stages);
        k.push_back( dydx
#ifdef REPARAMETRIZE_FLOWGRID
        * FlowGrid::dlambda_dt(t_value)
#endif
        );


        double Lambda_stage;
        double t_stage, stepsize;
        // Start at 1 because 0th stage is already contained in dydx parameter
        for (size_t stage = 1; stage < stages; stage++)
        {
#ifdef REPARAMETRIZE_FLOWGRID
            t_stage = t_value + t_step* tableau.get_node(stage);
            Lambda_stage = FlowGrid::lambda_from_t(t_stage);
            stepsize = t_step;
            //utils::print("\t current t_stage: ", t_stage, "\n" );
#else
            const double Lambda = FlowGrid::lambda_from_t(t_value);
            double dLambda = FlowGrid::lambda_from_t(t_value + t_step) - Lambda;
            Lambda_stage = Lambda + dLambda * tableau.get_node(stage);
            stepsize = dLambda;
#endif

            state_stage = y_init;
            for (size_t col_index = 0; col_index < stage; col_index++)
            {
                double factor = stepsize * tableau.get_a(stage, col_index);
                state_stage += k[col_index] * factor;
            }

            Y dydx_temp;
            rhs(state_stage, dydx_temp, Lambda_stage)
            ;
            k.push_back( dydx_temp
#ifdef REPARAMETRIZE_FLOWGRID
                    * FlowGrid::dlambda_dt(t_stage)
#endif
            );
        }

        result = y_init;
        for (size_t stage = 0; stage < stages; stage++)
        {
            result += k[stage] * stepsize * tableau.b_high[stage];
        }

        Y err = k[0] * stepsize * tableau.get_error_b(0);
        for (size_t stage = 1; stage < stages; stage++)
        {
            err += k[stage] * stepsize * tableau.get_error_b(stage);
        }
        Y y_scale = (abs(result) * config.a_State + abs(dydx*stepsize) * config.a_dState_dLambda) * config.relative_error + config.absolute_error;
        maxrel_error = max_rel_err(err, y_scale); // alternatively state yscal = abs_sum_tiny(integrated, h * dydx, tiny);
        if (VERBOSE) utils::print("ODE solver error estimate: ", maxrel_error, "\n");
    }

/**
 *
 * @tparam FlowGrid
 * @param state_i
 * @param Lambda_i
 * @param htry
 * @param hdid
 * @param hnext
 * @param min_t_step
 * @param max_t_step
 * @param lambda_checkpoints
 * @param rhs
 */
    template <typename Y, typename FlowGrid, size_t stages, typename System>
    void rkqs(butcher_tableau<stages> tableau, Y &state_i, double &Lambda_i, double htry, double &hdid, double &hnext,
              double min_t_step, double max_t_step,
              const System& rhs, size_t iteration, const ODE_solver_config& config, const bool verbose)
    {


        // Safety intentionally chosen smaller than in Numerical recipes, because the step
        // size estimates were consistently too large and repeated steps are very expensive
        const double SAFETY = 0.8, PGROW = -0.2, PSHRINK = -0.25, MAXGROW = 2.;

        int world_rank = mpi_world_rank();


        const double t_value = FlowGrid::t_from_lambda(Lambda_i);
        double t_step = FlowGrid::t_from_lambda(Lambda_i + htry) - t_value;
        Y dydx;
        rhs(state_i, dydx, Lambda_i); // const State_t& state_in, State_t& dState_dt, double Lambda_in
        bool rejected = false;
        double errmax;
        unsigned int attempts = 0;

        //const auto &lattice = state_i.vertex.lattice;
        Y temporary = state_i;
        for (;;)    // infinite loop
        {
            // === Evaluation ===
            if (verbose and world_rank == 0)
            {
                utils::print("Try stepsize t ", t_step, " (from Lambda = ", Lambda_i
                          , " to ", FlowGrid::lambda_from_t(t_value + t_step)
                          , ").\n");
                utils::print("Current t: ", t_value, "\n");
            };
            ode_solver_impl::rk_step<Y, FlowGrid>(tableau, state_i, dydx, temporary, t_value, t_step, errmax, rhs, config);

            if constexpr(std::is_same<State<state_datatype>, Y>::value) {
                rhs.rk_step = 1;
                rhs.stats.RKattempts += 1;
            }

            if (not tableau.adaptive) break;



            if (verbose and world_rank == 0)
            {
                utils::print("Just finished ODE step attempt \t ---> \t errmax/tolerance (ODE): ", errmax, "\n");
                if (std::abs(t_step) <= min_t_step)
                {
                    std::cout << "Step was taken with minimal step size " << -min_t_step
                              << "(from Lambda / J = " << Lambda_i
                              << " to " << FlowGrid::lambda_from_t(t_value + t_step)
                              << ") and will be accepted regardless of the error." << std::endl;
                }
            }

            // accept result
            // if step was minimal step size, accept step in any case. This keeps program from crashing
            if (errmax <= 1. || std::abs(t_step) <= min_t_step)
            {
                break;
            }
            if (attempts > config.max_stepResizing_attempts and tableau.adaptive) {
                utils::print("ODE solver reached maximal number of stepResizing attempts.");
                break;
            }

            // === Resize t_step ===

            if (verbose and world_rank == 0)
            {
                utils::print("Stepsize too big. Readjust..\n");
            }

            rejected = true;
            const double t_step_resized = SAFETY * t_step * pow(errmax, PSHRINK);
            t_step = sgn(t_step) * std::max({std::abs(t_step_resized), 0.1 * std::abs(t_step), min_t_step});

            if (t_value + t_step == t_value)
            {
                // At this point, recovery is impossible. Emergency abort.
                std::stringstream s;
                s << "Fatal: Stepsize underflow in adaptive Runge-Kutta routine rkqs. Current value of t is "
                  << t_value << "and the desired step size is " << t_step
                  << ". It is likely that the chosen value of t_step_min is too small. Will now abort.";
                std::cout << s.str();
                throw std::runtime_error(s.str());
            }
        }

        double t_next_step;
        if (errmax > std::pow(MAXGROW/SAFETY, PGROW))
        {
            t_next_step = SAFETY * t_step * std::pow(errmax, PGROW);

            //assert(t_next_step>=0);
        }
        else
        {
            t_next_step = MAXGROW * t_step;

            //assert(t_next_step>=0);
        }

        // Don't increase step size immediately after rejecting a step; further shrinking is ok
        if (rejected)
        {
            t_next_step = sgn(t_step) * std::min(std::abs(t_next_step), std::abs(t_step));
            //assert(t_next_step>=0);
        }

        // clip to interval [min_t_step, max_t_step]
        t_next_step = sgn(t_next_step) * std::min(std::max(min_t_step, std::abs(t_next_step)), max_t_step);

        // === Update output ref's ===
        Lambda_i = FlowGrid::lambda_from_t(t_value + t_step);
        hdid = Lambda_i - FlowGrid::lambda_from_t(t_value);
        hnext = FlowGrid::lambda_from_t(t_value + t_step + t_next_step) - Lambda_i;
        state_i = temporary;
        //assert(t_next_step>=0);
    }

} // namespace ode_solver_impl


/**
 * ODE solver, by default implementing a fourth-order Runge-Kutta (Cash-Karp) algorithm.
 * (Other options are possible but not recommended!)
 * @tparam Y Type of the data to be handled by the solver. Can be double, comp, State, ...
 * @tparam FlowGrid Suggests a set of step sizes:
 * - for non-adaptive rules, these are used directly --> lambdas_try
 * - for adaptive rules, FlowGrid only approximately "guides" the step sizes;
 *   e.g. for FlowGrid::exp_parametrization we have Lambda(t) = exp(-t) such that equal step sizes in t lead to
 *   exponentially decaying step sizes. Adaptive rules can grow or shrink the step sizes in terms of t!
 * @tparam System
 * @param result Final state.
 * @param state_ini Initial state of type Y.
 * @param rhs Callable instance of a class used to implement the RHS of the differential equation.
 * @param config Config struct holding all relevant parameters for the ODE.
 * @param verbose If true, additional information is printed into the log file. Recommendation: true.
 */
template <typename Y, typename FlowGrid = flowgrid::sqrt_parametrization, typename System
        >
void ode_solver(Y& result, const Y& state_ini, const System& rhs,
                const ODE_solver_config& config=ODE_solver_config(), const bool verbose=true) {
    int world_rank = mpi_world_rank();


    // quality-controlled RK-step (cf. Numerical recipes in C, page 723)

    const auto tableau = ode_solver_impl::cash_carp;
    if (verbose and world_rank == 0) {
        const std::string message =
                "\n-------------------------------------------------------------------------\n\tStarting ODE solver with method: " + tableau.name + " \n"
               +"-------------------------------------------------------------------------\n";
        std::cout << message;
    }


    const unsigned int MAXSTP = config.maximal_number_of_ODE_steps + config.lambda_checkpoints.size(); //maximal number of steps that is computed
    vec<double> lambdas (MAXSTP+1); // contains all lambdas (including starting point)
    lambdas[0] = config.Lambda_i;
    if constexpr (std::is_same_v<Y,State<state_datatype>>) {
        /// load lambdas from file to continue ODE solver
        if (config.iter_start > 0) {
            H5::H5File file_out(config.filename, H5F_ACC_RDONLY);
            read_from_hdf<double>(file_out, LAMBDA_LIST, lambdas);
            file_out.close();
        }
    }

    // get lambdas according to FlowGrid (for hybridization flow: + checkpoints acc. to U_NRG  ) -> for non-adaptive method
    const vec<double> lambdas_try = flowgrid::construct_flow_grid(config.Lambda_f, config.Lambda_i, FlowGrid::t_from_lambda, FlowGrid::lambda_from_t, config.maximal_number_of_ODE_steps, tableau.adaptive ? std::vector<double>() : config.lambda_checkpoints);

    const double max_t_step = 1e1;  // maximal step size in terms of t
    const double min_t_step = 1e-5; // minimal step size in terms of t

    double Lambda = config.Lambda_now;    // step size to try (in terms of Lambda)
    double h_try;
    if (tableau.adaptive and config.iter_start > 0) {
        /// next: try an equal stepsize in terms of the reparametrized flow parameter t
        double lambdas_0 = lambdas[config.iter_start-1];
        double lambdas_1 = lambdas[config.iter_start];
        double htry_last_reparametrized = FlowGrid::t_from_lambda(lambdas_1) - FlowGrid::t_from_lambda(lambdas_0);
        htry_last_reparametrized = std::min(std::abs(htry_last_reparametrized), max_t_step) ;
        htry_last_reparametrized = std::max(std::abs(htry_last_reparametrized), min_t_step) ;
        h_try = FlowGrid::lambda_from_t(FlowGrid::t_from_lambda(Lambda) + htry_last_reparametrized) - Lambda;
    }
    else {
        h_try = lambdas_try[config.iter_start + 1]-lambdas_try[config.iter_start];
    }

    double hnext, hdid, h_try_prev{}; // step size to try next; actually performed step size
    bool just_hit_a_lambda_checkpoint = false;
    result = state_ini;

    for (unsigned int i = config.iter_start; i < MAXSTP; i++)
    {
        if (verbose and world_rank == 0)
        {
            std::cout <<"-------------------------------------------------------------------------\n";
            utils::print("Now do ODE step number \t\t", i, true);
            utils::print("Lambda: ", Lambda, true);
        };
        if constexpr(std::is_same<State<state_datatype>, Y>::value) {
            rhs.iteration = i;
        }

        //if next step would get us outside the interval [Lambda_f, Lambda_i]
        if ((Lambda + h_try - config.Lambda_f) * (Lambda + h_try - config.Lambda_now) > 0.0)
        {
            // if remaining Lambda step is negligibly small
            if (std::abs(config.Lambda_f - Lambda) / config.Lambda_f < 1e-8)
            {
                if (verbose and world_rank == 0)
                {
                    std::cout << "Final Lambda=Lambda_f reached. Program terminated." << std::endl;
                };
                break;
            }
            else
            {
                h_try = config.Lambda_f - Lambda;
            };
        };

        if (just_hit_a_lambda_checkpoint) {h_try = std::max(h_try_prev, h_try_prev - h_try);} // jump to previously suggested Lambda_next if we just hit a Lambda checkpoint
        h_try_prev = h_try; // remember h_try from this iteration in case we hit a Lambda checkpoint
        just_hit_a_lambda_checkpoint = false;

        // === Checkpoints ===
        const double Lambda_next = config.Lambda_now + h_try;
        for (const double checkpoint : config.lambda_checkpoints)
        {
            // Guard against float arithmetic fails
            if ((config.Lambda_now - checkpoint) * (Lambda_next - checkpoint) < -1e-10 and tableau.adaptive)
            {
                h_try = checkpoint - config.Lambda_now;
                just_hit_a_lambda_checkpoint = true;
                break;
            }
        }

        // fix step size for non-adaptive methods
        if (not tableau.adaptive) h_try = lambdas_try[i+1] - lambdas_try[i];
        ode_solver_impl::rkqs<Y, FlowGrid>(tableau, result, Lambda, h_try, hdid, hnext, min_t_step, max_t_step, rhs, i, config, verbose);
        // Pick h_suggested and set Lambda_i for next iteration
        if (tableau.adaptive) {
            config.Lambda_now += hdid;
            if (just_hit_a_lambda_checkpoint){h_try = hdid;} else {h_try = hnext;}
        }



        lambdas[i+1] = Lambda;

        // if Y == State: save state in hdf5
        postRKstep_stuff<Y>(result, rhs, Lambda, lambdas, i, config.filename, config, verbose);



    };
}



#include <boost/numeric/odeint/integrate/integrate_adaptive.hpp>
#include <boost/numeric/odeint/integrate/detail/integrate_adaptive.hpp>
#include <boost/numeric/odeint.hpp>

namespace boost {
    namespace numeric {
        namespace odeint {
            namespace detail {


                template<class Stepper, typename State_t, typename FlowGrid, class System>
                State_t integrate_nonadaptive(
                        Stepper stepper, System system, State_t &start_state,
                        double &t_now, double t_final, const vec<double> &lambdas_try, vec<double> &lambdas_did,
                        const ODE_solver_config& config, const bool verbose
                )
                {
                    size_t integration_step_count = config.iter_start;
                    double dt = 1e-10*sgn(t_final - t_now);
                    double dLambda;



                    while( less_with_sign( t_now, t_final, dt ) )
                    {
                        if( less_with_sign( t_final, t_now + dt, dt ) )
                        {
                            dt = t_final - t_now;
                        }
                        if (verbose and mpi_world_rank()==0) {
                            utils::print("ODE iteration number: \t", integration_step_count, "\n");
                        }

                        dt = FlowGrid::t_from_lambda(lambdas_try[integration_step_count + 1]) - t_now; // current step size
                        dLambda = lambdas_try[integration_step_count + 1] - lambdas_try[integration_step_count]; // current step size
                            //utils::print("t_now: ", t_now, " -- t_final: ", t_final, " -- dt: ", dt, "\n");

#ifdef REPARAMETRIZE_FLOWGRID
                        if (verbose and mpi_world_rank()==0) { utils::print("current flow parameter t_now: ", t_now, " -- t_final: ", t_final, " -- try step size dt: ", dt, "\n"); }

                        auto rhs = [&system](const State_t& state_in, State_t& dState_dt, double t) -> void {system(state_in, dState_dt, FlowGrid::lambda_from_t(t)); dState_dt *= FlowGrid::dlambda_dt(t);};
                        stepper.do_step( rhs, start_state, t_now, dt);
#else
                        double Lambda_now = FlowGrid::lambda_from_t(t_now);
                        double dLambda = FlowGrid::lambda_from_t(t_now+dt) -  Lambda_now;
                        if (verbose and mpi_world_rank()==0) { utils::print("current flow parameter Lambda_now: ", Lambda_now, " -- Lambda_final: ", FlowGrid::lambda_from_t(t_final), " -- try step size dLambda: ", dLambda, "\n"); }
                        //if (verbose and mpi_world_rank()==0) { utils::print("current flow parameter t_now: ", t_now, " -- t_final: ", t_final, " -- try step size dt: ", dt, "\n"); }

                        auto rhs = [&system](const State_t& state_in, State_t& dState_dt, double Lambda_in) -> void {system(state_in, dState_dt, Lambda_in);};
                        stepper.do_step( rhs, start_state, Lambda_now, dLambda);  // if successful: updates Lambda_now and dt; if failed: updates only dt
#endif
                        t_now = FlowGrid::t_from_lambda(lambdas_try[integration_step_count+1]);   // t in next step

                        lambdas_did[integration_step_count+1] = FlowGrid::lambda_from_t(t_now);
                        postRKstep_stuff<State_t>(start_state, FlowGrid::lambda_from_t(t_now), lambdas_did, integration_step_count, config.filename, config, verbose);

                        if constexpr(std::is_same<State<state_datatype>, State_t>::value) {
                            system.rk_step = 0;
                            system.iteration++;
                        }

                        ++integration_step_count;
                    }

                    if (verbose and mpi_world_rank()==0) {
                        utils::print(" ODE solver finished with ", integration_step_count, " integration steps.", "\n\n");
                    }
                    return start_state;
                }

                template<class Stepper, typename State_t, typename FlowGrid, class System>
                State_t integrate_adaptive_check(
                        Stepper stepper, System system, State_t &start_state,
                        double &t_now, double t_final, double &dt, const size_t MAXSTP,
                         vec<double> &lambdas_did,
                         const ODE_solver_config& config, const bool verbose
                )
                {
                    if (verbose and mpi_world_rank()==0) {
                        utils::print("Initial t_now: ", t_now, " -- t_final: ", t_final, " -- dt: ", dt, "\n");
                    }

                    const char *error_string = "Integrate adaptive : Maximal number of iterations reached. A step size could not be found.";
                    size_t integration_step_count = config.iter_start;
                    bool previously_hit_lambda_checkpoint = false;
                    double dt_temp;
                    while( less_with_sign( t_now, t_final, dt ))
                    {
                        if(previously_hit_lambda_checkpoint) dt = dt_temp;  // try the old step size if the step size of decreased due to hitting a Lambda checkpoint:
                        if( less_with_sign( t_final, t_now + dt, dt ) ) //
                        {
                            dt = t_final - t_now;
                        }
                        double Lambda_now = FlowGrid::lambda_from_t(t_now);
                        double Lambda_next= FlowGrid::lambda_from_t(t_now+dt);
#ifndef REPARAMETRIZE_FLOWGRID
                        double dLambda = Lambda_next - Lambda_now;
#endif
                        /// step to lambda checkpoint if within reach
                        for (const double checkpoint : config.lambda_checkpoints)
                        {
                            // Guard against float arithmetic fails
                            if ((Lambda_now - checkpoint) * (Lambda_next - checkpoint) < -1e-14 ) // are we crossing a lambda checkpoint? If yes -> hit lambda checkpoint
                            {
                                previously_hit_lambda_checkpoint = true;
                                dt_temp = dt;
                                dt = FlowGrid::t_from_lambda(checkpoint) - FlowGrid::t_from_lambda(Lambda_now);
                                break;
                            }
                        }

                        if (verbose and mpi_world_rank()==0) {
                            utils::print("ODE iteration number: \t", integration_step_count, "\n");
                        }

                        unsigned int trials = 0;
                        controlled_step_result res = success;
                        do
                        {
                            #ifdef REPARAMETRIZE_FLOWGRID
                                double dt_old = dt;
                                if (verbose and mpi_world_rank()==0) { utils::print("current flow parameter t_now: ", t_now, " -- t_final: ", t_final, " -- try step size dt: ", dt, "\n"); }
                                auto rhs = [&system](const State_t& state_in, State_t& dState_dt, double t) -> void {system(state_in, dState_dt, FlowGrid::lambda_from_t(t)); dState_dt *= FlowGrid::dlambda_dt(t);};
                                res = stepper.try_step( rhs, start_state, t_now, dt);
                                if (verbose and mpi_world_rank()==0) utils::print( (res == fail ? "ODE step FAILED -- new dt: " + std::to_string(dt) : "ODE step PASSED with step size dt = " + std::to_string(dt_old)), "\n");
                            #else

                                double t_now_temp = t_now;
                                double dLambda_old = dLambda;
                                if (verbose and mpi_world_rank()==0) { utils::print("current flow parameter Lambda_now: ", Lambda_now, " -- Lambda_final: ", FlowGrid::lambda_from_t(t_final), " -- try step size dLambda: ", dLambda, "\n"); }

                                auto rhs = [&system](const State_t& state_in, State_t& dState_dt, double Lambda_in) -> void {system(state_in, dState_dt, Lambda_in);};
                                res = stepper.try_step( rhs, start_state, Lambda_now, dLambda);  // if successful: updates Lambda_now and dt; if failed: updates only dt
                                if (res == success) {
                                    // update t_now and dt for next step
                                    t_now = FlowGrid::t_from_lambda(Lambda_now);
                                    dt = FlowGrid::t_from_lambda(std::max(1e-15, Lambda_now + dLambda)) - t_now;
                                }

                                //Lambda_next = Lambda_now + dLambda;
                                if (verbose and mpi_world_rank()==0) utils::print( (res == fail ? "ODE step FAILED -- new dLambda: " + std::to_string(dLambda) : "ODE step PASSED with step size dLambda = " + std::to_string(dLambda_old)), "\n");
                            #endif

                            if constexpr(std::is_same<State<state_datatype>, State_t>::value) {
                                system.rk_step = 0;
                            }

                            ++trials;
                        }
                        while( ( res == fail ) && ( trials < config.max_stepResizing_attempts ) );
                        if( trials == config.max_stepResizing_attempts ) throw std::overflow_error(error_string );

                        lambdas_did[integration_step_count+1] = FlowGrid::lambda_from_t(t_now);
                        postRKstep_stuff<State_t>(start_state, system, FlowGrid::lambda_from_t(t_now), lambdas_did, integration_step_count, config.filename, config, verbose);
                        if constexpr(std::is_same<State<state_datatype>, State_t>::value) {
                            //utils::print("I'M A STATE!\n\n");
                            system.iteration = integration_step_count+1;
                        }
                        //else utils::print("I'M NOT A STATE!\n\n");

                        ++integration_step_count;
                        if(integration_step_count >= MAXSTP) {
                            if (mpi_world_rank()==0) utils::print("ODE solver reached maximal number of steps.");
                            break;
                        }
                    }
                    if (mpi_world_rank()==0) utils::print(" ODE solver finished with ", integration_step_count, " integration steps.\n\n");
                    return start_state;
                }

            } // Namespace detail

            template<typename State_t, typename FlowGrid, typename System>
            void ode_solver_boost //integrate_adaptive_check
                    (State_t& result, const State_t& state_ini, const System& rhs ,
                     const ODE_solver_config& config=ODE_solver_config(), const bool verbose=true)
            {

#define ERR_STEPPER runge_kutta_cash_karp54

                //double Lambda_now = Lambda_i;
                double t_now = FlowGrid::t_from_lambda(config.Lambda_i);
                double t_final = FlowGrid::t_from_lambda(config.Lambda_f);
                State_t state_now = state_ini;
                const size_t MAXSTP = config.maximal_number_of_ODE_steps + config.lambda_checkpoints.size(); //maximal number of steps that is computed
                // get lambdas according to FlowGrid (for hybridization flow: + checkpoints acc. to U_NRG  ) -> for non-adaptive method
                vec<double> lambdas_did(MAXSTP+1);
                lambdas_did[0] = config.Lambda_i;

                typedef ERR_STEPPER< State_t, double, State_t, double, boost::numeric::odeint::vector_space_algebra > error_stepper_t;
                typedef controlled_runge_kutta< error_stepper_t > controlled_error_stepper_t;
                controlled_error_stepper_t stepper(
                        default_error_checker< double , vector_space_algebra , default_operations >( config.absolute_error , config.relative_error , config.a_State , config.a_dState_dLambda ) );


                double dt = (config.Lambda_f - config.Lambda_i)*dLambda_initial > 1e-15 ? FlowGrid::t_from_lambda(config.Lambda_i + dLambda_initial) - FlowGrid::t_from_lambda(config.Lambda_i) : (FlowGrid::t_from_lambda(config.Lambda_f) - FlowGrid::t_from_lambda(config.Lambda_i)) / ((double) MAXSTP);
                result = detail::integrate_adaptive_check<controlled_error_stepper_t, State_t, FlowGrid>(
                        stepper, rhs, state_now,
                        t_now, t_final, dt, MAXSTP, lambdas_did, config, verbose);
            }

        } // Namespace odeint
    } // Namespace numeric
} // Namespace boost


#endif //KELDYSH_MFRG_SOLVERS_H
