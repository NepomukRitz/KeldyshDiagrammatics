
#include <bits/stdc++.h>
#include <iostream>          // text input/output
#include <sys/stat.h>
#include "../../parameters/master_parameters.hpp"
#include "saveIntegrand.hpp"

auto main(int argc, char * argv[]) -> int {
    std::string job = "U=" + std::to_string(glb_U);
    data_dir = "../Data_KF" + job + "/";
    std::string dir_str;
    char channel;
    int it_Lambda, k_class_int, rkStep, i0, i2, i_in;
    double w, v, vp;
    std::cout << "----  Getting integrand  ----" << std::endl;
    std::cout << "number of args: " << argc-1 << ", expected: 11" << std::endl;
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
    K_class k_class = static_cast<K_class>(k_class_int);

    /// print input arguments:
    std::cout << "Check the input arguments: " << std::endl;
    std::cout << "dir_str: " << dir_str << ", it_Lambda: " << it_Lambda << ", k_class_int: " << k_class_int
    << ", channel: " << channel << ", i0: " << i0 << ", i2: " << i2
    << ", w: " << w << ", v: " << v << ", vp: " << vp << ", i_in: " << i_in << std::endl;

    dir_str = dir_str + "intermediateResults/";
    const std::string file_Psi = dir_str + "Psi_RKstep"+std::to_string(rkStep);
    const std::string file_dPsi= dir_str + "dPsi_RKstep"+std::to_string(rkStep);


    std::string dir_integrand_str = "integrands/";
    utils::makedir(data_dir + dir_integrand_str);
    const std::string filename_prefix = dir_integrand_str + "dGamma1Loop_iLambda"+std::to_string(it_Lambda)+"_RKstep"+std::to_string(rkStep);


    saveIntegrand::dGamma_1Loop<state_datatype>(filename_prefix, file_Psi, file_dPsi, it_Lambda, k_class, channel, i0, i2, spin, w, v, vp, i_in);
    std::cout << "Integrand for dGamma1_loop successfully created." << std::endl;

    return 0;

}