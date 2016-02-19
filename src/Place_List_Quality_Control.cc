/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

void Place_List::report_school_distributions(int day) {
  int number_places = this->places.size();
  // original size distribution
  int count[21];
  int osize[21];
  int nsize[21];
  // size distribution of schools
  for(int c = 0; c <= 20; ++c) {
    count[c] = 0;
    osize[c] = 0;
    nsize[c] = 0;
  }
  for(int p = 0; p < number_places; ++p) {
    if(this->places[p]->get_type() == Place::TYPE_SCHOOL) {
      int os = this->places[p]->get_orig_size();
      int ns = this->places[p]->get_size();
      int n = os / 50;
      if(n > 20) {
        n = 20;
      }
      count[n]++;
      osize[n] += os;
      nsize[n] += ns;
    }
  }
  fprintf(Global::Statusfp, "SCHOOL SIZE distribution: ");
  for(int c = 0; c <= 20; ++c) {
    fprintf(Global::Statusfp, "%d %d %0.2f %0.2f | ",
	    c, count[c], count[c] ? (1.0 * osize[c]) / (1.0 * count[c]) : 0, count[c] ? (1.0 * nsize[c]) / (1.0 * count[c]) : 0);
  }
  fprintf(Global::Statusfp, "\n");

  return;

  int year = day / 365;
  char filename[FRED_STRING_SIZE];
  FILE* fp = NULL;

  sprintf(filename, "%s/school-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");
  for(int p = 0; p < number_places; ++p) {
    if(this->places[p]->get_type() == Place::TYPE_SCHOOL) {
      Place* h = this->places[p];
      fprintf(fp, "%s orig_size %d current_size %d\n", h->get_label(), h->get_orig_size()-h->get_staff_size(), h->get_size()-h->get_staff_size());
      /*
	School * s = static_cast<School*>(h);
	for(int a = 1; a <=20; ++a) {
	if(s->get_orig_students_in_grade(a) > 0) {
	fprintf(fp, "SCHOOL %s age %d orig %d current %d\n",
	s->get_label(), a, s->get_orig_students_in_grade(a), s->get_students_in_grade(a));
	}
	}
	fprintf(fp,"\n");
      */
    }
  }
  fclose(fp);

  sprintf(filename, "%s/work-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");
  for(int p = 0; p < number_places; ++p) {
    if(this->places[p]->get_type() == Place::TYPE_WORKPLACE) {
      Place* h = this->places[p];
      fprintf(fp, "%s orig_size %d current_size %d\n", h->get_label(), h->get_orig_size(), h->get_size());
    }
  }
  fclose(fp);

  sprintf(filename, "%s/house-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");
  for(int p = 0; p < number_places; ++p) {
    if(this->places[p]->is_household()) {
      Place* h = this->places[p];
      fprintf(fp, "%s orig_size %d current_size %d\n", h->get_label(), h->get_orig_size(), h->get_size());
    }
  }
  fclose(fp);
}

