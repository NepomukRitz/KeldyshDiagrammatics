#include "ODE_solvers.hpp"

template<> void postRKstep_stuff<State<state_datatype>>(State<state_datatype>& y_run, double x_run, vec<double> x_vals, int iteration, std::string filename, const bool verbose) {

    check_SE_causality(y_run); // check if the self-energy is causal at each step of the flow
    if (KELDYSH) check_FDTs(y_run); // check FDTs for Sigma and K1r at each step of the flow
    if (filename != "") {
        add_hdf(filename, iteration + 1, y_run, x_vals); // save result to hdf5 file
    }
    //y_run.update_grid(x_run); // rescales grid with Delta or U
    y_run.findBestFreqGrid(true);
    y_run.analyze_tails();
    y_run.vertex.half1().check_vertex_resolution();
    if (filename != "") {
        add_hdf(filename+"_postOpt", iteration + 1,  y_run, x_vals); // save result to hdf5 file
    }
}

