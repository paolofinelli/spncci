/****************************************************************
  explicit.cpp

  Tests of explicit SpNCCI basis construction in LSU3Shell basis.

  This code just tests normalization, but using clean refactored
  infrastructure.  Other deeper tests (of unit tensor matrix elements)
  were carried out in compute_unit_tensor_rmes.cpp.

  CAVEAT: Right now run parameters are hard coded in RunParameters
  constructor.

  Required data:

    Input files are generated by

      generate_lsu3shell_relative_operators.cpp

    which is invoked through scripting in

      compute_relative_tensors_lsu3shell_rmes.py

    Example: Z=3 N=3 twice_Nsigma0=22 Nmax=2 Nstep=2 N1v[=N1b]=1
   
    % python3 script/compute_relative_tensors_lsu3shell_rmes.py 3 3 22 2 2 1

    Only need .rme and .dat files.

    Not saved to repository since ~3.5 Mb...

       data/lsu3shell/lsu3shell_rme_6Li_Nmax02

    % ln -s ../../data/relative_observables/lsu3shell_rme_6Li_Nmax02/ lsu3shell_rme
                                  
  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  2/16/17 (mac): Created.  Based on compute_unit_tensor_rmes.cpp.
  2/18/17 (mac): Implement normalization test.
  2/20/17 (mac): Branch off spncci diagonalization code.
****************************************************************/

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sys/resource.h>

#include "cppformat/format.h"
#include "lgi/lgi_solver.h"
#include "mcutils/profiling.h"
#include "spncci/computation_control.h"
#include "spncci/explicit_construction.h"
#include "spncci/io_control.h"

// to vett as moved into computation_control 
#include "mcutils/eigen.h"
#include "spncci/branching_u3s.h"
#include "spncci/branching_u3lsj.h"
#include "u3shell/upcoupling.h"

////////////////////////////////////////////////////////////////
// WIP code
//
// to extract to spncci library when ready
////////////////////////////////////////////////////////////////

namespace spncci
{



}// end namespace

////////////////////////////////////////////////////////////////
// explicit construction checks
////////////////////////////////////////////////////////////////

void 
CheckOrthonormalityExplicit(
    const spncci::BabySpNCCISpace& baby_spncci_space,
    const basis::MatrixVector& spncci_expansions,
    double tolerance
  )
// Check orthonormality of SpNCCI basis vectors from explicit
// expansion in lsu3shell basis.
//
// Takes pairs of BabySpNCCI subspaces sharing the same U3SPN, and
// thus the same underlying lsu3shell subspace.
//
// Arguments:
//   ...
{

  for (int bra_subspace_index=0; bra_subspace_index<baby_spncci_space.size(); ++bra_subspace_index)
    for (int ket_subspace_index=bra_subspace_index; ket_subspace_index<baby_spncci_space.size(); ++ket_subspace_index)
      {
        // extract subspace info
        const spncci::BabySpNCCISubspace& bra_subspace = baby_spncci_space.GetSubspace(bra_subspace_index);
        const spncci::BabySpNCCISubspace& ket_subspace = baby_spncci_space.GetSubspace(ket_subspace_index);

        // short circuit if subspaces have different underlying lsu3shell subspaces
        if (not (bra_subspace.omegaSPN()==ket_subspace.omegaSPN()))
          continue;

        // calculate overlaps
        Eigen::MatrixXd overlap_matrix = spncci_expansions[bra_subspace_index].transpose()*spncci_expansions[ket_subspace_index];
        Eigen::MatrixXd overlap_matrix_minus_identity = overlap_matrix - Eigen::MatrixXd::Identity(overlap_matrix.rows(),overlap_matrix.cols());
        mcutils::ChopMatrix(overlap_matrix,tolerance);
        mcutils::ChopMatrix(overlap_matrix_minus_identity,tolerance);

        // check overlaps
        bool on_diagonal = (bra_subspace_index==ket_subspace_index);
        bool success = on_diagonal ? mcutils::IsZero(overlap_matrix_minus_identity)
          : mcutils::IsZero(overlap_matrix);
        
        std::cout
          << fmt::format(
              "  bra index {} labels {} ket index {} labels {}",
              bra_subspace_index,bra_subspace.LabelStr(),
              ket_subspace_index,ket_subspace.LabelStr()
            )
          << std::endl;
        std::cout << mcutils::FormatMatrix(overlap_matrix,"14.7e","  ") << std::endl;
        std::cout << fmt::format("  on_diagonal {}",on_diagonal)
                  << std::endl;
        std::cout << fmt::format("  {}",success ? "PASS" : "FAIL")
                  << std::endl;
        std::cout << mcutils::FormatMatrix(overlap_matrix,"8.5f","  ") << std::endl;
        std::cout << std::endl;
      }
}

////////////////////////////////////////////////////////////////
// run parameters
////////////////////////////////////////////////////////////////

