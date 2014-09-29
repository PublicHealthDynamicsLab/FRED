**FRED Web Analytics**

**John Grefenstette**

*University of Pittsburgh*

**28 Mar 2014**

.. image:: whitespace.pdf
   :width: 3in
   :height: 0.05in

This document describes how to view FRED simulation results using the
FRED web site.

First, aim your browser at ``http://fred.publichealth.pitt.edu``.

**Note:** If you access the site from a mobile device, you will
redirected to the mobile site. Click on the **Full Site** button to
access the full web site from a mobile device.

Now click on the **Analytics** tab. The **Analytics Page** lets you
access a large number of previously run FRED simulations.  You can
select results by location or by population characteristics.

.. image:: whitespace.pdf
   :width: 3in
   :height: 0.05in

**Results by Location**

#. Choose a country, state and county for any US location. Selected
   international locations are also available.

#. Click **Get Results**.

#. The **Results Page** will display a set of pending requests and a set
   of completed requests. Each job has a *key* that includes the FIPS
   code for the location and the value of key parameters.  For example,
   the key **FIPS=42003_R0.05.2** indicates that the location is
   Allegheny County, PA (FIPS code 42003) and the epidemic is calibrated
   to a basic reproductive number of R0 = 1.2.  Clicking on the key will
   take you to the results for that FRED job.

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

**Results by Population Characteristics**

1. Choose a population size, population density, and disease
   transmission. Click on **Get Results**.

#. You will see a list of all US counties that match the selected
   population size and density characteristics.

#. Click on **Visualize** to go the the results page for that county.

#. Click on **Download Incidence Data** to access a csv file for the
   given simulation.

.. image:: whitespace.pdf
   :width: 3in
   :height: 0.05in

**Feedback?**

We are very interested in receiving feedback about the FRED web
site. Please send any comments or suggestions for new features to
**gref@pitt.edu**.  Thanks!



