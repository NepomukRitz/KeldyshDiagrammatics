
#include <bits/stdc++.h>
#include <iostream>          // text input/output
#include <sys/stat.h>
#include "../../parameters/master_parameters.hpp"
#include "saveIntegrand.hpp"

auto main(int argc, char * argv[]) -> int {
    if (MPI_FLAG) {
        MPI_Init(nullptr, nullptr);
    }
    std::string job = "U=" + std::to_string(glb_U);
    data_dir = "../Data_KF" + job + "/";
    std::string dir_str;
    char channel;
    int it_Lambda, k_class_int, rkStep, i0, i2, i_in, i_loop;
    double w, v, vp;
    std::cout << "----  Getting integrand  ----" << std::endl;
    std::cout << "number of args: " << argc-1 << ", expected: 12" << std::endl;
    /// Parse input:
    dir_str = argv[1];
    it_Lambda = atoi(argv[2]);
    rkStep = atoi(argv[3]);
    k_class_int = atoi(argv[4]);
    channel = *(argv[5]);
    i0 = atoi(argv[6]);
    i2 = atoi(argv[7]);
    int spin = atoi(argv[8]);
    w = atof(argv[9]);
    v = atof(argv[10]);
    vp = atof(argv[11]);
    i_in = atoi(argv[12]);
    i_loop = atoi(argv[13]);
    K_class k_class = static_cast<K_class>(k_class_int);

    /// print input arguments:
    std::cout << "Check the input arguments: " << std::endl;
    std::cout << "dir_str: " << dir_str << ", it_Lambda: " << it_Lambda << ", k_class_int: " << k_class_int
    << ", channel: " << channel << ", i0: " << i0 << ", i2: " << i2
    << ", w: " << w << ", v: " << v << ", vp: " << vp << ", i_in: " << i_in << ", i_loop: " << i_loop << std::endl;

    dir_str = dir_str + "intermediateResults/";
    std::string file_Psi = dir_str + "Psi_RKstep"+std::to_string(rkStep);
    if (i_loop < 3) assert(false);
    std::string file_dPsi_L = dir_str+"dPsi_L_RKstep"+std::to_string(rkStep)+"_forLoop"+std::to_string(i_loop);
    std::string file_dPsi_R = dir_str+"dPsi_R_RKstep"+std::to_string(rkStep)+"_forLoop"+std::to_string(i_loop);



    std::string dir_integrand_str = "integrands/";
    utils::makedir(data_dir + dir_integrand_str);
    const std::string filename_prefix = dir_integrand_str + "dGammaC_left_insertion_iLambda"+std::to_string(it_Lambda)+"_RKstep"+std::to_string(rkStep) + "_iLoop" + std::to_string(i_loop);
    saveIntegrand::dGamma_C_left_insertion<state_datatype>(filename_prefix, file_Psi, file_dPsi_L, file_dPsi_R, it_Lambda, k_class, channel, i0, i2, w, v, vp, i_in);
    std::cout << "Integrand for dGammaC successfully created." << std::endl;

    if (MPI_FLAG) {
        MPI_Finalize();
    }


    return 0;

}