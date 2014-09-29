/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// FILE: ODE.cc
// re-implementation of re-implemenation of midas_ebm_0.cpp
//
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "ODE.h"
#include <stdio.h>
#include <assert.h>
#include "cvode/cvode.h"
#include "cvode/cvode_dense.h"
#include "cvode/cvode_band.h"
#include "nvector/nvector_serial.h"  // serial N_Vector types, fcts., and macros
#include "sundials/sundials_direct.h"
// #include <functional>

//  ...Constructors and Destructors...

void *ODE::cvode_mem_;
bool ODE::CVode_created_;


ODE::ODE(int numStrains) {
  set_num_strains(numStrains);
  set_num_equations(numStrains * 5 + 5);
  set_duration(8);
  set_absolute_tolerance(1.0e-09);
  set_relative_tolerance(1.0e-09);
  set_max_steps(5e6);

  initial_values_ = new realtype[get_num_equations()];

  for(int s=0; s < numStrains; s++) {
    set_V(0.5, s);
    set_I(0, s);
    set_P(1, s);
    set_A(1, s);
    set_S(0.1, s);
    }

  set_H(1);
  set_M(0);
  set_F(0);
  set_R(0);
  set_E(1);
  time_scale = 1;

  set_parameter_defaults();
  }

void ODE::setup() {
  initial_conditions = get_initial_conditions();

  cvode_mem_ = CVodeCreate(CV_BDF, CV_NEWTON);
  CVodeSetUserData(cvode_mem_, (ODE *) this);

  CVodeInit(cvode_mem_, right_hand_side_ADPT, T0, initial_conditions);
  CVodeSVtolerances(cvode_mem_, relative_tolerance, absolute_tolerance);
  CVDense(cvode_mem_, num_equations);
  //VDlsSetDenseJacFn(cvode_mem_, jacobian_ADPT);
  CVodeSetMaxNumSteps(cvode_mem_, max_steps);
  }


ODE::~ODE() {
  N_VDestroy_Serial(initial_conditions);
  N_VDestroy_Serial(absolute_tolerance);
  CVodeFree(&cvode_mem_);
  clear();
  }

//  ...Initialization Stuff...

void ODE::reset() {
  clear();
  N_VDestroy_Serial(absolute_tolerance);
  set_absolute_tolerance(1e-09);
  N_VDestroy_Serial(initial_conditions);
  initial_conditions = get_initial_conditions();
  CVodeReInit(cvode_mem_, T0, initial_conditions);
  }

void ODE::clear() {
  for (map<double, N_Vector>::iterator it = results.begin(); it != results.end(); ++it) {
    N_VDestroy_Serial(it->second);
    }

  results.clear();
  }

void ODE::set_absolute_tolerance(double abstol) {
  // Create serial vector of length num_equations for I.C. and abstol
  absolute_tolerance = N_VNew_Serial(num_equations);

  if (check_flag((void *) absolute_tolerance, "N_VNew_Serial", 0))
    cout << "disaster strikes!" << endl;

  // Set the vector absolute tolerance:
  double * tolerance_values = NV_DATA_S(absolute_tolerance);

  for (int eqn = 0; eqn < num_equations; ++eqn) {
    tolerance_values[eqn] = abstol;
    }
  }

//  ...Integrator...
N_Vector ODE::get_result(double time_point) {
  if (time_point == 0) {
    return get_initial_conditions();
    }

  if (results.count(time_point)) {
    return results[time_point];
    }

  double t;
  N_Vector solutions = N_VNew_Serial(num_equations);

  if (CVode(cvode_mem_, time_point, solutions, &t, CV_NORMAL) != CV_SUCCESS) {
    return NULL;
    }

  results[time_point] = solutions;
  return solutions;
  }

double ODE::get_datum (double time, int equation_number) {
  return NV_Ith_S(get_result(time), equation_number);
  }

double ODE::get_datum_avg(double start, double end, int steps, int equation_number) {
  double step_size = (end - start)/steps;
  double current = start;
  N_Vector result;
  double counter = 0;

  for (int i = 0; i < steps; i++) {
    result = get_result(current);
    counter += NV_Ith_S(result, equation_number);
    current += step_size;
    }

  return counter / steps;
  }

double * ODE::get_time_point_data (double time) {
  N_Vector result = get_result(time);
  return NV_DATA_S(result);
  }

