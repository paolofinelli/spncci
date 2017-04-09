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
#include "mcutils/parsing.h"
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
  void

    // spncci::WriteEigenValues(
    //   run_parameters.J_values, hw, 
    //   run_parameters.Nmax, run_parameters.Nsigma0_ex_max, 
    //   eigenvalues, scalar_observable_expectations, nonscalar_observable_expectations,
    //   run_parameters.num_observables
    // );

  WriteEigenValues(
    const std::vector<HalfInt>& J_values, double hw, 
    int Nmax, int Nsigma0_ex_max,
    std::map<HalfInt,Eigen::VectorXd>& eigenvalues,
    std::vector<std::string>& observable_filenames,
    std::vector<int>& scalar_observable_indices,
    std::vector<std::map<HalfInt,Eigen::VectorXd>>& scalar_observable_expectations,
    std::vector<int>& nonscalar_observable_indices,
    std::vector<std::map<spncci::JPair,Eigen::MatrixXd>>& nonscalar_observable_expectations
  )
  // for observables with J0=0, line them up with energy eigenvalue and read off diagonal matrix elements 
  // for observables with J0!=0, then have their own section --probably do this in the code as well, i.e.,
  {
    std::string filename=fmt::format("eigenvalues_Nmax{:02d}_Nsigma_ex{:02d}.dat",Nmax,Nsigma0_ex_max);
    std::cout<<"writing to file"<<std::endl;
    std::fstream fs;
    const int width=3;
    const int precision=16;
    fs.open (filename, std::fstream::out | std::fstream::app);
    fs << std::setprecision(precision);

    fs << "OUPTPUT from spncci Version 1"<<std::endl<<std::endl;;
    fs << "Scalar observables:";
    for(int i=0; i<scalar_observable_indices.size(); ++i)
      fs <<"  "<<observable_filenames[scalar_observable_indices[i]];
    fs << std::endl;

    fs <<"Nonscalar observables:";
    for(int i=0; i<nonscalar_observable_indices.size(); ++i)
      fs <<"  "<<observable_filenames[nonscalar_observable_indices[i]];

    fs << std::endl<<fmt::format("hw {:2.1f}", hw)<<std::endl;

    for(HalfInt J : J_values)
      {
        Eigen::VectorXd& eigenvalues_J=eigenvalues[J];
        // Eigen::VectorXd& observables=observable_expectations[J];

        for(int i=0; i<eigenvalues_J.size(); ++i)
          {
            double eigenvalue=eigenvalues_J(i);
            std::cout<<fmt::format("{:2d}   {}   {:8.5f}",i, J,eigenvalue);
            fs << fmt::format("{:2d}   {}   {:8.5f}",i, J,eigenvalue);
            
            for(int j=0; j<scalar_observable_indices.size(); ++j)  
            {          
              std::cout<<fmt::format("   {:8.5f}",scalar_observable_expectations[j][J](i))
              <<std::endl<<std::endl;
              fs <<fmt::format("   {:8.5f}",scalar_observable_expectations[j][J](i))
              <<std::endl<<std::endl;
            }
          }
      }
    for(HalfInt J : J_values)
      {
        Eigen::VectorXd& eigenvalues_J_initial=eigenvalues[J];
        for(int i=0; i<eigenvalues_J_initial.size(); ++i)
          for(HalfInt Jp :J_values)
          {
            Eigen::VectorXd& eigenvalues_J_final=eigenvalues[Jp];
            spncci::JPair J_pair(Jp,J);
            for(int ip=0; ip<eigenvalues_J_final.size(); ++ip)
              {
                double eigenvalue_initial=eigenvalues_J_initial(i);
                double eigenvalue_final=eigenvalues_J_final(ip);
                fs << fmt::format("{:2d}   {}   {:2d}   {}   {:8.5f}   {:8.5f}",
                      i,J,ip,Jp,eigenvalue_initial,eigenvalue_final);

                for(int j=0; j<nonscalar_observable_indices.size(); ++j)
                  {
                    // std::cout<<"(ip, i) ("<<ip<<","<<i<<")"<<std::endl;
                    Eigen::MatrixXd& obserable_matrix=nonscalar_observable_expectations[j][J_pair];
                    // std::cout<<obserable_matrix<<std::endl;
                    double observable=obserable_matrix(ip,i);
                    fs<<fmt::format("  {:8.5f}",observable);
                  }
                fs<<std::endl;
              }
          }
      }
    fs<<std::endl<<std::endl;
    fs.close();
  }

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
  RunParameters(int argc, char **argv); 

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
  std::string observable_directory;
  std::vector<std::string> observable_filenames;  // first observable is used as Hamiltonian
  std::vector<int> observable_Jvalues;
  int num_observables;
  std::vector<HalfInt> J_values;
  std::vector<double> hw_values;

  // eigensolver
  int num_eigenvalues;
  int eigensolver_num_convergence;  // whatever exactly this is...
  int eigensolver_max_iterations;
  double eigensolver_tolerance;

};

