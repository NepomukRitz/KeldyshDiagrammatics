#ifndef KELDYSH_MFRG_TESTFUNCTIONS_H
#define KELDYSH_MFRG_TESTFUNCTIONS_H

#include <cmath>                    // use M_PI as pi
#include "state.h"                  // State class
#include "loop.h"                   // self-energy loop
#include "bubbles.h"                // bubble function
#include "solvers.h"                // ODE solvers
#include "right_hand_sides.h"       // compute the right hand sides of flow equations
#include "write_data2file.h"        // writing data to txt or hdf5 file
#include "hdf5_routines.h"          // writing states to hdf5 file
#include "perturbation_theory.h"

// TODO: remove glb_w_lower

/**
 * Function that checks causality of self-energy: Im(Sigma^R)<=0.
 */
template <typename Q>
void check_SE_causality(SelfEnergy<Q> selfEnergy) {
    print("Causality check of self-energy: Im(Sigma^R)<=0.", true);

    vec<Q> Sigma = selfEnergy.Sigma;                        // take self-energy
    vec<Q> Sigma_R (&Sigma[0], &Sigma[Sigma.size()/2]);     // take first half of self-energy (retarded comp.)

    // check if Im(Sigma^R) is positive for every data point
    int cnt = 0;
    double sum = 0.;
    for (int i=0; i<Sigma_R.size(); ++i) {
        double val = Sigma_R[i].imag();
        if (val > 0.) {
            cnt += 1;
            sum += val;
        }
    }
    if (cnt > 0) {
        print("Selfenergy is non-causal: ", true);
        print(cnt, " values of Im(Sigma^R) are positive, with a sum of ", sum, true);
    } else
        print("Selfenergy is causal.", true);
}

/**
 * Function that checks FDTs for self-energy and K1 in all channels for given input state: Re(Sigma^K)=0, Re(K1r^K)=0.
 */
template <typename Q>
void check_FDTs(State<Q>& state) {
    print("Check of FDTs for self-energy and K1: Re(Sigma^K)=0, Re(K1r^K)=0.", true);

    /** 1st check: real part of Keldysh component of the selfenergy has to be zero */

    vec<Q> Sigma = state.selfenergy.Sigma;                          // take self-energy
    vec<Q> Sigma_K (&Sigma[Sigma.size()/2], &Sigma[Sigma.size()]);  // take second half of self-energy (Keldysh comp.)
    double max_Sigma_K = Sigma_K.real().max_norm();                 // take max. value of Re(Sigma^K)

    print("Maximal value of Re(Sigma^K): ", max_Sigma_K, true);

    /** 2nd check: real part of Keldysh component of K1 in all channels has to be zero */

    // take K1 vertices in all channels
    vec<Q> K1a = state.vertex[0].avertex.K1;
    vec<Q> K1p = state.vertex[0].pvertex.K1;
    vec<Q> K1t = state.vertex[0].tvertex.K1;
    // take second half of K1 vertices (Keldysh comp.)
    vec<Q> K1a_K (&K1a[K1a.size()/2], &K1a[K1a.size()]);
    vec<Q> K1p_K (&K1p[K1p.size()/2], &K1p[K1p.size()]);
    vec<Q> K1t_K (&K1t[K1t.size()/2], &K1t[K1t.size()]);
    // take max. value of Re(K1r^K)
    double max_K1a_K = K1a_K.real().max_norm();
    double max_K1p_K = K1p_K.real().max_norm();
    double max_K1t_K = K1t_K.real().max_norm();

    print("Maximal value of Re(K1a^K):   ", max_K1a_K, true);
    print("Maximal value of Re(K1p^K):   ", max_K1p_K, true);
    print("Maximal value of Re(K1t^K):   ", max_K1t_K, true);
}

/**
 * Function desgined to test the flow of the vertex in SOPT using the State class
 * @param input     : Input state (not used inside the function)
 * @param Lambda    : Lambda at which to calculate the rhs of the eq.
 * @return          : State carrying in the K1 vertex the results of the computation
 */
auto rhs_bubbles_flow_wstate(const State<comp>& input, double Lambda) -> State<comp>{
    State<comp> ans (Lambda);    //Initialize the answer-object

    //Calculating propagator objects of the required types
    Propagator g(Lambda, input.selfenergy, 'g');
    Propagator s(Lambda, input.selfenergy, 's');

    bubble_function(ans.vertex, input.vertex, input.vertex, g, s, 'a', true);
    return ans;
}

/**
 * Function to call when testing the rhs of the bubbles flow with the State class
 * @param N_ODE : Number of ODE steps to take between the globally defined Lambda_ini and Lambda_fin
 * @param Lambda_i : initial Lambda value of flow
 * @param Lambda_f : final Lambda value of flow
 * @param write_flag : whether to write output in hdf5
 */
void test_rhs_bubbles_flow_wstate(int N_ODE, double Lambda_i, double Lambda_f, bool write_flag = true) {
    State<comp> state_dir (Lambda_f), state_fin (Lambda_f), state_ini (Lambda_i); // direct, final, initial K1a_1
    state_dir.initialize(); // initialize
    state_ini.initialize(); // initialize

    Propagator G0ini(Lambda_i, state_ini.selfenergy, 'g'); // initial propagator
    Propagator G0dir(Lambda_f, state_dir.selfenergy, 'g'); // final propagator

    sopt_state(state_ini, Lambda_i); // direct calculation of initial K1a
    sopt_state(state_dir, Lambda_f); // direct calculation of direct K1a

    ODE_solver_RK4(state_fin, Lambda_f, state_ini, Lambda_i, rhs_bubbles_flow_wstate, N_ODE); // final K1a from ODE
    cvec K1a_dif = state_dir.vertex[0].avertex.K1 + ( state_fin.vertex[0].avertex.K1*(-1.) ); // difference in results
    print("Testing ODE for bare K1a_0 with State class. Using " +to_string(N_ODE)+ " ODE steps, the maximal difference between direct and ODE-final result is " +to_string(K1a_dif.max_norm())+ ".", true);
    if(write_flag) write_h5_rvecs("rhs_bubbles_flow_wstate.h5",
                                  {"v", "state_dir_R", "state_dir_I", "state_fin_R", "state_fin_I", "state_ini_R", "state_ini_I"},
                                  {bfreqs, state_dir.vertex[0].avertex.K1.real(), state_dir.vertex[0].avertex.K1.imag(),
                                                   state_fin.vertex[0].avertex.K1.real(), state_fin.vertex[0].avertex.K1.imag(),
                                                   state_ini.vertex[0].avertex.K1.real(), state_ini.vertex[0].avertex.K1.imag()});
}

/**
 * Function desgined to test the flow of the vertex in SOPT using the only cvecs
 * @param input     : Input cvec (not used inside the function)
 * @param Lambda    : Lambda at which to calculate the rhs of the eq.
 * @return          : The results of the calculation
 */
auto rhs_bubbles_flow(const cvec& input, double Lambda) -> cvec{
    cvec ans(nw1_a);   //Initialize the answer

    SelfEnergy<comp> selfini (Lambda);   //Initialize self energy
    selfini.initialize(glb_U/2., 0.);   //Hartree term

    Propagator g(Lambda, selfini, 'g'); //Regular propagator
    Propagator s(Lambda, selfini, 's'); //Single-scale propagator

    for(int i=0; i<nBOS; i++){
        double w = bfreqs[i];

        //Create the objects explicitly designed to return the determined Keldysh component needed
        IntegrandBubble integrandPia11(g, s, true, w, 11, 'a');     //KA
        IntegrandBubble integrandPia13(g, s, true, w, 13, 'a');     //RK

        //Calculate the contributions
        auto cont11 = integrator(integrandPia11, glb_w_lower, glb_w_lower);
        auto cont13 = integrator(integrandPia13, glb_w_lower, glb_w_lower);

        //Add the respective contributions to the respective bubble
        ans[i] = pow(-glb_U/2., 2.)*(cont11+ cont13);            //11+13 = OE => Keldysh comp0
    }

    return ans;
}

/**
 * Function to call when testing the rhs of the bubbles flow with cvecs
 * @param N_ODE : Number of ODE steps to take between the globally defined Lambda_ini and Lambda_fin
 */
void test_rhs_bubbles_flow(int N_ODE){
    bool write_flag = true; // whether to write output in hdf5
    cvec K1a_dir(nw1_a), K1a_fin(nw1_a), K1a_ini(nw1_a); // direct, final, initial K1a_1
    SelfEnergy<comp> SEin (Lambda_ini); // trivial self-energy
    SEin.initialize(glb_U/2., 0.); // initialize with Hartree term
    Propagator G0ini(Lambda_ini, SEin, 'g'); // initial propagator
    Propagator G0dir(Lambda_fin, SEin, 'g'); // final propagator

    // direct calculation of initial K1a
    for(int i=0; i<nw1_a; ++i) {
        double w = bfreqs[i];

        //Create the objects explicitly designed to return the determined Keldysh component needed
        IntegrandBubble integrandPia11(G0ini, G0ini, false, w, 11, 'a');     //KA
        IntegrandBubble integrandPia13(G0ini, G0ini, false, w, 13, 'a');     //RK

        //Calculate the contributions
        auto cont11 = integrator(integrandPia11, glb_w_lower, glb_w_lower);
        auto cont13 = integrator(integrandPia13, glb_w_lower, glb_w_lower);

        K1a_ini[i] = pow(-glb_U/2.,2.)*(cont11 + cont13);
    }

    // direct calculation of direct K1a
    for(int i=0; i<nw1_a; ++i) {
        double w = bfreqs[i];

        //Create the objects explicitly designed to return the determined Keldysh component needed
        IntegrandBubble integrandPia11(G0dir, G0dir, false, w, 11, 'a');     //KA
        IntegrandBubble integrandPia13(G0dir, G0dir, false, w, 13, 'a');     //RK

        //Calculate the contributions
        auto cont11 = integrator(integrandPia11, glb_w_lower, glb_w_lower);
        auto cont13 = integrator(integrandPia13, glb_w_lower, glb_w_lower);

        K1a_dir[i] = pow((-glb_U/2.),2.)*(cont11 + cont13);
    }

    ODE_solver_RK4(K1a_fin, Lambda_fin, K1a_ini, Lambda_ini, rhs_bubbles_flow, N_ODE); // final K1a from ODE
    cvec K1a_dif = K1a_dir + ( K1a_fin*(-1.) ); // difference in results
    print("Testing ODE for bare K1a_0. Using " +to_string(N_ODE)+ " ODE steps, the maximal difference between direct and ODE-final result is " +to_string(K1a_dif.max_norm())+ ".", true);
    if(write_flag) write_h5_rvecs("rhs_bubbles_flow.h5",
                                  {"v", "K1a_dir_R", "K1a_dir_I", "K1a_fin_R", "K1a_fin_I", "K1a_ini_R", "K1a_ini_I"},
                                  {bfreqs, K1a_dir.real(), K1a_dir.imag(), K1a_fin.real(), K1a_fin.imag(), K1a_ini.real(), K1a_ini.imag()});
}



/**
 * Function to test the loop function and the calculation of the SelfEnergy
 * @param state : State initialized with initial conditions
 */