double * ODE::get_time_point_data_avg (double start, double end, int steps) {
  double step_size = (end - start)/steps;
  double current = start;
  double * return_array = new double[num_equations];
  N_Vector result;

  for (int i = 0; i < steps; i++) {
    result = get_result(current);
    double * result_data = NV_DATA_S(result);

    for (int j = 0; j < num_equations; j++)  {
      return_array[j] += result_data[j];
      }

    current += step_size;
    }

  for (int i = 0; i < num_equations; i++) {
    return_array[i] /= steps;
    }

  return return_array;
  }

double * ODE::get_equation_data (int equation_number, int days) {
  double * equation_data = new double[days];

  for (int time = 0; time < days; time++) {
    equation_data[time] = NV_Ith_S(get_result(time), equation_number);
    }

  return equation_data;
  }

//  ...Set Parameters and Initial Conditions...
//    ...Initial Conditions...

N_Vector ODE::get_initial_conditions() {
  N_Vector snapshot = N_VNew_Serial(num_equations);
  double * snapshot_data = NV_DATA_S(snapshot);

  for (int eqn = 0; eqn < num_equations; ++eqn) {
    snapshot_data[eqn] = initial_values_[eqn];
    }

  return snapshot;
  }

//    ...All Default Parameters...

void ODE::set_parameter_defaults() {
  int ns = get_num_strains();
  g_v = new double[ns];
  g_vh = new double[ns];
  g_hv = new double[ns];
  b_pm = new double[ns];
  b_mv = new double[ns];
  g_av = new double[ns];

  for(int s=0; s < get_num_strains(); s++) {
    //////////////////////////////////////////////////////////////////// Variable Parameters:
    // g_v: (proxy for) "transmissibility"; how many free virions does an infected cell population
    //  produce per number of infected cells per unit time; Rate constant of influenza A virus
    //  particles secretion per infected epithelial cells
    g_v[s] = 582.0;

    //////////////////////////////////////////////////////////////////// Invariant Parameters:
    // g_vh: Given the interaction of virions and a target cell, how many
    //  virions are lost per number target cells per unit time; Rate constant of adsorption of
    //  IAV by infected epithelial cells
    g_vh[s] = 1.23;

    // g_hv: Given the interaction of virions and a target cell, how many target cells become
    //  infected cells per number of virions per unit time; Rate constant of epithelial cells
    //  infected by IAV
    g_hv[s] = 0.688;

    // remainder of parameters are expected to be the same for all influenza virus strains;
    // descriptions can be found in table 2 from Hancioglu et al., J. Theo. Biol. 246 (2007) 70-86
    b_pm[s] = 2, b_mv[s] = 0.0867, g_av[s] = 277;
    }

  // a_i: The half life of an infected cell; how long on average from the time of infection
  // until the cell bursts
  a_i = 1.63;

  // a_v: What is the effective half-life of  a free virion that does not attach
  //  to a target cell
  a_v = 3.9;

  // remainder of parameters are expected to be the same for all influenza virus strains;
  // descriptions can be found in table 2 from Hancioglu et al., J. Theo. Biol. 246 (2007) 70-86
  g_va = 343, a_v1 = 78.9, a_v2 = 17764;
  b_hd = 4.75, a_r = 2, b_hf = 0.019;
  b_ie = 0.0088, b_md = 4.18, a_m = 0.658;
  b_f = 85203, c_f = 1024, b_fh = 19.8, a_f = 4.41;
  b_em = 2.65, b_ei = 1.16, a_e = 0.75;
  a_p = 0.0736;
  b_a = 0.00484, a_a = 0.00484, r_s = 1.1e-5;

  // note that the "burst size" is defined as g_v divided by a_i
  }

//  ...Functions (with Adapters) Used by the Integrator...
//    ...sode: Right-Hand Side Function...
int ODE::right_hand_side_ADPT(realtype t, N_Vector y, N_Vector ydot, void *user_data) {
  // This is the static adapter function for the right-hand side (type CVRhsFn)
  // function.  Calls member-fucntion using pointer to the ODE class
  ODE * ebm = (ODE *) user_data;
  return (ebm->right_hand_side(t, y, ydot));
  }
