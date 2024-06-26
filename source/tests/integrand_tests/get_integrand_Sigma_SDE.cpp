
#include <bits/stdc++.h>
#include <iostream>          // text input/output
#include <sys/stat.h>
#include "../../parameters/master_parameters.hpp"
#include "saveIntegrand.hpp"

auto main(int argc, char * argv[]) -> int {
    std::string job;
    job = "_unrotK2";
    data_dir = "../Data_KF" + job + + "_GRID" + std::to_string(GRID) + "/";
    std::string dir_str;
    char channel;
    int it_Lambda, k_class_int, rkStep, i0, i2, i_in;
    double v;
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
    v = atof(argv[9]);
    i_in = atoi(argv[10]);
    K_class k_class = static_cast<K_class>(k_class_int);
    std::string filename = argv[11]; // e.g. "Psi_SDE_a_l";

    /// print input arguments:
    std::cout << "Check the input arguments: " << std::endl;
    std::cout << "dir_str: " << dir_str << ", it_Lambda: " << it_Lambda << ", k_class_int: " << k_class_int
    << ", channel: " << channel << ", i0: " << i0 << ", i2: " << i2
    << ", v: " << v << ", i_in: " << i_in << std::endl;

    //dir_str = dir_str;
    const std::string file_Psi = dir_str + filename;


    std::string dir_integrand_str = "integrands/";
    makedir(data_dir + dir_integrand_str);
    const std::string filename_prefix = dir_integrand_str + "Sigma_SDE";


    saveIntegrand::Sigma_SDE<state_datatype>(filename_prefix, file_Psi, it_Lambda, i2, v, i_in);
    std::cout << "Integrand for dGamma1_loop successfully created." << std::endl;

    return 0;

}