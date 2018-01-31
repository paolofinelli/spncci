/****************************************************************
  spncci.cpp

  Tests of explicit SpNCCI basis construction in LSU3Shell basis.

  This code just tests normalization, but using clean refactored
  infrastructure.  Other deeper tests (of unit tensor matrix elements)
  were carried out in compute_unit_tensor_rmes.cpp.

  Required data:

  * Relative operator lsu3shell rme input files are generated by

      generate_lsu3shell_relative_operators.cpp

    which is invoked through scripting in

      compute_relative_tensors_lsu3shell_rmes.py

    Example: Z=3 N=3 twice_Nsigma0=22 Nmax=2 Nstep=2 N1v[=N1b]=1
   
    % python3 script/compute_relative_tensors_lsu3shell_rmes.py 3 3 22 2 2 1

    Only need .rme and .dat files.

    Not saved to repository since ~3.5 Mb...

       data/lsu3shell/lsu3shell_rme_6Li_Nmax02
    
       * Relative Hamiltonian (and observable) upcoupled rme files are
    generated by

      generate_relative_u3st_operators

    which is invoked manually for now as

      generate_relative_u3st_operators A Nmax N1v basename

   Example:

       ../operators/generate_relative_u3st_operators 6 2 1 hamiltonian

       with hamiltonian.load containing

       20    // hw
       Tintr 1.0    // coef
       INT 1.0 4 0 0 0 relative_observables/JISP16_Nmax20_hw20.0_rel.dat // coef Jmax J0 T0 g0 interaction_filename

       ../operators/generate_relative_u3st_operators 6 2 1 Nintr

       with Nintr.load containing

       20    // hw
       Nintr 1.0    // coef

       ../operators/generate_relative_u3st_operators 6 2 1 r2intr

       with r2intr.load containing

       20    // hw
       r2intr 1.0    // coef

   % ln -s ../../data/relative_observables/

         
     

  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  2/20/17 (mac): Created (starting from explicit.cpp).
  4/9/17 (aem): Incorporated baby spncci hypersectors
  6/5/17 (mac): Read relative rather than intrinsic symplectic operators.
  6/16/17 (aem) : offload to computation and io control
  10/4/17 (aem) : Fixed basis construction and recurrence for A<6
  1/16/18 (aem) : Offloaded explicit construction and recurrence
    checks to explicit_construction.h
  1/30/18 (aem): Overhalled seed generation and recurrence
****************************************************************/

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sys/resource.h>
#include <omp.h>  

#include "SymEigsSolver.h"  // from spectra
#include "cppformat/format.h"

#include "spncci/recurrence.h"
#include "lgi/lgi_unit_tensors.h"
#include "mcutils/profiling.h"
#include "spncci/branching.h"
#include "spncci/branching_u3s.h"
#include "spncci/branching_u3lsj.h"
#include "spncci/decomposition.h"
#include "spncci/eigenproblem.h"
#include "spncci/explicit_construction.h"
#include "spncci/io_control.h"
#include "spncci/parameters.h"

#include "spncci/results_output.h"

////////////////////////////////////////////////////////////////
// WIP code
//
// to extract to spncci library when ready
//////////////////////////////////////////////////////////////// 
namespace spncci
{}// end namespace

