**Running FRED Simulation via the Web**

**John Grefenstette**

*University of Pittsburgh*

**28 Mar 2014**

.. image:: whitespace.pdf
   :width: 3in
   :height: 0.05in

This document describes how to set up FRED simulations using the
FRED web site.

First, aim your browser at ``http://fred.publichealth.pitt.edu``.

**Note:** If you access the site from a mobile device, you will
redirected to the mobile site. Click on the **Full Site** button to
access the full web site from a mobile device.

Now click on the **Simulator** tab. The **Simulation Page** lets you
request a FRED simulation for a specific location and epidemic scenario.

.. image:: whitespace.pdf
   :width: 3in
   :height: 0.05in

**Setting Simulation Parameters**

#. Choose a county and state from the pull-down menues. Alternatively,
   you can type in the name of a city, in which case the County and
   State menus will be ignored.  To enter a city, use the format "CITY,
   STATE_ABBREVIATION", e.g. "Pittsburgh, PA"

#. Select epidemic parameters. You can select values of the basic
   reproductive rate R0 from the pull-down menu.  You can also select
   how many days to run the simulation.

#. Select control parameters. You can set values for selected epidemic
   mitigation control strategies.  You can currently set School Closure
   and Immunization values.  

   * **School Closure** is implemented on the basis of individual
     schools within the simulation area. Each school will close when a
     set number of cases occurs on a given school day.  The number of
     cases is set by the "School Closure Trigger" menu.  The duration of
     the school closure in weeks is set by the **School Closure
     Duration** menu.

   * **Immunization** means that the selected fraction of the population
     is immunized prior to the start of the epidemic.

#. Contact Information. If you enter an email address into the Contact
   field, you will eb sent an email notification when the simulation
   results are ready (usually a few minutes.)

#. Click **Submit**.

#. The display will change to include a link to the Results page for the
   requested simulation.  The results will be available immediately if
   the requested simulation has already been requested
   previously. Otherwise, the results should be available within a few
   minutes.

#. The results page for a FRED simulation includes:

   * The set of FRED parameters used to set up the job.

   * A movie showing the spread of the epidemic in the selected
     location.

   * Plots of the daily **Incidence**, **Prevalence**, and **Attack
     Rate** (cumulative incidence).  The plots may also include the
     plots for the baseline case with no interventions.

#. You can also download the incidence and symtomatic_incidence data
   from the linked csv files.

.. image:: whitespace.pdf
   :width: 3in
   :height: 0.05in

**Feedback?**

We are very interested in receiving feedback about the FRED web
site. Please send any comments or suggestions for new features to
**gref@pitt.edu**.  Thanks!



