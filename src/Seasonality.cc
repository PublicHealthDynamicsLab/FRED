#include "Seasonality.h"
#include "Geo_Utils.h"
#include "Random.h"
#include "Timestep_Map.h"
#include "Seasonality_Timestep_Map.h"
#include "Disease.h"
#include <vector>
#include <iterator>
#include <stdlib.h>

Seasonality::Seasonality(Abstract_Grid * abstract_grid) {
  grid = abstract_grid;
  string param_name_str("seasonality_timestep");
  seasonality_timestep_map = new Seasonality_Timestep_Map(param_name_str);
  seasonality_timestep_map->read_map();
  seasonality_timestep_map->print();
  seasonality_values = new double * [grid->get_rows()];
  for (int i = 0; i < grid->get_rows(); i++) {
    seasonality_values[i] = new double [grid->get_cols()];
  }
  for (int d = 0; d < Global::Diseases; d++) {
    seasonality_multiplier.push_back(new double * [grid->get_rows()]);
    for (int i = 0; i < grid->get_rows(); i++) {
      seasonality_multiplier.back()[i] = new double [grid->get_cols()];
    }
  }
}

void Seasonality::update(int day) {
  vector <point> points;
  points.clear();
  Seasonality_Timestep_Map::iterator it = seasonality_timestep_map->begin();
  while (it != seasonality_timestep_map->end()) {
    Seasonality_Timestep_Map::Seasonality_Timestep * cts = *it; 
    if (cts->is_applicable(day, Global::Epidemic_offset)) {
      double x,y;
      Geo_Utils::translate_to_cartesian(cts->get_lat(),cts->get_lon(),
          &x,&y,grid->get_min_lat(),grid->get_min_lon());
      int row = grid->get_rows() - 1 - (int) (y/grid->get_grid_cell_size());
      int col = (int) (x/grid->get_grid_cell_size());
      points.push_back(point(row,col,cts->get_seasonality_value()));
    }
    it++;
  }
  nearest_neighbor_interpolation(points, &seasonality_values);
  // store the climate modulated transmissibilities for all diseses
  // so that we don't have to re-calculate this for every place that we visit
  update_seasonality_multiplier();
}

void Seasonality::update_seasonality_multiplier() {
  for (int d = 0; d < Global::Diseases; d++) {
    Disease * disease = Global::Pop.get_disease(d);
    if (Global::Enable_Climate) { // should seasonality values be interpreted by Disease as specific humidity?
      for (int r = 0; r < grid->get_rows(); r++) {
        for (int c = 0; c < grid->get_cols(); c++) {
          seasonality_multiplier[d][r][c] = disease->calculate_climate_multiplier(seasonality_values[r][c]);
        }
      }
    } else { // just use seasonality values as they are
      seasonality_multiplier[d] = seasonality_values;
    }
  }
}

double Seasonality::get_seasonality_multiplier_by_lat_lon(fred::geo lat, fred::geo lon, int disease_id) {
  double x, y;
  Geo_Utils::translate_to_cartesian(lat,lon,&x,&y,grid->get_min_lat(),grid->get_min_lon());
  return get_seasonality_multiplier_by_cartesian(x,y,disease_id);
}

double Seasonality::get_seasonality_multiplier_by_cartesian(double x, double y, int disease_id) {
  int row, col;
  row = grid->get_rows()-1 - (int) (y/grid->get_grid_cell_size());
  col = (int) (x/grid->get_grid_cell_size());
  return get_seasonality_multiplier(row, col, disease_id);
}

double Seasonality::get_seasonality_multiplier(int row, int col, int disease_id) {
  if ( row >= 0 && col >= 0 && row < grid->get_rows() && col < grid->get_cols() )
    return seasonality_multiplier[disease_id][row][col];
  else
    return 0;
}

void Seasonality::nearest_neighbor_interpolation(vector <point> points, double *** field) {
  int d1, d2, ties;
  for (int r=0; r < grid->get_rows(); r++) {
    for (int c=0; c < grid->get_cols(); c++) {
      d1 = grid->get_rows() + grid->get_cols() + 1;
      ties = 0;
      for (vector <point>::iterator pit = points.begin(); pit != points.end(); pit++) {
        d2 = abs( pit->x - r) + abs( pit->y - c );
        if (d1 < d2) {
          continue;
        }
        if (d1 == d2) {
          if ((RANDOM()*((double)(ties+1)))>ties) {
            (*field)[r][c] = pit->value;
          }
          ties++;
        }
        else {
          (*field)[r][c] = pit->value; d1 = d2;
        }
      }
    }
  }
}

void Seasonality::print() {
  return;
  cout << "Seasonality Values" << endl;
  print_field(&seasonality_values);
  cout << endl;
  for (int d = 0; d < Global::Diseases; d++) {
    printf("Seasonality Modululated Transmissibility for Disease[%d]\n",d);
    print_field(&(seasonality_multiplier[d]));
    cout << endl;
  }
}

void Seasonality::print_field(double *** field) {
  return;
  for (int r=0; r < grid->get_rows(); r++) {
    for (int c=0; c < grid->get_cols(); c++) {
      printf(" %4.4f", (*field)[r][c]);
    }
    cout << endl;
  }
}

double Seasonality::get_average_seasonality_multiplier(int disease_id) {
  double total = 0;
  for (int row = 0; row < grid->get_rows(); row++) {
    for (int col = 0; col < grid->get_cols(); col++) {
      total += get_seasonality_multiplier(row, col, disease_id);
    }
  }
  return total / (grid->get_rows() * grid->get_cols());
}

void Seasonality::print_summary() {
  return;
  for (int disease_id = 0; disease_id < Global::Diseases; disease_id++) { 
    double min = 9999999;
    double max = 0;
    double total = 0;
    double daily_avg = 0;
    printf("\nSeasonality Summary for disease %d:\n\n",disease_id);
    for (int day = 0; day < Global::Days; day++) {
      update(day);
      daily_avg = get_average_seasonality_multiplier(disease_id);
      if (min > daily_avg) { min = daily_avg; }
      if (max < daily_avg) { max = daily_avg; }
      total += daily_avg;
      printf(" %4.4f", daily_avg);
    }
    cout << endl;
    printf(" minimum: %4.4f\n",min);
    printf(" maximum: %4.4f\n",max);
    printf(" average: %4.4f\n\n",total/Global::Days);
  }
  update(0);
}








