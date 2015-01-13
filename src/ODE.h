
// Libraries:

#include <map>
//#include <utility>
#include <string>
//#include <vector>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "cvode/cvode.h"             // prototypes for CVODE fcts and consts
#include "nvector/nvector_serial.h"  // serial N_Vector types, fcts., and macros
#include "sundials/sundials_direct.h"
using namespace std;

#define T0    RCONST(0.0)       // initial time      
//#define SUNDIALS_DOUBLE_PRECISION

// V, Viral load per epithelial cell
// H, Proportion of healthy cells
// I, Proportion of infected cells
// M, Activated antigen presenting cells per homeostatic level
// F, Interferon per homeostatic level of macrophages
// R, Proportion of resistant cells
// E, Effector cells per homeostatic level
// P, Plasma cells per homeostatic level
// A, Antibodies per homeostatic level
// S, Antigenic distance (???)

class ODE {

  public:

    ODE(int numStrains);
    ~ODE();

    void setup();
    N_Vector get_initial_conditions();
    void set_parameter_defaults();
    void reset();
    void clear();
    void print_solutions();

    void set_num_equations(int neq) {
      num_equations = neq;
      }
    void set_num_strains(int ns) {
      num_strains = ns;
      }
    void set_duration(int dur) {
      duration = dur;
      }
    void set_absolute_tolerance(double abstol);
    void set_relative_tolerance(double reltol) {
      relative_tolerance = reltol;
      }
    void set_max_steps(double s) {
      max_steps = s;
      }
    void set_time_scale(int t) {
      time_scale = t;
      }
    int get_time_scale () {
      return time_scale;
      }

    //Getters/setters for constants
    void set_g_v   (double val_g_v, int strain )  {
      g_v[strain]   = val_g_v  ;
      }
    void set_g_vh  (double val_g_vh, int strain)  {
      g_vh[strain]  = val_g_vh ;
      }
    void set_g_hv  (double val_g_hv, int strain)  {
      g_hv[strain]  = val_g_hv ;
      }
    void set_a_v   (double val_a_v )  {
      a_v   = val_a_v  ;
      }
    void set_a_i   (double val_a_i )  {
      a_i   = val_a_i  ;
      }
    double get_g_v   (int strain) {
      return g_v[strain]   ;
      }
    double get_g_vh  (int strain) {
      return g_vh[strain]  ;
      }
    double get_g_hv  (int strain) {
      return g_hv[strain]  ;
      }
    double get_a_v   () {
      return a_v   ;
      }
    double get_a_i   () {
      return a_i   ;
      }

    //Getters/setters for initial values
    void set_V (double val_V, int s) {
      initial_values_[V + s * offset] = val_V;
      }
    void set_I (double val_I, int s) {
      initial_values_[I + s * offset] = val_I;
      }
    void set_P (double val_P, int s) {
      initial_values_[P + s * offset] = val_P;
      }
    void set_A (double val_A, int s) {
      initial_values_[A + s * offset] = val_A;
      }
    void set_S (double val_S, int s) {
      initial_values_[S + s * offset] = val_S;
      }

    void set_H (double val_H) {
      initial_values_[H] = val_H;
      }
    void set_M (double val_M) {
      initial_values_[M] = val_M;
      }
    void set_F (double val_F) {
      initial_values_[F] = val_F;
      }
    void set_R (double val_R) {
      initial_values_[R] = val_R;
      }
    void set_E (double val_E) {
      initial_values_[E] = val_E;
      }

    double get_V (int s) {
      return initial_values_[V + s * offset];
      }
    double get_I (int s) {
      return initial_values_[I + s * offset];
      }
    double get_P (int s) {
      return initial_values_[P + s * offset];
      }
    double get_A (int s) {
      return initial_values_[A + s * offset];
      }
    double get_S (int s) {
      return initial_values_[S + s * offset];
      }

    double get_H () {
      return initial_values_[H];
      }
    double get_M () {
      return initial_values_[M];
      }
    double get_F () {
      return initial_values_[F];
      }
    double get_R () {
      return initial_values_[R];
      }
    double get_E () {
      return initial_values_[E];
      }


    int get_duration() {
      return duration;
      }
    int get_num_equations() {
      return num_equations;
      }
    int get_num_strains() {
      return num_strains;
      }

    N_Vector get_result (double time_point);
    double get_datum(double time, int equation_number);
    double get_datum_avg(double start, double end, int steps, int equation_number);

    double * get_time_point_data(double time);
    double * get_time_point_data_avg (double start, double end, int steps);
    double * get_equation_data(int equation_number) {
      return get_equation_data(equation_number, duration);
      }
    double * get_equation_data(int equation_number, int days);

    double * get_viral_titer_data(int s)    {
      return get_equation_data(V + offset * s);
      }
    double * get_infected_cells_data(int s) {
      return get_equation_data(I + offset * s);
      }
    double * get_interferon_data()     {
      return get_equation_data(F);
      }

    /*
      double * get_dead_cells_data();

      double get_viral_titer_datum(double time)   { return get_datum(time, V); }
      double get_dead_cells_datum (double time)   { return 1 - get_datum(time,H) - get_datum(time,I) - get_datum(time,R); }

      double get_viral_titer_avg(double start, double end, int steps)   { return get_datum_avg(start, end, steps, V); }
      double get_dead_cells_avg(double start, double end, int steps);
    */

  private:
    std::map<double, N_Vector> results;
    static void *cvode_mem_;
    double *initial_values_;
    static bool CVode_created_;
    // Problem Constants...
    int num_equations;
    int num_strains;
    int duration;
    int time_scale;

    N_Vector initial_conditions;
    N_Vector absolute_tolerance;
    double relative_tolerance;
    double max_steps;

    // Equation Numbers...
    static const int H = 0, M = 1, F = 2, R = 3, E = 4;
    static const int V = 5, I = 6, P = 7, A = 8, S = 9;
    static const int offset = 5;

    // Equation Parameters:
    // strain/host specific parameters:
    double *g_v, *g_vh, *g_hv, *b_mv, *b_pm, *g_av;

    // parameters common to all strains/hosts:
    double g_va, a_v , a_v1, a_v2 ;
    double b_hd , a_r , b_hf ;
    double b_ie , a_i  , b_md, a_m ;
    double b_f  , c_f  , b_fh , a_f ;
    double b_em , b_ei , a_e ;
    double a_p ;
    double b_a  , a_a  , r_s ;

    double ** nvector_to_array (N_Vector vector);

    // Functions (with adapters) used by solver:
    // Right-Hand Side Function:
    static int right_hand_side_ADPT (realtype t, N_Vector y, N_Vector ydot, void *user_data);
    virtual int right_hand_side (realtype t, N_Vector y, N_Vector ydot);

    // Jacobian:
    //  static int jacobian_ADPT (int N, realtype t, N_Vector y, N_Vector fy, DlsMat J, void *user_data, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);
    //  virtual int jacobian (int N, realtype t, N_Vector y, N_Vector fy, DlsMat J, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);

    // Checkpoint:
    int check_flag (void *flagvalue, char *funcname, int opt);
  };

