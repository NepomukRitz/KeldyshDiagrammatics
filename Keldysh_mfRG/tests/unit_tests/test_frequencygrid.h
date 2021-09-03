#ifndef KELDYSH_MFRG_TESTING_TEST_FREQUENCYGRID_H
#define KELDYSH_MFRG_TESTING_TEST_FREQUENCYGRID_H

#include "../../grids/frequency_grid.h"
#include "../../utilities/util.h"


TEST_CASE( "bosonic frequency grid correctly initialized and accessed?", "[bosonic frequency_grid]" ) {

    bool isright = true;
    double issymmetric = 0.;
    double symmetry_tolerance = 1e-10;
    FrequencyGrid Bosfreqs('b', 1, 0.);
    bool existNoDoubleOccurencies = not is_doubleOccurencies(Bosfreqs.ws);
    for (int i = 1; i < nBOS-1; i++) {

        // It doesn't harm if fconv() retrieves a neighboring index. fconv() is only needed for interpolations.
        if (std::abs(Bosfreqs.fconv(Bosfreqs.ws[i]) - i) > 1) isright = false;
        issymmetric += std::abs(Bosfreqs.ws[i] + Bosfreqs.ws[nBOS - i - 1]);
    }

    SECTION( "Is the correct index retrieved by fconv()?" ) {
        REQUIRE( isright );
    }

    SECTION( "Is the frequency grid symmetric?" ) {
        REQUIRE( issymmetric < symmetry_tolerance );
    }

    SECTION( "Are there frequencies which appear  more than once in the vector?" ) {
        REQUIRE( existNoDoubleOccurencies );
    }


}

TEST_CASE( "fermionic frequency grid correctly initialized and accessed?", "[fermionic frequency_grid]" ) {

    bool isright = true;
    double issymmetric = 0.;
    double symmetry_tolerance = 1e-10;
    FrequencyGrid Ferfreqs('f', 1, 0.);
    bool existNoDoubleOccurencies = not is_doubleOccurencies(Ferfreqs.ws);
    for (int i = 1; i < nFER-1; i++) {

        // It doesn't harm if fconv() retrieves a neighboring index. fconv() is only needed for interpolations.
        if (std::abs(Ferfreqs.fconv(Ferfreqs.ws[i]) - i) > 1) isright = false;
        issymmetric += std::abs(Ferfreqs.ws[i] + Ferfreqs.ws[nFER - i - 1]);
        if (std::abs(Ferfreqs.ws[i] + Ferfreqs.ws[nFER - i - 1]) > symmetry_tolerance) {
            std::cout << Ferfreqs.ws[i] << " != " << Ferfreqs.ws[nFER - i - 1] << "\n";
        }
    }

    SECTION( "Is the correct index retrieved by fconv()?" ) {
        REQUIRE( isright );
    }

    SECTION( "Is the frequency grid symmetric?" ) {
        REQUIRE( issymmetric < symmetry_tolerance );
    }

    SECTION( "Are there frequencies which appear  more than once in the vector?" ) {
        REQUIRE( existNoDoubleOccurencies );
    }


}


#endif //KELDYSH_MFRG_TESTING_TEST_FREQUENCYGRID_H