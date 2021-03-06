
#pragma once

#include <string>
#include <iostream>
#include <vector>
#include "basic.hpp"
#include "read_write.hpp"

class SimInfo {
  /* container for general simulation data */
  public:
    
    SimInfo(std::string, std::string, bool, double**&, double**&); 
    int nsteps, nbeads, nsamp, ncells;
    double lx, ly, dt, eps, rho, fp, areak, kappa, bl, sigma;
    std::vector<int> nbpc;
    std::vector<int> cid;
  
};

SimInfo::SimInfo(std::string filename, std::string cells_or_beads, 
		 bool get_image, double **&x, double **&y) {
  
  // read in general simulation data

  nsteps = nbeads = nsamp = ncells = 0;
  lx = ly = dt = eps = rho = fp = areak = kappa = bl = sigma = 0.;

  read_sim_data(filename, nsteps, nbeads, nsamp, ncells, lx, ly, 
		dt, eps, rho, fp, areak, kappa, bl, sigma);

  int nbpc_buffer[ncells];
  for (int i = 0; i < ncells; i++) nbpc_buffer[i] = 0;
  read_integer_array(filename, "/cells/nbpc", nbpc_buffer);  
  for (int i = 0; i < ncells; i++) nbpc.push_back(nbpc_buffer[i]);

  // read in position data -either for cells or beads-
  /* the position data is stored in the following format:
  (nsteps, 2, ncells)
  the data will be loaded as follows:
  (nsteps, ncells) in x and y separately 
  */
  
  // choose to read in cells or beads
  
  int ndata;
  std::string data_path;
  bool load_cid = false;
  if (cells_or_beads == "cells") {
    ndata = ncells;
    data_path = "/cells/comu";
  }
  else if (cells_or_beads == "beads") {
    ndata = nbeads;
    data_path = "/beads/xu";
    load_cid = true;
  }
  else {
    throw "cells_or_beads variable should be cells or beads!";
  }
  
  // allocate and initialize positions
  
  x = new double*[nsteps];
  for (int i = 0; i < nsteps; i++) x[i] = new double[ndata];
  
  y = new double*[nsteps];
  for (int i = 0; i < nsteps; i++) y[i] = new double[ndata];
  
  for (int i = 0; i < nsteps; i++) {
    for (int j = 0; j < ndata; j++) {
      x[i][j] = 0.; y[i][j] = 0.;
    }
  }
 
  // read the positions
  
  read_all_pos_data(filename, x, y, nsteps, ndata, data_path);
  
  // get the image positions in the central unit box if chosen so
  
  if (get_image) 
    get_img_pos(x, y, nsteps, ndata, lx, ly);
  
  if (load_cid) {
    int cid_buffer[nbeads];
    for (int i = 0; i < nbeads; i++) cid_buffer[i] = 0;
    read_integer_array(filename, "/beads/cid", cid_buffer);  
    for (int i = 0; i < nbeads; i++) cid.push_back(cid_buffer[i]);    
  }
  
  return;
}