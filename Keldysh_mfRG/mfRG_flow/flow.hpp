/**
 * Initialize and run the fRG flow.
 */

#ifndef KELDYSH_MFRG_FLOW_HPP
#define KELDYSH_MFRG_FLOW_HPP

#include <string>                // for file name for saving result
#include "../parameters/master_parameters.hpp"          // system parameters (e.g. initial Lambda)
#include "../correlation_functions/state.hpp"               // state including vertex and self-energy
#include "../perturbation_theory_and_parquet/perturbation_theory.hpp" // for initialization with SOPT at the beginning of the flow, using sopt_state
#include "../grids/flow_grid.hpp"     // for flow grid
#include "../ODE_solvers/ODE_solvers.hpp"         // for ODE solver (Runge Kutta 4)
#include "right_hand_sides.hpp"    // to compute right hand side of flow equation
#include "../perturbation_theory_and_parquet/parquet_solver.hpp"      // to compute the parquet solution as alternative starting point of the flow
#include "../postprocessing/postprocessing.hpp"      // to check the fulfillment of vertex sum-rules at the end of the flow
#include "../utilities/hdf5_routines.hpp"       // file management

/**
 * Compute n-loop flow, with number of loops specified by N_LOOPS in parameters.h.
 * Initialize the flow with second order PT at Lambda_ini, compute the flow with RK4 ODE solver up to Lambda_fin.
 */
State<state_datatype> n_loop_flow(std::string outputFileName, bool save_intermediate_results=false){

    State<state_datatype> state_fin (Lambda_fin), state_ini(Lambda_ini);   // create final and initial state


    /// Iterate:
    /// 1. compute parquet solution
    /// 2. optimize grid
    /// 3. restart with optimized grid
    for (int i = 0; i < 1; i++) {

        State<state_datatype> state_temp = state_ini;
        state_temp.initialize();             // initialize state with bare vertex and Hartree term in selfenergy
        // initialize the flow with SOPT at Lambda_ini (important!)
        sopt_state(state_temp, Lambda_ini);
        // TODO(high): For the Hubbard model, compute the SOPT contribution to the self-energy via FFTs and worry about loops later...

        parquet_solver(data_dir + "parqueInit4_temp" + std::to_string(i) + "_n1=" + std::to_string(nBOS) + "_n2=" + std::to_string(nBOS2) + "_n3=" + std::to_string(nBOS3) + ".h5", state_temp, Lambda_ini, 1e-4, 2);

        state_temp.vertex.half1().check_vertex_resolution();
        state_temp.findBestFreqGrid(true);
        state_temp.vertex.half1().check_vertex_resolution();

        state_ini.set_frequency_grid(state_temp); // copy frequency grid
    }


    state_ini.initialize();     // initialize state with bare vertex and Hartree term in selfenergy
    // initialize the flow with SOPT at Lambda_ini (important!)
    sopt_state(state_ini, Lambda_ini);

    parquet_solver(data_dir + "parqueInit4_final_n1=" + std::to_string(nBOS) + "_n2=" + std::to_string(nBOS2) + "_n3=" + std::to_string(nBOS3) + ".h5", state_ini, Lambda_ini);


    //// better: read state from converged parquet solution
    //state_ini = read_hdf(data_dir + "parqueInit4_n1=" + std::to_string(nBOS) + "_n2=" + std::to_string(nBOS2) + "_n3=" + std::to_string(nBOS3) + ".h5", 4, 51);
    //state_ini.selfenergy.asymp_val_R = glb_U / 2.;


    write_hdf(outputFileName, Lambda_ini,  nODE + U_NRG.size() + 1, state_ini);  // save the initial state to hdf5 file
    state_ini.vertex.half1().check_vertex_resolution();
    state_ini.findBestFreqGrid(true);
    state_ini.vertex.half1().check_vertex_resolution();
    write_hdf(outputFileName+"_postOpt", Lambda_ini,  nODE + U_NRG.size() + 1, state_ini);  // save the initial state to hdf5 file

    //if (save_intermediate_results) {
    //    write_hdf(outputFileName+"_RKstep1", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);
    //    write_hdf(outputFileName+"_RKstep2", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);
    //    write_hdf(outputFileName+"_RKstep3", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);
    //    write_hdf(outputFileName+"_RKstep4", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);  // save the initial state to hdf5 file
    //}


    compare_with_FDTs(state_ini.vertex, Lambda_ini, 0, outputFileName, true, nODE + U_NRG.size() + 1);

    std::vector<double> Lambda_checkpoints = flowgrid::get_Lambda_checkpoints(U_NRG);

    // compute the flow using an ODE solver
    ode_solver<State<state_datatype>, flowgrid::sqrt_parametrization>(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_n_loop_flow,
                              Lambda_checkpoints, outputFileName);


    return state_fin;
}

/**
 * Checkpointing: Continue to compute an n-loop flow that has been canceled before, e.g. due to running into the wall
 * time. For given iteration it_start, read the state at this iteration from previously computed results, then continue
 * to compute the flow up to Lambda_fin.
 *
 * Usage: Check the number <Nmax> of the last Lambda layer of a given file <inputFileName> that has been successfully
 *        computed. (See log file: "Successfully saved in hdf5 file: <inputFileName> in Lambda layer <Nmax>.)
 *        Use this number <Nmax> as input <it_start> for this function.
 */
State<state_datatype> n_loop_flow(std::string inputFileName, const int it_start, bool save_intermediate_results=false) {
    if (it_start < nODE + U_NRG.size() + 1) { // start iteration needs to be within the range of values

        State<state_datatype> state_ini = read_hdf(inputFileName, it_start); // read initial state
        State<state_datatype> state_fin (Lambda_fin);
        if (save_intermediate_results) {
            write_hdf(inputFileName+"_RKstep1", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);
            write_hdf(inputFileName+"_RKstep2", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);
            write_hdf(inputFileName+"_RKstep3", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);
            write_hdf(inputFileName+"_RKstep4", 0*Lambda_ini, nODE + U_NRG.size() + 1, state_ini);  // save the initial state to hdf5 file
        }
        state_ini.findBestFreqGrid(true);
        state_ini.vertex.half1().check_vertex_resolution();

        std::vector<double> Lambda_checkpoints = flowgrid::get_Lambda_checkpoints(U_NRG);

        compare_with_FDTs(state_ini.vertex, Lambda_ini, 0, inputFileName, true, nODE + U_NRG.size() + 1);

        // compute the flow using RK4 solver
        //ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_n_loop_flow,   // use one-loop-flow rhs
        //               flowgrid::sq_substitution, flowgrid::sq_resubstitution,      // use substitution for Lambda steps
        //               nODE, Lambda_checkpoints,
        //               inputFileName,                           // save state at each step during flow
        //               it_start, save_intermediate_results);                               // start from iteration it_start
        ode_solver<State<state_datatype>, flowgrid::sqrt_parametrization>(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_n_loop_flow,
                Lambda_checkpoints, inputFileName, it_start);

        return state_fin;
    }
    else {
        print("Error: Start iteration is too large.", true);
    }
}


#endif //KELDYSH_MFRG_FLOW_HPP