struct RunParameters
// Structure to store input parameters for run.
//
// Data members:
//   A (int): Atomic mass.
//   ...
{

  // constructor
  RunParameters(); 

  // basis parameters
  int A;
  HalfInt Nsigma_0;
  int Nsigma0_ex_max;
  int N1v;
  int Nmax;

  // filenames
  std::string lsu3shell_rme_directory;
  std::string lsu3shell_basis_filename;
  std::string Brel_filename;
  std::string Arel_filename;
  std::string Nrel_filename;
  std::string relative_unit_tensor_filename_template;
};

RunParameters::RunParameters()
{
  // read from command line arguments
  //
  // TODO reorder filenames 
  // if (argc<8)
  //   {
  //     std::cout << "Syntax: A twice_Nsigma0 Nsigma0_ex_max N1B Nmax <basis filename> <Nrel filename> <Brel filename> <Arel filename>" 
  //               << std::endl;
  //     std::exit(1);
  //   }
  // int A = std::stoi(argv[1]); 
  // int twice_Nsigma0= std::stoi(argv[2]);
  // int Nsigma0_ex_max=std::stoi(argv[3]);
  // int N1v=std::stoi(argv[4]);
  // int Nmax = std::stoi(argv[5]);
  // std::string lsu3shell_basis_filename = argv[6];
  // std::string Nrel_filename = argv[7];
  // std::string Brel_filename = argv[8];
  // std::string Arel_filename = argv[9];
  // HalfInt Nsigma_0=HalfInt(twice_Nsigma0,2);

  // basis parameters
  A = 6;
  int twice_Nsigma0 = 22;
  Nsigma_0=HalfInt(twice_Nsigma0,2);

  Nsigma0_ex_max = 4;
  N1v = 1;
  Nmax = 4;

  lsu3shell_rme_directory = "lsu3shell_rme";
  lsu3shell_basis_filename = lsu3shell_rme_directory + "/" + "lsu3shell_basis.dat";
  Brel_filename = lsu3shell_rme_directory + "/" + fmt::format("Brel_06_Nmax{:02d}.rme",Nmax);
  Arel_filename = lsu3shell_rme_directory + "/" + fmt::format("Arel_06_Nmax{:02d}.rme",Nmax);
  Nrel_filename = lsu3shell_rme_directory + "/" + fmt::format("Nrel_06_Nmax{:02d}.rme",Nmax);
  relative_unit_tensor_filename_template = lsu3shell_rme_directory + "/" + "relative_unit_{:06d}.rme";

}