RunParameters::RunParameters(int argc, char **argv)
{
  // read from command line arguments
  //
  // TODO reorder filenames 
  if (argc<5)
    {
      std::cout << "Syntax: A twice_Nsigma0 Nsigma0_ex_max N1v Nmax num_eigenvalues <load file>"
       // <basis filename> <Nrel filename> <Brel filename> <Arel filename>" 
                << std::endl;
      std::exit(1);
    }
  A = std::stoi(argv[1]); 
  int twice_Nsigma0= std::stoi(argv[2]);
  Nsigma0_ex_max=std::stoi(argv[3]);
  Nsigma_0=HalfInt(twice_Nsigma0,2);
  N1v=std::stoi(argv[4]);
  Nmax = std::stoi(argv[5]);
  num_eigenvalues=std::stoi(argv[6]);
  std::string load_file=argv[7];

  // std::cout<< fmt::format("{} {} {} {} {} {}",A, twice_Nsigma0, Nsigma_0, Nsigma0_ex_max, N1v, Nmax)<<std::endl;
  
  // many-body problem
  // observable_filenames = std::vector<std::string>({"hamiltonian_u3st.dat"});

  // Reading in from load life 
  int line_count=0;
  int twice_Jmin, twice_Jmax, J_step;
  double hw_min, hw_max, hw_step;
  std::string line, observable;
  std::ifstream is(fmt::format("{}.load",load_file));
  
  assert(is);
  int J0;
  while(std::getline(is,line))
    {
      std::istringstream line_stream(line);
      ++line_count;
      if(line_count==1)
      {
        line_stream >> twice_Jmin >> twice_Jmax >> J_step;
        ParsingCheck(line_stream,line_count,line);
      }
      else if(line_count==2)
      {
        line_stream >> hw_min >> hw_max >> hw_step;
        ParsingCheck(line_stream,line_count,line);
      }
      else
      {
        line_stream >> observable >> J0;
        ParsingCheck(line_stream,line_count,line);
        observable_filenames.push_back(observable);
        observable_Jvalues.push_back(J0);
      }
    }

  num_observables = observable_filenames.size();
  observable_directory="relative_observables";
  // generate list of J values 
  HalfInt Jmin(twice_Jmin,2);
  HalfInt Jmax(twice_Jmax,2);
  for(HalfInt J=Jmin; J<=Jmax; J+=J_step)
    J_values.push_back(J);

  std::cout<<"J values are: ";
  for(auto J : J_values)
    std::cout<<J<<"  ";
  std::cout<<std::endl;

  for(double hw=hw_min; hw<=hw_max; hw+=hw_step)
    hw_values.push_back(hw);

  std::cout<<"hw values are: ";
  for(auto hw : hw_values)
    std::cout<<hw<<"  ";
  std::cout<<std::endl;


  // hard-coded directory structure and filenames
  lsu3shell_rme_directory = "lsu3shell_rme";
  lsu3shell_basis_filename = lsu3shell_rme_directory + "/" + "lsu3shell_basis.dat";
  Brel_filename = lsu3shell_rme_directory + "/" + fmt::format("Brel_06_Nmax{:02d}.rme",Nsigma0_ex_max);
  Arel_filename = lsu3shell_rme_directory + "/" + fmt::format("Arel_06_Nmax{:02d}.rme",Nsigma0_ex_max);
  Nrel_filename = lsu3shell_rme_directory + "/" + fmt::format("Nrel_06_Nmax{:02d}.rme",Nsigma0_ex_max);
  relative_unit_tensor_filename_template = lsu3shell_rme_directory + "/" + "relative_unit_{:06d}.rme";

  // hard-coded eigen solver parameters   
  eigensolver_num_convergence = 2*num_eigenvalues;    // docs for SymEigsSolver say to take "ncv>=2*nev"
  eigensolver_max_iterations = 100*num_eigenvalues;
  eigensolver_tolerance = 1e-8;
}