void testSelfEnergy_and_K1(State<comp>& state, double Lambda){

    Propagator g(Lambda, state.selfenergy, 'g');

    //Calculate the vertex
    sopt_state(state, Lambda);

    Vertex<comp> temp_vertex_a (1, Lambda), temp_vertex_p (1, Lambda); //All zeros
    temp_vertex_a[0].avertex = state.vertex[0].avertex;
    temp_vertex_p[0].pvertex = state.vertex[0].pvertex;

    loop(state.selfenergy, temp_vertex_a, g, false);//Calculate the SelfEnergy in SOPT

    //Print results in .h5 format
    cvec SER(nFER);
    cvec SEK(nFER);

    for(int iv = 0; iv<nFER; iv++){
        SER[iv] = state.selfenergy.val(0, iv, 0);
        SEK[iv] = state.selfenergy.val(1, iv, 0);
    }

    //Print results
    write_h5_rvecs("SOPT_test.h5", {"v", "ReSER", "ImSER", "ReSEK", "ImSEK",
                                  "K1a_R", "K1a_I", "K1p_R", "K1p_I", "K1t_R", "K1t_I"},
                                 {ffreqs, SER.real(), SER.imag(), SEK.real(), SEK.imag(),
                                  state.vertex[0].avertex.K1.real(),
                                  state.vertex[0].avertex.K1.imag(),
                                  state.vertex[0].pvertex.K1.real(),
                                  state.vertex[0].pvertex.K1.imag(),
                                  state.vertex[0].tvertex.K1.real(),
                                  state.vertex[0].tvertex.K1.imag()});

}


/**
 * Function to implement the flow of a State in SOPT.
 * @param Psi   : Known state of the State at Lambda
 * @param Lambda:  Scale at which the calculation is being performed
 * @return dPsi : The derivative at Lambda, which includes the differential vertex as well as self-energy at scale Lambda
 */
auto rhs_state_flow_SOPT(const State<comp>& Psi, const double Lambda, const int feedback) -> State<comp>{
    State<comp> dPsi (Lambda);   //Answer object

    State<comp> bare (Lambda);   //Bare state
    bare.initialize();  //Initialize bare state
    Propagator G(Lambda, bare.selfenergy,'g');    //Initialization of Propagator objects
    Propagator S(Lambda, bare.selfenergy,'s');    //Initialization of Propagator objects

    if(!(feedback==0 || feedback==3)){  //Check whether Self Energy feedback to Propagators is wanted
        G=Propagator(Lambda, Psi.selfenergy, 'g');
        S=Propagator(Lambda, Psi.selfenergy, 's');
    }

    //Self energy loop
    loop(dPsi.selfenergy, Psi.vertex, S, true);  //Loop for the Self-Energy calculation


    if(feedback>=3){    //If feedback>=3, there is vertex feedback
        bare.vertex = Psi.vertex;
    }

    if(feedback==2 || feedback==5) {  //These two options make use of the Katanin substitution
        S=Propagator(Lambda, Psi.selfenergy, dPsi.selfenergy, 'k');
    }

    //Vertex flow
    bubble_function(dPsi.vertex, bare.vertex, bare.vertex, G, S, 'a', true);  //Differentiated bubble in the a-channel
    bubble_function(dPsi.vertex, bare.vertex, bare.vertex, G, S, 'p', true);  //Differentiated bubble in the p-channel
    bubble_function(dPsi.vertex, bare.vertex, bare.vertex, G, S, 't', true);  //Differentiated bubble in the t-channel

    return dPsi;
}

//No feedback
auto rhs_state_flow_SOPT_0(const State<comp>& Psi, const double Lambda) -> State<comp>{
    return rhs_state_flow_SOPT(Psi, Lambda, 0);
}

//Self-Energy fed back into Propagators
auto rhs_state_flow_SOPT_1(const State<comp>& Psi, const double Lambda) -> State<comp>{
    return rhs_state_flow_SOPT(Psi, Lambda, 1);
}

//As above + Katanin
auto rhs_state_flow_SOPT_2(const State<comp>& Psi, const double Lambda) -> State<comp>{
    return rhs_state_flow_SOPT(Psi, Lambda, 2);
}

//Vertex feedback, free propagators
auto rhs_state_flow_SOPT_3(const State<comp>& Psi, const double Lambda) -> State<comp>{
    return rhs_state_flow_SOPT(Psi, Lambda, 3);
}

//Self_energy fed back into Propagators + Vertex feedback
auto rhs_state_flow_SOPT_4(const State<comp>& Psi, const double Lambda) -> State<comp>{
    return rhs_state_flow_SOPT(Psi, Lambda, 4);
}

//As above + Katanin
auto rhs_state_flow_SOPT_5(const State<comp>& Psi, const double Lambda) -> State<comp>{
    return rhs_state_flow_SOPT(Psi, Lambda, 5);
}

/**
 * Function to test the correctness of the flow of the State
 * @param N_ODE : Numbres of ODE-solver steps to be taken
 */
void test_rhs_state_flow_SOPT(int N_ODE, int feedback){
    bool write_flag = true; // whether to write output in hdf5
    State<comp> state_dir (Lambda_fin), state_fin (Lambda_fin), state_ini (Lambda_ini); // direct, final, initial K1a_1
    state_dir.initialize(); // initialize state
    state_ini.initialize(); // initialize state

    Propagator G0ini(Lambda_ini, state_ini.selfenergy, 'g'); // initial propagator
    Propagator G0dir(Lambda_fin, state_dir.selfenergy, 'g'); // final propagator

    sopt_state(state_ini, Lambda_ini); // direct calculation of initial K1a
    sopt_state(state_dir, Lambda_fin); // direct calculation of direct K1a

    string name;
    switch (feedback){
        case 1:
            ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_state_flow_SOPT_1, N_ODE); // final K1a from ODE
            name = "rhs_state_flow_feedback_1.h5";
            break;
        case 2:
            ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_state_flow_SOPT_2, N_ODE); // final K1a from ODE
            name = "rhs_state_flow_feedback_2.h5";
            break;
        case 3:
            ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_state_flow_SOPT_3, N_ODE); // final K1a from ODE
            name = "rhs_state_flow_feedback_3.h5";
            break;
        case 4:
            ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_state_flow_SOPT_4, N_ODE); // final K1a from ODE
            name = "rhs_state_flow_feedback_4.h5";
            break;
        case 5:
            ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_state_flow_SOPT_5, N_ODE); // final K1a from ODE
            name = "rhs_state_flow_feedback_5.h5";
            break;
        default:
            ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_state_flow_SOPT_0, N_ODE); // final K1a from ODE
            name = "rhs_state_flow_feedback_0.h5";
    }

    cvec K1a0_dif(nBOS);
    for(int i=0; i<nBOS; ++i){
        K1a0_dif[i] = state_dir.vertex[0].avertex.K1_val(0, i, 0) - state_fin.vertex[0].avertex.K1_val(0, i, 0);
    }
    cvec SER_dif(nFER);
    cvec SEK_dif(nFER);
    for(int i=0; i<nFER; ++i){
        SER_dif[i] = state_dir.selfenergy.val(0, i, 0) - state_fin.selfenergy.val(0, i, 0);
        SEK_dif[i] = state_dir.selfenergy.val(1, i, 0) - state_fin.selfenergy.val(1, i, 0);
    }

    if(mpi_world_rank()==0) {
        print("Confirming correctness of the bubbles. The max diff between direct and final results is " +to_string(K1a0_dif.max_norm())+". \n "+
        "Testing ODE for SelfEnergyR. Using " +to_string(N_ODE)+ " ODE steps, the maximal difference between direct and ODE-final result is " +
        to_string(SER_dif.max_norm())+ ". \n" +
        "Testing ODE for SelfEnergyK. Using " + to_string(N_ODE)+ " ODE steps, the maximal difference between direct and ODE-final result is " +
        to_string(SEK_dif.max_norm()) +".", true);
    }
    if(write_flag) write_h5_rvecs(name,
                                  {"v", "dir_SE_R", "dir_SE_I", "fin_SE_R", "fin_SE_I", "ini_SE_R", "ini_SE_I",
                                   "dir_K1a_R", "dir_K1a_I", "fin_K1a_R", "fin_K1a_I", "ini_K1a_R", "ini_K1a_I",
                                   "dir_K1p_R", "dir_K1p_I", "fin_K1p_R", "fin_K1p_I", "ini_K1p_R", "ini_K1p_I",
                                   "dir_K1t_R", "dir_K1t_I", "fin_K1t_R", "fin_K1t_I", "ini_K1t_R", "ini_K1t_I"},
                                  {ffreqs,
                                   state_dir.selfenergy.Sigma.real(), state_dir.selfenergy.Sigma.imag(),
                                   state_fin.selfenergy.Sigma.real(), state_fin.selfenergy.Sigma.imag(),
                                   state_ini.selfenergy.Sigma.real(), state_ini.selfenergy.Sigma.imag(),
                                   state_dir.vertex[0].avertex.K1.real(), state_dir.vertex[0].avertex.K1.imag(),
                                   state_fin.vertex[0].avertex.K1.real(), state_fin.vertex[0].avertex.K1.imag(),
                                   state_ini.vertex[0].avertex.K1.real(), state_ini.vertex[0].avertex.K1.imag(),
                                   state_dir.vertex[0].pvertex.K1.real(), state_dir.vertex[0].pvertex.K1.imag(),
                                   state_fin.vertex[0].pvertex.K1.real(), state_fin.vertex[0].pvertex.K1.imag(),
                                   state_ini.vertex[0].pvertex.K1.real(), state_ini.vertex[0].pvertex.K1.imag(),
                                   state_dir.vertex[0].tvertex.K1.real(), state_dir.vertex[0].tvertex.K1.imag(),
                                   state_fin.vertex[0].tvertex.K1.real(), state_fin.vertex[0].tvertex.K1.imag(),
                                   state_ini.vertex[0].tvertex.K1.real(), state_ini.vertex[0].tvertex.K1.imag()});

}



/**
 * Function that prints out a .h5 file with the value of the rhs of a SOPT flow at the given Lambda for both an FFT and an fRG calculation
 * @param Lambda    : Lambda at which the derivatives are to be calculated
 */
void test_derivatives_K1a(double Lambda){
    cvec blah(nw1_a);
    cvec rhs_SOPT_FFT_K1a = dSOPT_FFT_K1a_rhs(blah, Lambda);
    cvec rhs_flow = rhs_bubbles_flow(blah, Lambda);

    write_h5_rvecs("derivatives_K1a.h5",
                   {"v", "FFT_R", "FFT_I", "SOPT_R", "SOPT_I"},
                   {ffreqs, rhs_SOPT_FFT_K1a.real(), rhs_SOPT_FFT_K1a.imag(), rhs_flow.real(), rhs_flow.imag()});
}

/**
 * Function that prints out a .h5 file with the value of the rhs of a SOPT flow at the given Lambda for both an FFT and an fRG calculation
 * @param Lambda    : Lambda at which the derivatives are to be calculated
 */
void test_derivatives_SE(double Lambda){
    cvec blah(nw1_a);
    State<comp> sopt (Lambda);
    sopt.initialize();
    cvec rhs_SOPT_FFT_K1a = dSOPT_FFT_SE_rhs(blah, Lambda);

    sopt_state(sopt, Lambda);
    State<comp> rhs_flow = rhs_state_flow_SOPT_0(sopt, Lambda);

    cvec SER_dif(nFER);
    for(int iv=0; iv<nFER;++iv){
        SER_dif[iv] = rhs_flow.selfenergy.val(0, iv, 0) -rhs_SOPT_FFT_K1a[iv];
    }

    print("Testing derivatives of the Self Energy. Using Lambda=" +to_string(Lambda)+ ", the max difference between fRG and FFT results is " +to_string(SER_dif.max_norm())+ ".", true);
    write_h5_rvecs("derivativess_SE.h5",
                   {"v", "FFT_R", "FFT_I", "SOPT_R", "SOPT_I"},
                   {ffreqs, rhs_SOPT_FFT_K1a.real(), rhs_SOPT_FFT_K1a.imag(),
                    rhs_flow.selfenergy.Sigma.real(), rhs_flow.selfenergy.Sigma.imag()});
}