void Place_List::report_household_distributions() {
  int number_places = this->places.size();

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        int n = this->places[p]->get_size();
        if(n <= 10) {
          count[n]++;
        } else {
          count[10]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "Household size distribution: N = %d ", total);
    for(int c = 0; c <= 10; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) ",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");

    // original size distribution
    int hsize[20];
    total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
      hsize[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        int n = this->places[p]->get_orig_size();
	int hs = this->places[p]->get_size();
        if(n <= 10) {
          count[n]++;
          hsize[n] += hs;
        } else {
          count[10]++;
          hsize[10] += hs;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "Household orig distribution: N = %d ", total);
    for(int c = 0; c <= 10; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) %0.2f ",
	      c, count[c], (100.0 * count[c]) / total, count[c] ? ((double)hsize[c] / (double)count[c]) : 0.0);
    }
    fprintf(Global::Statusfp, "\n");
  }

  return;

  if(Global::Verbose) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        for(int i = 0; i < h->get_size(); ++i) {
          if(h->get_enrollee(i)->is_householder()) {
            per = h->get_enrollee(i);
          }
        }
        if(per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
		       h->get_id(), h->get_label(), h->is_group_quarters()?1:0);
          count[0]++;
        } else {
          int a = per->get_age();
          if(a < 100) {
            count[a]++;
          } else {
            count[99]++;
          }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // adult distribution of households
    for (int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(places[p]);
        int n = h->get_adults();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // children distribution of households
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        int n = h->get_children();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold children size distribution: %d households\n", total);
    for (int c = 0; c < 15; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // adult distribution of households with children
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        int n = h->get_adults();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold w/ children, adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  // relationship between children and decision maker
  if(Global::Verbose > 1 && Global::Enable_Behaviors) {
    // find adult decision maker for each child
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        int size = h->get_size();
        for(int i = 0; i < size; ++i) {
          Person* child = h->get_enrollee(i);
          int ch_age = child->get_age();
          if (ch_age < 18) {
            int ch_rel = child->get_relationship();
	    Person* dm = child->get_health_decision_maker();
	    if(dm == NULL) {
	      printf("DECISION_MAKER: household %d %s  child: %d %d is making own health decisions\n",
		     h->get_id(), h->get_label(), ch_age, ch_rel);
	    } else {
	      int dm_age = dm->get_age();
	      int dm_rel = dm->get_relationship();
	      if(dm_rel != 1 || ch_rel != 3) {
		printf("DECISION_MAKER: household %d %s  decision_maker: %d %d child: %d %d\n",
		       h->get_id(), h->get_label(), dm_age, dm_rel, ch_age, ch_rel);
	      }
	    }
	  }
        }
      }
    }
  }

  if(Global::Verbose) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
	Person* per = NULL;
        for(int i = 0; i < h->get_size(); ++i) {
          if(h->get_enrollee(i)->is_householder()) {
            per = h->get_enrollee(i);
          }
        }
        if(per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
		       h->get_id(), h->get_label(), h->is_group_quarters() ? 1 : 0);
          count[0]++;
        } else {
          int a = per->get_age();
          if(a < 100) {
            count[a]++;
          } else {
            count[99]++;
          }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[100];
    int total = 0;
    int children = 0;
    // age distribution of heads of households with children
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0){
          continue;
        }
        children += h->get_children();
        for(int i = 0; i < h->get_size(); ++i) {
          if(h->get_enrollee(i)->is_householder()) {
            per = h->get_enrollee(i);
          }
        }
        if(per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
		       h->get_id(), h->get_label(), h->is_group_quarters()?1:0);
          count[0]++;
        } else {
          int a = per->get_age();
          if(a < 100) {
            count[a]++;
          } else {
            count[99]++;
          }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households with children: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "children = %d\n", children);
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count_has_school_age = 0;
    int count_has_school_age_and_unemployed_adult = 0;
    int total_hh = 0;

    //Households with school-age children and at least one unemployed adult
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
        total_hh++;
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        if(h->has_school_aged_child()) {
          count_has_school_age++;
        }
        if(h->has_school_aged_child_and_unemployed_adult()) {
          count_has_school_age_and_unemployed_adult++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nHouseholds with school-aged children and at least one unemployed adult\n");
    fprintf(Global::Statusfp, "Total Households: %d\n", total_hh);
    fprintf(Global::Statusfp, "Total Households with school-age children: %d\n", count_has_school_age);
    fprintf(Global::Statusfp, "Total Households with school-age children and at least one unemployed adult: %d\n", count_has_school_age_and_unemployed_adult);
  }

}


