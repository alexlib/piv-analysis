/*
Calculate the "dumb" phase average
The timesteps have to be constant and a multiple of the 
period so every  n files can be simple averaged together

Copyright (C) 2014  Philippe Miron

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/> 
*/

#include "include.hpp"
#include "tecplotio.hpp"
#include "stats.hpp"

string filein, fileout;

int main()
{
	
// Parameters
vector<string> prefixe(77,"/home/pissarro/phmir/1-donnees_piv/Ladyf/150610-Piston-Trig-VR/s");
for (size_t i(0); i<prefixe.size(); i++)
	prefixe[i] += to_string(i+1) + "/B";

string suffixe = ".dat";
int first_filenumber = 200;
//int N =  5000-first_filenumber+1;
int N = 550;
int N_Phase = 550;
double Flow_Period = 550.0;
//double cam_f =  200.0; // Hz
int Nx = 78;
int Ny = 100;

// Average convergence
vector<pair<int, int>> convergence_index;

// Add indexes where convergence will be evaluated
// make_pair(index_x, index_y)
// make sure it is between [0, Nx-1] and [0, Ny-1] !
convergence_index.emplace_back(make_pair(13, 91));
convergence_index.emplace_back(make_pair(30, 91));
convergence_index.emplace_back(make_pair(30, 65));
convergence_index.emplace_back(make_pair(68, 73));
convergence_index.emplace_back(make_pair(68, 55));
convergence_index.emplace_back(make_pair(68, 35));
convergence_index.emplace_back(make_pair(53, 14));


vector2d<double> u_avg_last, v_avg_last;
vector3d<double> u_avg, v_avg, u_error, v_error;
Resize(u_avg_last, convergence_index.size(), N_Phase);
Resize(v_avg_last, convergence_index.size(), N_Phase);
Resize(u_avg, convergence_index.size(), N_Phase, prefixe.size());
Resize(v_avg, convergence_index.size(), N_Phase, prefixe.size());
Resize(u_error, convergence_index.size(), N_Phase, prefixe.size());
Resize(v_error, convergence_index.size(), N_Phase, prefixe.size());


// Create arrays
double*** x = Construct3D(N_Phase, Ny, Nx);
double*** y = Construct3D(N_Phase, Ny, Nx);
double*** u = Construct3D(N_Phase, Ny, Nx);
double*** v = Construct3D(N_Phase, Ny, Nx);
double*** u2 = Construct3D(N_Phase, Ny, Nx);
double*** v2 = Construct3D(N_Phase, Ny, Nx);
double*** uv = Construct3D(N_Phase, Ny, Nx);
vector<int> count_per_phase(N_Phase, 0);

// Read the velocities of all the fields
for (size_t folder(0); folder<prefixe.size(); folder++) {
	cout << folder << endl;
	for (int i=0; i<N; i++) {
		// Calculate index
		int j = int( floor(fmod(double(i), Flow_Period) * N_Phase / Flow_Period) );

		// Get the filename
		filein = Filename(prefixe[folder], suffixe, first_filenumber+i);

		// Read and fill up the arrays
		Read_Tp2D_Average(filein, Nx, Ny, x[j], y[j], u[j], v[j], uv[j], u2[j], v2[j]);
		count_per_phase[j]++;
	}
	
  // Evaluate convergence of the average of u and v after each series
  // at location specify with the convergence indexes
  for (size_t cp(0); cp<convergence_index.size(); cp++) {
    int cp_i(convergence_index[cp].first);
    int cp_j(convergence_index[cp].second);

    for (int j(0); j<N_Phase; j++) {
      if (count_per_phase[j] != 0) {
	      u_avg[cp][j][folder] = u[j][cp_j][cp_i]/(double) count_per_phase[j];
	      v_avg[cp][j][folder] = v[j][cp_j][cp_i]/(double) count_per_phase[j];
	
	      u_error[cp][j][folder] = u_avg[cp][j][folder] - u_avg_last[cp][j];
	      v_error[cp][j][folder] = v_avg[cp][j][folder] - v_avg_last[cp][j];
	
	      // keep last average to calculate error at next file
	      u_avg_last[cp][j] = u_avg[cp][j][folder];
	      v_avg_last[cp][j] = v_avg[cp][j][folder];
      }
    }
  }
}

// Calculate the average
for (int i=0; i < N_Phase; i++) {
	for (int j=0; j < Ny; j++) {
		for (int k=0; k < Nx; k++) {
			u[i][j][k]  /= (double) count_per_phase[i];
			v[i][j][k]  /= (double) count_per_phase[i];
			u2[i][j][k] /= (double) count_per_phase[i];
			v2[i][j][k] /= (double) count_per_phase[i];
			uv[i][j][k] /= (double) count_per_phase[i];
		}
	}
}

// RMS
double*** u_rms = Construct3D(N_Phase, Ny, Nx);
double*** v_rms = Construct3D(N_Phase, Ny, Nx);
double*** uv_rms = Construct3D(N_Phase, Ny, Nx);

for (int i=0; i < N_Phase; i++) {
	RMS(Nx, Ny, u[i], u2[i], u_rms[i]);
	RMS(Nx, Ny, v[i], v2[i], v_rms[i]);
	RMS(Nx, Ny, u[i], v[i], uv[i], uv_rms[i]);
}

// Write data
for (int i=0; i < N_Phase; i++) {
	fileout = "./phase_average_" + to_string(i) + ".plt";
	//Write_Tp2D_AvgVelocities(fileout, double(i)/cam_f, Nx, Ny, x[i], y[i], u[i], v[i], u_rms[i], v_rms[i], uv_rms[i]);
	fileout = "./phase_average_" + to_string(i) + ".dat";
	//Write_Tp2D_Velocities(fileout, Nx, Ny, x[i], y[i], u[i], v[i]);
}

// Write error convergence
for (size_t i(0); i<convergence_index.size(); i++) {
	string filename = "./avg_convergence_" + to_string(convergence_index[i].first) + "-" + to_string(convergence_index[i].second) + ".plt";
	Write_Avg_Convergence(filename, convergence_index[i], u_avg[i], v_avg[i], u_error[i], v_error[i]);
}

// Delete arrays created
Destruct3D(x);
Destruct3D(y);
Destruct3D(u);
Destruct3D(v);
Destruct3D(uv);
Destruct3D(u2);
Destruct3D(v2);
Destruct3D(u_rms);
Destruct3D(v_rms);
Destruct3D(uv_rms);

return 0;
}