/*
int ODE::right_hand_side(realtype t, N_Vector y, N_Vector ydot) {
    // Called by *_ADPT adapter function

    double * eq = NV_DATA_S( y );
    double * ydotvalues = NV_DATA_S(ydot);
    // define equations;

    ydotvalues[V + offsetset] = g_v[0] * eq[I + offset] - g_va * eq[S + offset] * eq[A + offset] * eq[V + offset] - g_vh[0]
        * eq[H] * eq[V + offset] - a_v * eq[V + offset] - a_v1 * eq[V + offset] / (1 + a_v2 * eq[V + offset]);

    ydotvalues[H] = b_hd * (1 - eq[H] - eq[R] - eq[I]) * (eq[H] + eq[R])
        + a_r * eq[R] - g_hv[0] * eq[V] * eq[H] - b_hf * eq[F] * eq[H];
    ydotvalues[I] = g_hv[0] * eq[V] * eq[H] - b_ie * eq[E] * eq[I] - a_i
        * eq[I];
    ydotvalues[M] = (1 - eq[M]) * (b_md * (1 - eq[H] - eq[R] - eq[I])
            + b_mv[0] * eq[V]) - a_m * eq[M];
    ydotvalues[F] = b_f * eq[M] + c_f * eq[I] - b_fh * eq[H] * eq[F] - a_f
        * eq[F];
    ydotvalues[R] = b_hf * eq[F] * eq[H] - a_r * eq[R];
    ydotvalues[E] = b_em * eq[M] * eq[E] - b_ei * eq[I] * eq[E] + a_e * (1
            - eq[E]);
    ydotvalues[P] = b_pm[0] * eq[M] * eq[P] + a_p * (1 - eq[P]);
    ydotvalues[A] = b_a * eq[P] - g_av[0] * eq[S] * eq[A] * eq[V] - a_a
        * eq[A];
    ydotvalues[S] = r_s * eq[P] * (1 - eq[S]);

    return (0);
}
*/

int ODE::right_hand_side(realtype t, N_Vector y, N_Vector ydot) {
  // Called by *_ADPT adapter function

  realtype *eq = ( (N_VectorContent_Serial) (y->content) ) -> data;
  realtype *ydotvalues = ( (N_VectorContent_Serial) (ydot->content) ) -> data;

  // Common Eqns
  ydotvalues[F] = b_f * eq[M] - b_fh * eq[H] * eq[F] - a_f * eq[F];

  ydotvalues[R] = b_hf * eq[F] * eq[H] - a_r * eq[R];

  ydotvalues[E] = b_em * eq[M] * eq[E] - b_ei * eq[I] * eq[E] + a_e * (1 - eq[E]);

  ydotvalues[H] = b_hd * (1 - eq[H] - eq[R]) * (eq[H] + eq[R])
                  + a_r * eq[R] - b_hf * eq[F] * eq[H];

  ydotvalues[M] = - a_m * eq[M];

  // Strain Specific  Eqns

  double term = 0;

  for(int s=0; s < get_num_strains(); s++) {
    int offset = s*5;

    ydotvalues[V + offset] = g_v[s] * eq[I + offset] - g_va * eq[S + offset] * eq[A + offset] * eq[V + offset]
                             - g_vh[s] * eq[H] * eq[V + offset] - a_v * eq[V + offset]
                             - a_v1 * eq[V + offset] / (1 + a_v2 * eq[V + offset]);

    ydotvalues[P + offset] = b_pm[s] * eq[M] * eq[P + offset] + a_p * (1 - eq[P + offset]);

    ydotvalues[A + offset] = a_a * ( eq[P + offset] - eq[A + offset]) - g_av[s]
                             * eq[S + offset] * eq[A + offset] * eq[V + offset];

    ydotvalues[S + offset] = r_s * eq[P + offset] * (1 - eq[S + offset]);

    ydotvalues[H] -= b_hd * eq[I + offset] * (eq[H] + eq[R]);

    ydotvalues[F] += c_f * eq[I + offset];

    ydotvalues[H] -= g_hv[s] * eq[V + offset] * eq[H];

    ydotvalues[M] += (1 - eq[M]) * (b_md * (1 - eq[H] - eq[R] - eq[I]) + b_mv[s] * eq[V + offset]);

    ydotvalues[I + offset] = - b_ie * eq[E] * eq[I + offset] - a_i * eq[I + offset];

    term += g_hv[0] * eq[V + offset] * eq[H];
    }

  for(int s=0; s < get_num_strains(); s++) {
    ydotvalues[I + s*5] += term;
    }

  return 0;
  }