////////////////////////////////////////////////////////////////
// main body
////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  std::cout<<"entering spncci"<<std::endl;
  ////////////////////////////////////////////////////////////////
  // initialization
  ////////////////////////////////////////////////////////////////
  bool check_unit_tensors=false;

  // SU(3) caching
  u3::U3CoefInit();
  u3::UCoefCache u_coef_cache;
  u3::PhiCoefCache phi_coef_cache;
  u3::g_u_cache_enabled = true;

  // parameters for certain calculations
  spncci::g_zero_tolerance = 1e-6;
  spncci::g_suppress_zero_sectors = true;


  // Default binary mode, unless environment variable SPNCCI_RME_MODE
  // set to "text".
  //
  // This is meant as an ad hoc interface until text mode i/o is abolished.
  lsu3shell::g_rme_binary_format = true;
  char* spncci_rme_mode_cstr = std::getenv("SPNCCI_RME_MODE");
  if (spncci_rme_mode_cstr!=NULL)
    {
      const std::string spncci_rme_mode = std::getenv("SPNCCI_RME_MODE");
      if (spncci_rme_mode=="text")
        lsu3shell::g_rme_binary_format = false;
    }

  // run parameters
  std::cout << "Reading control file..." << std::endl;
  spncci::RunParameters run_parameters;

  // Eigen OpenMP multithreading mode
  Eigen::initParallel();
  // Eigen::setNbThreads(0);

  // open output files
  std::ofstream results_stream("spncci.res");

  // results output: code information
  spncci::StartNewSection(results_stream,"CODE");
  spncci::WriteCodeInformation(results_stream,run_parameters);

  // results output: run parameters
  spncci::StartNewSection(results_stream,"PARAMETERS");
  spncci::WriteRunParameters(results_stream,run_parameters);

  std::cout<<"Nmax="<<run_parameters.Nmax<<std::endl;

  /////////////////////////////////////////////////////////////////////////////////////
  // set up SpNCCI space
  ////////////////////////////////////////////////////////////////
  // Get LGI families
  std::string lgi_filename="lgi_families.dat";
  lgi::MultiplicityTaggedLGIVector lgi_families;
  lgi::ReadLGISet(lgi_filename, run_parameters.Nsigma0,lgi_families);

  std::cout << "Set up SpNCCI space..." << std::endl;

  // build SpNCCI irrep branchings
  spncci::SpNCCISpace spncci_space;
  spncci::SigmaIrrepMap sigma_irrep_map;  // persistent container to store branchings
  spncci::NmaxTruncator truncator(run_parameters.Nsigma0,run_parameters.Nmax);
  bool restrict_sp3r_to_u3_branching=false;
  if(run_parameters.A<6)
    restrict_sp3r_to_u3_branching=true;

  // spncci::GenerateSpNCCISpace(lgi_families_truncated,truncator,spncci_space,sigma_irrep_map,restrict_sp3r_to_u3_branching);
  spncci::GenerateSpNCCISpace(lgi_families,truncator,spncci_space,sigma_irrep_map,restrict_sp3r_to_u3_branching);

  for(int i=0; i<spncci_space.size(); ++i)
    std::cout<<i<<"  "<<spncci_space[i].Str()<<spncci_space[i].gamma_max()<<std::endl;

  // diagnostics
  std::cout << fmt::format("  Irrep families {}",spncci_space.size()) << std::endl;
  std::cout << fmt::format("  TotalU3Subspaces {}",spncci::TotalU3Subspaces(spncci_space)) << std::endl;
  std::cout << fmt::format("  TotalDimensionU3S {}",spncci::TotalDimensionU3S(spncci_space)) << std::endl;

  // build baby spncci space 
  spncci::BabySpNCCISpace baby_spncci_space(spncci_space);

  // build SpU3S gathered space
  std::cout << "Build SpU3S space..." << std::endl;
  spncci::SpaceSpU3S spu3s_space(baby_spncci_space);
  std::cout
    << fmt::format("  subspaces {} dimension {} full_dimension {}",
                   spu3s_space.size(),spu3s_space.Dimension(),spu3s_space.FullDimension()
      )
    << std::endl;
  std::cout
    << fmt::format("  compare... TotalDimensionU3S {}",
                   TotalDimensionU3S(spncci_space)
      )
    << std::endl;
  // std::cout << spu3s_space.DebugStr(true);

  // build SpLS branched space
  std::cout << "Build SpLS space..." << std::endl;
  spncci::SpaceSpLS spls_space(spu3s_space);
  std::cout
    << fmt::format("  subspaces {} dimension {} full_dimension {}",
                   spls_space.size(),spls_space.Dimension(),spls_space.FullDimension()
      )
    << std::endl;
  std::cout
    << fmt::format("  compare... TotalDimensionU3LS {}",TotalDimensionU3LS(spncci_space))
    << std::endl;
  // std::cout << splss_space.DebugStr(true);

  // build SpJ branched space
  std::cout << "Build SpJ space..." << std::endl;
  spncci::SpaceSpJ spj_space(run_parameters.J_values,spls_space);
  std::cout
    << spj_space.DebugStr(false)
    << std::endl;
  std::cout
    << fmt::format("  subspaces {}",spj_space.size())
    << std::endl;


  // results output: basis information
  spncci::StartNewSection(results_stream,"BASIS");
  spncci::WriteBasisStatistics(results_stream,spncci_space,baby_spncci_space,spu3s_space,spls_space,spj_space);
  spncci::WriteSpU3SSubspaceListing(results_stream,baby_spncci_space,run_parameters.Nsigma0);
  spncci::WriteBabySpNCCISubspaceListing(results_stream,baby_spncci_space,run_parameters.Nsigma0);

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // Enumerate unit tensor space 
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  int J0_for_unit_tensors = -1;  // all J0
  int T0_for_unit_tensors = -1;  // all T0
  const bool restrict_positive_N0 = false;  // don't restrict to N0 positive

  // get full set of possible unit tensor labels up to Nmax, N1v truncation
  std::vector<u3shell::RelativeUnitTensorLabelsU3ST> unit_tensor_labels;  
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(
      run_parameters.Nmax, run_parameters.N1v,
      unit_tensor_labels,J0_for_unit_tensors,T0_for_unit_tensors,
      restrict_positive_N0
    );

  // for(auto tensor :unit_tensor_labels)
  //   std::cout<<tensor.Str()<<std::endl;

  // generate unit tensor subspaces 
  u3shell::RelativeUnitTensorSpaceU3S 
    unit_tensor_space(run_parameters.Nmax,run_parameters.N1v,unit_tensor_labels);

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //  Read in observables  
  ///////////////////////////////////////////////////////////////////////////////////////////////////        

  std::cout << "Reading observables..." << std::endl;

  // Initialize containers for rmes and their symmetries 
  // Stored by hw, then by observable
  std::vector<std::vector<u3shell::RelativeRMEsU3SSubspaces>> observables_relative_rmes(run_parameters.hw_values.size());
  std::vector<std::vector<u3shell::IndexedOperatorLabelsU3S>> observable_symmetries_u3s(run_parameters.num_observables); 

  spncci::ReadRelativeObservables(
      run_parameters.Nmax, run_parameters.N1v, run_parameters.hw_values,
      run_parameters.observable_directory,run_parameters.observable_filenames, 
      unit_tensor_space, observables_relative_rmes, observable_symmetries_u3s
    );

  ////////////////////////////////////////////////////////////////
  // Enumerate U3S sectors for observables 
  ////////////////////////////////////////////////////////////////
  std::cout << "Enumerating u3s sectors..." << std::endl;

  // enumerate u3S space from baby spncci for each observable 
  spncci::SpaceU3S space_u3s(baby_spncci_space);

  // vector of sectors for each observable
  std::vector<std::vector<spncci::SectorLabelsU3S>> observables_sectors_u3s;//(run_parameters.num_observables);
  
  // vector of blocks for u3 sectors for each hbar omega,for each observable
  std::vector<std::vector<spncci::OperatorBlocks>> observables_blocks_u3s;//(run_parameters.hw_values.size());

  spncci::InitializeU3SSectors(
      space_u3s,run_parameters.num_observables, 
      observable_symmetries_u3s,observables_sectors_u3s
    );

  spncci::WriteU3SSectorInformation(
      results_stream, space_u3s,run_parameters.num_observables, 
      observables_sectors_u3s
      );

  ////////////////////////////////////////////////////////////////
  // terminate counting only run
  ////////////////////////////////////////////////////////////////
  // We now have to do all termination manually.  But, when the
  // control code is properly refactored, we can just have a single
  // termination, and the rest of the run can be in an "if
  // (!count_only)"...

  if (run_parameters.count_only)
    {

      // termination
      results_stream.close();

      std::cout << "End of counting-only run" << std::endl;
      std::exit(EXIT_SUCCESS);
    }


  ////////////////////////////////////////////////////////////////
  // Allocate U3S sectors for observables 
  ////////////////////////////////////////////////////////////////
  std::cout << "Allocating u3s blocks..." << std::endl;

  spncci::InitializeU3SBlocks(
      space_u3s,
      run_parameters.num_observables, 
      run_parameters.hw_values,
      observables_sectors_u3s,
      observables_blocks_u3s
    );

  ////////////////////////////////////////////////////////////////
  // precompute K matrices
  ////////////////////////////////////////////////////////////////
  std::cout << "Precompute K matrices..." << std::endl;

  // timing start
  mcutils::SteadyTimer timer_k_matrices;
  timer_k_matrices.Start();

  // traverse distinct sigma values in SpNCCI space, generating K
  // matrices for each
  // spncci::KMatrixCache k_matrix_cache;
  spncci::KMatrixCache k_matrix_cache, kinv_matrix_cache;
  spncci::PrecomputeKMatrices(sigma_irrep_map,k_matrix_cache,kinv_matrix_cache,restrict_sp3r_to_u3_branching);

  // timing stop
  timer_k_matrices.Stop();
  std::cout << fmt::format("(Task time: {})",timer_k_matrices.ElapsedTime()) << std::endl;

  std::cout<<"Kmatrices "<<std::endl;
  for(auto it=k_matrix_cache.begin(); it!=k_matrix_cache.end(); ++it)
    {
      std::cout<<"sigma "<<it->first.Str()<<std::endl;
      for(auto it2=it->second.begin();  it2!=it->second.end(); ++it2)
      {
        std::cout<<"  omega"<<it2->first.Str()<<std::endl;
        auto matrix=it2->second;
        std::cout<<matrix<<std::endl;
        // std::cout<<matrix.inverse()<<std::endl;
      }
    }

  ///////////////////////////////////////////////////////////////////////////////////////////////
  std::cout<<"setting up lgi unit tensor blocks"<<std::endl;
  
  // map of {lgi pair : list of hypersector indices organized by Nsum}
  // Read in lsu3shell unit tensors
  // transform block for each unit tensor to spncci
  // identify unit tensors with non-zero rmes's between each lgi pair 
  // Generate unit tensor labels for recurrence for each lgi pair
  // put seed blocks into hypersector blocks for each lgi pair 

  // Get list of unit tensor labels between lgi's 
  std::vector<u3shell::RelativeUnitTensorLabelsU3ST> lgi_unit_tensor_labels;
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(
      run_parameters.Nsigmamax, run_parameters.N1v,
      lgi_unit_tensor_labels,J0_for_unit_tensors,T0_for_unit_tensors,
      restrict_positive_N0
    );

  // explicit construction of spncci basis
  basis::MatrixVector spncci_expansions;
  if(check_unit_tensors)
    spncci::ExplicitBasisConstruction(
      run_parameters,spncci_space,baby_spncci_space,
      k_matrix_cache, kinv_matrix_cache,
      restrict_sp3r_to_u3_branching,spncci_expansions
      );

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  std::cout<<"Starting recurrence and contraction"<<std::endl;
 
  for(int irrep_family_index_bra=0; irrep_family_index_bra<lgi_families.size(); ++irrep_family_index_bra)
    for(int irrep_family_index_ket=0; irrep_family_index_ket<lgi_families.size(); ++irrep_family_index_ket)
      {

      // Reads in unit tensor labels for seed sectors stores them in a vector.  The rho0 of the sector defined by
      // <bra|unit_tensor|ket>_rho0 are stored in a corresponding vector rho0_values; 
        std::vector<u3shell::RelativeUnitTensorLabelsU3ST> lgi_unit_tensors;
        std::vector<int> rho0_values;
        std::string lgi_unit_tensor_filename
          =fmt::format("seeds/operators_{:06d}_{:06d}.dat",irrep_family_index_bra,irrep_family_index_ket);
        bool files_found=lgi::ReadUnitTensorLabels(lgi_unit_tensor_filename,lgi_unit_tensors,rho0_values);

      // Reads in unit tensor seed blocks and stores them in a vector of blocks. Order
      // corresponds to order of (unit_tensor,rho0) pairs in corresponding operator file. 
        basis::MatrixVector unit_tensor_seed_blocks;
        std::string seed_filename
          =fmt::format("seeds/seeds_{:06d}_{:06d}.rmes",irrep_family_index_bra,irrep_family_index_ket);
        files_found&=lgi::ReadBlocks(seed_filename, lgi_unit_tensors.size(), unit_tensor_seed_blocks);

        if(not files_found)
          {
            std::cout<<"seeds and operators for "<<irrep_family_index_bra<<"  "<<irrep_family_index_ket<<" not found"<<std::endl;
            continue;
          }
 
        // Identify unit tensor subspaces for recurrence
        std::map<spncci::NnPair,std::set<int>> unit_tensor_subspace_subsets;
        spncci::GenerateRecurrenceUnitTensors(
          run_parameters.Nmax, run_parameters.N1v,
          lgi_unit_tensors,unit_tensor_space,
          unit_tensor_subspace_subsets
        );

        std::cout<<"generate Nn0 hypersectors"<<std::endl;
        // Generate Nn=0 hypersectors to be computed by conjugation
        bool Nn0_conjugate_hypersectors=true;
        std::vector<std::vector<int>> unit_tensor_hypersector_subsets_Nn0;
        spncci::BabySpNCCIHypersectors baby_spncci_hypersectors_Nn0(
          run_parameters.Nmax, baby_spncci_space, unit_tensor_space,
          unit_tensor_subspace_subsets, unit_tensor_hypersector_subsets_Nn0,
          irrep_family_index_ket, irrep_family_index_bra,
          Nn0_conjugate_hypersectors
        );

        // {
        //   for(int i=0; i<unit_tensor_hyperblocks.size(); ++i)
        //     for(int j=0; j<unit_tensor_hyperblocks[i].size(); ++j)
        //       {
        //         auto& hypersector=baby_spncci_hypersectors.GetHypersector(i);
        //         int bra, ket, tensor, rho0;
        //         std::tie(bra,ket,tensor,rho0)=hypersector.Key();
        //         auto& bra_subspace=baby_spncci_space.GetSubspace(bra);
        //         auto& ket_subspace=baby_spncci_space.GetSubspace(ket);
        //         auto& tensor_subspace=unit_tensor_space.GetSubspace(tensor);
        //         int Sp,Tp,S,T,T0;
        //         std::tie(T0,Sp,Tp,S,T)=tensor_subspace.GetStateLabels(j);
        //         // const Eigen::MatrixXd matrix1=unit_tensor_hyperblocks[i][j];
        //         std::cout<<bra_subspace.LabelStr()<<"  "<<ket_subspace.LabelStr()<<"  "<<tensor_subspace.LabelStr()<<"  "
        //                      << rho0<<std::endl;
        //         std::cout<<"   "<<T0<<"  "<<Sp<<"  "<<Tp<<"  "<<S<<"  "<<T<<std::endl;
        //       }

        // }

        // Generate all other hypersectors for Nnp>=Nn
        std::cout<<" generate hypersectors"<<std::endl;
        Nn0_conjugate_hypersectors=false;
        std::vector<std::vector<int>> unit_tensor_hypersector_subsets;
        
        spncci::BabySpNCCIHypersectors baby_spncci_hypersectors(
          run_parameters.Nmax,
          baby_spncci_space, unit_tensor_space,
          unit_tensor_subspace_subsets, unit_tensor_hypersector_subsets,
          irrep_family_index_bra,irrep_family_index_ket,
          Nn0_conjugate_hypersectors
        );

        // zero initialize hypersectors 
        basis::OperatorHyperblocks<double> unit_tensor_hyperblocks_Nn0;
        basis::SetHyperoperatorToZero(baby_spncci_hypersectors_Nn0,unit_tensor_hyperblocks_Nn0);

        basis::OperatorHyperblocks<double> unit_tensor_hyperblocks;
        basis::SetHyperoperatorToZero(baby_spncci_hypersectors,unit_tensor_hyperblocks);

        // Initialize hypersectors with seeds
        // Add lgi unit tensor blocks to hyperblocks for both Nn=0 and all remaining sectors 
        std::cout<<" populate hypersectors with seeds"<<std::endl;
        spncci::PopulateHypersectorsWithSeeds(
          irrep_family_index_bra, irrep_family_index_ket,lgi_families,
          baby_spncci_space,unit_tensor_space,
          baby_spncci_hypersectors_Nn0,baby_spncci_hypersectors,
          lgi_unit_tensors,rho0_values,unit_tensor_seed_blocks,
          unit_tensor_hyperblocks_Nn0,unit_tensor_hyperblocks
        );

        // Compute Nn=0 blocks
        spncci::ComputeUnitTensorHyperblocks(
          run_parameters.Nmax,run_parameters.N1v,u_coef_cache,phi_coef_cache,
          k_matrix_cache,kinv_matrix_cache,spncci_space,baby_spncci_space,
          unit_tensor_space,baby_spncci_hypersectors_Nn0,
          unit_tensor_hypersector_subsets_Nn0,unit_tensor_hyperblocks_Nn0
        );

        spncci::AddNn0BlocksToHyperblocks(
          baby_spncci_space,unit_tensor_space,
          baby_spncci_hypersectors_Nn0,baby_spncci_hypersectors,
          unit_tensor_hyperblocks_Nn0,unit_tensor_hyperblocks
        );
       
        // Compute unit tensor hyperblocks
        spncci::ComputeUnitTensorHyperblocks(
          run_parameters.Nmax,run_parameters.N1v,u_coef_cache,phi_coef_cache,
          k_matrix_cache,kinv_matrix_cache,spncci_space,baby_spncci_space,
          unit_tensor_space,baby_spncci_hypersectors,
          unit_tensor_hypersector_subsets,unit_tensor_hyperblocks
        );

        // std::cout<<"hypersectors"<<std::endl;
        // spncci::PrintHypersectors(
        //   baby_spncci_space,unit_tensor_space, 
        //   baby_spncci_hypersectors,unit_tensor_hyperblocks
        //   );


        check_unit_tensors=false;
        if(check_unit_tensors)
          CheckHyperBlocks(
            irrep_family_index_bra,irrep_family_index_ket,
            run_parameters,spncci_space,unit_tensor_space,
            lgi_unit_tensor_labels,baby_spncci_space,spncci_expansions,
            baby_spncci_hypersectors,unit_tensor_hyperblocks
          );

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Contract and regroup
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // std::cout<<"contracting over observables "<<std::endl;
        for(int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
          for(int hw_index=0; hw_index<run_parameters.hw_values.size(); ++hw_index)
            {

              // std::cout<<"observable "<<observable_index<<" hw "<<run_parameters.hw_values[hw_index]<<std::endl;
              const u3shell::RelativeRMEsU3SSubspaces& relative_observable=observables_relative_rmes[hw_index][observable_index];
              const std::vector<spncci::SectorLabelsU3S>& sectors_u3s=observables_sectors_u3s[observable_index];
              spncci::OperatorBlocks& blocks_u3s=observables_blocks_u3s[hw_index][observable_index];
      

              // const std::vector<spncci::SectorLabelsSpU3S>& sectors_spu3s=observables_sectors_spu3s[observable_index];
              // basis::OperatorBlocks<double>& blocks_spu3s=observables_blocks_spu3s[hw_index][observable_index];

              ContractAndRegroupU3S(
                  unit_tensor_space,baby_spncci_space,space_u3s,relative_observable,
                  baby_spncci_hypersectors,unit_tensor_hyperblocks,sectors_u3s,blocks_u3s
                );


              // spncci::ContractAndRegroupSpU3S(
              //     unit_tensor_space, baby_spncci_space,
              //     spu3s_space,relative_observable,
              //     baby_spncci_hypersectors,unit_tensor_hyperblocks,
              //     sectors_spu3s,blocks_spu3s);

              // for(int i=0; i<blocks_u3s.size(); ++i)
              //   if(not mcutils::IsZero(blocks_u3s[i]-blocks_spu3s[i],1e-6))
              //   {
              //     std::cout<<"blocks "<<i<<" do not match"<<std::endl
              //     <<blocks_u3s[i]<<std::endl<<std::endl<<blocks_spu3s[i]<<std::endl<<std::endl; 
              //     assert(mcutils::IsZero(blocks_u3s[i]-blocks_spu3s[i]));
              //   }

            }
      }// end lgi_pair

  // spncci::PrintU3SSector(
  //   run_parameters.hw_values,
  //   observables_sectors_u3s,observables_blocks_u3s,  
  //   space_u3s, run_parameters.num_observables
  // );

  // timer_recurrence.Stop();
  
  // std::cout << fmt::format("(Task time: {})",timer_recurrence.ElapsedTime()) << std::endl;

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // At this point observable rmes should be fully computed and unit tensor cache, Ucoef cache and Kmatrix cache deleted 
  // Delete Kmatrix
  // Delete Unit tensor Cache
  // Delete Ucoef Cache 
  //
  // Note: The clean way to do that is to encapsulate the unit tensor
  // setup phase of the code in a subroutine...  That breaks the
  // problem up into clean, structured subunits.  Anything that should
  // persist is clearly marked by the fact that it is passed in as a
  // reference.  And anything that is no longer needed automatically
  // gets destroyed as it goes out of scope.
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // loop over hw values, branch matrix sectors and compute eigenvalues
  
  ////////////////////////////////////////////////////////////////
  // set up indexing for branching
  ////////////////////////////////////////////////////////////////

  std::cout << "Set up basis indexing for branching..." << std::endl;

  // W coefficient cache -- needed for observable branching
  u3::WCoefCache w_cache;

  // determine J sectors for each observable
  std::vector<spncci::SectorsSpJ> observable_sectors;
  observable_sectors.resize(run_parameters.num_observables);
  for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
    {
      const int J0=run_parameters.observable_J0_values[observable_index];
      observable_sectors[observable_index] = spncci::SectorsSpJ(spj_space,J0);
    }

  // set up basis indexing for branching
  std::map<HalfInt,spncci::SpaceLS> spaces_lsj;  // map: J -> space
  std::map<HalfInt,spncci::SpaceSpLS> spaces_splsj;
  for (const HalfInt J : run_parameters.J_values)
    {

      std::cout << fmt::format("Build LS space for J={}...",J.Str()) << std::endl;
      spaces_lsj[J] = spncci::SpaceLS(space_u3s,J);
      std::cout
        << fmt::format(
            "  subspaces {} dimension {}",
            J.Str(),
            spaces_lsj[J].size(),spaces_lsj[J].Dimension()
          ) << std::endl;

      // comparison tests with new basis branching construction
      std::cout << fmt::format("Build SpLS space for J={}...",J.Str()) << std::endl;
      spaces_splsj[J]=spncci::SpaceSpLS(spu3s_space,J);
      const auto& spls_space=spaces_splsj.at(J);
      // spncci::SpaceSpLS spls_space(spu3s_space,J);
      std::cout
        << fmt::format("  subspaces {} dimension {} full_dimension {}",
                       spls_space.size(),spls_space.Dimension(),spls_space.FullDimension()
          )
        << std::endl;
      std::cout
        << fmt::format("  compare... TotalDimensionU3LSJConstrained {}",TotalDimensionU3LSJConstrained(spncci_space,J))
        << std::endl;
    }


  ////////////////////////////////////////////////////////////////
  // calculation mesh master loop
  ////////////////////////////////////////////////////////////////


  std::cout << "Calculation mesh master loop..." << std::endl;

  // timing start
  mcutils::SteadyTimer timer_mesh;
  timer_mesh.Start();

  // for each hw value, solve eigen problem and get expectation values 
  for(int hw_index=0; hw_index<run_parameters.hw_values.size(); ++hw_index)
    {

      // retrieve mesh parameters
      double hw = run_parameters.hw_values[hw_index];
            
      // results output: log start of individual mesh calculation
      spncci::StartNewSection(results_stream,"RESULTS");
      spncci::WriteCalculationParameters(results_stream,hw);

      ////////////////////////////////////////////////////////////////
      // eigenproblem
      ////////////////////////////////////////////////////////////////

      std::cout<<"Solve eigenproblem..."<<std::endl;
      mcutils::SteadyTimer timer_eigenproblem;
      timer_eigenproblem.Start();


      std::vector<spncci::Vector> eigenvalues;  // eigenvalues by J subspace
      std::vector<spncci::Matrix> eigenvectors;  // eigenvectors by J subspace
      eigenvalues.resize(spj_space.size());
      eigenvectors.resize(spj_space.size());
      for (int subspace_index=0; subspace_index<spj_space.size(); ++subspace_index)
        {
          HalfInt J = spj_space.GetSubspace(subspace_index).J();
          std::cout
            << fmt::format("J = {}",J)
            << std::endl;
          
          // branch Hamiltonian
          spncci::OperatorBlock hamiltonian_matrix;
          const int observable_index = 0;  // for Hamiltonian
          const int sector_index = subspace_index;  // for Hamiltonian (scalar)
          const int J0 = run_parameters.observable_J0_values[observable_index];
          assert(J0==0);
          spncci::ConstructBranchedBlock(
              w_cache,
              space_u3s,
              observables_sectors_u3s[observable_index],
              observables_blocks_u3s[hw_index][observable_index],
              spaces_lsj,
              J0,
              observable_sectors[observable_index].GetSector(sector_index),
              hamiltonian_matrix
            );

          std::cout
            << fmt::format("J = {}: {}x{}",J,hamiltonian_matrix.rows(),hamiltonian_matrix.cols())
            << std::endl;
          // std::cout<<mcutils::FormatMatrix(hamiltonian_matrix, ".1f")<<std::endl<<std::endl;

          // solve eigenproblem
          spncci::Vector& eigenvalues_J = eigenvalues[subspace_index];
          spncci::Matrix& eigenvectors_J = eigenvectors[subspace_index];
          // std::cout<<hamiltonian_matrix<<std::endl;
          std::cout << fmt::format("  Diagonalizing: J={}",J) << std::endl;
          spncci::SolveHamiltonian(
              hamiltonian_matrix,
              run_parameters.num_eigenvalues,
              run_parameters.eigensolver_num_convergence,  // whatever exactly this is...
              run_parameters.eigensolver_max_iterations,
              run_parameters.eigensolver_tolerance,
              eigenvalues_J,eigenvectors_J
            );
        }

      // end timing
      timer_eigenproblem.Stop();
      std::cout << fmt::format("  (Eigenproblem: {})",timer_eigenproblem.ElapsedTime()) << std::endl;

      // results output: eigenvalues
      spncci::WriteEigenvalues(results_stream,spj_space,eigenvalues,run_parameters.gex);

      ////////////////////////////////////////////////////////////////
      // do decompositions
      ////////////////////////////////////////////////////////////////

      std::cout << "Calculate eigenstate decompositions..." << std::endl;
      mcutils::SteadyTimer timer_decompositions;
      timer_decompositions.Start();

      // decomposition matrices:
      //   - vector over J subspace index
      //   - matrix over (basis_subspace_index,eigenstate_index)
      //
      // That is, decompositions are stored as column vectors, within a
      // matrix, much like the eigenstates themselves.
      std::vector<spncci::Matrix> Nex_decompositions;
      std::vector<spncci::Matrix> baby_spncci_decompositions;
      Nex_decompositions.resize(spj_space.size());
      baby_spncci_decompositions.resize(spj_space.size());

      // calculate decompositions
      spncci::CalculateNexDecompositions(
          spj_space,
          eigenvectors,
          Nex_decompositions,
          run_parameters.Nsigma0,run_parameters.Nmax
        );

      spncci::CalculateBabySpNCCIDecompositions(
          spj_space,
          eigenvectors,
          baby_spncci_decompositions,
          baby_spncci_space.size()
        );

      // end timing
      timer_decompositions.Stop();
      std::cout << fmt::format("  (Decompositions: {})",timer_decompositions.ElapsedTime()) << std::endl;

      // results output: decompositions
      spncci::WriteDecompositions(
          results_stream,
          "Nex",".6f",
          spj_space,
          Nex_decompositions,
          run_parameters.gex
        );

      spncci::WriteDecompositions(
          results_stream,
          "BabySpNCCI",".4e",
          spj_space,
          baby_spncci_decompositions,
          run_parameters.gex
        );


      ////////////////////////////////////////////////////////////////
      // calculate observable RMEs
      ////////////////////////////////////////////////////////////////

      std::cout << "Calculate observable results..." << std::endl;
      mcutils::SteadyTimer timer_observables;
      timer_observables.Start();

      // observable_results_matrices:
      //   - vector over observable_index
      //   - vector over sector_index
      //   - matrix over (bra_eigenstate_index,ket_eigenstate_index)
      std::vector<spncci::OperatorBlocks> observable_results_matrices;
      observable_results_matrices.resize(run_parameters.num_observables);

      // calculate observable results
      for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
        {

          // retrieve sectors
          const spncci::SectorsSpJ& sectors = observable_sectors[observable_index];

          // calculate observable on each sector
          observable_results_matrices[observable_index].resize(sectors.size());
          for (int sector_index=0; sector_index<sectors.size(); ++sector_index)
            {
            
              // retrieve sector information
              const spncci::SectorsSpJ::SectorType& sector = sectors.GetSector(sector_index);
              const int bra_subspace_index = sector.bra_subspace_index();
              const int ket_subspace_index = sector.ket_subspace_index();

              // branch observable block
              spncci::OperatorBlock observable_block;
              const int J0 = run_parameters.observable_J0_values[observable_index];  // well, J0 had better be 0!
              spncci::ConstructBranchedBlock(
                  w_cache,
                  space_u3s,
                  observables_sectors_u3s[observable_index],
                  observables_blocks_u3s[hw_index][observable_index],
                  spaces_lsj,
                  J0,
                  observable_sectors[observable_index].GetSector(sector_index),
                  observable_block
                );

              // calculate observable results
              Eigen::MatrixXd& observable_results_matrix = observable_results_matrices[observable_index][sector_index];
              observable_results_matrix = eigenvectors[bra_subspace_index].transpose()
                * observable_block
                * eigenvectors[ket_subspace_index];

              // print diagnostics
              const HalfInt bra_J = sector.bra_subspace().J();
              const HalfInt ket_J = sector.ket_subspace().J();
              std::cout
                << fmt::format("Observable {} bra_J {} ket_J {}",observable_index,bra_J,ket_J)
                << std::endl;
              // std::cout
              //   << mcutils::FormatMatrix(observable_results_matrix,"8.5f")
              //   << std::endl
              //   << std::endl;
            }

        }

      // end timing
      timer_observables.Stop();
      std::cout << fmt::format("  (Observables: {})",timer_observables.ElapsedTime()) << std::endl;

      // results output: observables
      spncci::WriteObservables(
          results_stream,
          observable_sectors,
          observable_results_matrices,
          run_parameters.gex
        );

    }

  // timing stop
  timer_mesh.Stop();
  std::cout << fmt::format("(Mesh master loop: {})",timer_mesh.ElapsedTime()) << std::endl;

  ////////////////////////////////////////////////////////////////
  // termination
  ////////////////////////////////////////////////////////////////
  results_stream.close();

}
