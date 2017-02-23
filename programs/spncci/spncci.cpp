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
       INT 1.0 4 0 0 0 relative_observables/JISP16_Nmax20_hw20.0_rel.dat      // coef Jmax J0 T0 g0 interaction_filename

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
****************************************************************/

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sys/resource.h>

#include "SymEigsSolver.h"  // from spectra

#include "cppformat/format.h"
#include "lgi/lgi_solver.h"
#include "mcutils/profiling.h"
#include "spncci/computation_control.h"
#include "spncci/explicit_construction.h"
#include "spncci/io_control.h"

// to vett as moved into computation_control 
#include "mcutils/eigen.h"

////////////////////////////////////////////////////////////////
// WIP code
//
// to extract to spncci library when ready
////////////////////////////////////////////////////////////////

namespace spncci
{



}// end namespace


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

  // many-body problem
  std::vector<std::string> observable_filenames;  // first observable is used as Hamiltonian
  int num_observables;
  std::vector<HalfInt> J_values;

  // eigensolver
  int num_eigenvalues;
  int eigensolver_num_convergence;  // whatever exactly this is...
  int eigensolver_max_iterations;
  double eigensolver_tolerance;

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

  // many-body problem
  observable_filenames = std::vector<std::string>({"hamiltonian_u3st.dat","r2intr_hw20.0_Nmax02_u3st.dat"});
  num_observables = observable_filenames.size();
  J_values = std::vector<HalfInt>({0,1});
  num_eigenvalues = 10;
  eigensolver_num_convergence = 2*num_eigenvalues;    // docs for SymEigsSolver say to take "ncv>=2*nev"
  eigensolver_max_iterations = 100*num_eigenvalues;
  eigensolver_tolerance = 1e-8;
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
  double zero_threshold=1e-8;

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


  lgi::MultiplicityTaggedLGIVector lgi_families;
  basis::MatrixVector lgi_expansions;
  bool keep_zero_sectors=true;
  lgi::GenerateLGIExpansion(
      lsu3shell_space, 
      Brel_sectors,Brel_matrices,Ncm_sectors,Ncm_matrices,
      run_parameters.Nsigma_0,
      lgi_families,lgi_expansions,
      keep_zero_sectors
    );

  // diagnostics
  std::cout << fmt::format("  LGI families {}",lgi_families.size()) << std::endl;
  if (false)
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

  // timing stop
  timer_k_matrices.Stop();
  std::cout << fmt::format("(Task time: {})",timer_k_matrices.ElapsedTime()) << std::endl;

  ////////////////////////////////////////////////////////////////
  // read lsu3shell seed unit tensor rmes
  ////////////////////////////////////////////////////////////////
  // timing start
  Timer timer_read_seeds;
  timer_read_seeds.Start();

  std::cout << "Read seed unit tensor rmes..." << std::endl;

  // storage for seed unit tensor rmes
  //
  //   lgi_unit_tensor_labels: vector of labels for seed unit tensors
  //   lgi_unit_tensor_lsu3shell_sectors: vector of lsu3shell sectors for seed unit tensors
  //   lgi_unit_tensor_matrices: vector of matrices for these sectors
  std::vector<u3shell::RelativeUnitTensorLabelsU3ST> lgi_unit_tensor_labels;
  std::vector<u3shell::SectorsU3SPN> lgi_unit_tensor_sectors;
  std::vector<basis::MatrixVector> lgi_unit_tensor_lsu3shell_matrices;

  // determine set of seed unit tensors
  //
  // i.e., those for which we calculate seed rmes among the LGIs
  //
  // Note: Should be consistant with set of tensors generated by
  // generate_lsu3shell_relative_operators.
  int Nmax_for_unit_tensors = run_parameters.Nsigma0_ex_max+2*run_parameters.N1v;  // max quanta for pair in LGI (?)
  int J0_for_unit_tensors = -1;  // all J0
  int T0_for_unit_tensors = 0;
  const bool restrict_positive_N0 = false;  // don't restrict to N0 positive
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(
      Nmax_for_unit_tensors,lgi_unit_tensor_labels,
      J0_for_unit_tensors,T0_for_unit_tensors,restrict_positive_N0
    );

  // diagnostic
  std::cout << fmt::format("  seed unit tensors {}",lgi_unit_tensor_labels.size()) << std::endl;

  spncci::ReadLSU3ShellSeedUnitTensorRMEs(
      lsu3shell_basis_table,lsu3shell_space,
      lgi_unit_tensor_labels,
      run_parameters.relative_unit_tensor_filename_template,
      lgi_unit_tensor_sectors,
      lgi_unit_tensor_lsu3shell_matrices
    );

  timer_read_seeds.Stop();
  std::cout << fmt::format("(Task time: {})",timer_read_seeds.ElapsedTime()) << std::endl;


  ////////////////////////////////////////////////////////////////
  // transform and store seed rmes for use in SpNCCI recurrence
  ////////////////////////////////////////////////////////////////

  std::cout << "Transform and store seed unit tensor rmes..." << std::endl;

  // transform to SpNCCI LGI RMEs
  std::vector<basis::MatrixVector> lgi_unit_tensor_spncci_matrices;
  spncci::TransformSeedUnitTensorRMEs(
      lgi_expansions,
      lgi_unit_tensor_labels,
      lgi_unit_tensor_sectors,
      lgi_unit_tensor_lsu3shell_matrices,
      lgi_unit_tensor_spncci_matrices
    );

  // store unit tensor matrix elements for recurrence
  spncci::UnitTensorMatricesByIrrepFamily unit_tensor_matrices;
  spncci::StoreSeedUnitTensorRMEs(
      lgi_unit_tensor_labels,
      lgi_unit_tensor_sectors,
      lgi_unit_tensor_spncci_matrices,
      unit_tensor_matrices,
      zero_threshold
    );

  ////////////////////////////////////////////////////////////////
  // recurse unit tensor rmes to full SpNCCI basis
  ////////////////////////////////////////////////////////////////

  std::cout << "Recurse unit tensor rmes..." << std::endl;

  // determine full set of unit tensors for rme calculation
  std::map<int,std::vector<u3shell::RelativeUnitTensorLabelsU3ST>> unit_tensor_labels;
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(
      Nmax_for_unit_tensors,unit_tensor_labels,
      J0_for_unit_tensors,T0_for_unit_tensors,restrict_positive_N0
    );

  // timing start
  Timer timer_recurse;
  timer_recurse.Start();

  // recurrence
  //
  // Currently over "all" unit tensors, subject to some constraints...
  RecurseUnitTensors(
      run_parameters.N1v, run_parameters.Nmax,spncci_space,
      k_matrix_cache,u_coef_cache,phi_coef_cache,
      unit_tensor_labels,unit_tensor_matrices
    );

  // timing stop
  timer_recurse.Stop();
  std::cout << fmt::format("(Task time: {})",timer_recurse.ElapsedTime()) << std::endl;

  ////////////////////////////////////////////////////////////////
  // read relative operators
  ////////////////////////////////////////////////////////////////

  std::cout << "Read observable relative rmes..." << std::endl;

  std::vector<u3shell::RelativeRMEsU3ST> observable_relative_rmes;
  observable_relative_rmes.resize(run_parameters.num_observables);
  for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
    {
      std::cout << fmt::format("  Reading {}...",run_parameters.observable_filenames[observable_index])
                << std::endl;
      u3shell::ReadRelativeOperatorU3ST(
          run_parameters.observable_filenames[observable_index],
          observable_relative_rmes[observable_index]
        );
    }
  ////////////////////////////////////////////////////////////////
  // contract and regroup observables
  ////////////////////////////////////////////////////////////////

  std::cout << "Set up basis indexing for contracting and regrouping..." << std::endl;

  // set up basis indexing for regrouping
  spncci::BabySpNCCISpace baby_spncci_space(spncci_space);
  spncci::SpaceU3S space_u3s(baby_spncci_space);

  std::cout << "Constract and regroup observable matrices..." << std::endl;

  // timing start
  Timer timer_regrouping;
  timer_regrouping.Start();

  // contract and regroup observable matrices at U3S level
  std::vector<std::vector<spncci::SectorLabelsU3S>> observable_sectors_u3s;
  std::vector<basis::MatrixVector> observable_matrices_u3s;
  observable_sectors_u3s.resize(run_parameters.num_observables);
  observable_matrices_u3s.resize(run_parameters.num_observables);
  for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
    spncci::ConstructObservableU3S(
        run_parameters.Nmax,run_parameters.N1v,
        baby_spncci_space,
        space_u3s,
        unit_tensor_matrices,observable_relative_rmes[observable_index],
        observable_sectors_u3s[observable_index],observable_matrices_u3s[observable_index]
      );

  // timing stop
  timer_regrouping.Stop();
  std::cout << fmt::format("(Task time: {})",timer_regrouping.ElapsedTime()) << std::endl;

  // diagnostic
  if (false)
    {
      for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
        {
          std::cout << fmt::format("  observable {}",observable_index) << std::endl;
          const auto& sectors_u3s = observable_sectors_u3s[observable_index];
          const auto& matrices_u3s = observable_matrices_u3s[observable_index];
          for (int sector_index=0; sector_index<sectors_u3s.size(); ++sector_index)
            {
              std::cout << fmt::format("    sector {}",sector_index) << std::endl;
              std::cout << sectors_u3s[sector_index].Str() << std::endl;
              std::cout << mcutils::FormatMatrix(matrices_u3s[sector_index],"8.3f") << std::endl
                        <<std::endl;
            }
        }
    }

  ////////////////////////////////////////////////////////////////
  // branch observables
  ////////////////////////////////////////////////////////////////

  // Note: Right now, only supports J0=0.  Will have to implement more
  // generally to work with (J_bra,J_ket) pairs when go to nonscalar
  // operators.
  int J0 = 0;

  std::cout << "Set up basis indexing for branching..." << std::endl;

  // set up basis indexing for branching
  std::map<HalfInt,spncci::SpaceLS> spaces_lsj;  // map: J -> space
  for (const HalfInt J : run_parameters.J_values)
    {
      spaces_lsj[J] = spncci::SpaceLS(space_u3s,J);
    }

  std::cout << "Construct branched observable matrices..." << std::endl;

  // timing start
  Timer timer_branching;
  timer_branching.Start();

  // populate fully-branched many-body matrices for observables
  // map: observable -> J ->  matrix
  std::vector<std::map<HalfInt,Eigen::MatrixXd>> observable_matrices;  
  observable_matrices.resize(run_parameters.num_observables);
  for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
    for (const HalfInt J : run_parameters.J_values)
      {
        // set up aliases (for current observable and J space)
        const std::vector<spncci::SectorLabelsU3S>& sectors_u3s = observable_sectors_u3s[observable_index];
        const basis::MatrixVector& matrices_u3s = observable_matrices_u3s[observable_index];
        const spncci::SpaceLS& space_lsj = spaces_lsj[J];
        Eigen::MatrixXd& observable_matrix = observable_matrices[observable_index][J];

        // determine set of (L0,S0) labels for this observable (triangular with J0)
        std::vector<spncci::OperatorLabelsLS> operator_labels_ls;
        // Note: to update when J0 varies by observable
        spncci::GenerateOperatorLabelsLS(J0,operator_labels_ls);

        // determine allowed LS sectors
        const spncci::SpaceLS& bra_space_lsj = space_lsj;
        const spncci::SpaceLS& ket_space_lsj = space_lsj;
        const HalfInt bra_J = J;
        const HalfInt ket_J = J;
        std::vector<spncci::SectorLabelsLS> sectors_lsj;
        spncci::GetSectorsLS(bra_space_lsj,ket_space_lsj,operator_labels_ls,sectors_lsj);

        // branch LS sectors to LSJ
        basis::MatrixVector matrices_lsj;  
        spncci::ContractAndRegroupLSJ(
            bra_J,J0,ket_J,
            space_u3s,sectors_u3s,matrices_u3s,
            bra_space_lsj,ket_space_lsj,sectors_lsj,matrices_lsj
          );

        // collect LSJ sectors into J matrix
        //
        // Note: Interface needs to be generalized to handle J_bra != J_ket.
        ConstructOperatorMatrix(
            space_lsj,sectors_lsj,matrices_lsj,
            observable_matrix
          );
      }

  // timing stop
  timer_branching.Stop();
  std::cout << fmt::format("(Task time: {})",timer_branching.ElapsedTime()) << std::endl;

  // diagnostics: branched matrices
  if (false)
    {
      for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
        for (const HalfInt J : run_parameters.J_values)
          {
            Eigen::MatrixXd& observable_matrix = observable_matrices[observable_index][J];

            const HalfInt bra_J = J;
            const HalfInt ket_J = J;
            std::cout
              << fmt::format("Observable {} bra_J {} ket_J {}",observable_index,bra_J,ket_J)
              << std::endl;
            std::cout
              << mcutils::FormatMatrix(observable_matrix,"8.5f")
              << std::endl
              << std::endl;

          }
    }

  ////////////////////////////////////////////////////////////////
  // eigenstuff
  ////////////////////////////////////////////////////////////////

  std::cout << "Solve Hamiltonian eigenproblem..." << std::endl;

  // timing start
  Timer timer_eigenproblem;
  timer_eigenproblem.Start();

  std::map<HalfInt,Eigen::VectorXd> eigenvalues;  // map: J -> eigenvalues
  std::map<HalfInt,Eigen::MatrixXd> eigenvectors;  // map: J -> eigenvectors

  for (const HalfInt J : run_parameters.J_values)
    {
      // set up aliases
      Eigen::MatrixXd& hamiltonian_matrix = observable_matrices[0][J];
      
      std::cout << fmt::format("  Diagonalizing: J={}",J) << std::endl;

      // define eigensolver and compute
      typedef Eigen::MatrixXd MatrixType;  // allow for possible future switch to more compact single-precision matrix
      typedef double FloatType;
      Spectra::DenseSymMatProd<FloatType> matvec(hamiltonian_matrix);
      Spectra::SymEigsSolver<FloatType,Spectra::SMALLEST_ALGE,Spectra::DenseSymMatProd<FloatType>>
        eigensolver(
            &matvec,
            run_parameters.num_eigenvalues,
            run_parameters.eigensolver_num_convergence
          );
      eigensolver.init();
      int converged_eigenvectors = eigensolver.compute(
          run_parameters.eigensolver_max_iterations,
          run_parameters.eigensolver_tolerance,
          Spectra::SMALLEST_ALGE  // sorting rule
        );
      int eigensolver_status = eigensolver.info();
      std::cout
        << fmt::format("  Eigensolver reports: eigenvectors {} status {}",converged_eigenvectors,eigensolver_status)
        << std::endl;
      assert(converged_eigenvectors=eigensolver.eigenvalues().size());  // should this always be true?
      assert(converged_eigenvectors=eigensolver.eigenvectors().cols());  // should this always be true?

      // save eigenresults
      eigenvalues[J] = eigensolver.eigenvalues();
      eigenvectors[J] = eigensolver.eigenvectors();
      std::cout << fmt::format("  Eigenvalues (J={}):",J) << std::endl
                << mcutils::FormatMatrix(eigenvalues[J],"8.5f","    ")
                << std::endl;

      // check eigenvector norms
      Eigen::VectorXd eigenvector_norms(eigenvectors[J].cols());
      for (int eigenvector_index=0; eigenvector_index<converged_eigenvectors; ++eigenvector_index)
        {
          eigenvector_norms(eigenvector_index) = eigenvectors[J].col(eigenvector_index).norm();
          const double norm_tolerance=1e-8;
          assert(fabs(eigenvector_norms(eigenvector_index)-1)<norm_tolerance);
        }
      if (false)
        {
          std::cout << fmt::format("  Norms (J={}):",J) << std::endl
                    << mcutils::FormatMatrix(eigenvector_norms,"8.5f","    ")
                    << std::endl;
        }

      // normalize eigenvectors -- redundant with Spectra eigensolver
      for (int eigenvector_index=0; eigenvector_index<converged_eigenvectors; ++eigenvector_index)
        eigenvectors[J].col(eigenvector_index).normalize();

      // diagnostics
      if (false)
        {
          std::cout << fmt::format("  Eigenvectors -- norm (J={}):",J) << std::endl
                    << mcutils::FormatMatrix(eigenvectors[J],"8.5f","    ")
                    << std::endl;
        }
    }

  
  // timing stop
  timer_eigenproblem.Stop();
  std::cout << fmt::format("(Task time: {})",timer_eigenproblem.ElapsedTime()) << std::endl;

  // observable expectation values (assumes J0=0)
  std::vector<std::map<HalfInt,Eigen::VectorXd>> observable_expectations;  // map: observable -> J -> eigenvalues
  observable_expectations.resize(run_parameters.num_observables);
  for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
    for (const HalfInt J : run_parameters.J_values)
      {
        const int converged_eigenvectors = eigenvalues[J].size();
        const Eigen::MatrixXd& observable_matrix = observable_matrices[observable_index][J];
        observable_expectations[observable_index][J].resize(converged_eigenvectors);
        for (int eigenvector_index=0; eigenvector_index<converged_eigenvectors; ++eigenvector_index)
          {
            const Eigen::VectorXd eigenvector = eigenvectors[J].col(eigenvector_index);
            double expectation_value = eigenvector.dot(observable_matrix*eigenvector);
            observable_expectations[observable_index][J][eigenvector_index] = expectation_value;
          }
      }

  // eigenvalue output

  for (const HalfInt J : run_parameters.J_values)
    {

      // eigenvalues
      std::cout << fmt::format("  Eigenvalues (J={}):",J) << std::endl
                << mcutils::FormatMatrix(eigenvalues[J],"8.5f","    ")
                << std::endl;

      // expectations
      for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
        {
          std::cout
            << fmt::format(
                "  Operator {} ({}) expectations (J={}):",
                observable_index,
                run_parameters.observable_filenames[observable_index],
                J
              )
            << std::endl
            << mcutils::FormatMatrix(observable_expectations[observable_index][J],"8.5f","    ")
            << std::endl;
        }

      // diagnostics
      if (false)
        {
          std::cout << fmt::format("  Eigenvectors -- norm (J={}):",J) << std::endl
                    << mcutils::FormatMatrix(eigenvectors[J],"8.5f","    ")
                    << std::endl;
        }
    }

}
