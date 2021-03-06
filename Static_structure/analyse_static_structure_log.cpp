
/* calculate static structure factor */

// COMPILATION AND RUN COMMANDS:
// g++ -Wl,-rpath=$HOME/hdf5/lib -L$HOME/hdf5/lib -I$HOME/hdf5/include ${spath}/analyse_static_structure_log.cpp ${upath}/read_write.cpp -lhdf5 -fopenmp -o analyse_static_struct
// ./analyse_static_struct out.h5 ${path}/Static_struct.txt

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cmath>
#include "omp.h"
#include "../Utility/read_write.hpp"
#include "../Utility/basic.hpp"
#include "../Utility/sim_info.hpp"

#define pi M_PI

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////

double compute_static_structure (double **x, double **y, double *S, double *kvec, 
				 int Nmax, int natoms, int nsteps, double l,
				 double kmax, double kmin, int njump, double delk, int nks) {
  /* calculate static structure per timestep with a running average */

  // set preliminary data
  
  double wbin = (kmax-kmin)/Nmax;

//   cout << "delta k = " << delk << "\n" << "wbin = " << wbin << "\n" << "kmin = " << exp(kmin) << "\n" << "kmax = " << exp(kmax) << "\n" << "Nmax = " << Nmax << "\n" << endl;
  cout << "delta k = " << delk << "\n" << "wbin = " << wbin << "\n" << "kmin = " << kmin << "\n" << "kmax = " << kmax << "\n" << "Nmax = " << Nmax << "\n" << endl;
  
  // populate log-spaced absolute value of the k vectors
  
  for (int j = 0; j < Nmax; j++) kvec[j] = kmin + j*wbin;
/*  for (int j = 0; j < Nmax; j++) kvec[j] = exp(kvec[j]);*/
  
  // populate circular orientation of the k vector 
  
  double *kxs = new double[nks];
  double *kys = new double[nks];
  for (int j = 0; j < nks; j++) {
    kxs[j] = cos(2*pi*j/nks);
    kys[j] = sin(2*pi*j/nks);
  }
      
  for (int step = 0; step < nsteps; step += njump) {
    
    for (int k = 0; k < Nmax; k++) {
      double term = 0.0;

	for (int l = 0; l < nks; l++) {
	  double kx = kxs[l]*kvec[k];
	  double ky = kys[l]*kvec[k];
	  double costerm = 0.;
	  double sinterm = 0.;

	  omp_set_num_threads(4);
	  #pragma omp parallel for reduction(+:costerm,sinterm)
	  for (int j = 0; j < natoms; j++) {
	    double dotp = kx*x[step][j] + ky*y[step][j];
	    costerm += cos(dotp);
	    sinterm += sin(dotp);
    
	  } // particle loop

	  term += costerm*costerm + sinterm*sinterm;

	} // orientation loop
	
	term /= nks;
	S[k] += term;
	
    } // absolute value loop  
  
  } // timesteps
  
  // perform the normalization
  
  double totalData = nsteps/njump;
  for (int j = 0; j < Nmax; j++)  { 
    S[j] /= (natoms*totalData); 
  }  
  
  return totalData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv[]) {
 
  // get the file name by parsing and load simulation data
  
  string filename = argv[1];
  cout << "Calculating static structure factor of the following file: \n" << filename << endl;
  SimInfo sim(filename);  
  cout << "nsteps = " << sim.nsteps << endl;
  cout << "ncells = " << sim.ncells << endl;
    
  // read in the cell position data all at once
  
  /* the position data is stored in the following format:
  (nsteps, 2, ncells)
  the data will be loaded as follows:
  (nsteps, ncells) in x and y separately 
  */
  
  double **x = new double*[sim.nsteps];
  for (int i = 0; i < sim.nsteps; i++) x[i] = new double[sim.ncells];
  
  double **y = new double*[sim.nsteps];
  for (int i = 0; i < sim.nsteps; i++) y[i] = new double[sim.ncells];
  
  for (int i = 0; i < sim.nsteps; i++) {
    for (int j = 0; j < sim.ncells; j++) {
      x[i][j] = 0.; y[i][j] = 0.;
    }
  }
  
  read_all_pos_data(filename, x, y, sim.nsteps, sim.ncells, "/cells/comu");
  
  // get the image positions in the central box
  
  get_img_pos(x, y, sim.nsteps, sim.ncells, sim.lx, sim.ly);
   
  // allocate the arrays and set preliminary information

  double delk = 2*pi/sim.lx;
  double lamda_min = 2.; 
  double lamda_max = sim.lx;
  double kmax = 2*pi/lamda_min;
  double kmin = 2*pi/lamda_max;
  int nks = 4;
  int njump = 20;
/*  kmax = log(kmax);
  kmin = log(kmin);*/  
  int Nmax = static_cast<int>((kmax-kmin)/delk);
  double wbin = (kmax-kmin)/Nmax;
  double *kvec = new double[Nmax];
  double *S = new double[Nmax];
  for (int j = 0; j < Nmax; j++) {
    S[j] = 0.;  kvec[j] = 0.;
  }
  
  // calculate the static structure factor
  
  double ndata = compute_static_structure(x, y, S, kvec, Nmax, sim.ncells, sim.nsteps, sim.lx, kmax, kmin, njump, delk, nks);  
  
  // write the computed data
  
  string outfilepath = argv[2];
  cout << "Writing static structure factor to the following file: \n" << outfilepath << endl;  
  write_2d_analysis_data(kvec, S, Nmax, outfilepath);
  
  // deallocate the arrays
  
  for (int i = 0; i < sim.nsteps; i++) {
    delete [] x[i];
    delete [] y[i];
  }
  delete [] x;
  delete [] y;
  
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