#if DIAG_CLASS >= 2
/**
 * This function checks the consistency of the K2 class
 * @param Lambda    : Scale
 * @param r         : Channel to be tested i.e. K_2^r
 */
auto test_K2_consistency(double Lambda, const char r) -> bool{
    State<comp> bare (Lambda);   //Create a bare state
    bare.initialize();  //Initialize bare state

    Propagator G(Lambda, bare.selfenergy, 'g'); //Bare propagator at scale Lambda>

    //Create and initialize states to save the channel contributions in
    State<comp> K1a (Lambda);
    State<comp> K1p (Lambda);
    State<comp> K1t (Lambda);
    K1a.initialize();
    K1p.initialize();
    K1t.initialize();

    //Calculate K1a, K1p and K1t contributions separately
    bubble_function(K1a.vertex, bare.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(K1p.vertex, bare.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(K1t.vertex, bare.vertex, bare.vertex, G, G, 't', false);

    //Create and initialize the K2r-objects to test
    State<comp> test_K2r_with_K1a (Lambda);
    State<comp> test_K2r_with_K1p (Lambda);
    State<comp> test_K2r_with_K1t (Lambda);
    test_K2r_with_K1a.initialize();
    test_K2r_with_K1p.initialize();
    test_K2r_with_K1t.initialize();


    if(r=='p' ||  r=='t') {
        //Perform TOPT calculation of K2r with K1a
        bubble_function(test_K2r_with_K1a.vertex, K1a.vertex, bare.vertex, G, G, r, false);
    }

    if(r=='a' || r=='t') {
        //Perform TOPT calculation of K2r with K1p
        bubble_function(test_K2r_with_K1p.vertex, K1p.vertex, bare.vertex, G, G, r, false);
    }

    if(r=='a' || r=='p') {
        //Perform TOPT calculation of K2r with K1t
        bubble_function(test_K2r_with_K1t.vertex, K1t.vertex, bare.vertex, G, G, r, false);
    }


    //TOPT-consistency checks for K2

    bool empty = true;  //Boolean that stays true if everything is zero
    if(r=='a' || r=='p') {
#pragma omp parallel
        //Check parallelized that everything in the K2 vertex is zero.
        for (int index = 0; index < test_K2r_with_K1t.vertex[0].avertex.K2.size(); ++index) {
            if (test_K2r_with_K1t.vertex[0].avertex.K2_acc(index) != 0.) {
                empty = false;  //If any one element is not zero, K2 is not empty
                break;  //Exit the for-loop early
            }
        }
        //Print result of consistency check
        if (empty) print("TOPT-consistency check passed. K2a or K2p with K1t is zero everywhere.", true);
        else print("TOPT-consistency check failed. K2a or K2p with K1t is not zero everywhere.", true);
    }
    else if(r=='t'){
#pragma omp parallel
        //Check parallelized that everything in the K2 vertices is zero.
        for (int index = 0; index < test_K2r_with_K1a.vertex[0].avertex.K2.size(); ++index) {
            if (test_K2r_with_K1a.vertex[0].avertex.K2_acc(index) != 0. ||
                test_K2r_with_K1p.vertex[0].avertex.K2_acc(index) != 0.) {
                empty = false;  //If any one element is not zero, K2 is not empty
                break;  //Exit the for-loop early
            }
        }
        //Print result of consistency check
        if (empty) print("TOPT-consistency check passed. K2t with K1a and K1p are zero everywhere.", true);
        else print("TOPT-consistency check failed. K2t with K1a and K1p are not zero everywhere.", true);
    }

    return empty;
}
#endif

/**
 * Function that computes K1 (and K2, K3) up to PT4, and performs FDT checks
 */
void test_PT4(double Lambda, bool write_flag = false) {
    print("Compute K1 (and K2, K3) up to PT4.", true);
    // Initialize a bare state
    State<comp> bare (Lambda);
    bare.initialize();

    // Initialize a bare propagator
    Propagator G(Lambda, bare.selfenergy, 'g');

    // Compute K1 in PT2
    State<comp> PT2_K1a (Lambda);
    State<comp> PT2_K1p (Lambda);
    State<comp> PT2_K1t (Lambda);

    double t0 = get_time();
    bubble_function(PT2_K1a.vertex, bare.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT2_K1p.vertex, bare.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT2_K1t.vertex, bare.vertex, bare.vertex, G, G, 't', false);
    print("Computed K1 in PT2.", true);
    get_time(t0);
    if (write_flag) {
        write_hdf("PT2_K1a_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT2_K1a);
        write_hdf("PT2_K1p_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT2_K1p);
        write_hdf("PT2_K1t_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT2_K1t);
    }

    // Compute K1 in PT3, using K1 in PT2
    State<comp> PT3_K1a (Lambda);
    State<comp> PT3_K1p (Lambda);
    State<comp> PT3_K1t (Lambda);

    t0 = get_time();
    bubble_function(PT3_K1a.vertex, PT2_K1a.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT3_K1p.vertex, PT2_K1p.vertex, bare.vertex, G, G, 'p', false);
    // for K1t in PT3, need a-vertex in PT2 due to a <-> t symmetry
    bubble_function(PT3_K1t.vertex, PT2_K1t.vertex + PT2_K1a.vertex, bare.vertex, G, G, 't', false);
#if DIAG_CLASS >= 2
    // set K2 part of this vertex to zero
    PT3_K1t.vertex[0].tvertex.K2 = vec<comp> (PT3_K1t.vertex[0].tvertex.K2.size());
#endif
    print("Computed K1 in PT3.", true);
    get_time(t0);
    if (write_flag) {
        write_hdf("PT3_K1a_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K1a);
        write_hdf("PT3_K1p_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K1p);
        write_hdf("PT3_K1t_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K1t);
    }

    // Compute K2 in PT3, using K1p, K1t in PT2
    State<comp> PT3_K2a (Lambda);
    State<comp> PT3_K2p (Lambda);
    State<comp> PT3_K2t (Lambda);
    State<comp> PT3_K2t_a (Lambda);
    State<comp> PT3_K2t_p (Lambda);

    t0 = get_time();
    bubble_function(PT3_K2a.vertex, PT2_K1p.vertex, bare.vertex, G, G, 'a', false);   // K2a in PT3 (PT2_K1t = 0)
    bubble_function(PT3_K2p.vertex, PT2_K1a.vertex, bare.vertex, G, G, 'p', false);   // K2p in PT3 (PT2_K1t = 0)
    bubble_function(PT3_K2t_a.vertex, PT2_K1a.vertex, bare.vertex, G, G, 't', false);   // contribution of K2t in PT3 obtained by inserting K1a in PT2
    bubble_function(PT3_K2t_p.vertex, PT2_K1p.vertex, bare.vertex, G, G, 't', false);   // contribution of K2t in PT3 obtained by inserting K1p in PT2
    // PT3_K2t_a should also have a K1-contribution due to a-t symmetry (PT2_K1t implicitly inserted) --> set to zero
    PT3_K2t_a.vertex[0].tvertex.K1 = vec<comp> (PT3_K2t_a.vertex[0].tvertex.K1.size());
    PT3_K2t.vertex = PT3_K2t_a.vertex + PT3_K2t_p.vertex; // sum of contributions from a- and p-insertions

    // K2' in PT3 would be obtained by flipping the left and right vertex, but since K2' is not saved, these terms would give zero
    print("Computed K2 in PT3.", true);
    get_time(t0);
    if (write_flag) {
        write_hdf("PT3_K2a_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K2a);
        write_hdf("PT3_K2p_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K2p);
        write_hdf("PT3_K2t_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K2t);
        write_hdf("PT3_K2t_a_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K2t_a);
        write_hdf("PT3_K2t_p_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT3_K2t_p);
    }


    // Compute K3 in PT4, using K1 and K2 in PT2 and PT3
    State<comp> PT4_31 (Lambda);
    State<comp> PT4_31_a_a1 (Lambda);
    State<comp> PT4_31_a_p1 (Lambda);
    State<comp> PT4_31_a_t1 (Lambda);
    State<comp> PT4_31_a_a2 (Lambda);
    State<comp> PT4_31_a_p2 (Lambda);
    State<comp> PT4_31_a_t2 (Lambda);
    State<comp> PT4_31_p_a1 (Lambda);
    State<comp> PT4_31_p_p1 (Lambda);
    State<comp> PT4_31_p_t1 (Lambda);
    State<comp> PT4_31_p_a2 (Lambda);
    State<comp> PT4_31_p_p2 (Lambda);
    State<comp> PT4_31_p_t2 (Lambda);
//    State<comp> PT4_31_t_at1 (Lambda);/// TODO: Remove??
    State<comp> PT4_31_t_a1 (Lambda);
    State<comp> PT4_31_t_p1 (Lambda);
    State<comp> PT4_31_t_t1 (Lambda);
//    State<comp> PT4_31_t_at2 (Lambda);
    State<comp> PT4_31_t_a2 (Lambda);
    State<comp> PT4_31_t_p2 (Lambda);
    State<comp> PT4_31_t_t2 (Lambda);

    State<comp> PT4_13 (Lambda);
    State<comp> PT4_13_a_a1 (Lambda);
    State<comp> PT4_13_a_p1 (Lambda);
    State<comp> PT4_13_a_t1 (Lambda);
    State<comp> PT4_13_a_a2 (Lambda);
    State<comp> PT4_13_a_p2 (Lambda);
    State<comp> PT4_13_a_t2 (Lambda);
    State<comp> PT4_13_p_a1 (Lambda);
    State<comp> PT4_13_p_p1 (Lambda);
    State<comp> PT4_13_p_t1 (Lambda);
    State<comp> PT4_13_p_a2 (Lambda);
    State<comp> PT4_13_p_p2 (Lambda);
    State<comp> PT4_13_p_t2 (Lambda);
//    State<comp> PT4_13_t_at1 (Lambda);
    State<comp> PT4_13_t_a1 (Lambda);
    State<comp> PT4_13_t_p1 (Lambda);
    State<comp> PT4_13_t_t1 (Lambda);
//    State<comp> PT4_13_t_at2 (Lambda);
    State<comp> PT4_13_t_a2 (Lambda);
    State<comp> PT4_13_t_p2 (Lambda);
    State<comp> PT4_13_t_t2 (Lambda);

    State<comp> PT4_22 (Lambda);
    State<comp> PT4_22_a_aa (Lambda);
    State<comp> PT4_22_a_ap (Lambda);
    State<comp> PT4_22_a_pa (Lambda);
    State<comp> PT4_22_a_pp (Lambda);
    State<comp> PT4_22_p_aa (Lambda);
    State<comp> PT4_22_p_ap (Lambda);
    State<comp> PT4_22_p_pa (Lambda);
    State<comp> PT4_22_p_pp (Lambda);
    State<comp> PT4_22_t_aa (Lambda);
    State<comp> PT4_22_t_ap (Lambda);
    State<comp> PT4_22_t_pa (Lambda);
    State<comp> PT4_22_t_pp (Lambda);

    t0 = get_time();

    // a-channel:
    // 3-1: insert all possible PT3 vertices on the left, bare vertex on the right
    bubble_function(PT4_31_a_a1.vertex, PT3_K1a.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT4_31_a_p1.vertex, PT3_K1p.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT4_31_a_t1.vertex, PT3_K1t.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT4_31_a_a2.vertex, PT3_K2a.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT4_31_a_p2.vertex, PT3_K2p.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT4_31_a_t2.vertex, PT3_K2t.vertex, bare.vertex, G, G, 'a', false);

    if (write_flag) {
        write_hdf("PT4_31_a_a1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_a_a1);
        write_hdf("PT4_31_a_p1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_a_p1);
        write_hdf("PT4_31_a_t1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_a_t1);
        write_hdf("PT4_31_a_a2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_a_a2);
        write_hdf("PT4_31_a_p2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_a_p2);
        write_hdf("PT4_31_a_t2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_a_t2);
    }

    // sum of contributions obtained from different insertions
    PT4_31.vertex[0].avertex = PT4_31_a_a1.vertex[0].avertex
                             + PT4_31_a_p1.vertex[0].avertex
                             + PT4_31_a_t1.vertex[0].avertex
                             + PT4_31_a_a2.vertex[0].avertex
                             + PT4_31_a_p2.vertex[0].avertex
                             + PT4_31_a_t2.vertex[0].avertex;

    // 1-3: insert bare vertex on the left, all possible PT3 vertices on the right
    // this can only give a K1 contribution, since we do not compute K2'
    bubble_function(PT4_13_a_a1.vertex, bare.vertex, PT3_K1a.vertex, G, G, 'a', false);
    // the following should all give zero
    bubble_function(PT4_13_a_p1.vertex, bare.vertex, PT3_K1p.vertex, G, G, 'a', false);
    bubble_function(PT4_13_a_t1.vertex, bare.vertex, PT3_K1t.vertex, G, G, 'a', false);
    bubble_function(PT4_13_a_a2.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false);
    bubble_function(PT4_13_a_p2.vertex, bare.vertex, PT3_K2p.vertex, G, G, 'a', false);
    bubble_function(PT4_13_a_t2.vertex, bare.vertex, PT3_K2t.vertex, G, G, 'a', false);

    if (write_flag) {
        write_hdf("PT4_13_a_a1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_a_a1);
        write_hdf("PT4_13_a_p1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_a_p1);
        write_hdf("PT4_13_a_t1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_a_t1);
        write_hdf("PT4_13_a_a2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_a_a2);
        write_hdf("PT4_13_a_p2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_a_p2);
        write_hdf("PT4_13_a_t2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_a_t2);
    }

    // sum of contributions obtained from different insertions
    PT4_13.vertex[0].avertex = PT4_13_a_a1.vertex[0].avertex
                             + PT4_13_a_p1.vertex[0].avertex
                             + PT4_13_a_t1.vertex[0].avertex
                             + PT4_13_a_a2.vertex[0].avertex
                             + PT4_13_a_p2.vertex[0].avertex
                             + PT4_13_a_t2.vertex[0].avertex;

    // 2-2: insert all possible PT2 vertices on the left and right (PT2_K1t is always zero -> neglect it)
    bubble_function(PT4_22_a_aa.vertex, PT2_K1a.vertex, PT2_K1a.vertex, G, G, 'a', false);
    bubble_function(PT4_22_a_ap.vertex, PT2_K1a.vertex, PT2_K1p.vertex, G, G, 'a', false);
    bubble_function(PT4_22_a_pa.vertex, PT2_K1p.vertex, PT2_K1a.vertex, G, G, 'a', false);
    bubble_function(PT4_22_a_pp.vertex, PT2_K1p.vertex, PT2_K1p.vertex, G, G, 'a', false);

    if (write_flag) {
        write_hdf("PT4_22_a_aa_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_a_aa);
        write_hdf("PT4_22_a_ap_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_a_ap);
        write_hdf("PT4_22_a_pa_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_a_pa);
        write_hdf("PT4_22_a_pp_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_a_pp);
    }

    // sum of contributions obtained from different insertions
    PT4_22.vertex[0].avertex = PT4_22_a_aa.vertex[0].avertex
                             + PT4_22_a_ap.vertex[0].avertex
                             + PT4_22_a_pa.vertex[0].avertex
                             + PT4_22_a_pp.vertex[0].avertex;

    print("Computed a-channel in PT4.", true);

    // p-channel:
    // 3-1: insert all possible PT3 vertices on the left, bare vertex on the right
    bubble_function(PT4_31_p_a1.vertex, PT3_K1a.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT4_31_p_p1.vertex, PT3_K1p.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT4_31_p_t1.vertex, PT3_K1t.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT4_31_p_a2.vertex, PT3_K2a.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT4_31_p_p2.vertex, PT3_K2p.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT4_31_p_t2.vertex, PT3_K2t.vertex, bare.vertex, G, G, 'p', false);

    if (write_flag) {
        write_hdf("PT4_31_p_a1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_p_a1);
        write_hdf("PT4_31_p_p1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_p_p1);
        write_hdf("PT4_31_p_t1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_p_t1);
        write_hdf("PT4_31_p_a2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_p_a2);
        write_hdf("PT4_31_p_p2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_p_p2);
        write_hdf("PT4_31_p_t2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_p_t2);
    }

    // sum of contributions obtained from different insertions
    PT4_31.vertex[0].pvertex = PT4_31_p_a1.vertex[0].pvertex
                             + PT4_31_p_p1.vertex[0].pvertex
                             + PT4_31_p_t1.vertex[0].pvertex
                             + PT4_31_p_a2.vertex[0].pvertex
                             + PT4_31_p_p2.vertex[0].pvertex
                             + PT4_31_p_t2.vertex[0].pvertex;

    // 1-3: insert bare vertex on the left, all possible PT3 vertices on the right
    // this can only give a K1 contribution, since we do not compute K2'
    bubble_function(PT4_13_p_p1.vertex, bare.vertex, PT3_K1p.vertex, G, G, 'p', false);
    // the following should all give zero
    bubble_function(PT4_13_p_a1.vertex, bare.vertex, PT3_K1a.vertex, G, G, 'p', false);
    bubble_function(PT4_13_p_t1.vertex, bare.vertex, PT3_K1t.vertex, G, G, 'p', false);
    bubble_function(PT4_13_p_a2.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'p', false);
    bubble_function(PT4_13_p_p2.vertex, bare.vertex, PT3_K2p.vertex, G, G, 'p', false);
    bubble_function(PT4_13_p_t2.vertex, bare.vertex, PT3_K2t.vertex, G, G, 'p', false);

    if (write_flag) {
        write_hdf("PT4_13_p_a1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_p_a1);
        write_hdf("PT4_13_p_p1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_p_p1);
        write_hdf("PT4_13_p_t1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_p_t1);
        write_hdf("PT4_13_p_a2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_p_a2);
        write_hdf("PT4_13_p_p2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_p_p2);
        write_hdf("PT4_13_p_t2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_p_t2);
    }

    // sum of contributions obtained from different insertions
    PT4_13.vertex[0].pvertex = PT4_13_p_a1.vertex[0].pvertex
                             + PT4_13_p_p1.vertex[0].pvertex
                             + PT4_13_p_t1.vertex[0].pvertex
                             + PT4_13_p_a2.vertex[0].pvertex
                             + PT4_13_p_p2.vertex[0].pvertex
                             + PT4_13_p_t2.vertex[0].pvertex;

    // 2-2: insert all possible PT2 vertices on the left and right (PT2_K1t is always zero -> neglect it)
    bubble_function(PT4_22_p_aa.vertex, PT2_K1a.vertex, PT2_K1a.vertex, G, G, 'p', false);
    bubble_function(PT4_22_p_ap.vertex, PT2_K1a.vertex, PT2_K1p.vertex, G, G, 'p', false);
    bubble_function(PT4_22_p_pa.vertex, PT2_K1p.vertex, PT2_K1a.vertex, G, G, 'p', false);
    bubble_function(PT4_22_p_pp.vertex, PT2_K1p.vertex, PT2_K1p.vertex, G, G, 'p', false);

    if (write_flag) {
        write_hdf("PT4_22_p_aa_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_p_aa);
        write_hdf("PT4_22_p_ap_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_p_ap);
        write_hdf("PT4_22_p_pa_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_p_pa);
        write_hdf("PT4_22_p_pp_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_p_pp);
    }

    // sum of contributions obtained from different insertions
    PT4_22.vertex[0].pvertex = PT4_22_p_aa.vertex[0].pvertex
                             + PT4_22_p_ap.vertex[0].pvertex
                             + PT4_22_p_pa.vertex[0].pvertex
                             + PT4_22_p_pp.vertex[0].pvertex;

    print("Computed p-channel in PT4.", true);

    // t-channel:
    // in the t-channel, we need to insert a and t simultaneously due to a <-> t symmetry // TODO: remove?
    // (spin sum in the t-channel makes use of this symmetry)                             // TODO: remove?

    // 3-1: insert all possible PT3 vertices on the left, bare vertex on the right
    bubble_function(PT4_31_t_a1.vertex, PT3_K1a.vertex, bare.vertex, G, G, 't', false);
    bubble_function(PT4_31_t_p1.vertex, PT3_K1p.vertex, bare.vertex, G, G, 't', false);
    bubble_function(PT4_31_t_t1.vertex, PT3_K1t.vertex, bare.vertex, G, G, 't', false);
    bubble_function(PT4_31_t_a2.vertex, PT3_K2a.vertex, bare.vertex, G, G, 't', false);
    bubble_function(PT4_31_t_p2.vertex, PT3_K2p.vertex, bare.vertex, G, G, 't', false);
    bubble_function(PT4_31_t_t2.vertex, PT3_K2t.vertex, bare.vertex, G, G, 't', false);

    if (write_flag) {
        write_hdf("PT4_31_t_a1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_t_a1);
        write_hdf("PT4_31_t_p1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_t_p1);
        write_hdf("PT4_31_t_t1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_t_t1);
        write_hdf("PT4_31_t_a2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_t_a2);
        write_hdf("PT4_31_t_p2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_t_p2);
        write_hdf("PT4_31_t_t2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31_t_t2);
    }

    // sum of contributions obtained from different insertions
    PT4_31.vertex[0].tvertex = PT4_31_t_a1.vertex[0].tvertex
                             + PT4_31_t_p1.vertex[0].tvertex
                             + PT4_31_t_t1.vertex[0].tvertex
                             + PT4_31_t_a2.vertex[0].tvertex
                             + PT4_31_t_p2.vertex[0].tvertex
                             + PT4_31_t_t2.vertex[0].tvertex;

    // 1-3: insert bare vertex on the left, all possible PT3 vertices on the right
    bubble_function(PT4_13_t_a1.vertex, bare.vertex, PT3_K1a.vertex, G, G, 't', false);
    bubble_function(PT4_13_t_p1.vertex, bare.vertex, PT3_K1p.vertex, G, G, 't', false);
    bubble_function(PT4_13_t_t1.vertex, bare.vertex, PT3_K1t.vertex, G, G, 't', false);
    bubble_function(PT4_13_t_a2.vertex, bare.vertex, PT3_K2a.vertex, G, G, 't', false);
    bubble_function(PT4_13_t_p2.vertex, bare.vertex, PT3_K2p.vertex, G, G, 't', false);
    bubble_function(PT4_13_t_t2.vertex, bare.vertex, PT3_K2t.vertex, G, G, 't', false);

    if (write_flag) {
        write_hdf("PT4_13_t_a1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_t_a1);
        write_hdf("PT4_13_t_p1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_t_p1);
        write_hdf("PT4_13_t_t1_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_t_t1);
        write_hdf("PT4_13_t_a2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_t_a2);
        write_hdf("PT4_13_t_p2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_t_p2);
        write_hdf("PT4_13_t_t2_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13_t_t2);
    }

    // sum of contributions obtained from different insertions
    PT4_13.vertex[0].tvertex = PT4_13_t_a1.vertex[0].tvertex
                             + PT4_13_t_p1.vertex[0].tvertex
                             + PT4_13_t_t1.vertex[0].tvertex
                             + PT4_13_t_a2.vertex[0].tvertex
                             + PT4_13_t_p2.vertex[0].tvertex
                             + PT4_13_t_t2.vertex[0].tvertex;

    // 2-2: insert all possible PT2 vertices on the left and right (PT2_K1t is always zero -> neglect it)
    bubble_function(PT4_22_t_aa.vertex, PT2_K1a.vertex, PT2_K1a.vertex, G, G, 't', false);
    bubble_function(PT4_22_t_ap.vertex, PT2_K1a.vertex, PT2_K1p.vertex, G, G, 't', false);
    bubble_function(PT4_22_t_pa.vertex, PT2_K1p.vertex, PT2_K1a.vertex, G, G, 't', false);
    bubble_function(PT4_22_t_pp.vertex, PT2_K1p.vertex, PT2_K1p.vertex, G, G, 't', false);

    if (write_flag) {
        write_hdf("PT4_22_t_aa_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_t_aa);
        write_hdf("PT4_22_t_ap_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_t_ap);
        write_hdf("PT4_22_t_pa_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_t_pa);
        write_hdf("PT4_22_t_pp_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22_t_pp);
    }

    // sum of contributions obtained from different insertions
    PT4_22.vertex[0].tvertex = PT4_22_t_aa.vertex[0].tvertex
                             + PT4_22_t_ap.vertex[0].tvertex
                             + PT4_22_t_pa.vertex[0].tvertex
                             + PT4_22_t_pp.vertex[0].tvertex;


    print("Computed t-channel in PT4.", true);

    if (write_flag) {
        write_hdf("PT4_31_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_31);
        write_hdf("PT4_13_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_13);
        write_hdf("PT4_22_U" + to_string(1. / ((glb_Gamma + Lambda) / 2.)) + ".h5", Lambda, 1, PT4_22);
    }

    print("Computed K1, K2, K3 in PT4.", true);
    get_time(t0);

    /** Make automated checks of all diagrams: Compute the values of all diagrams at all frequencies equal to zero,
     * and for all pairs of diagrams that should cancel, compute the relative deviation of their sum from zero.
     * Print all results to log.
     * */

    print("--- CHECK RESULTS: ---", true);
    print("--- print relative error of quantities that should be zero ---", true);

    // input variables: all frequencies equal to zero
    VertexInput input_a (0, 0., 0., 0., 0, 0, 'a');
    VertexInput input_p (0, 0., 0., 0., 0, 0, 'p');
    VertexInput input_t (0, 0., 0., 0., 0, 0, 't');

    vec<int> iK2s = {1, 2, 4}; // Keldysh indices of fully retarded components of K2

    // labels to be printed to log
    string check_labels[] {"PT2: K1a + K1p: ", "PT2: K1t: ",
                           "PT3: K1a - exact: ", "PT3: K1p - exact: ", "PT3: K1t - exact: ",
                           "PT3: K2a[1] - exact: ", "PT3: K2p[1] - exact: ", "PT3: K2t[1] - exact: ",
                           "PT3: K2a[2] - exact: ", "PT3: K2p[2] - exact: ", "PT3: K2t[2] - exact: ",
                           "PT3: K2a[4] - exact: ", "PT3: K2p[4] - exact: ", "PT3: K2t[4] - exact: ",
                           "PT4: (K2a[1] <- K1p) + (K2p[1] <- K1a): ",
                           "PT4: (K2a[2] <- K1p) + (K2p[2] <- K1a): ",
                           "PT4: (K2a[4] <- K1p) + (K2p[4] <- K1a): ",
                           "PT4: (K2a[1] <- K1t) + (K2p[1] <- K1t): ",
                           "PT4: (K2a[2] <- K1t) + (K2p[2] <- K1t): ",
                           "PT4: (K2a[4] <- K1t) + (K2p[4] <- K1t): ",
                           "PT4: (K2a[1] <- K2a) + (K2p[1] <- K2p): ",
                           "PT4: (K2a[2] <- K2a) + (K2p[2] <- K2p): ",
                           "PT4: (K2a[4] <- K2a) + (K2p[4] <- K2p): ",
                           "PT4: (K2a[1] <- K2p) + (K2p[1] <- K2a): ",
                           "PT4: (K2a[2] <- K2p) + (K2p[2] <- K2a): ",
                           "PT4: (K2a[4] <- K2p) + (K2p[4] <- K2a): ",
                           "PT4: (K2a[1] <- K2t) + (K2p[1] <- K2t): ",
                           "PT4: (K2a[2] <- K2t) + (K2p[2] <- K2t): ",
                           "PT4: (K2a[4] <- K2t) + (K2p[4] <- K2t): ",
                           "PT4: (K2t[1] <- K2a) + (K2t[1] <- K2t): ",
                           "PT4: (K2t[2] <- K2a) + (K2t[2] <- K2t): ",
                           "PT4: (K2t[4] <- K2a) + (K2t[4] <- K2t): ",
                           "PT4: K3a + K3p: ",
                           "PT4: K3t (aa) + K3t (ap) + K3t (pa): "
                           };

    // K1 in PT2
    comp PT2_K1a_0 = PT2_K1a.vertex[0].avertex.K1_valsmooth(input_a);
    comp PT2_K1p_0 = PT2_K1p.vertex[0].pvertex.K1_valsmooth(input_p);
    comp PT2_K1t_0 = PT2_K1t.vertex[0].tvertex.K1_valsmooth(input_t);

    // K1 in PT3
    comp PT3_K1a_0 = PT3_K1a.vertex[0].avertex.K1_valsmooth(input_a);
    comp PT3_K1p_0 = PT3_K1p.vertex[0].pvertex.K1_valsmooth(input_p);
    comp PT3_K1t_0 = PT3_K1t.vertex[0].tvertex.K1_valsmooth(input_t);
    comp PT3_K1_exact = -(1./2.) * pow(glb_U / (M_PI * (glb_Gamma + Lambda) / 2.), 2);

    // K1 in PT4
    comp PT4_K1a_0_ladder = PT4_31_a_a1.vertex[0].avertex.K1_valsmooth(input_a);
    comp PT4_K1p_0_ladder = PT4_31_p_p1.vertex[0].pvertex.K1_valsmooth(input_p);
    comp PT4_K1a_0_nonladder = PT4_13_a_a2.vertex[0].avertex.K1_valsmooth(input_a);
    comp PT4_K1p_0_nonladder = PT4_13_p_p2.vertex[0].pvertex.K1_valsmooth(input_p);
    comp PT4_K1t_0_nonladder_a = PT4_13_t_a2.vertex[0].tvertex.K1_valsmooth(input_t);
    comp PT4_K1t_0_nonladder_t = PT4_13_t_t2.vertex[0].tvertex.K1_valsmooth(input_t);

    // K2 in PT3
    vec<comp> PT3_K2a_0 (3);
    vec<comp> PT3_K2p_0 (3);
    vec<comp> PT3_K2t_0 (3);
    // K2 in PT4
    vec<comp> PT4_K2a_0_p1 (3);
    vec<comp> PT4_K2p_0_a1 (3);
    vec<comp> PT4_K2a_0_t1 (3);
    vec<comp> PT4_K2p_0_t1 (3);
    vec<comp> PT4_K2a_0_a2 (3);
    vec<comp> PT4_K2a_0_p2 (3);
    vec<comp> PT4_K2a_0_t2 (3);
    vec<comp> PT4_K2p_0_a2 (3);
    vec<comp> PT4_K2p_0_p2 (3);
    vec<comp> PT4_K2p_0_t2 (3);
    vec<comp> PT4_K2t_0_a2 (3);
    vec<comp> PT4_K2t_0_t2 (3);

#if DIAG_CLASS >= 2
    for (int iK2=0; iK2<2; ++iK2) {
        input_a.iK = iK2s[iK2];
        input_p.iK = iK2s[iK2];
        input_t.iK = iK2s[iK2];
        // K2 in PT3
        PT3_K2a_0[iK2] = PT3_K2a.vertex[0].avertex.K2_valsmooth(input_a);
        PT3_K2p_0[iK2] = PT3_K2p.vertex[0].pvertex.K2_valsmooth(input_p);
        PT3_K2t_0[iK2] = PT3_K2t.vertex[0].tvertex.K2_valsmooth(input_t);
        // K2 in PT4
        PT4_K2a_0_p1[iK2] = PT4_31_a_p1.vertex[0].avertex.K2_valsmooth(input_a);
        PT4_K2p_0_a1[iK2] = PT4_31_p_a1.vertex[0].pvertex.K2_valsmooth(input_p);
        PT4_K2a_0_t1[iK2] = PT4_31_a_t1.vertex[0].avertex.K2_valsmooth(input_a);
        PT4_K2p_0_t1[iK2] = PT4_31_p_t1.vertex[0].pvertex.K2_valsmooth(input_p);
        PT4_K2a_0_a2[iK2] = PT4_31_a_a2.vertex[0].avertex.K2_valsmooth(input_a);
        PT4_K2a_0_p2[iK2] = PT4_31_a_p2.vertex[0].avertex.K2_valsmooth(input_a);
        PT4_K2a_0_t2[iK2] = PT4_31_a_t2.vertex[0].avertex.K2_valsmooth(input_a);
        PT4_K2p_0_a2[iK2] = PT4_31_p_a2.vertex[0].avertex.K2_valsmooth(input_p);
        PT4_K2p_0_p2[iK2] = PT4_31_p_p2.vertex[0].avertex.K2_valsmooth(input_p);
        PT4_K2p_0_t2[iK2] = PT4_31_p_t2.vertex[0].avertex.K2_valsmooth(input_p);
        PT4_K2t_0_a2[iK2] = PT4_31_t_a2.vertex[0].avertex.K2_valsmooth(input_t);
        PT4_K2t_0_t2[iK2] = PT4_31_t_t2.vertex[0].avertex.K2_valsmooth(input_t);
    }
#endif
    comp PT3_K2_exact = -(1./2.) * (2. - M_PI*M_PI/4.) * pow(glb_U / (M_PI * (glb_Gamma + Lambda) / 2.), 2);

    // K3 in PT4
    input_a.iK = 5;
    input_p.iK = 5;
    input_t.iK = 5;
    comp PT4_K3a_0;
    comp PT4_K3p_0;
    comp PT4_K3t_0_aa;
    comp PT4_K3t_0_ap;
    comp PT4_K3t_0_pa;

#if DIAG_CLASS == 3
    PT4_K3a_0 = PT4_22_a_pp.vertex[0].avertex.K3_valsmooth(input_a);
    PT4_K3p_0 = PT4_22_p_aa.vertex[0].pvertex.K3_valsmooth(input_p);
    PT4_K3t_0_aa = PT4_22_t_aa.vertex[0].tvertex.K3_valsmooth(input_t);
    PT4_K3t_0_ap = PT4_22_t_ap.vertex[0].tvertex.K3_valsmooth(input_t);
    PT4_K3t_0_pa = PT4_22_t_pa.vertex[0].tvertex.K3_valsmooth(input_t);
#endif

    // values to be printed to log
    vec<comp> check_values {PT2_K1a_0 + PT2_K1p_0,
                            PT2_K1t_0,
                            (PT3_K1a_0 - PT3_K1_exact)/PT3_K1_exact,
                            (PT3_K1p_0 - PT3_K1_exact)/PT3_K1_exact,
                            (PT3_K1t_0 - PT3_K1_exact)/PT3_K1_exact,
                            (PT3_K2a_0[0] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2p_0[0] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2t_0[0] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2a_0[1] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2p_0[1] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2t_0[1] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2a_0[2] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2p_0[2] - PT3_K2_exact)/PT3_K2_exact,
                            (PT3_K2t_0[2] - PT3_K2_exact)/PT3_K2_exact,
                            (PT4_K2a_0_p1[0] + PT4_K2p_0_a1[0])/(abs(PT4_K2a_0_p1[0]) + abs(PT4_K2p_0_a1[0])),
                            (PT4_K2a_0_p1[1] + PT4_K2p_0_a1[1])/(abs(PT4_K2a_0_p1[1]) + abs(PT4_K2p_0_a1[2])),
                            (PT4_K2a_0_p1[2] + PT4_K2p_0_a1[2])/(abs(PT4_K2a_0_p1[2]) + abs(PT4_K2p_0_a1[0])),
                            (PT4_K2a_0_t1[0] + PT4_K2p_0_t1[0])/(abs(PT4_K2a_0_t1[0]) + abs(PT4_K2p_0_t1[0])),
                            (PT4_K2a_0_t1[1] + PT4_K2p_0_t1[1])/(abs(PT4_K2a_0_t1[1]) + abs(PT4_K2p_0_t1[2])),
                            (PT4_K2a_0_t1[2] + PT4_K2p_0_t1[2])/(abs(PT4_K2a_0_t1[2]) + abs(PT4_K2p_0_t1[0])),
                            (PT4_K2a_0_a2[0] + PT4_K2p_0_p2[0])/(abs(PT4_K2a_0_a2[0]) + abs(PT4_K2p_0_p2[0])),
                            (PT4_K2a_0_a2[1] + PT4_K2p_0_p2[1])/(abs(PT4_K2a_0_a2[1]) + abs(PT4_K2p_0_p2[2])),
                            (PT4_K2a_0_a2[2] + PT4_K2p_0_p2[2])/(abs(PT4_K2a_0_a2[2]) + abs(PT4_K2p_0_p2[0])),
                            (PT4_K2a_0_p2[0] + PT4_K2p_0_a2[0])/(abs(PT4_K2a_0_p2[0]) + abs(PT4_K2p_0_a2[0])),
                            (PT4_K2a_0_p2[1] + PT4_K2p_0_a2[1])/(abs(PT4_K2a_0_p2[1]) + abs(PT4_K2p_0_a2[2])),
                            (PT4_K2a_0_p2[2] + PT4_K2p_0_a2[2])/(abs(PT4_K2a_0_p2[2]) + abs(PT4_K2p_0_a2[0])),
                            (PT4_K2a_0_t2[0] + PT4_K2p_0_t2[0])/(abs(PT4_K2a_0_t2[0]) + abs(PT4_K2p_0_t2[0])),
                            (PT4_K2a_0_t2[1] + PT4_K2p_0_t2[1])/(abs(PT4_K2a_0_t2[1]) + abs(PT4_K2p_0_t2[2])),
                            (PT4_K2a_0_t2[2] + PT4_K2p_0_t2[2])/(abs(PT4_K2a_0_t2[2]) + abs(PT4_K2p_0_t2[0])),
                            (PT4_K2t_0_a2[0] + PT4_K2t_0_t2[0])/(abs(PT4_K2t_0_a2[0]) + abs(PT4_K2t_0_t2[0])),
                            (PT4_K2t_0_a2[1] + PT4_K2t_0_t2[1])/(abs(PT4_K2t_0_a2[1]) + abs(PT4_K2t_0_t2[2])),
                            (PT4_K2t_0_a2[2] + PT4_K2t_0_t2[2])/(abs(PT4_K2t_0_a2[2]) + abs(PT4_K2t_0_t2[0])),
                            (PT4_K3a_0 + PT4_K3p_0)/(abs(PT4_K3a_0) + abs(PT4_K3p_0)),
                            (PT4_K3t_0_aa + PT4_K3t_0_ap + PT4_K3t_0_pa)/(abs(PT4_K3t_0_aa) + abs(PT4_K3t_0_ap) + abs(PT4_K3t_0_pa))
                            };

    // print to log
    for (int i=0; i<check_values.size(); ++i) {
        print(check_labels[i], check_values[i], true);
    }


    /*
    // Compute K1a contributions in PT4, using
    // (22):   K1a in PT2
    // (13_1): K1a in PT3
    // (13_2): K2a in PT3
    // (31_2): K2a in PT3
    State<comp> PT4_K1a22 (Lambda);
    State<comp> PT4_K1a13_1 (Lambda);
    State<comp> PT4_K1a13_2 (Lambda);
    State<comp> PT4_K1a31_2 (Lambda);

    t0 = get_time();
    bubble_function(PT4_K1a22.vertex, PT2_K1a.vertex, PT2_K1a.vertex, G, G, 'a', false);
    bubble_function(PT4_K1a13_1.vertex, bare.vertex, PT3_K1a.vertex, G, G, 'a', false);
    bubble_function(PT4_K1a13_2.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false);
    bubble_function(PT4_K1a31_2.vertex, PT3_K2a.vertex, bare.vertex, G, G, 'a', false);
    print("Computed K1 in PT4.", true);
    get_time(t0);

    // FDT checks
    print("Check K1a in PT4 (22):", true);
    check_FDTs(PT4_K1a22);
    print("Check K1a in PT4 (13_1):", true);
    check_FDTs(PT4_K1a13_1);
    print("Check K1a in PT4 (13_2):", true);
    check_FDTs(PT4_K1a13_2);
    print("Check K1a in PT4 (31_2):", true);
    check_FDTs(PT4_K1a31_2);
    // */
}

#if DIAG_CLASS == 3
/**
 * Test K3 dynamics by computing SE diagrams in PT4 using different PT4 vertices, which should all give the same result.
 */
void test_K3_dynamics_SE_PT4(double Lambda) {
    // Initialize a bare state
    State<comp> bare (Lambda);
    bare.initialize();

    // Initialize a bare propagator
    Propagator G(Lambda, bare.selfenergy, 'g');

    // Compute K1a and K1p in PT2
    State<comp> PT2_K1a (Lambda);
    State<comp> PT2_K1p (Lambda);

    bubble_function(PT2_K1a.vertex, bare.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT2_K1p.vertex, bare.vertex, bare.vertex, G, G, 'p', false);

    // Compute K1a and K1p in PT3
    State<comp> PT3_K1a (Lambda);
    State<comp> PT3_K1p (Lambda);

    bubble_function(PT3_K1a.vertex, PT2_K1a.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT3_K1p.vertex, PT2_K1p.vertex, bare.vertex, G, G, 'p', false);

    // Compute K3a, K1p (ladder), K3p, K1a (ladder) in PT4
    State<comp> PT4_22_a_pp (Lambda);
    State<comp> PT4_13_p_p1 (Lambda);
    State<comp> PT4_22_p_aa (Lambda);
    State<comp> PT4_13_a_a1 (Lambda);

    bubble_function(PT4_22_a_pp.vertex, PT2_K1p.vertex, PT2_K1p.vertex, G, G, 'a', false);
    bubble_function(PT4_13_p_p1.vertex, bare.vertex, PT3_K1p.vertex, G, G, 'p', false);
    bubble_function(PT4_22_p_aa.vertex, PT2_K1a.vertex, PT2_K1a.vertex, G, G, 'p', false);
    bubble_function(PT4_13_a_a1.vertex, bare.vertex, PT3_K1a.vertex, G, G, 'a', false);

    // a-channel:
    // close K3a (single diagram: PT2_K1p - a-bubble - PT2_K1p)
    State<comp> SE_K3a (Lambda);
    loop(SE_K3a.selfenergy, PT4_22_a_pp.vertex, G, false);

    // close K1p ladder (use 1-3 vertex in p-channel, since it contains only p-ladder)
    State<comp> SE_K1p_ladder (Lambda);
    loop(SE_K1p_ladder.selfenergy, PT4_13_p_p1.vertex, G, false);

    // p-channel:
    // close K3p (single diagram: PT2_K1a - p-bubble - PT2_K1a)
    State<comp> SE_K3p (Lambda);
    loop(SE_K3p.selfenergy, PT4_22_p_aa.vertex, G, false);

    // close K1a ladder (use 1-3 vertex in a-channel, since it contains only a-ladder)
    State<comp> SE_K1a_ladder (Lambda);
    loop(SE_K1a_ladder.selfenergy, PT4_13_a_a1.vertex, G, false);

    write_hdf("SE_K3a_U" + to_string(1./((glb_Gamma+Lambda)/2.)) + ".h5", Lambda, 1, SE_K3a);
    write_hdf("SE_K1p_ladder_U" + to_string(1./((glb_Gamma+Lambda)/2.)) + ".h5", Lambda, 1, SE_K1p_ladder);
    write_hdf("SE_K3p_U" + to_string(1./((glb_Gamma+Lambda)/2.)) + ".h5", Lambda, 1, SE_K3p);
    write_hdf("SE_K1a_ladder_U" + to_string(1./((glb_Gamma+Lambda)/2.)) + ".h5", Lambda, 1, SE_K1a_ladder);
}
#endif



#if DIAG_CLASS >= 2
/**
 * Function to test correctness of K2a when calculating a susceptibility (a K1a-object) //Notice that the same calculation
 * can be performed in the p-channel.
 * @param Lambda : Scale at which the calculation is done.
 */
void test_K2_correctness(double Lambda){

    bool write_flag = true; //Write out results in a HDF5 file

    State<comp> bare (Lambda);   //Bare state
    bare.initialize();  //Initialize bare state

    Propagator G(Lambda, bare.selfenergy, 'g'); //Bare propagator
    Propagator S(Lambda, bare.selfenergy, 's'); //Bare single-scale propagator

    //Create states for K1-calculations
    State<comp> PT2_K1a (Lambda);
    State<comp> PT2_K1p (Lambda);
    State<comp> PT2_K1t (Lambda);

    //Save K1-bubbles in separate objects - SOPT
    double t0 = get_time();
    bubble_function(PT2_K1a.vertex, bare.vertex, bare.vertex, G, G, 'a', false);
    bubble_function(PT2_K1p.vertex, bare.vertex, bare.vertex, G, G, 'p', false);
    bubble_function(PT2_K1t.vertex, bare.vertex, bare.vertex, G, G, 't', false);
    get_time(t0);

    State<comp> PT2_SE_a (Lambda);
    State<comp> PT2_SE_p (Lambda);
    State<comp> PT2_SE_t (Lambda);
    State<comp> PT2_SE_p_1 (Lambda);
    State<comp> PT2_SE_p_4 (Lambda);
    State<comp> PT2_SE_p_5 (Lambda);

    loop(PT2_SE_a.selfenergy, PT2_K1a.vertex, S, true);
    loop(PT2_SE_p.selfenergy, PT2_K1p.vertex, S, true);
    loop(PT2_SE_t.selfenergy, PT2_K1t.vertex, S, true);
#ifdef DEBUG_MODE
    loop(PT2_SE_p_1.selfenergy, PT2_K1p.vertex, S, true, 1);
    loop(PT2_SE_p_4.selfenergy, PT2_K1p.vertex, S, true, 4);
    loop(PT2_SE_p_5.selfenergy, PT2_K1p.vertex, S, true, 5);
#endif

    State<comp> PT3_K2a (Lambda);    //Create state for K2a calculation
    State<comp> PT3_K2a_ia (Lambda);
    State<comp> PT3_K2a_ib (Lambda);
    State<comp> PT3_K2a_iia (Lambda);
    State<comp> PT3_K2a_iib (Lambda);
    State<comp> PT3_K2a_iva (Lambda);
    State<comp> PT3_K2a_ivb (Lambda);
    State<comp> PT3_K2a_t (Lambda);

    State<comp> PT3_K2p (Lambda);
    State<comp> PT3_K2t (Lambda);
    State<comp> PT3_K2t_a (Lambda);
    State<comp> PT3_K2t_p (Lambda);

    //Do appropriate calculation for K2a with K1p and K1t being fed back into the left vertex. Notice part = 'L' to ensure
    //that the correct contributions are added on both sides. - TOPT
    t0 = get_time();
    bubble_function(PT3_K2a.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false);   // K2a in PT3

#ifdef DEBUG_MODE
    bubble_function(PT3_K2a_ia.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false, 'L', 16, 16, 9, 6);
    bubble_function(PT3_K2a_ib.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false, 'L', 16, 16, 6, 9);
    bubble_function(PT3_K2a_iia.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false, 'L', 16, 16, 11, 14);
    bubble_function(PT3_K2a_iib.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false, 'L', 16, 16, 7, 9);
    bubble_function(PT3_K2a_iva.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false, 'L', 16, 16, 9, 6);
    bubble_function(PT3_K2a_ivb.vertex, PT2_K1p.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'a', false, 'L', 16, 16, 6, 15);
#endif

    bubble_function(PT3_K2a_t.vertex, PT2_K1t.vertex, bare.vertex, G, G, 'a', false);   // K2a in PT3
    //PT3_K2a = read_hdf("PT4_check_of_K2a_K2_switchedcc_adap_m3m9_g501_101_nI1501_state_PT3_K2a", 0, 1);

    bubble_function(PT3_K2p.vertex, PT2_K1a.vertex + PT2_K1t.vertex, bare.vertex, G, G, 'p', false);    // K2p  in PT3
    //bubble_function(PT3_K2t.vertex, PT2_K1a.vertex + PT2_K1p.vertex, bare.vertex, G, G, 't', false);    // K2t  in PT3
    bubble_function(PT3_K2t_a.vertex, PT2_K1a.vertex, bare.vertex, G, G, 't', false);    // K2t  in PT3
    bubble_function(PT3_K2t_p.vertex, PT2_K1p.vertex, bare.vertex, G, G, 't', false);    // K2t  in PT3
    PT3_K2t.vertex = PT3_K2t_a.vertex + PT3_K2t_p.vertex;
    get_time(t0);

    // full K2 in PT3
    State<comp> PT3_K2 (Lambda);
    PT3_K2.vertex[0].avertex = PT3_K2a_t.vertex[0].avertex;
    //PT3_K2.vertex[0].pvertex = PT3_K2p.vertex[0].pvertex;
    PT3_K2.vertex[0].tvertex = PT3_K2t_a.vertex[0].tvertex;

    State<comp> PT3_K2at (Lambda);

    // K2 contribution to self-energy flow
    State<comp> PT3_SE (Lambda);
    State<comp> PT3_SE_a (Lambda);
    State<comp> PT3_SE_p (Lambda);
    State<comp> PT3_SE_t (Lambda);
    State<comp> PT3_SE_t_1 (Lambda);
    State<comp> PT3_SE_t_4 (Lambda);
    State<comp> PT3_SE_t_5 (Lambda);
    State<comp> PT3_SE_t_a (Lambda);
    State<comp> PT3_SE_t_a_1 (Lambda);
    State<comp> PT3_SE_t_a_4 (Lambda);
    State<comp> PT3_SE_t_a_5 (Lambda);
    State<comp> PT3_SE_t_p (Lambda);
    State<comp> PT3_SE_t_p_1 (Lambda);
    State<comp> PT3_SE_t_p_4 (Lambda);
    State<comp> PT3_SE_t_p_5 (Lambda);

    loop(PT3_SE.selfenergy, PT3_K2.vertex, S, true);
    loop(PT3_SE_a.selfenergy, PT3_K2a.vertex, S, true);
    loop(PT3_SE_p.selfenergy, PT3_K2p.vertex, S, true);
    loop(PT3_SE_t.selfenergy, PT3_K2t.vertex, S, true);

    loop(PT3_SE_t_a.selfenergy, PT3_K2t_a.vertex, S, true);
    loop(PT3_SE_t_p.selfenergy, PT3_K2t_p.vertex, S, true);
#ifdef DEBUG_MODE
    loop(PT3_SE_t_1.selfenergy, PT3_K2t.vertex, S, true, 1);
    loop(PT3_SE_t_4.selfenergy, PT3_K2t.vertex, S, true, 4);
    loop(PT3_SE_t_5.selfenergy, PT3_K2t.vertex, S, true, 5);
    loop(PT3_SE_t_a_1.selfenergy, PT3_K2t_a.vertex, S, true, 1);
    loop(PT3_SE_t_a_4.selfenergy, PT3_K2t_a.vertex, S, true, 4);
    loop(PT3_SE_t_a_5.selfenergy, PT3_K2t_a.vertex, S, true, 5);
    loop(PT3_SE_t_p_1.selfenergy, PT3_K2t_p.vertex, S, true, 1);
    loop(PT3_SE_t_p_4.selfenergy, PT3_K2t_p.vertex, S, true, 4);
    loop(PT3_SE_t_p_5.selfenergy, PT3_K2t_p.vertex, S, true, 5);
#endif

    State<comp> PT3_K1a (Lambda);    //Create state to compare with K1a
    t0 = get_time();
    bubble_function(PT3_K1a.vertex, PT2_K1a.vertex, bare.vertex, G, G, 'a', false);
    get_time(t0);

    State<comp> PT123_a = bare + PT2_K1a + PT3_K1a + PT3_K2a;  //Create vertex of the right side of BSE

    State<comp> PT4_K1a22 (Lambda);
    State<comp> PT4_K1a13_1 (Lambda);
    State<comp> PT4_K1a13_2 (Lambda);
    State<comp> PT4_K1a31_2 (Lambda);
    State<comp> PT4_K1a13_2_11e (Lambda); // A
    State<comp> PT4_K1a13_2_21e (Lambda); // B
    State<comp> PT4_K1a13_2_11o (Lambda); // C
    State<comp> PT4_K1a13_2_21o (Lambda); // D
    State<comp> PT4_K1a13_2_12o (Lambda); // TST3TC D
    State<comp> PT4_K1a13_2_22o (Lambda); // F
    State<comp> PT4_K1a13_2_ia (Lambda);
    State<comp> PT4_K1a13_2_ib (Lambda);
    State<comp> PT4_K1a13_2_iia (Lambda);
    State<comp> PT4_K1a13_2_iib (Lambda);
    State<comp> PT4_K1a13_2_iva (Lambda);
    State<comp> PT4_K1a13_2_ivb (Lambda);

    t0 = get_time();
    bubble_function(PT4_K1a22.vertex, PT2_K1a.vertex, PT2_K1a.vertex, G, G, 'a', false);
    bubble_function(PT4_K1a13_1.vertex, bare.vertex, PT3_K1a.vertex, G, G, 'a', false);

    bubble_function(PT4_K1a13_2.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false);
    bubble_function(PT4_K1a31_2.vertex, PT3_K2a.vertex, bare.vertex, G, G, 'a', false);
    get_time(t0);
    t0 = get_time();
#ifdef DEBUG_MODE
    bubble_function(PT4_K1a13_2_11e.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false, 0, 16, 16, 16); // A
    bubble_function(PT4_K1a13_2_21e.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false, 1, 16, 16, 16); // B
    bubble_function(PT4_K1a13_2_11o.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false, 2, 16, 16, 16); // C
    bubble_function(PT4_K1a13_2_21o.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false, 3, 16, 16, 16); // D
    bubble_function(PT4_K1a13_2_12o.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false, 10, 16, 16, 16); // TST3TC D
    bubble_function(PT4_K1a13_2_22o.vertex, bare.vertex, PT3_K2a.vertex, G, G, 'a', false, 11, 16, 16, 16); // F

    bubble_function(PT4_K1a13_2_ia.vertex, bare.vertex, PT3_K2a_ia.vertex, G, G, 'a', false, 11, 6, 16, 16);
    bubble_function(PT4_K1a13_2_ib.vertex, bare.vertex, PT3_K2a_ib.vertex, G, G, 'a', false, 2, 9, 16, 16);
    bubble_function(PT4_K1a13_2_iia.vertex, bare.vertex, PT3_K2a_iia.vertex, G, G, 'a', false, 11, 6, 16, 16);
    bubble_function(PT4_K1a13_2_iib.vertex, bare.vertex, PT3_K2a_iib.vertex, G, G, 'a', false, 10, 11, 16, 16);
    bubble_function(PT4_K1a13_2_iva.vertex, bare.vertex, PT3_K2a_iva.vertex, G, G, 'a', false, 11, 15, 16, 16);
    bubble_function(PT4_K1a13_2_ivb.vertex, bare.vertex, PT3_K2a_ivb.vertex, G, G, 'a', false, 2, 9, 16, 16);
#endif
    get_time(t0);

    cvec K1a_diff(nBOS);
    for(int iw=0; iw<nBOS; ++iw){
        K1a_diff[iw] = PT4_K1a22.vertex[0].avertex.K1_val(0, iw, 0) - PT2_K1a.vertex[0].avertex.K1_val(0, iw, 0);
    }

    print("Testing correctness of K2a. Using U=" +to_string(glb_U)+ " and Lambda="+to_string(Lambda)+", the maximal difference between direct K1a and K1a over integration of K2a is " +to_string(K1a_diff.max_norm())+"." , true);
    if(write_flag) write_h5_rvecs("PT4_check_of_K2a_cleanup_GL_gW20_51_21_nI1501_U1", {"w",
                                                       "PT2_K1a_R", "PT2_K1a_I",
                                                       "PT2_K1p_R", "PT2_K1p_I",
                                                       "PT2_K1t_R", "PT2_K1t_I",
                                                       "PT2_SE_a_R", "PT2_SE_a_I",
                                                       "PT2_SE_p_R", "PT2_SE_p_I",
                                                       "PT2_SE_t_R", "PT2_SE_t_I",
                                                       "PT2_SE_p_1_R", "PT2_SE_p_1_I",
                                                       "PT2_SE_p_4_R", "PT2_SE_p_4_I",
                                                       "PT2_SE_p_5_R", "PT2_SE_p_5_I",
                                                       "PT3_K1a_R", "PT3_K1a_I",
                                                       "PT3_K2a_R", "PT3_K2a_I",
                                                       "PT3_K2a_ia_R", "PT3_K2a_ia_I",
                                                       "PT3_K2a_ib_R", "PT3_K2a_ib_I",
                                                       "PT3_K2a_iia_R", "PT3_K2a_iia_I",
                                                       "PT3_K2a_iib_R", "PT3_K2a_iib_I",
                                                       "PT3_K2a_iva_R", "PT3_K2a_iva_I",
                                                       "PT3_K2a_ivb_R", "PT3_K2a_ivb_I",
                                                       "PT3_K2a_t_R", "PT3_K2a_t_I",
                                                       "PT3_K2p_R", "PT3_K2p_I",
                                                       "PT3_K2t_R", "PT3_K2t_I",
                                                       "PT3_K2t_a_a_R", "PT3_K2t_a_a_I",
                                                       "PT3_K2t_a_p_R", "PT3_K2t_a_p_I",
                                                       "PT3_K2t_a_t_R", "PT3_K2t_a_t_I",
                                                       "PT3_SE_R", "PT3_SE_I",
                                                       "PT3_SE_a_R", "PT3_SE_a_I",
                                                       "PT3_SE_p_R", "PT3_SE_p_I",
                                                       "PT3_SE_t_R", "PT3_SE_t_I",
                                                       "PT3_SE_t_1_R", "PT3_SE_t_1_I",
                                                       "PT3_SE_t_4_R", "PT3_SE_t_4_I",
                                                       "PT3_SE_t_5_R", "PT3_SE_t_5_I",
                                                       "PT3_SE_t_a_R", "PT3_SE_t_a_I",
                                                       "PT3_SE_t_a_1_R", "PT3_SE_t_a_1_I",
                                                       "PT3_SE_t_a_4_R", "PT3_SE_t_a_4_I",
                                                       "PT3_SE_t_a_5_R", "PT3_SE_t_a_5_I",
                                                       "PT3_SE_t_p_R", "PT3_SE_t_p_I",
                                                       "PT3_SE_t_p_1_R", "PT3_SE_t_p_1_I",
                                                       "PT3_SE_t_p_4_R", "PT3_SE_t_p_4_I",
                                                       "PT3_SE_t_p_5_R", "PT3_SE_t_p_5_I",
                                                       "PT4_K1a22_R", "PT4_K1a22_I",
                                                       "PT4_K1a13_1_R", "PT4_K1a13_1_I",
                                                       "PT4_K1a13_2_R", "PT4_K1a13_2_I",
                                                       "PT4_K1a31_2_R", "PT4_K1a31_2_I",
                                                       "PT4_K1a13_2_11e_R", "PT4_K1a13_2_11e_I",
                                                       "PT4_K1a13_2_21e_R", "PT4_K1a13_2_21e_I",
                                                       "PT4_K1a13_2_11o_R", "PT4_K1a13_2_11o_I",
                                                       "PT4_K1a13_2_21o_R", "PT4_K1a13_2_21o_I",
                                                       "PT4_K1a13_2_12o_R", "PT4_K1a13_2_12o_I",
                                                       "PT4_K1a13_2_22o_R", "PT4_K1a13_2_22o_I",
                                                       "PT4_K1a13_2_ia_R", "PT4_K1a13_2_ia_I",
                                                       "PT4_K1a13_2_ib_R", "PT4_K1a13_2_ib_I",
                                                       "PT4_K1a13_2_iia_R", "PT4_K1a13_2_iia_I",
                                                       "PT4_K1a13_2_iib_R", "PT4_K1a13_2_iib_I",
                                                       "PT4_K1a13_2_iva_R", "PT4_K1a13_2_iva_I",
                                                       "PT4_K1a13_2_ivb_R", "PT4_K1a13_2_ivb_I"
                                                       },
                                  {bfreqs,
                                   PT2_K1a.vertex[0].avertex.K1.real(), PT2_K1a.vertex[0].avertex.K1.imag(),
                                   PT2_K1p.vertex[0].pvertex.K1.real(), PT2_K1p.vertex[0].pvertex.K1.imag(),
                                   PT2_K1t.vertex[0].tvertex.K1.real(), PT2_K1t.vertex[0].tvertex.K1.imag(),
                                   PT2_SE_a.selfenergy.Sigma.real(), PT2_SE_a.selfenergy.Sigma.imag(),
                                   PT2_SE_p.selfenergy.Sigma.real(), PT2_SE_p.selfenergy.Sigma.imag(),
                                   PT2_SE_t.selfenergy.Sigma.real(), PT2_SE_t.selfenergy.Sigma.imag(),
                                   PT2_SE_p_1.selfenergy.Sigma.real(), PT2_SE_p_1.selfenergy.Sigma.imag(),
                                   PT2_SE_p_4.selfenergy.Sigma.real(), PT2_SE_p_4.selfenergy.Sigma.imag(),
                                   PT2_SE_p_5.selfenergy.Sigma.real(), PT2_SE_p_5.selfenergy.Sigma.imag(),
                                   PT3_K1a.vertex[0].avertex.K1.real(), PT3_K1a.vertex[0].avertex.K1.imag(),
                                   PT3_K2a.vertex[0].avertex.K2.real(), PT3_K2a.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_ia.vertex[0].avertex.K2.real(), PT3_K2a_ia.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_ib.vertex[0].avertex.K2.real(), PT3_K2a_ib.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_iia.vertex[0].avertex.K2.real(), PT3_K2a_iia.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_iib.vertex[0].avertex.K2.real(), PT3_K2a_iib.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_iva.vertex[0].avertex.K2.real(), PT3_K2a_iva.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_ivb.vertex[0].avertex.K2.real(), PT3_K2a_ivb.vertex[0].avertex.K2.imag(),
                                   PT3_K2a_t.vertex[0].avertex.K2.real(), PT3_K2a_t.vertex[0].avertex.K2.imag(),
                                   PT3_K2p.vertex[0].avertex.K2.real(), PT3_K2p.vertex[0].avertex.K2.imag(),
                                   PT3_K2t.vertex[0].avertex.K2.real(), PT3_K2t.vertex[0].avertex.K2.imag(),
                                   PT3_K2t_a.vertex[0].avertex.K2.real(), PT3_K2t_a.vertex[0].avertex.K2.imag(),
                                   PT3_K2t_a.vertex[0].pvertex.K2.real(), PT3_K2t_a.vertex[0].pvertex.K2.imag(),
                                   PT3_K2t_a.vertex[0].tvertex.K2.real(), PT3_K2t_a.vertex[0].tvertex.K2.imag(),
                                   PT3_SE.selfenergy.Sigma.real(), PT3_SE.selfenergy.Sigma.imag(),
                                   PT3_SE_a.selfenergy.Sigma.real(), PT3_SE_a.selfenergy.Sigma.imag(),
                                   PT3_SE_p.selfenergy.Sigma.real(), PT3_SE_p.selfenergy.Sigma.imag(),
                                   PT3_SE_t.selfenergy.Sigma.real(), PT3_SE_t.selfenergy.Sigma.imag(),
                                   PT3_SE_t_1.selfenergy.Sigma.real(), PT3_SE_t_1.selfenergy.Sigma.imag(),
                                   PT3_SE_t_4.selfenergy.Sigma.real(), PT3_SE_t_4.selfenergy.Sigma.imag(),
                                   PT3_SE_t_5.selfenergy.Sigma.real(), PT3_SE_t_5.selfenergy.Sigma.imag(),
                                   PT3_SE_t_a.selfenergy.Sigma.real(), PT3_SE_t_a.selfenergy.Sigma.imag(),
                                   PT3_SE_t_a_1.selfenergy.Sigma.real(), PT3_SE_t_a_1.selfenergy.Sigma.imag(),
                                   PT3_SE_t_a_4.selfenergy.Sigma.real(), PT3_SE_t_a_4.selfenergy.Sigma.imag(),
                                   PT3_SE_t_a_5.selfenergy.Sigma.real(), PT3_SE_t_a_5.selfenergy.Sigma.imag(),
                                   PT3_SE_t_p.selfenergy.Sigma.real(), PT3_SE_t_p.selfenergy.Sigma.imag(),
                                   PT3_SE_t_p_1.selfenergy.Sigma.real(), PT3_SE_t_p_1.selfenergy.Sigma.imag(),
                                   PT3_SE_t_p_4.selfenergy.Sigma.real(), PT3_SE_t_p_4.selfenergy.Sigma.imag(),
                                   PT3_SE_t_p_5.selfenergy.Sigma.real(), PT3_SE_t_p_5.selfenergy.Sigma.imag(),
                                   PT4_K1a22.vertex[0].avertex.K1.real(), PT4_K1a22.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_1.vertex[0].avertex.K1.real(), PT4_K1a13_1.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2.vertex[0].avertex.K1.real(), PT4_K1a13_2.vertex[0].avertex.K1.imag(),
                                   PT4_K1a31_2.vertex[0].avertex.K1.real(), PT4_K1a31_2.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_11e.vertex[0].avertex.K1.real(), PT4_K1a13_2_11e.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_21e.vertex[0].avertex.K1.real(), PT4_K1a13_2_21e.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_11o.vertex[0].avertex.K1.real(), PT4_K1a13_2_11o.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_21o.vertex[0].avertex.K1.real(), PT4_K1a13_2_21o.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_12o.vertex[0].avertex.K1.real(), PT4_K1a13_2_12o.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_22o.vertex[0].avertex.K1.real(), PT4_K1a13_2_22o.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_ia.vertex[0].avertex.K1.real(), PT4_K1a13_2_ia.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_ib.vertex[0].avertex.K1.real(), PT4_K1a13_2_ib.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_iia.vertex[0].avertex.K1.real(), PT4_K1a13_2_iia.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_iib.vertex[0].avertex.K1.real(), PT4_K1a13_2_iib.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_iva.vertex[0].avertex.K1.real(), PT4_K1a13_2_iva.vertex[0].avertex.K1.imag(),
                                   PT4_K1a13_2_ivb.vertex[0].avertex.K1.real(), PT4_K1a13_2_ivb.vertex[0].avertex.K1.imag()});

    //write_hdf("PT4_check_of_K2a_K2_switchedcc_t_update_symmrev_new11_SE_symm_full_adap_m3m9_gW10_501_101_nI1501_U1_state_PT3_K2a", 0, 1, PT3_K2a);
}

/**
 * Master function to test both consistency and correctness of K2-class
 * @param Lambda
 */
void test_K2(double Lambda, bool test_consistency){


    //First test consistency
    if(test_consistency) {
        bool K2a = test_K2_consistency(Lambda, 'a');    //Consistency of a-channel
        bool K2p = test_K2_consistency(Lambda, 'p');    //Consistency of p-channel
        bool K2t = test_K2_consistency(Lambda, 't');    //Consistency of t-channel

        if(K2a&&K2p&&K2t)
            test_K2_correctness(Lambda);
    }

    test_K2_correctness(Lambda);

}
#endif

#ifdef STATIC_FEEDBACK
/**
 * Compute the right hand side of the flow equations according to Severin Jakobs' channel decomposition with
 * approximated channel feedback and modified self-energy feedback (only static level shift to avoid
 * overbroadening of spectral features)
 * @param Psi    : state at which to compute right hand side
 * @param Lambda : Lambda at which to compute right hand side
 * @return       : dPsi (right hand side of flow equation)
 */
auto rhs_channel_decomposition(const State<comp>& Psi, const double Lambda) -> State<comp> {
    State<comp> dPsi; // result

    SelfEnergy<comp> selfEnergy;
    comp static_shift = real(Psi.selfenergy.valsmooth(0, glb_mu, 0));  // only use a static level shift as self-energy
    selfEnergy.initialize(static_shift, 0.);

    Propagator G(Lambda, selfEnergy, 'g');    //Initialization of Propagator objects
    Propagator S(Lambda, selfEnergy, 's');    //Initialization of Propagator objects

    // Self-energy flow
    loop(dPsi.selfenergy, Psi.vertex, S, true);  // self-energy loop

    // Vertex flow
    bubble_function(dPsi.vertex, Psi.vertex, Psi.vertex, G, S, 'a', true); // diff. bubble in the a-channel
    bubble_function(dPsi.vertex, Psi.vertex, Psi.vertex, G, S, 'p', true); // diff. bubble in the p-channel
    bubble_function(dPsi.vertex, Psi.vertex, Psi.vertex, G, S, 't', true); // diff. bubble in the t-channel

    return dPsi;
}

/**
 * FRG flow according to Severin Jakobs' channel decomposition with approximated channel feedback
 * and modified self-energy feedback (only static level shift to avoid overbroadening of spectral features).
 * Only correct if parameter STATIC_FEEDBACK is defined.
 * @param N_ODE : number of Runge-Kutta ODE iterations
 */
void test_channel_decomposition(int N_ODE) {
    State<comp> state_ini, state_fin;   // create initial and final state
    state_ini.initialize();             // initialize initial state

//    sopt_state(state_ini, Lambda_ini);  // set initial state to SOPT (necessary if Lambda_ini is too small)

    ODE_solver_RK4(state_fin, Lambda_fin, state_ini, Lambda_ini, rhs_channel_decomposition,
                    log_substitution, log_resubstitution, N_ODE); // compute flow

    string name = "test_channel_decomposition.h5";
    write_h5_rvecs(name, {"v", "Sigma_re", "Sigma_im", "Sigma_ini_re", "Sigma_ini_im"},
                         {ffreqs,
                          state_fin.selfenergy.Sigma.real(), state_fin.selfenergy.Sigma.imag(),
                          state_ini.selfenergy.Sigma.real(), state_ini.selfenergy.Sigma.imag()});

    write_hdf("channel_decomposition.h5", 0, 1, state_fin);
}
#endif

#endif //KELDYSH_MFRG_TESTFUNCTIONS_H
