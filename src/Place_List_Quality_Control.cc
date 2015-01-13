void Place_List::quality_control(char *directory) {
  int number_places = places.size();

  FRED_STATUS(0, "places quality control check for %d places\n", number_places);

  if (Global::Verbose>1) {
    if (Global::Enable_Large_Grid) {
      char filename [256];
      sprintf(filename, "%s/houses.dat", directory);
      FILE *fp = fopen(filename, "w");
      for (int p = 0; p < number_places; p++) {
        if (places[p]->get_type() == HOUSEHOLD) {
          fred::geo lat = places[p]->get_latitude();
          fred::geo lon = places[p]->get_longitude();
          double x, y;
          Global::Large_Cells->translate_to_cartesian(lat, lon, &x, &y);
          // get coordinates of large grid as alinged to global grid
          double min_x = Global::Large_Cells->get_min_x();
          double min_y = Global::Large_Cells->get_min_y();
          fprintf(fp, "%f %f\n", min_x+x, min_y+y);
        }
      }
      fclose(fp);
    }
  }

  if (Global::Verbose) {
    int count[20];
    int total = 0;
    // size distribution of households
    for (int c = 0; c < 15; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == HOUSEHOLD) {
        int n = places[p]->get_size();
        if (n < 15) { count[n]++; }
        else { count[14]++; }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold size distribution: %d households\n", total);
    for (int c = 0; c < 15; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[20];
    int total = 0;
    // adult distribution of households
    for (int c = 0; c < 15; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == HOUSEHOLD) {
        Household *h = (Household *) places[p];
        int n = h->get_adults();
        if (n < 15) { count[n]++; }
        else { count[14]++; }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold adult size distribution: %d households\n", total);
    for (int c = 0; c < 15; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[20];
    int total = 0;
    // children distribution of households
    for (int c = 0; c < 15; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == HOUSEHOLD) {
        Household *h = (Household *) places[p];
        int n = h->get_children();
        if (n < 15) { count[n]++; }
        else { count[14]++; }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold children size distribution: %d households\n", total);
    for (int c = 0; c < 15; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[20];
    int total = 0;
    // adult distribution of households with children
    for (int c = 0; c < 15; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == HOUSEHOLD) {
        Household *h = (Household *) places[p];
        if (h->get_children() == 0) continue;
        int n = h->get_adults();
        if (n < 15) { count[n]++; }
        else { count[14]++; }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold w/ children, adult size distribution: %d households\n", total);
    for (int c = 0; c < 15; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  // relationship between children and decision maker
  if (Global::Verbose > 1) {
    // find adult decision maker for each child
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == HOUSEHOLD) {
        Household *h = (Household *) places[p];
        if (h->get_children() == 0) continue;
        int size = h->get_size();
        for (int i = 0; i < size; i++) {
          Person * child = h->get_housemate(i);
          int ch_age = child->get_age();
          if (ch_age < 18) {
            int ch_rel = child->get_relationship();
            //int dm_age = child->get_adult_decision_maker()->get_age(); // TODO segfault on above line!
            int dm_rel = child->get_adult_decision_maker()->get_relationship();
            if (dm_rel != 1 || ch_rel != 3) {
              //printf("DECISION_MAKER: household %d %s  decision_maker: %d %d child: %d %d\n",
              //h->get_id(), h->get_label(), dm_age, dm_rel, ch_age, ch_rel);
            }
          }
        }
      }
    }
  }

  if (Global::Verbose) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for (int c = 0; c < 100; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      Person * per = NULL;
      if (places[p]->get_type() == HOUSEHOLD) {
        Household *h = (Household *) places[p];
        for (int i = 0; i < h->get_size(); i++) {
          if (h->get_housemate(i)->is_householder())
            per = h->get_housemate(i);
        }
        if (per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s\n",
              h->get_id(), h->get_label());
          count[0]++;
        }
        else {
          int a = per->get_age();
          if (a < 100) { count[a]++; }
          else { count[99]++; }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for (int c = 0; c < 100; c++) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[100];
    int total = 0;
    int children = 0;
    // age distribution of heads of households with children
    for (int c = 0; c < 100; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      Person * per = NULL;
      if (places[p]->get_type() == HOUSEHOLD) {
        Household *h = (Household *) places[p];
        if (h->get_children() == 0) continue;
        children += h->get_children();
        for (int i = 0; i < h->get_size(); i++) {
          if (h->get_housemate(i)->is_householder())
            per = h->get_housemate(i);
        }
        if (per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s\n",
              h->get_id(), h->get_label());
          count[0]++;
        }
        else {
          int a = per->get_age();
          if (a < 100) { count[a]++; }
          else { count[99]++; }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households with children: %d households\n", total);
    for (int c = 0; c < 100; c++) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "children = %d\n", children);
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[100];
    int total = 0;
    // size distribution of schools
    for (int c = 0; c < 20; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == SCHOOL) {
        int s = places[p]->get_size();
        int n = s / 50;
        if (n < 20) { count[n]++; }
        else { count[19]++; }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nSchool size distribution: %d schools\n", total);
    for (int c = 0; c < 20; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          (c+1)*50, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    // age distribution in schools
    fprintf(Global::Statusfp, "\nSchool age distribution:\n");
    int count[100];
    for (int c = 0; c < 100; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == SCHOOL) {
        // places[p]->print(0);
        for (int c = 0; c < 100; c++) {
          count[c] += ((School *) places[p])->children_in_grade(c);
        }
      }
    }
    for (int c = 0; c < 100; c++) {
      fprintf(Global::Statusfp, "age = %2d  students = %6d\n",
          c, count[c]);;
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[50];
    int total = 0;
    // size distribution of classrooms
    for (int c = 0; c < 50; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == CLASSROOM) {
        int s = places[p]->get_size();
        int n = s;
        if (n < 50) { count[n]++; }
        else { count[50-1]++; }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nClassroom size distribution: %d classrooms\n", total);
    for (int c = 0; c < 50; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if (Global::Verbose) {
    int count[20];
    int small_employees = 0;
    int med_employees = 0;
    int large_employees = 0;
    int xlarge_employees = 0;
    int total_employees = 0;
    int total = 0;
    // size distribution of workplaces
    for (int c = 0; c < 20; c++) { count[c] = 0; }
    for (int p = 0; p < number_places; p++) {
      if (places[p]->get_type() == WORKPLACE) {
        int s = places[p]->get_size();
        int n = s / 50;
        if (n < 20) { count[n]++; }
        else { count[19]++; }
        total++;
        if (s < 50) {
          small_employees += s;
        }
        else if (s < 100) {
          med_employees += s;
        }
        else if (s < 500) {
          large_employees += s;
        }
        else {
          xlarge_employees += s;
        }
        total_employees += s;
      }
    }
    fprintf(Global::Statusfp, "\nWorkplace size distribution: %d workplaces\n", total);
    for (int c = 0; c < 20; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          (c+1)*50, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n\n");

    fprintf(Global::Statusfp, "employees at small workplaces (1-49): ");
    fprintf(Global::Statusfp, "%d\n", small_employees);

    fprintf(Global::Statusfp, "employees at medium workplaces (50-99): ");
    fprintf(Global::Statusfp, "%d\n", med_employees);

    fprintf(Global::Statusfp, "employees at small workplaces (100-499): ");
    fprintf(Global::Statusfp, "%d\n", large_employees);

    fprintf(Global::Statusfp, "employees at small workplaces (500-up): ");
    fprintf(Global::Statusfp, "%d\n", xlarge_employees);

    fprintf(Global::Statusfp, "total employees: %d\n\n", total_employees);
  }

  /*
     if (Global::Verbose) {
     int covered[4];
     int all[4];
// distribution of sick leave in workplaces
for (int c = 0; c < 4; c++) { all[c] = covered[c] = 0; }
for (int p = 0; p < number_places; p++) {
if (places[p]->get_type() == WORKPLACE) {
Workplace * work = (Workplace *) places[p];
char s = work->get_size_code();
bool sl = work->is_sick_leave_available();
switch(s) {
case 'S':
all[0] += s;
if (sl) covered[0] += s;
break;
case 'M':
all[1] += s;
if (sl) covered[1] += s;
break;
case 'L':
all[2] += s;
if (sl) covered[2] += s;
break;
case 'X':
all[3] += s;
if (sl) covered[3] += s;
break;
}
}
}
fprintf(Global::Statusfp, "\nWorkplace sick leave coverage: ");
for (int c = 0; c < 4; c++) {
fprintf(Global::Statusfp, "%3d: %d/%d %5.2f | ", 
c, covered[c], all[c], (all[c]? (1.0*covered[c])/all[c] : 0));
}
fprintf(Global::Statusfp, "\n");
}
*/

if (Global::Verbose) {
  int count[60];
  int total = 0;
  // size distribution of offices
  for (int c = 0; c < 60; c++) { count[c] = 0; }
  for (int p = 0; p < number_places; p++) {
    if (places[p]->get_type() == OFFICE) {
      int s = places[p]->get_size();
      int n = s;
      if (n < 60) { count[n]++; }
      else { count[60-1]++; }
      total++;
    }
  }
  fprintf(Global::Statusfp, "\nOffice size distribution: %d offices\n", total);
  for (int c = 0; c < 60; c++) {
    fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
        c, count[c], (100.0*count[c])/total);
  }
  fprintf(Global::Statusfp, "\n");
}
if (Global::Verbose) {
  fprintf(Global::Statusfp, "places quality control finished\n");
  fflush(Global::Statusfp);
}
}
