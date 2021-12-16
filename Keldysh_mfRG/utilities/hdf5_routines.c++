#include "hdf5_routines.hpp"

// Create the memory data type for storing complex numbers in file
H5::CompType def_mtype_comp() {
    H5::CompType mtype_comp(sizeof(h5_comp));
    mtype_comp.insertMember(RE, HOFFSET(h5_comp, re), H5::PredType::NATIVE_DOUBLE);
    mtype_comp.insertMember(IM, HOFFSET(h5_comp, im), H5::PredType::NATIVE_DOUBLE);
    return mtype_comp;
}

hsize_t h5_cast(int dim) {
    return static_cast<hsize_t>(dim);
}

rvec read_Lambdas_from_hdf(const H5std_string FILE_NAME){



    // Open the file. Access rights: read-only
    H5::H5File *file = 0;
    file = new H5::H5File(FILE_NAME, H5F_ACC_RDONLY);

    H5::DataSet lambda_dataset = file->openDataSet("lambdas");

    // Prepare a buffer to which data from file is written. Buffer is copied to result eventually.
    //Buffer buffer;

    // Create the memory data type for storing complex numbers in file
    //H5::CompType mtype_comp = def_mtype_comp();

    // Create the dimension arrays for objects in file and in buffer
    //Dims dims(buffer, Lambda_size);

    // Read the data sets from the file to copy their content into the buffer
    //DataSets dataSets(file);

    // Create the data spaces for the data sets in file and for buffer objects
    H5::DataSpace dataSpace_Lambda = lambda_dataset.getSpace();
    /*
     * Get the number of dimensions in the dataspace.
     */
    int rank = dataSpace_Lambda.getSimpleExtentNdims();
    /*
     * Get the dimension size of each dimension in the dataspace and
     * display them.
     */
    hsize_t dims_out[2];
    int ndims = dataSpace_Lambda.getSimpleExtentDims( dims_out, NULL);
    //std::cout << "rank " << rank << ", dimensions " <<
    //     (unsigned long)(dims_out[0]) << " x " <<
    //     (unsigned long)(dims_out[1]) << std::endl;


    H5::DataSpace dataSpace_Lambda_buffer(1, dims_out);

    /// load data into buffer

    //hsize_t start_1D[1] = {Lambda_it};
    //hsize_t stride_1D[1]= {1};
    //hsize_t count_1D[1] = {1};
    //hsize_t block_1D[1] = {1};
    //dataSpaces_Lambda.selectHyperslab(H5S_SELECT_SET, count_1D, start_1D, stride_1D, block_1D);
    double Lambdas_arr[dims_out[0]];
    lambda_dataset.read(Lambdas_arr, H5::PredType::NATIVE_DOUBLE,
                        dataSpace_Lambda_buffer, dataSpace_Lambda);

    rvec Lambdas (Lambdas_arr, Lambdas_arr + sizeof(Lambdas_arr) / sizeof(double));

    // Terminate
    lambda_dataset.close();
    file->close();
    delete file;

    return Lambdas;


}