/*
//    ...Jacobian for Single-Strain ODE...

int ODE::jacobian_ADPT(int N, realtype t, N_Vector y, N_Vector fy, DlsMat J,
        void *user_data, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3) {

    // This is the static adapter function for the jacobian
    ODE * ebm = (ODE *) user_data;
    return (ebm->jacobian(N, t, y, fy, J, tmp1, tmp2, tmp3));
}

int ODE::jacobian(int N, realtype t, N_Vector y, N_Vector fy, DlsMat J,
        N_Vector tmp1, N_Vector tmp2, N_Vector tmp3) {

    double * eq = NV_DATA_S( y );

    // define jacobian matrix

    double* col0 = DENSE_COL(J, 0);
    double* col1 = DENSE_COL(J, 1);
    double* col2 = DENSE_COL(J, 0);
    double* col3 = DENSE_COL(J, 0);
    double* col4 = DENSE_COL(J, 0);
    double* col5 = DENSE_COL(J, 0);
    double* col6 = DENSE_COL(J, 0);
    double* col7 = DENSE_COL(J, 0);
    double* col8 = DENSE_COL(J, 0);
    double* col9 = DENSE_COL(J, 0);

    col0[0] = -g_va * eq[S] * eq[A] - g_vh * eq[H] - a_v - a_v1 / pow((1 + a_v2 * eq[V]), 2);
    col0[1] = -g_hv * eq[H];
    col0[2] = g_hv * eq[H];
    col0[3] = b_mv * (1 - eq[M]);
    col0[8] = -g_av * eq[S] * eq[A];

    col1[0] = -g_vh * eq[V];
    col1[1] = b_hd * (1 - 2 * eq[H] - 2 * eq[R] - eq[I]) - g_hv * eq[V] - b_hf * eq[F];
    col1[2] = g_hv * eq[V];
    col1[3] = -b_md * (1 - eq[M]);
    col1[4] = -b_fh * eq[F];
    col1[5] = b_hf * eq[F];

    col2[0] = g_v;
    col2[1] = -b_hd * (eq[H] + eq[R]);
    col2[2] = -b_ie * eq[E] - a_i;
    col2[3] = -b_md * (1 - eq[M]);
    col2[4] = c_f;
    col2[6] = -b_ei * eq[E];

    col3[3] = -a_m - (b_mv * eq[V] + b_md * (1 - eq[H] - eq[R] - eq[I]));
    col3[4] = b_f;
    col3[6] = b_em * eq[E];
    col3[7] = b_pm * eq[P];

    col4[1] = -b_hf * eq[H];
    col4[4] = -a_f - b_fh * eq[H];
    col4[5] = b_hf * eq[H];

    col5[1] = b_hd * (1 - 2 * eq[H] - 2 * eq[R] - eq[I]);
    col5[3] = -b_md * (1 - eq[M]);
    col5[5] = -a_r;

    col6[2] = -b_ie * eq[I];
    col6[6] = b_em * eq[M] - b_ei * eq[I] - a_e;

    col7[7] = b_pm * eq[P] - a_p;
    col7[8] = b_a;
    col7[9] = r_s * (1 - eq[S]);

    col8[0] = -g_va * eq[S] * eq[V];
    col8[8] = -a_a - g_av * eq[S] * eq[V];

    col9[0] = -g_va * eq[A] * eq[V];
    col9[8] = -g_av * eq[A] * eq[V];
    col9[9] = -r_s * eq[P];
    return (0);
}
*/
//  ...Utility Functions...

int ODE::check_flag(void *flagvalue, char *funcname, int opt) {
  int *errflag;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if (opt == 0 && flagvalue == NULL) {
    fprintf(stderr,
            "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
            funcname);
    return (1);
    }

  /* Check if flag < 0 */
  else if (opt == 1) {
    errflag = (int *) flagvalue;

    if (*errflag < 0) {
      fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
              funcname, *errflag);
      return (1);
      }
    }

  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && flagvalue == NULL) {
    fprintf(stderr,
            "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
            funcname);
    return (1);
    }

  return (0);
  }

void ODE::print_solutions() {
  for (int a = 0; a < duration; a++) {
    //N_VPrint_Serial(solutions[a]);
    }
  }