////////////////////////////////////////////////////////////////
// main body
////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  std::cout<<"entering spncci"<<std::endl;
  ////////////////////////////////////////////////////////////////
  // initialization
  ////////////////////////////////////////////////////////////////
  
  // SU(3) caching
  u3::U3CoefInit();
  u3::UCoefCache u_coef_cache;
  u3::PhiCoefCache phi_coef_cache;
  u3::g_u_cache_enabled = true;

  // numerical parameter for certain calculations
  double zero_threshold=1e-8;  // DEPRECATED but still may be used some places
  spncci::g_zero_tolerance = 1e-6;
  spncci::g_suppress_zero_sectors = true;

  // run parameters
  RunParameters run_parameters(argc,argv);

  // Eigen OpenMP multithreading mode
  Eigen::initParallel();
  // Eigen::setNbThreads(0);

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
  // bool keep_zero_sectors=true;
  lgi::GenerateLGIExpansion(
      lsu3shell_space, 
      Brel_sectors,Brel_matrices,Ncm_sectors,Ncm_matrices,
      run_parameters.Nsigma_0,
      lgi_families,lgi_expansions
      // keep_zero_sectors
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

  // DEPRECIATED
  // std::vector<u3shell::SectorsU3SPN> lgi_unit_tensor_sectors;
  // std::vector<basis::MatrixVector> lgi_unit_tensor_lsu3shell_matrices;

  // determine set of seed unit tensors
  //
  // i.e., those for which we calculate seed rmes among the LGIs
  //
  // Note: Should be consistant with set of tensors generated by
  // generate_lsu3shell_relative_operators.
  // int Nmax_for_lgi_unit_tensors = run_parameters.Nsigma0_ex_max+2*run_parameters.N1v;  // max quanta for pair in LGI (?)
  int J0_for_unit_tensors = -1;  // all J0
  int T0_for_unit_tensors = -1;
  const bool restrict_positive_N0 = false;  // don't restrict to N0 positive

  u3shell::GenerateRelativeUnitTensorLabelsU3ST(
    run_parameters.Nsigma0_ex_max, run_parameters.N1v,
    lgi_unit_tensor_labels,J0_for_unit_tensors,T0_for_unit_tensors,
    restrict_positive_N0
    );

  // for each unit tensor, read in and transform.

  // diagnostic
  std::cout << fmt::format("  seed unit tensors {}",lgi_unit_tensor_labels.size()) << std::endl;

  ////////////////////////////////////////////////////////////////
  // transform and store seed rmes for use in SpNCCI recurrence
  ////////////////////////////////////////////////////////////////
  std::cout << "Transform and store seed unit tensor rmes..." << std::endl;
  spncci::UnitTensorMatricesByIrrepFamily unit_tensor_matrices;
  for (int unit_tensor_index=0; unit_tensor_index<lgi_unit_tensor_labels.size(); ++unit_tensor_index)
    {

      const u3shell::RelativeUnitTensorLabelsU3ST& unit_tensor_labels = lgi_unit_tensor_labels[unit_tensor_index];

      // basis::MatrixVector lgi_unit_tensor_lsu3shell_matrices;
      u3shell::SectorsU3SPN unit_tensor_sectors;
      std::string filename = fmt::format(run_parameters.relative_unit_tensor_filename_template,unit_tensor_index);
      basis::MatrixVector unit_tensor_spncci_matrices;

      // transform to SpNCCI LGI RMEs
      spncci::ReadAndTransformSeedUnitTensorRMEs(
          lsu3shell_basis_table,lsu3shell_space, lgi_expansions,
          unit_tensor_labels,filename,
          unit_tensor_sectors,
          unit_tensor_spncci_matrices);

      // store unit tensor matrix elements for recurrence
      HalfInt Nsigma_max=run_parameters.Nsigma0_ex_max+run_parameters.Nsigma_0;
      spncci::StoreSeedUnitTensorRMEs(
          unit_tensor_labels,
          unit_tensor_sectors,
          unit_tensor_spncci_matrices,
          unit_tensor_matrices,
          Nsigma_max,
          zero_threshold
        );
    }
  timer_read_seeds.Stop();
  std::cout << fmt::format("(Task time: {})",timer_read_seeds.ElapsedTime()) << std::endl;
  std::cout<<"number of seed sectors "<<unit_tensor_matrices.size()<<std::endl;


  ////////////////////////////////////////////////////////////////
  // recurse unit tensor rmes to full SpNCCI basis
  ////////////////////////////////////////////////////////////////

  std::cout << "Recurse unit tensor rmes..." << std::endl;

  // determine full set of unit tensors for rme calculation
  int Nmax_for_unit_tensors=run_parameters.Nmax+2*run_parameters.N1v;
  std::map<int,std::vector<u3shell::RelativeUnitTensorLabelsU3ST>> unit_tensor_labels;
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(
      run_parameters.Nmax, run_parameters.N1v,unit_tensor_labels,
      J0_for_unit_tensors,T0_for_unit_tensors,restrict_positive_N0
    );

  // std::cout<<"unit tensor labels size "<<unit_tensor_labels.size()<<std::endl;

  // timing start
  Timer timer_recurse;
  timer_recurse.Start();

  // recurrence
  //
  // Currently over "all" unit tensors, subject to some constraints...
  RecurseUnitTensors(
      run_parameters.N1v, run_parameters.Nmax,spncci_space,
      k_matrix_cache,u_coef_cache,phi_coef_cache,
      unit_tensor_labels,unit_tensor_matrices,
      true  // verbose
    );

  // timing stop
  timer_recurse.Stop();
  std::cout << fmt::format("(Task time: {})",timer_recurse.ElapsedTime()) << std::endl;
  

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Observables 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  for(double hw : run_parameters.hw_values)
  {

    ////////////////////////////////////////////////////////////////
    // read relative operators
    ////////////////////////////////////////////////////////////////

    std::cout << "Read observable relative rmes..." << std::endl;

    std::vector<u3shell::RelativeRMEsU3ST> observable_relative_rmes;
    observable_relative_rmes.resize(run_parameters.num_observables);
    for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
      {
        std::string observable_filename=fmt::format("{}/{}_hw{:2.1f}_Nmax{:02d}_u3st.dat", 
            run_parameters.observable_directory,
            run_parameters.observable_filenames[observable_index],hw,run_parameters.Nmax);

        std::cout << fmt::format("  Reading {}...",observable_filename)<< std::endl;
        u3shell::ReadRelativeOperatorU3ST(observable_filename,observable_relative_rmes[observable_index]);
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
    // int J0 = 0;

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
    std::vector<std::map<spncci::JPair,spncci::MatrixType>> observable_matrices;  
    observable_matrices.resize(run_parameters.num_observables);

    spncci::ConstructBranchedObservables(space_u3s,observable_sectors_u3s,
      observable_matrices_u3s, spaces_lsj,run_parameters.num_observables,run_parameters.J_values,
      run_parameters.observable_Jvalues, observable_matrices);

    // timing stop
    timer_branching.Stop();
    std::cout << fmt::format("(Task time: {})",timer_branching.ElapsedTime()) << std::endl;

    // diagnostics: branched matrices
    if (false)
      {
        for (int observable_index=0; observable_index<run_parameters.num_observables; ++observable_index)
          for (const HalfInt bra_J : run_parameters.J_values)
            for (const HalfInt ket_J : run_parameters.J_values)
              {
                int J0=run_parameters.observable_Jvalues[observable_index];
                if(not am::AllowedTriangle(bra_J,J0,ket_J))
                  continue;

                spncci::JPair J_pair(bra_J,ket_J);
                spncci::MatrixType& observable_matrix = observable_matrices[observable_index][J_pair];

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
    std::map<HalfInt,spncci::MatrixType> eigenvectors;  // map: J -> eigenvectors
    
    assert(run_parameters.observable_Jvalues[0]==0);
    for (const HalfInt J : run_parameters.J_values)
      {
        // set up aliases
        spncci::JPair J_pair(J,J);
        spncci::MatrixType& hamiltonian_matrix = observable_matrices[0][J_pair];

        int num_eigenvalues;
        int num_convergence;
        if((hamiltonian_matrix.cols()/2)<run_parameters.num_eigenvalues)
        {
          num_eigenvalues=int(hamiltonian_matrix.cols()/2);
          num_convergence = 2*num_eigenvalues;    // docs for SymEigsSolver say to take "ncv>=2*nev"
        }
        else
        {
          num_eigenvalues=run_parameters.num_eigenvalues;
          num_convergence=run_parameters.eigensolver_num_convergence;
        }
        spncci::SolveHamiltonian(hamiltonian_matrix,J,
            num_eigenvalues,
            num_convergence,
            run_parameters.eigensolver_max_iterations,
            run_parameters.eigensolver_tolerance,
            eigenvalues,  eigenvectors
          );
      }
    // timing stop
    timer_eigenproblem.Stop();
    std::cout << fmt::format("(Task time: {})",timer_eigenproblem.ElapsedTime()) << std::endl;


    // map: observable -> J -> eigenvalues
    // scalar observables 
    std::vector<std::map<HalfInt,Eigen::VectorXd>> scalar_observable_expectations;
    std::vector<int> scalar_observable_indices;
    // Non-scalar observables 
    std::vector<std::map<spncci::JPair,Eigen::MatrixXd>> nonscalar_observable_expectations;
    std::vector<int> nonscalar_observable_indices;

    // observable_index=0 correspond to Hamiltonian.
    std::cout<<"break into scalar and non-scalar"<<std::endl;
    for (int observable_index=1; observable_index<run_parameters.num_observables; ++observable_index)
      {
        if(run_parameters.observable_Jvalues[observable_index]==0)
          scalar_observable_indices.push_back(observable_index);
        else
          nonscalar_observable_indices.push_back(observable_index);
      }
    
    // scalar observables  
    scalar_observable_expectations.resize(scalar_observable_indices.size());
    for (int i=0; i<scalar_observable_indices.size(); ++i)
      {
        int observable_index=scalar_observable_indices[i];
        for (const HalfInt J : run_parameters.J_values)
        {
          int converged_eigenvectors = eigenvalues[J].size();
          spncci::JPair J_pair(J,J);
          const spncci::MatrixType& observable_matrix = observable_matrices[observable_index][J_pair];
          scalar_observable_expectations[i][J];//=Eigen::VectorXd(converged_eigenvectors);
          scalar_observable_expectations[i][J].resize(converged_eigenvectors);
          // std::cout<<observable_matrix<<std::endl;
          for (int eigenvector_index=0; eigenvector_index<converged_eigenvectors; ++eigenvector_index)
            {
              const Eigen::VectorXd eigenvector = eigenvectors[J].col(eigenvector_index);
              double expectation_value = eigenvector.dot(observable_matrix*eigenvector);
              scalar_observable_expectations[i][J][eigenvector_index]=expectation_value;
            }
        }
      }

    // non-scalar observables
    std::cout<<"non-scalar observables"<<std::endl; 
    nonscalar_observable_expectations.resize(nonscalar_observable_indices.size());
    for (int i=0; i<nonscalar_observable_indices.size(); ++i)
      for (const HalfInt bra_J : run_parameters.J_values)
        for (const HalfInt ket_J : run_parameters.J_values)
          {
            int observable_index=nonscalar_observable_indices[i];
            int J0=run_parameters.observable_Jvalues[observable_index];
            // std::cout<<bra_J<<"  "<<J0<<"  "<<ket_J<<std::endl;
            spncci::JPair J_pair(bra_J,ket_J);
            const int bra_converged_eigenvectors = eigenvalues[bra_J].size();
            const int ket_converged_eigenvectors = eigenvalues[ket_J].size();
            const spncci::MatrixType& observable_matrix = observable_matrices[observable_index][J_pair];
            // nonscalar_observable_expectations[observable_index][J_pair].resize(converged_eigenvectors);
            // std::cout<<observable_matrix<<std::endl;
            nonscalar_observable_expectations[i][J_pair]=eigenvectors[bra_J].transpose()*observable_matrix*eigenvectors[ket_J];

            // for (int bra_eigenvector_index=0; bra_eigenvector_index<bra_converged_eigenvectors; ++bra_eigenvector_index)
            //   for (int ket_eigenvector_index=0; ket_eigenvector_index<ket_converged_eigenvectors; ++ket_eigenvector_index)
            //     {
            //       // std::cout<<eigenvectors[bra_J].transpose().cols()<<"  "<<observable_matrix.rows()<<"  "<<observable_matrix.cols()
            //       // <<"  "<<eigenvectors[ket_J].rows()<<std::endl;
            //       const Eigen::VectorXd bra_eigenvector = eigenvectors[bra_J].col(bra_eigenvector_index);
            //       const Eigen::VectorXd ket_eigenvector = eigenvectors[ket_J].col(ket_eigenvector_index);
            //       // double expectation_value = bra_eigenvector.dot(observable_matrix*ket_eigenvector);
            //       nonscalar_observable_expectations[i][J_pair]=bra_eigenvector.transpose()*observable_matrix*ket_eigenvector;
                  
            //       std::cout<<nonscalar_observable_expectations[i][J_pair]<<std::endl;
            //       // [eigenvector_index] = expectation_value;
            //   }
          }

    spncci::WriteEigenValues(
      run_parameters.J_values, hw, 
      run_parameters.Nmax, run_parameters.Nsigma0_ex_max,
      eigenvalues,
      run_parameters.observable_filenames,
      scalar_observable_indices,
      scalar_observable_expectations,
      nonscalar_observable_indices,
      nonscalar_observable_expectations
    );
    std::cout<<"wrote eigenvalues to file"<<std::endl;

    // eigenvalue output
    for (const HalfInt J : run_parameters.J_values)
      {
        // eigenvalues
        std::cout << fmt::format("  Eigenvalues (J={}, hw={}):",J,hw) << std::endl
                  << mcutils::FormatMatrix(eigenvalues[J],"8.5f","    ")
                  << std::endl;

      //   // expectations
      //   for(const HalfInt Jp : run_parameters.J_values)
      //     {
      //       spncci::JPair J_pair(Jp,J);
      //       for (int observable_index=1; observable_index<run_parameters.num_observables; ++observable_index)
      //         {
      //           std::cout
      //             << fmt::format(
      //                 "  Operator {} ({}) expectations (J {}->{}):",
      //                 observable_index,
      //                 run_parameters.observable_filenames[observable_index],
      //                 J,Jp
      //               )
      //             << std::endl
      //             << mcutils::FormatMatrix(scalar_observable_expectations[observable_index][J],"8.5f","    ")
      //             << std::endl;
      //         }

      //       // diagnostics
      //       if (false)
      //         {
      //           std::cout << fmt::format("  Eigenvectors -- norm (J={}):",J) << std::endl
      //                     << mcutils::FormatMatrix(eigenvectors[J],"8.5f","    ")
      //                     << std::endl;
      //         }
      //     }
      }
  }

}