State<state_datatype> read_hdf(const H5std_string FILE_NAME, size_t Lambda_it){
    State<state_datatype> result(Lambda_ini);   // Initialize with ANY frequency grid, read grid from HDF file later
    long Lambda_size = read_Lambdas_from_hdf(FILE_NAME).size();
    if (Lambda_it < Lambda_size) {

        // Open the file. Access rights: read-only
        H5::H5File *file = 0;
        file = new H5::H5File(FILE_NAME, H5F_ACC_RDONLY);

        // Prepare a buffer to which data from file is written. Buffer is copied to result eventually.
        Buffer buffer;

        // Create the memory data type for storing complex numbers in file
        H5::CompType mtype_comp = def_mtype_comp();

        // Create the dimension arrays for objects in file and in buffer
        Dims dims(buffer, Lambda_size);

        // Read the data sets from the file to copy their content into the buffer
        DataSets dataSets(file);

        // Create the data spaces for the data sets in file and for buffer objects
        H5::DataSpace dataSpaces_Lambda = dataSets.lambda.getSpace();
        H5::DataSpace dataSpaces_freq_params;
        try {   // storing frequency gri parameters was implemented later --> old files do not have it
            dataSpaces_freq_params = dataSets.freq_params.getSpace();
        }
        catch (H5::DataSetIException error) {
            error.printErrorStack();
        }
        H5::DataSpace dataSpaces_selfenergy = dataSets.self.getSpace();
        H5::DataSpace dataSpaces_irreducible = dataSets.irred.getSpace();

        H5::DataSpace dataSpaces_Lambda_buffer(1, dims.Lambda);
        H5::DataSpace dataSpaces_freq_params_buffer(RANK_freqs-1, dims.freq_params_buffer_dims);
        H5::DataSpace dataSpaces_selfenergy_buffer(RANK_self-1, dims.selfenergy_buffer);
        H5::DataSpace dataSpaces_irreducible_buffer(RANK_irreducible-1, dims.irreducible_buffer);

#if MAX_DIAG_CLASS >= 1
        H5::DataSpace dataSpaces_K1_a = dataSets.K1_a.getSpace();
        H5::DataSpace dataSpaces_K1_p = dataSets.K1_p.getSpace();
        H5::DataSpace dataSpaces_K1_t = dataSets.K1_t.getSpace();

        H5::DataSpace dataSpaces_K1_a_buffer(RANK_K1-1, dims.K1_buffer);
        H5::DataSpace dataSpaces_K1_p_buffer(RANK_K1-1, dims.K1_buffer);
        H5::DataSpace dataSpaces_K1_t_buffer(RANK_K1-1, dims.K1_buffer);
#endif

#if MAX_DIAG_CLASS >= 2
        H5::DataSpace dataSpaces_K2_a = dataSets.K2_a.getSpace();
        H5::DataSpace dataSpaces_K2_p = dataSets.K2_p.getSpace();
        H5::DataSpace dataSpaces_K2_t = dataSets.K2_t.getSpace();

        H5::DataSpace dataSpaces_K2_a_buffer(RANK_K2-1, dims.K2_buffer);
        H5::DataSpace dataSpaces_K2_p_buffer(RANK_K2-1, dims.K2_buffer);
        H5::DataSpace dataSpaces_K2_t_buffer(RANK_K2-1, dims.K2_buffer);
#endif

#if MAX_DIAG_CLASS >= 3
        H5::DataSpace dataSpaces_K3_a = dataSets.K3_a.getSpace();
        H5::DataSpace dataSpaces_K3_p = dataSets.K3_p.getSpace();
        H5::DataSpace dataSpaces_K3_t = dataSets.K3_t.getSpace();

        H5::DataSpace dataSpaces_K3_a_buffer(RANK_K3-1, dims.K3_buffer);
        H5::DataSpace dataSpaces_K3_p_buffer(RANK_K3-1, dims.K3_buffer);
        H5::DataSpace dataSpaces_K3_t_buffer(RANK_K3-1, dims.K3_buffer);
#endif


        //Select hyperslab in the file where the data should be located
        hsize_t start[2];
        hsize_t stride[2];
        hsize_t count[2];
        hsize_t block[2];

        start[0] = Lambda_it;
        start[1] = 0;
        for (int i = 0; i < 2; i++) {
            stride[i] = 1;
            block[i] = 1;
        }
        count[0] = 1;

        /// load data into buffer

        //hsize_t start_1D[1] = {Lambda_it};
        //hsize_t stride_1D[1]= {1};
        //hsize_t count_1D[1] = {1};
        //hsize_t block_1D[1] = {1};
        //dataSpaces_Lambda.selectHyperslab(H5S_SELECT_SET, count_1D, start_1D, stride_1D, block_1D);
        //dataSets.lambda.read(buffer.lambda, H5::PredType::NATIVE_DOUBLE,
        //                   dataSpaces_Lambda_buffer, dataSpaces_Lambda);

        int rank = dataSpaces_Lambda.getSimpleExtentNdims();
        /*
         * Get the dimension size of each dimension in the dataspace and
         * display them.
         */
        hsize_t dims_out[2];
        int ndims = dataSpaces_Lambda.getSimpleExtentDims( dims_out, NULL);
        //std::cout << "rank " << rank << ", dimensions " <<
        //     (unsigned long)(dims_out[0]) << " x " <<
        //     (unsigned long)(dims_out[1]) << std::endl;


        H5::DataSpace dataSpace_Lambda_buffer(1, dims_out);

        /// load data into buffer
        double Lambdas_arr[dims_out[0]];
        dataSets.lambda.read(Lambdas_arr, H5::PredType::NATIVE_DOUBLE,
                             dataSpace_Lambda_buffer, dataSpaces_Lambda);
        *buffer.lambda = Lambdas_arr[Lambda_it];


        count[1] = N_freq_params;
        try {   // storing frequency gri parameters was implemented later --> old files do not have it
            dataSpaces_freq_params.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
            dataSets.freq_params.read(buffer.freq_params, H5::PredType::NATIVE_DOUBLE,
                                      dataSpaces_freq_params_buffer, dataSpaces_freq_params);
        }
        catch (H5::DataSpaceIException error) {
            error.printErrorStack();
        }

        count[1] = buffer.self_dim;
        dataSpaces_selfenergy.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSets.self.read(buffer.selfenergy, mtype_comp,
                           dataSpaces_selfenergy_buffer, dataSpaces_selfenergy);

        count[1] = buffer.irred_dim;
        dataSpaces_irreducible.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSets.irred.read(buffer.irreducible_class, mtype_comp,
                            dataSpaces_irreducible_buffer, dataSpaces_irreducible);


#if MAX_DIAG_CLASS >= 1
        count[1] = buffer.K1_dim;
        dataSpaces_K1_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K1_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K1_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        dataSets.K1_a.read(buffer.K1_class_a, mtype_comp, dataSpaces_K1_a_buffer, dataSpaces_K1_a);
        dataSets.K1_p.read(buffer.K1_class_p, mtype_comp, dataSpaces_K1_p_buffer, dataSpaces_K1_p);
        dataSets.K1_t.read(buffer.K1_class_t, mtype_comp, dataSpaces_K1_t_buffer, dataSpaces_K1_t);
#endif
#if MAX_DIAG_CLASS >= 2
        count[1] = buffer.K2_dim;
        dataSpaces_K2_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K2_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K2_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        dataSets.K2_a.read(buffer.K2_class_a, mtype_comp, dataSpaces_K2_a_buffer, dataSpaces_K2_a);
        dataSets.K2_p.read(buffer.K2_class_p, mtype_comp, dataSpaces_K2_p_buffer, dataSpaces_K2_p);
        dataSets.K2_t.read(buffer.K2_class_t, mtype_comp, dataSpaces_K2_t_buffer, dataSpaces_K2_t);
#endif
#if MAX_DIAG_CLASS >= 3
        count[1] = buffer.K3_dim;
        dataSpaces_K3_a.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K3_p.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);
        dataSpaces_K3_t.selectHyperslab(H5S_SELECT_SET, count, start, stride, block);

        dataSets.K3_a.read(buffer.K3_class_a, mtype_comp, dataSpaces_K3_a_buffer, dataSpaces_K3_a);
        dataSets.K3_p.read(buffer.K3_class_p, mtype_comp, dataSpaces_K3_p_buffer, dataSpaces_K3_p);
        dataSets.K3_t.read(buffer.K3_class_t, mtype_comp, dataSpaces_K3_t_buffer, dataSpaces_K3_t);

#endif

        // Initialize the frequency grids of the result State using the parameters stored in buffer
        result_set_frequency_grids(result, buffer);

        // Copy the buffered result into State object
        copy_buffer_to_result(result, buffer);

        // Terminate
        dataSets.close(true);
        file->close();
        delete file;

        return result;

    } else {
        throw std::runtime_error("Cannot read from file " + FILE_NAME + " since Lambda layer out of range");
    }
}

