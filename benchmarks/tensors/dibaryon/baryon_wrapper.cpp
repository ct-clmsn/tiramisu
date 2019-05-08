#include "Halide.h"
#include <tiramisu/utils.h>
#include <cstdlib>
#include <iostream>
#include "benchmarks.h"

#include "baryon_wrapper.h"
#include "baryon_ref.cpp"

int main(int, char **)
{
    std::vector<std::chrono::duration<double,std::milli>> duration_vector_1;
    std::vector<std::chrono::duration<double,std::milli>> duration_vector_2;

    std::complex<double> Blocal[Nsrc][Nc][Ns][Nc][Ns][Nc][Ns][Vsnk][Lt];
    std::complex<double> prop[Nq][Nc][Ns][Nc][Ns][Vsnk][Lt][Vsrc];
    int color_weights[Nw][Nq];
    int spin_weights[Nw][Nq];
    double weights[Nw];
    std::complex<double> psi[Nsrc][Vsrc];

    // Blocal
    // Blocal_r: tiramisu real part of Blocal.
    // Blocal_i: tiramisu imaginary part of Blocal.
    Halide::Buffer<double> Blocal_r(Lt, Vsnk, Ns, Nc, Ns, Nc, Ns, Nc, Nsrc, "Blocal_r");
    Halide::Buffer<double> Blocal_i(Lt, Vsnk, Ns, Nc, Ns, Nc, Ns, Nc, Nsrc, "Blocal_i");

    // prop
    Halide::Buffer<double> prop_r(Vsrc, Lt, Vsnk, Ns, Nc, Ns, Nc, Nq, "prop_r");
    Halide::Buffer<double> prop_i(Vsrc, Lt, Vsnk, Ns, Nc, Ns, Nc, Nq, "prop_i");

    // psi
    Halide::Buffer<double> psi_r(Vsrc, Nsrc, "psi_r");
    Halide::Buffer<double> psi_i(Vsrc, Nsrc, "psi_i");

    Halide::Buffer<int> color_weights_t(Nq, Nw, "color_weights_t");
    Halide::Buffer<int> spin_weights_t(Nq, Nw, "spin_weights_t");
    Halide::Buffer<double> weights_t(Nw, "weights_t");

    // Initialization
   for (int wnum=0; wnum<Nw; wnum++)
   {
       double v = rand();
       weights[wnum] = v;
       weights_t(wnum) = v;
   }

   for (int n=0; n<Nsrc; n++)
     for (int y=0; y<Vsrc; y++)
     {
        double v = rand();
	psi[n][y] = v;
	psi_r(y, n) = v;
     }

   for (int tri=0; tri<Nq; tri++)
       for (int iCprime=0; iCprime<Nc; iCprime++)
	  for (int iSprime=0; iSprime<Ns; iSprime++)
	     for (int jCprime=0; jCprime<Nc; jCprime++)
		for (int jSprime=0; jSprime<Ns; jSprime++)
                   for (int x=0; x<Vsnk; x++)
                      for (int t=0; t<Lt; t++)
		        for (int y=0; y<Vsrc; y++)
			{
			    double v = rand();
			    prop[tri][iCprime][iSprime][jCprime][jSprime][x][t][y] = v;
			    prop_r(y, t, x, jSprime, jCprime, iSprime, iCprime, tri) = v;
 		        }


    for (int i = 0; i < NB_TESTS; i++)
    {
	    auto start2 = std::chrono::high_resolution_clock::now();

	    make_local_block(Blocal, prop, color_weights, spin_weights, weights, psi);

	    auto end2 = std::chrono::high_resolution_clock::now();
	    std::chrono::duration<double,std::milli> duration2 = end2 - start2;
	    duration_vector_2.push_back(duration2);
    }

    for (int i = 0; i < NB_TESTS; i++)
    {
	    auto start1 = std::chrono::high_resolution_clock::now();

	    tiramisu_generated_code(Blocal_r.raw_buffer(),
				    Blocal_i.raw_buffer(),
				    prop_r.raw_buffer(),
				    prop_i.raw_buffer(),
				    weights_t.raw_buffer(),
				    psi_r.raw_buffer(),
				    psi_i.raw_buffer(),
				    color_weights_t.raw_buffer(),
				    spin_weights_t.raw_buffer());

	    auto end1 = std::chrono::high_resolution_clock::now();
	    std::chrono::duration<double,std::milli> duration1 = end1 - start1;
	    duration_vector_1.push_back(duration1);
    }

    // Compare outputs.
    for (int iCprime=0; iCprime<Nc; iCprime++)
      for (int iSprime=0; iSprime<Ns; iSprime++)
         for (int jCprime=0; jCprime<Nc; jCprime++)
            for (int jSprime=0; jSprime<Ns; jSprime++)
               for (int kCprime=0; kCprime<Nc; kCprime++)
                  for (int kSprime=0; kSprime<Ns; kSprime++)
                     for (int x=0; x<Vsnk; x++)
                        for (int t=0; t<Lt; t++)
                           for (int n=0; n<Nsrc; n++)
                              if (std::abs(Blocal[n][iCprime][iSprime][jCprime][jSprime][kCprime][kSprime][x][t].real() -
				  Blocal_r(t, x, kSprime, kCprime, jSprime, jCprime, iSprime, iCprime, n)) >= 0.01)
			      {
				  std::cout << "Error: different computed values! Ref = " << Blocal[n][iCprime][iSprime][jCprime][jSprime][kCprime][kSprime][x][t].real() << " - Tiramisu = " << Blocal_r(t, x, kSprime, kCprime, jSprime, jCprime, iSprime, iCprime, n) << std::endl;
				  exit(1);
			      }

    std::cout << "\n\n\033[1;32mSuccess: computed values are equal!\033[0m\n\n" << std::endl;

    print_time("performance_CPU.csv", "dibaryon", {"Ref", "Tiramisu"}, {median(duration_vector_2), median(duration_vector_1)});

    return 0;
}