////////////////////////////////////////////////////////////////
// main body
////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{

  ////////////////////////////////////////////////////////////////
  // initialization
  ////////////////////////////////////////////////////////////////
  
  // SU(3) caching
  u3::U3CoefInit();
  u3::UCoefCache u_coef_cache;
  u3::PhiCoefCache phi_coef_cache;
  u3::g_u_cache_enabled = true;

  // numerical parameter for certain calculations
  double tolerance=1e-6;

  // run parameters
  RunParameters run_parameters;

  // Eigen OpenMP multithreading mode
  Eigen::initParallel();
  Eigen::setNbThreads(0);  // disable Eigen internal multithreading

  ////////////////////////////////////////////////////////////////
  // read lsu3shell basis
  ////////////////////////////////////////////////////////////////

  std::cout << "Read lsu3shell basis..." << std::endl;

  // read lsu3shell basis (regroup into U3SPN subspaces)
  lsu3shell::LSU3BasisTable lsu3shell_basis_table;
  lsu3shell::U3SPNBasisLSU3Labels lsu3shell_basis_provenance;
  u3shell::SpaceU3SPN lsu3shell_space;
  lsu3shell::ReadLSU3Basis(
      run_parameters.Nsigma_0,run_parameters.lsu3shell_basis_filename,
      lsu3shell_basis_table,lsu3shell_basis_provenance,lsu3shell_space
    );

  ////////////////////////////////////////////////////////////////
  // solve for LGIs
  ////////////////////////////////////////////////////////////////

  std::cout << "Solve for LGIs..." << std::endl;

  // timing start
  Timer timer_lgi;
  timer_lgi.Start();


    // diagnostics
    // std::cout << "Arel operator..." << std::endl;
    // std::cout << "Arel sectors" << std::endl;
    // std::cout << Arel_sectors.DebugStr();
    // std::cout << "Arel matrices" << std::endl;
    // for (int sector_index=0; sector_index<Arel_sectors.size(); ++sector_index)
    //   {
    //     std::cout << fmt::format("  sector {}",sector_index) << std::endl;
    //     std::cout << mcutils::FormatMatrix(Arel_matrices[sector_index],"8.5f","  ") << std::endl;
    //   }


  u3shell::SectorsU3SPN Brel_sectors, Arel_sectors, Nrel_sectors;
  basis::MatrixVector Brel_matrices, Arel_matrices, Nrel_matrices;
  spncci::ReadLSU3ShellSymplecticOperatorRMEs(
      lsu3shell_basis_table,lsu3shell_space, 
      run_parameters.Brel_filename,Brel_sectors,Brel_matrices,
      run_parameters.Arel_filename,Arel_sectors,Arel_matrices,
      run_parameters.Nrel_filename,Nrel_sectors,Nrel_matrices
    );

  const u3shell::SectorsU3SPN& Ncm_sectors = Nrel_sectors;
  basis::MatrixVector Ncm_matrices;
  lsu3shell::GenerateLSU3ShellNcmRMEs(
      lsu3shell_space,Nrel_sectors,Nrel_matrices,
      run_parameters.A,
      Ncm_matrices
    );

  //Removed keep_empty_subspaces flag set to false.  May need to make changes here to accomodate probable empty sectors. 
  lgi::MultiplicityTaggedLGIVector lgi_families;
  basis::MatrixVector lgi_expansions;
  lgi::GenerateLGIExpansion(
      lsu3shell_space, 
      Brel_sectors,Brel_matrices,Ncm_sectors,Ncm_matrices,
      run_parameters.Nsigma_0,
      lgi_families,lgi_expansions
    );

  // diagnostics
  std::cout << fmt::format("  LGI families {}",lgi_families.size()) << std::endl;
  lgi::WriteLGILabels(lgi_families,std::cout);

  // timing stop
  timer_lgi.Stop();
  std::cout << fmt::format("(Task time: {})",timer_lgi.ElapsedTime()) << std::endl;

  ////////////////////////////////////////////////////////////////
  // set up SpNCCI space
  ////////////////////////////////////////////////////////////////

  std::cout << "Set up SpNCCI space..." << std::endl;

  // build SpNCCI irrep branchings
  spncci::SpNCCISpace spncci_space;
  spncci::SigmaIrrepMap sigma_irrep_map;  // persistent container to store branchings
  spncci::NmaxTruncator truncator(run_parameters.Nsigma_0,run_parameters.Nmax);
  spncci::GenerateSpNCCISpace(lgi_families,truncator,spncci_space,sigma_irrep_map);

  // put SpNCCI space into standard linearized container
  spncci::BabySpNCCISpace baby_spncci_space(spncci_space);

  // diagnostics
  std::cout << fmt::format("  Irrep families {}",spncci_space.size()) << std::endl;
  std::cout << fmt::format("  TotalU3Subspaces {}",spncci::TotalU3Subspaces(spncci_space)) << std::endl;
  std::cout << fmt::format("  TotalDimensionU3 {}",spncci::TotalDimensionU3S(spncci_space)) << std::endl;


  ////////////////////////////////////////////////////////////////
  // precompute K matrices
  ////////////////////////////////////////////////////////////////

  std::cout << "Precompute K matrices..." << std::endl;

  // timing start
  Timer timer_k_matrices;
  timer_k_matrices.Start();

  // traverse distinct sigma values in SpNCCI space, generating K
  // matrices for each
  spncci::KMatrixCache k_matrix_cache;
  bool intrinsic = true;
  spncci::PrecomputeKMatrices(sigma_irrep_map,k_matrix_cache,intrinsic);

  // diagnostics
  for (const auto& sigma_irrep_pair : sigma_irrep_map)
    {
      // extract sigma and irrep contents
      const u3::U3& sigma = sigma_irrep_pair.first;
      const sp3r::Sp3RSpace& sp_irrep = sigma_irrep_pair.second;
      for (auto& omega_matrix_pair : k_matrix_cache[sigma])
        {
          const u3::U3& omega = omega_matrix_pair.first;
          const Eigen::MatrixXd& k_matrix = omega_matrix_pair.second;
          std::cout << fmt::format("  sigma {} omega {}",sigma.Str(),omega.Str()) << std::endl;
          std::cout << k_matrix << std::endl;
        }
    }

  // timing stop
  timer_k_matrices.Stop();
  std::cout << fmt::format("(Task time: {})",timer_k_matrices.ElapsedTime()) << std::endl;

  ////////////////////////////////////////////////////////////////
  // do explicit subspace constructions
  ////////////////////////////////////////////////////////////////

  std::cout << "Explicitly construct SpNCCI basis states using Arel..." << std::endl;
  basis::MatrixVector spncci_expansions;
  
  spncci::ConstructSpNCCIBasisExplicit(
      lsu3shell_space,spncci_space,lgi_expansions,
      baby_spncci_space,k_matrix_cache,
      Arel_sectors,Arel_matrices,spncci_expansions
    );

  std::cout << "Check orthonormality for all SpNCCI subspaces sharing same underlying lsu3shell subspace..." << std::endl;
  CheckOrthonormalityExplicit(baby_spncci_space,spncci_expansions,tolerance);

}