void Place_List::quality_control() {
  //Can't do the quality control until all of the population files have been read
  assert(Global::Pop.is_load_completed());

  int number_places = this->places.size();

  FRED_STATUS(0, "places quality control check for %d places\n", number_places);

  if(Global::Verbose) {
    int hn = 0;
    int nn = 0;
    int sn = 0;
    int wn = 0;
    double hsize = 0.0;
    double nsize = 0.0;
    double ssize = 0.0;
    double wsize = 0.0;
    // mean size by place type
    for(int p = 0; p < number_places; ++p) {
      int n = this->places[p]->get_size();
      if(this->places[p]->is_household()) {
	      hn++;
	      hsize += n;
      }
      if(this->places[p]->get_type() == Place::TYPE_NEIGHBORHOOD) {
	      nn++;
	      nsize += n;
      }
      if(this->places[p]->get_type() == Place::TYPE_SCHOOL) {
	      sn++;
	      ssize += n;
      }
      if(this->places[p]->get_type() == Place::TYPE_WORKPLACE) {
	      wn++;
	      wsize += n;
      }
    }
    if(hn) {
      hsize /= hn;
    }
    if(nn) {
      nsize /= nn;
    }
    if(sn) {
      ssize /= sn;
    }
    if(wn) {
      wsize /= wn;
    }
    fprintf(Global::Statusfp, "\nMEAN PLACE SIZE: H %.2f N %.2f S %.2f W %.2f\n",
	    hsize, nsize, ssize, wsize);
  }

  if(Global::Verbose>1) {
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/houses.dat", Global::Simulation_directory);
    FILE* fp = fopen(filename, "w");
    for(int p = 0; p < number_places; p++) {
      if(this->places[p]->is_household()) {
	      Place* h = this->places[p];
	      double x = Geo::get_x(h->get_longitude());
	      double y = Geo::get_y(h->get_latitude());
	      fprintf(fp, "%f %f\n", x, y);
      }
    }
    fclose(fp);
  }

 printf("HOUSEHOLD_TYPE DISTRIBUTION\n");
  if(Global::Quality_control > 1) {
    // household type
    int htypes = 17;
    enum htype_t {
      SINGLE_FEMALE,
      SINGLE_MALE,
      OPP_SEX_SIM_AGE_PAIR,
      OPP_SEX_DIF_AGE_PAIR,
      OPP_SEX_TWO_PARENTS,
      SINGLE_PARENT,
      SINGLE_PAR_MULTI_GEN_FAMILY,
      TWO_PAR_MULTI_GEN_FAMILY,
      UNATTENDED_KIDS,
      OTHER_FAMILY,
      YOUNG_ROOMIES,
      OLDER_ROOMIES,
      MIXED_ROOMIES,
      SAME_SEX_SIM_AGE_PAIR,
      SAME_SEX_DIF_AGE_PAIR,
      SAME_SEX_TWO_PARENTS,
      UNKNOWN,
    };

    string htype[17] = {
      "single-female",
      "single-male",
      "opp-sex-sim-age-pair",
      "opp-sex-dif-age-pair",
      "opp-sex-two-parent-family",
      "single-parent-family",
      "single-parent-multigen-family",
      "two-parent-multigen-family",
      "unattended_minors",
      "other-family",
      "young-roomies",
      "older-roomies",
      "mixed-roomies",
      "same-sex-sim-age-pair",
      "same-sex-dif-age-pair",
      "same-sex-two-parent-family",
      "unknown",
    };

    int type[htypes];
    int ttotal[htypes];
    int hnum = 0;
    for (int i = 0; i < htypes; i++) { type[i] = 0; ttotal[i] = 0;}
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
	hnum++;
        Household* h = static_cast<Household*>(this->places[p]);
	int count[76];
	for (int i = 0; i < 76; i++) { count[i] = 0;}
	int hsize = 0;
	int male_adult = 0;
	int female_adult = 0;
	int male_minor = 0;
	int female_minor = 0;
	int max_minor_age = -1;
	int max_adult_age = -1;
	int min_minor_age = 100;
	int min_adult_age = 100;
        for(int i = 0; i < h->get_size(); ++i) {
	  per = h->get_enrollee(i);
	  if(per != NULL) {
	    hsize++;
	    int a = per->get_age()/1;
	    if (a > 75) {
	      a = 75;
	    }
	    count[a]++;
	    if (a >= 18 && a < min_adult_age) { min_adult_age = a; }
	    if (a >= 18 && a > max_adult_age) { max_adult_age = a; }
	    if (a < 18 && a < min_minor_age) { min_minor_age = a; }
	    if (a < 18 && a > max_minor_age) { max_minor_age = a; }
	    if (a >= 18) {
	      if (per->get_sex() == 'F') {
		female_adult++;
	      }
	      else {
		male_adult++;
	      }
	    }
	    else {
	      if (per->get_sex() == 'F') {
		female_minor++;
	      }
	      else {
		male_minor++;
	      }
	    }
	  }
	}
	htype_t t = UNKNOWN;

	if (max_minor_age > -1) {
	// households with minors:

	  // more than two adults
	  if (male_adult+female_adult>2) {
	    // check for adult children in household
	    int max_child_age = -1;
	    for(int i = 0; i < hsize; ++i) {
	      per = h->get_enrollee(i);
	      int age = per->get_age();
	      // find age of oldest "child"
	      if (age < 30 && age < max_minor_age+15 && age > max_child_age) {
		max_child_age = age;
	      }
	    }
	    // count potential parents
	    int males = 0;
	    int females = 0;
	    int max_par_age = -1;
	    int min_par_age = 100;
	    for(int i = 0; i < hsize; ++i) {
	      per = h->get_enrollee(i);
	      int age = per->get_age();
	      // count potential parents
	      if (age >= max_child_age+15) {
		if (age > max_par_age) { max_par_age = age; }
		if (age < min_par_age) { min_par_age = age; }
		if (per->get_sex() == 'F') {
		  females++;
		}
		else {
		  males++;
		}
	      }
	    }
	    if (0 < males+females && males+females <= 2 && max_par_age < min_par_age+15) {
	      // we have at least one potential biological parent
	      if (males==1 && females==1) {
		t = OPP_SEX_TWO_PARENTS;
	      }
	      else if (males+females==2) {
		t = SAME_SEX_TWO_PARENTS;
	      }
	      else if (males+females==1) {
		t = SINGLE_PARENT;
	      }
	    }
	    else if (max_par_age >= min_par_age + 15) {
	      // multi-gen family:
	      // delete the older generation
	      int ma = males;
	      int fa = females;
	      for(int i = 0; i < hsize; ++i) {
		per = h->get_enrollee(i);
		int age = per->get_age();
		if (age >= min_par_age+15) {
		  if (per->get_sex() == 'F') {
		    fa--;
		  }
		  else {
		    ma--;
		  }
		}
	      }
	      if (ma+fa==1) {
		t = SINGLE_PAR_MULTI_GEN_FAMILY;
	      }
	      else if (ma+fa==2) {
		t = TWO_PAR_MULTI_GEN_FAMILY;
	      }
	      else {
		t = OTHER_FAMILY;
	      }
	    }
	    else {
	      t = OTHER_FAMILY;
	    }
	  } // end more than 2 adults

	  // two adult
	  if (male_adult+female_adult == 2) {
	    if (max_adult_age < min_adult_age + 15) {
	      if (male_adult==1 && female_adult==1) {
		t = OPP_SEX_TWO_PARENTS;
	      }
	      else {
		t = SAME_SEX_TWO_PARENTS;
	      }
	    }
	    else {
	      t = SINGLE_PAR_MULTI_GEN_FAMILY;
	    }
	  } // end two adults


	  // one adult
	  if (male_adult+female_adult == 1) {
	    t = SINGLE_PARENT;
	  } // end one adult

	  // no adults
	  if (male_adult+female_adult==0) {
	    if (hsize==2 && max_minor_age >= 15 && min_minor_age <= max_minor_age-14) {
	      t = SINGLE_PARENT;
	    } 
	    else if (count[15]+count[16]+count[17]==2 && min_minor_age <= max_minor_age-14) {
	      t = OPP_SEX_TWO_PARENTS;
	    } 
	    else if (hsize==2 && count[15]+count[16]+count[17]==2) {
	      if (female_minor && male_minor) {
		t = OPP_SEX_SIM_AGE_PAIR;
	      }
	      else {
		t = SAME_SEX_SIM_AGE_PAIR;
	      }
	    } 
	    else if (hsize == 1 && max_minor_age > 14) {
	      if (female_minor) {
		t = SINGLE_FEMALE;
	      }
	      else {
		t = SINGLE_MALE;
	      }
	    }
	    else if (hsize > 2 && count[17]==hsize) {
	      t = YOUNG_ROOMIES;
	    }
	    else {
	      t = UNATTENDED_KIDS;
	    }
	  }
	} // end households with minors

	else {

	  // single adults
	  if (hsize == 1) {
	    if (female_adult || female_minor) {
	      t = SINGLE_FEMALE;
	    }
	    else {
	      t = SINGLE_MALE;
	    }
	  }

	  // pairs of adults
	  if (hsize == 2) {
	    if (max_adult_age < min_adult_age+15) {
	      if (male_adult && female_adult) {
		t = OPP_SEX_SIM_AGE_PAIR;
	      }
	      else {
		t = SAME_SEX_SIM_AGE_PAIR;
	      }
	    }
	    else {
	      if (male_adult && female_adult) {
		t = OPP_SEX_DIF_AGE_PAIR;
	      }
	      else {
		t = SAME_SEX_DIF_AGE_PAIR;
	      }
	    }
	  } // end adults pairs

	  // more than 2 adults
	  if (hsize > 2) {
	    if (max_adult_age < 30) {
	      t = YOUNG_ROOMIES;
	    }
	    else if (min_adult_age >= 30) {
	      t = OLDER_ROOMIES;
	    }
	    else {
	      t = MIXED_ROOMIES;
	    }
	  }
	} // end adult-only households

	h->set_subtype((char) t);
	type[t]++;
	ttotal[t] += h->get_size();
	printf("HOUSEHOLD_TYPE: %s size = %d ", htype[t].c_str(), hsize);
	for(int c = 0; c < 76; ++c) {
	  if (count[c]) {
	    printf("%d=%d ",c, count[c]);
	  }
	}
	printf("\n");
      }
    }
    for(int t = 0; t < htypes; ++t) {
      printf("HOUSEHOLD TYPE DIST: ");
      printf("%30s: %8d households (%5.1f%%) with %8d residents (%5.1f%%)\n", htype[t].c_str(), type[t], (100.0*type[t])/hnum, ttotal[t], (100.0*ttotal[t]/Global::Pop.get_population_size()));
    }

    FILE* hfp = fopen("households.txt", "w");
    FILE* pfp = fopen("people.txt", "w");
    int hi = 0;
    int pi = 0;
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
	Place* house = this->places[p];
	int ht = (int) (house->get_subtype());
	fprintf(hfp, "hid %d size %d htype %d %s\n", hi, house->get_size(), ht, htype[ht].c_str());
	int hsize = house->get_size();
	for(int j = 0; j < hsize; ++j) {
	  Person* person = house->get_enrollee(j);
	  fprintf(pfp, "pid %d age %d sex %c hid %d htype %d %s\n", pi++, person->get_age(), person->get_sex(), hi, ht, htype[ht].c_str());
	}
	hi++;
      }
    }
    fclose(hfp);
    fclose(pfp);
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // size distribution of households
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        int n = this->places[p]->get_size();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // adult distribution of households
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        int n = h->get_adults();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // children distribution of households
    for (int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        int n = h->get_children();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold children size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // adult distribution of households with children
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        int n = h->get_adults();
        if(n < 15) {
          count[n]++;
        } else {
          count[14]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nHousehold w/ children, adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  // relationship between children and decision maker
  if(Global::Verbose > 1 && Global::Enable_Behaviors) {
    // find adult decision maker for each child
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        int size = h->get_size();
        for(int i = 0; i < size; ++i) {
          Person* child = h->get_enrollee(i);
          int ch_age = child->get_age();
          if(ch_age < 18) {
            int ch_rel = child->get_relationship();
	          Person* dm = child->get_health_decision_maker();
	          if(dm == NULL) {
	            printf("DECISION_MAKER: household %d %s  child: %d %d is making own health decisions\n",
		           h->get_id(), h->get_label(), ch_age, ch_rel);
	          } else {
	            int dm_age = dm->get_age();
	            int dm_rel = dm->get_relationship();
	            if(dm_rel != 1 || ch_rel != 3) {
		            printf("DECISION_MAKER: household %d %s  decision_maker: %d %d child: %d %d\n",
		            h->get_id(), h->get_label(), dm_age, dm_rel, ch_age, ch_rel);
	            }
	          }
	        }
        }
      }
    }
  }

  if(Global::Verbose) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        for(int i = 0; i < h->get_size(); ++i) {
          if(h->get_enrollee(i)->is_householder()) {
            per = h->get_enrollee(i);
          }
        }
        if(per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s size %d groupquarters: %d\n",
		       h->get_id(), h->get_label(), h->get_size(), h->is_group_quarters() ? 1 : 0);
          count[0]++;
        } else {
          int a = per->get_age();
          if(a < 100) {
            count[a]++;
          } else {
            count[99]++;
          }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[100];
    int total = 0;
    int children = 0;
    // age distribution of heads of households with children
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        children += h->get_children();
        for(int i = 0; i < h->get_size(); ++i) {
          if(h->get_enrollee(i)->is_householder()) {
            per = h->get_enrollee(i);
          }
        }
        if(per == NULL) {
          FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
		       h->get_id(), h->get_label(), h->is_group_quarters() ? 1 : 0);
          count[0]++;
        } else {
          int a = per->get_age();
          if(a < 100) {
            count[a]++;
          } else {
            count[99]++;
          }
          total++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households with children: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "children = %d\n", children);
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count_has_school_age = 0;
    int count_has_school_age_and_unemployed_adult = 0;
    int total_hh = 0;

    //Households with school-age children and at least one unemployed adult
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      if(this->places[p]->is_household()) {
        total_hh++;
        Household* h = static_cast<Household*>(this->places[p]);
        if(h->get_children() == 0) {
          continue;
        }
        if(h->has_school_aged_child()) {
          count_has_school_age++;
        }
        if(h->has_school_aged_child_and_unemployed_adult()) {
          count_has_school_age_and_unemployed_adult++;
        }
      }
    }
    fprintf(Global::Statusfp, "\nHouseholds with school-aged children and at least one unemployed adult\n");
    fprintf(Global::Statusfp, "Total Households: %d\n", total_hh);
    fprintf(Global::Statusfp, "Total Households with school-age children: %d\n", count_has_school_age);
    fprintf(Global::Statusfp, "Total Households with school-age children and at least one unemployed adult: %d\n", count_has_school_age_and_unemployed_adult);
  }

  if(Global::Verbose) {
    int count[100];
    int total = 0;
    // size distribution of schools
    for(int c = 0; c < 20; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->get_type() == Place::TYPE_SCHOOL) {
        int s = this->places[p]->get_size();
        int n = s / 50;
        if(n < 20) {
          count[n]++;
        } else {
          count[19]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nSchool size distribution: %d schools\n", total);
    for(int c = 0; c < 20; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      (c+1)*50, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    // age distribution in schools
    fprintf(Global::Statusfp, "\nSchool age distribution:\n");
    int count[100];
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->get_type() == Place::TYPE_SCHOOL) {
        // places[p]->print(0);
        for(int c = 0; c < 100; ++c) {
          count[c] += (static_cast<School*>(this->places[p]))->get_students_in_grade(c);
        }
      }
    }
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age = %2d  students = %6d\n",
	      c, count[c]);;
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[50];
    int total = 0;
    // size distribution of classrooms
    for(int c = 0; c < 50; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->get_type() == Place::TYPE_CLASSROOM) {
        int s = this->places[p]->get_size();
        int n = s;
        if(n < 50) {
          count[n]++;
        } else {
          count[50-1]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nClassroom size distribution: %d classrooms\n", total);
    for(int c = 0; c < 50; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose) {
    int count[101];
    int small_employees = 0;
    int med_employees = 0;
    int large_employees = 0;
    int xlarge_employees = 0;
    int total_employees = 0;
    int total = 0;
    // size distribution of workplaces
    for(int c = 0; c <= 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->get_type() == Place::TYPE_WORKPLACE || places[p]->get_type() == Place::TYPE_SCHOOL) {
        int s = this->places[p]->get_size();
	      if(this->places[p]->get_type() == Place::TYPE_SCHOOL) {
	        School* school = static_cast<School*>(this->places[p]);
	        s = school->get_staff_size();
	      }
        int n = s;
        if(n <= 100) {
          count[n]++;
        } else {
          count[100]++;
        }
        total++;
        if(s < 50) {
          small_employees += s;
        } else if (s < 100) {
          med_employees += s;
        } else if (s < 500) {
          large_employees += s;
        } else {
          xlarge_employees += s;
        }
        total_employees += s;
      }
    }
    fprintf(Global::Statusfp, "\nWorkplace size distribution: %d workplaces\n", total);
    for(int c = 0; c <= 100; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      (c+1)*1, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n\n");

    fprintf(Global::Statusfp, "employees at small workplaces (1-49): ");
    fprintf(Global::Statusfp, "%d\n", small_employees);

    fprintf(Global::Statusfp, "employees at medium workplaces (50-99): ");
    fprintf(Global::Statusfp, "%d\n", med_employees);

    fprintf(Global::Statusfp, "employees at large workplaces (100-499): ");
    fprintf(Global::Statusfp, "%d\n", large_employees);

    fprintf(Global::Statusfp, "employees at xlarge workplaces (500-up): ");
    fprintf(Global::Statusfp, "%d\n", xlarge_employees);

    fprintf(Global::Statusfp, "total employees: %d\n\n", total_employees);
  }

  /*
    if(Global::Verbose) {
    int covered[4];
    int all[4];
    // distribution of sick leave in workplaces
    for(int c = 0; c < 4; ++c) { all[c] = covered[c] = 0; }
    for(int p = 0; p < number_places; ++p) {
    if(this->places[p]->get_type() == WORKPLACE) {
    Workplace* work = static_cast<Workplace*>(this->places[p]);
    char s = work->get_size_code();
    bool sl = work->is_sick_leave_available();
    switch(s) {
    case 'S':
    all[0] += s;
    if(sl) {
    covered[0] += s;
    }
    break;
    case 'M':
    all[1] += s;
    if(sl) {
    covered[1] += s;
    }
    break;
    case 'L':
    all[2] += s;
    if(sl) {
    covered[2] += s;
    }
    break;
    case 'X':
    all[3] += s;
    if(sl) {
    covered[3] += s;
    }
    break;
    }
    }
    }
    fprintf(Global::Statusfp, "\nWorkplace sick leave coverage: ");
    for(int c = 0; c < 4; ++c) {
    fprintf(Global::Statusfp, "%3d: %d/%d %5.2f | ",
    c, covered[c], all[c], (all[c]? (1.0*covered[c])/all[c] : 0));
    }
    fprintf(Global::Statusfp, "\n");
    }
  */

  if(Global::Verbose) {
    int count[60];
    int total = 0;
    // size distribution of offices
    for(int c = 0; c < 60; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_places; ++p) {
      if(this->places[p]->get_type() == Place::TYPE_OFFICE) {
        int s = this->places[p]->get_size();
        int n = s;
        if(n < 60) {
          count[n]++;
        } else {
          count[60 - 1]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nOffice size distribution: %d offices\n", total);
    for(int c = 0; c < 60; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "places quality control finished\n");
    fflush(Global::Statusfp);
  }
}