void test_hdf5(H5std_string FILE_NAME, int i, State<state_datatype>& state) {
    // test hdf5: read files and compare to original file
    int cnt = 0;
    State<state_datatype> out = read_hdf(FILE_NAME, i);
    for (int iK=0; iK<2; ++iK) {
        for (int iSE = 0; iSE < nSE; ++iSE) {
            if (state.selfenergy.val(iK, iSE, 0) != out.selfenergy.val(iK, iSE, 0)) {
                std::cout << "Self-energy not equal, " << iK << ", " << iSE << std::endl;
                cnt += 1;
            }
        }
    }

    int spin = 0;
    for (int iK=0; iK<nK_K1; ++iK) {
        for (int i_in=0; i_in<n_in; ++i_in) {
#if MAX_DIAG_CLASS >= 1
            for (int iw1=0; iw1<nBOS; ++iw1) {
                if (state.vertex.avertex().K1.val(iK, spin, iw1, i_in) != out.vertex.avertex().K1.val(iK, spin, iw1, i_in)) {
                    std::cout << "Vertex not equal, " << iK << ", " << iw1 << std::endl;
                    cnt += 1;
                }
                if (state.vertex.pvertex().K1.val(iK, spin, iw1, i_in) != out.vertex.pvertex().K1.val(iK, spin, iw1, i_in)) {
                    std::cout << "Vertex not equal, " << iK << ", " << iw1 << std::endl;
                    cnt += 1;
                }
                if (state.vertex.tvertex().K1.val(iK, spin, iw1, i_in) != out.vertex.tvertex().K1.val(iK, spin, iw1, i_in)) {
                    std::cout << "Vertex not equal, " << iK << ", " << iw1 << std::endl;
                    cnt += 1;
                }
#if MAX_DIAG_CLASS >= 2
                for (int iw2=0; iw2<nFER; ++iw2) {
                    if (state.vertex.avertex().K2.val(iK, spin, iw1, iw2, i_in) != out.vertex.avertex().K2.val(iK, spin, iw1, iw2, i_in)) {
                        std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << std::endl;
                        cnt += 1;
                    }
                    if (state.vertex.pvertex().K2.val(iK, spin, iw1, iw2, i_in) != out.vertex.pvertex().K2.val(iK, spin, iw1, iw2, i_in)) {
                        std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << std::endl;
                        cnt += 1;
                    }
                    if (state.vertex.tvertex().K2.val(iK, spin, iw1, iw2, i_in) != out.vertex.tvertex().K2.val(iK, spin, iw1, iw2, i_in)) {
                        std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << std::endl;
                        cnt += 1;
                    }
#if MAX_DIAG_CLASS == 3
                    for (int iw3=0; iw3<nFER; ++iw3) {
                        if (state.vertex.avertex().K3.val(iK, spin, iw1, iw2, iw3, i_in) != out.vertex.avertex().K3.val(iK, spin, iw1, iw2, iw3, i_in)) {
                            std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << ", " << iw3 << std::endl;
                            cnt += 1;
                        }
                        if (state.vertex.pvertex().K3.val(iK, spin, iw1, iw2, iw3, i_in) != out.vertex.pvertex().K3.val(iK, spin, iw1, iw2, iw3, i_in)) {
                            std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << ", " << iw3 << std::endl;
                            cnt += 1;
                        }
                        if (state.vertex.tvertex().K3.val(iK, spin, iw1, iw2, iw3, i_in) != out.vertex.tvertex().K3.val(iK, spin, iw1, iw2, iw3, i_in)) {
                            std::cout << "Vertex not equal, " << iK << ", " << iw1 << ", " << iw2 << ", " << iw3 << std::endl;
                            cnt += 1;
                        }
                    }
#endif
                }
#endif
            }
#endif
        }
    }
    if (cnt == 0) print("HDF5 test successful.", true);
    else print("HDF5 test failed. Number of differences: ", cnt, true);
}
