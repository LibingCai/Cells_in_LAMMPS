
/* calculate polar order parameter from the velocity vector */

// COMPILATION AND RUN COMMANDS:
// g++ -O3 -lm -Wl,-rpath=$HOME/hdf5/lib -L$HOME/hdf5/lib -I$HOME/hdf5/include ${spath}/calc_polar_order.cpp ${upath}/read_write.cpp -lhdf5 -o calc_polar_order
// ./calc_polar_order out.h5 ${path}/Polar_order.txt

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include "../Utility/read_write.hpp"
#include "../Utility/basic.hpp"

#define pi M_PI

// decide on which version to use
#define SUBTRACT_COM		// subtract center of mass velocities per frame

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void calc_velocity (vector<double> &vx, vector<double> &vy,
                    double **x, double **y,
                    const double lx, const double ly, const int delta,
                    const int ncells, const int nvels, const double dt) {
  /* calculate velocities with dt as delta */
  
  const double deltaDt = delta*dt;
  for (int i = 0; i < nvels; i++) {
    
    #ifdef SUBTRACT_COM
    long double comvx = 0.;
    long double comvy = 0.;
    #endif
    
    for (int j = 0; j < ncells; j++) {
      
      int curr_step = i*delta;
      int next_step = curr_step + delta;
      
      // note that UNWRAPPED COORDS ARE ASSUMED!
      
      double dx = x[next_step][j] - x[curr_step][j];
      vx[i*ncells+j] = dx/deltaDt;
      
      double dy = y[next_step][j] - y[curr_step][j];
      vy[i*ncells+j] = dy/deltaDt;
      
      #ifdef SUBTRACT_COM
      comvx += vx[i*ncells+j];
      comvy += vy[i*ncells+j];
      #endif
      
    }   // cell loop
    
    #ifdef SUBTRACT_COM
    comvx /= ncells;
    comvy /= ncells;    
    
    for (int j = 0; j < ncells; j++) {
      vx[i*ncells+j] -= comvx;
      vy[i*ncells+j] -= comvy;
    } 	// cells loop
    #endif
    
  }     // velocity step loop
  
  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

double calc_polar_order_param (const vector<double> &vx, const vector<double> &vy, 
			       const int nsteps, const int ncells) {
  /* calculate the polar order parameter */
    
  double polar_order_param = 0.;
  
  for (int step = 0; step < nsteps; step++) {
    
    double cost = 0.;
    double sint = 0.;
    
    for (int j = 0; j < ncells; j++) {
      
      double angle = atan2(vy[step*ncells+j], vx[step*ncells+j]);
      cost += cos(angle);
      sint += sin(angle);
      
    }	// cell loop
    
    polar_order_param += sqrt(cost*cost + sint*sint);
    
  }  	// timestep loop
  
  polar_order_param /= (nsteps*ncells);
    
  return polar_order_param;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv[]) {

  // get the file name by parsing
  
  string filename = argv[1];
  cout << "Calculating polar order parameter of the following file: \n" << filename << endl;

  // read in general simulation data
  
  int nsteps, nbeads, nsamp, ncells;
  nsteps = nbeads = nsamp = ncells = 0;
  double lx, ly, dt, eps, rho, fp, areak, bl, sigma;
  lx = ly = dt = eps = rho = fp = areak = bl = sigma = 0.;
  
  read_sim_data(filename, nsteps, nbeads, nsamp, ncells, lx, ly,
                dt, eps, rho, fp, areak, bl, sigma);
  
  // print simulation information
  
  cout << "nsteps = " << nsteps << endl;
  cout << "ncells = " << ncells << endl;
  
  // read in array data
  
  int nbpc[ncells];
  read_integer_array(filename, "/cells/nbpc", nbpc);
 
  // read in the cell position data all at once
  
  /* the position data is stored in the following format:
  (nsteps, 2, ncells)
  the data will be loaded as follows:
  (nsteps, ncells) in x and y separately 
  */
  
  double **x = new double*[nsteps];
  for (int i = 0; i < nsteps; i++) x[i] = new double[ncells];
  
  double **y = new double*[nsteps];
  for (int i = 0; i < nsteps; i++) y[i] = new double[ncells];
  
  for (int i = 0; i < nsteps; i++) {
    for (int j = 0; j < ncells; j++) {
      x[i][j] = 0.;  y[i][j] = 0.;
    }
  }
  
  read_all_pos_data(filename, x, y, nsteps, ncells, "/cells/comu");

  // set variables related to the analysis
  
  const int delta = 20;                 // number of data points between two steps
                                        // to calculate velocity
  const int nvels = nsteps/delta;       // number of data points in the velocity array
  
  // calculate the velocities
  
  vector<double> vx(nvels*ncells);
  vector<double> vy(nvels*ncells);
  calc_velocity(vx, vy, x, y, lx, ly, delta, ncells, nvels, dt);
  
  // calculate the velocity correlation in space
  
  double polar_order_param = calc_polar_order_param(vx, vy, nvels, ncells);

  // write the computed data
  
  string outfilepath = argv[2];
  cout << "Writing polar order parameter to the following file: \n" << outfilepath << endl;
  write_single_analysis_data(polar_order_param, outfilepath);
  
  // deallocate the arrays
  
  for (int i = 0; i < nsteps; i++) {
    delete [] x[i];
    delete [] y[i];
  }
  delete [] x;
  delete [] y;
  
  return 0;
}  

//////////////////////////////////////////////////////////////////////////////////////////////////////////
