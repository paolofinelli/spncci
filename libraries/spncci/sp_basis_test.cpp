/****************************************************************
  sp_basis_test.cpp

  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  3/10/16 (aem,mac): Created.

****************************************************************/

#include "spncci/sp_basis.h"


int main(int argc, char **argv)
{

  ////////////////////////////////////////////////////////////////
  // LGI list
  ////////////////////////////////////////////////////////////////

  HalfInt Nsigma_0 = HalfInt(11,1);
  std::string filename = "libraries/spncci/lgi-3-3-2-fql-mini.dat";
  
  spncci::LGIVectorType lgi_vector;
  spncci::GenerateLGIVector(lgi_vector,filename,Nsigma_0);

  std::cout << "LGI vector" << std::endl;
  for (int i=0; i<lgi_vector.size(); ++i)
    std::cout << i << " " << lgi_vector[i].Str() << std::endl;
  std::cout << "********************************" << std::endl;

  ////////////////////////////////////////////////////////////////
  // build subspaces
  ////////////////////////////////////////////////////////////////

  int Nmax = 4;

  spncci::SigmaIrrepMapType sigma_irrep_map;
  spncci::NmaxTruncator truncator(Nsigma_0,Nmax);
  spncci::GenerateSp3RIrreps(lgi_vector,sigma_irrep_map,truncator);

  // examine irreps
  std::cout << "irreps (by sigma)" << std::endl;
  for (auto it = sigma_irrep_map.begin(); it != sigma_irrep_map.end(); ++it)
    {
      u3::U3 sigma = it->first;
      const sp3r::Sp3RSpace& irrep = it->second;

      std::cout << irrep.DebugString();
      std::cout << std::endl;
    }
  std::cout << "********************************" << std::endl;





} //main