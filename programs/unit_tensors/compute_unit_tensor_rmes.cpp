/****************************************************************
  compute_unit_tensor_rmes.cpp

  compute relative unit tensor rmes in spncci basis using recurrance
                                  
  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  12/3/16 (aem): Created.  Based on unit_tensor_test.cpp
  1/6/17 (aem) : Added contraction of unit tensors and interaction
****************************************************************/
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sys/resource.h>
#include "cppformat/format.h"
#include "am/am.h"

#include "lgi/lgi.h"
#include "lgi/lgi_solver.h"
#include "lsu3shell/lsu3shell_basis.h"
#include "lsu3shell/lsu3shell_rme.h"
#include "mcutils/eigen.h"
#include "sp3rlib/u3coef.h"
#include "sp3rlib/vcs.h" 
#include "spncci/unit_tensor.h"
#include "spncci/branching_u3s.h"
#include "spncci/branching_u3lsj.h"
#include "u3shell/relative_operator.h"
#include "u3shell/upcoupling.h"


namespace spncci
{
  // typedef std:: map< std::pair<int,int>, std::map<std::pair<int,int>,spncci::UnitTensorSectorsCache >
  //                     > LGIUnitTensorSectorCache;
  typedef spncci::UnitTensorMatricesByIrrepFamily LGIUnitTensorSectorCache;


  // void 
  // ContractAndRegroupLSJ(
  //   const HalfInt& Jp,const HalfInt& J0, const HalfInt& J,
  //   spncci::SpaceU3S& u3s_space,
  //   const std::vector<spncci::SectorLabelsU3S>& source_sector_labels,
  //   basis::MatrixVector& source_sectors,
  //   const spncci::SpaceLS& target_space,
  //   std::vector<spncci::SectorLabelsLS>& target_sector_labels,
  //   basis::MatrixVector& target_sectors
  //   )
  // {
  //   // For a given Jp,J0,J sector
  //   // spncci::SpaceLS ls_space(u3s_space, J);
  //   //for each target, get sources and multiply by appropriate coefficient.
  //   target_sectors.resize(target_sector_labels.size());
  //   for(int t=0; t<target_sector_labels.size(); ++t)
  //     {
  //       const spncci::SectorLabelsLS& sector_labels=target_sector_labels[t];
  //       const spncci::SubspaceLS& 
  //         ket_subspace=target_space.GetSubspace(sector_labels.ket_index());
  //       const spncci::SubspaceLS& 
  //         bra_subspace=target_space.GetSubspace(sector_labels.bra_index());
        
  //       int target_dim_bra=bra_subspace.sector_dim();
  //       int target_dim_ket=ket_subspace.sector_dim();
  //       // Zero initialize
  //       Eigen::MatrixXd& target_sector=target_sectors[t];
  //       target_sector=Eigen::MatrixXd::Zero(target_dim_bra,target_dim_ket);
       
  //       // Extract target labels 
  //       int L0(sector_labels.L0());
  //       HalfInt S0(sector_labels.S0());
        
  //       int L, Lp;
  //       HalfInt S,Sp;

  //       std::tie(Lp,Sp)=bra_subspace.GetSubspaceLabels();
  //       std::tie(L,S)=ket_subspace.GetSubspaceLabels();

  //       double Jcoef=am::Unitary9J(L,S,J,L0,S0,J0,Lp,Sp,Jp);
  //       // std::cout<<fmt::format("{} {} {}  {} {} {}  {} {} {}    {}",L,S,J,L0,S0,J0,Lp,Sp,Jp,Jcoef)<<std::endl;
        
  //       for(int s=0; s<source_sector_labels.size(); ++s)
  //         {
  //           const spncci::SectorLabelsU3S& source_labels=source_sector_labels[s];
  //           Eigen::MatrixXd& source_sector=source_sectors[s];

  //           if(L0!=source_labels.L0())
  //             continue;
            
  //           int source_index_ket=source_labels.ket_index();
  //           int source_index_bra=source_labels.bra_index();
            
  //           // Check if sector is source for target sector
  //           if(not bra_subspace.ContainsState(source_index_bra))
  //             continue;
  //           if(not ket_subspace.ContainsState(source_index_ket))
  //             continue;

  //           // (indexp,index)->position of upper left corner of subsector
  //           int indexp=bra_subspace.sector_index(bra_subspace.LookUpStateIndex(source_index_bra));
  //           int index=ket_subspace.sector_index(ket_subspace.LookUpStateIndex(source_index_ket));

  //           //Extract source operator labels 
  //           const u3::SU3& x0=source_labels.x0();
  //           const HalfInt& S0=source_labels.S0();
  //           int kappa0=source_labels.kappa0();
  //           int rho0=source_labels.rho0();

  //           const spncci::SubspaceU3S& 
  //             u3s_subspace_bra=u3s_space.GetSubspace(source_index_bra);
  //           const spncci::SubspaceU3S& 
  //             u3s_subspace_ket=u3s_space.GetSubspace(source_index_ket);

  //           //source sector dimensions 
  //           int source_dimp=u3s_subspace_bra.sector_dim();
  //           int source_dim=u3s_subspace_ket.sector_dim();

  //           // Extract source state labels 
  //           const u3::U3S& omegaSp=u3s_subspace_bra.GetSubspaceLabels();
  //           const u3::U3S& omegaS=u3s_subspace_ket.GetSubspaceLabels();
  //           assert(omegaSp.S()==Sp);
  //           assert(omegaS.S()==S);
  //           u3::SU3 xp(omegaSp.U3().SU3());
  //           u3::SU3 x(omegaS.U3().SU3());

  //           // Get branching multiplicity
  //           int kappa_max_p=u3::BranchingMultiplicitySO3(xp,Lp);
  //           int kappa_max=u3::BranchingMultiplicitySO3(x,L);
  //           for(int kappa_p=1; kappa_p<=kappa_max_p; ++kappa_p)
  //             for(int kappa=1; kappa<=kappa_max; ++kappa)
  //               {
  //                 // Generate coefficient for each kappa and kappa_p and accumulate in
  //                 // target sector. Source sector dimensions are source_dimp x source_dim
  //                 // starting position given by :
  //                 //    ((kappa_p-1)*source_dimp+indexp, (kappa-1)*source_dim+index) 
  //                 double Wcoef=u3::W(x,kappa,L,x0,kappa0,L0,xp,kappa_p,Lp,rho0);
  //                 int start_indexp=(kappa_p-1)*source_dimp+indexp;
  //                 int start_index=(kappa-1)*source_dim+index;
  //                 target_sector.block(start_indexp,start_index,source_dimp,source_dim)+=Jcoef*Wcoef*source_sector;
  //               }
  //         }
  //     }
  // }

  // void
  // ConstructOperatorMatrix(
  //   const spncci::SpaceLS& source_space,
  //   std::vector<spncci::SectorLabelsLS>& source_sector_labels,
  //   basis::MatrixVector& source_sectors,
  //   Eigen::MatrixXd& operator_matrix
    
  //   )
  // {
  //   // Get size of matrix
  //   // Generate look up table for sector indices
  //   int index=0;
  //   std::map<int,int> matrix_index_lookup;
  //   for(int s=0; s<source_space.size(); ++s)
  //     {
  //       const spncci::SubspaceLS& subspace=source_space.GetSubspace(s);
  //       int sector_dim=subspace.sector_dim();
  //       matrix_index_lookup[s]=index;
  //       index+=sector_dim;
  //     }
  //   int matrix_dim=index;
  //   operator_matrix=Eigen::MatrixXd::Zero(matrix_dim,matrix_dim);
    
  //   // For each sector in source sectors, get bra and ket indices, look-up matrix index
  //   // and accumulate in full matrix 
  //   for(int s=0; s<source_sectors.size(); ++s)
  //     {
  //       const spncci::SectorLabelsLS& sector_labels=source_sector_labels[s];
  //       int subspace_index_bra=sector_labels.bra_index();
  //       int subspace_index_ket=sector_labels.ket_index();

  //       int matrix_index_bra=matrix_index_lookup[subspace_index_bra];
  //       int matrix_index_ket=matrix_index_lookup[subspace_index_ket];

  //       Eigen::MatrixXd& sector=source_sectors[s];
  //       operator_matrix.block(matrix_index_bra,matrix_index_ket,sector.rows(),sector.cols())
  //         +=sector;
  //     }
  // }


  void 
  ConstructSpNCCIBasisExplicit(
    const spncci::SpNCCISpace& sp_irrep_vector,
    basis::MatrixVector& lgi_expansion_matrix_vector,
    const u3shell::SpaceU3SPN& space,
    const u3shell::SectorsU3SPN& sectors,
    const u3shell::SectorsU3SPN& arel_sectors,
    basis::MatrixVector& Arel_matrices,
    std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>>& k_matrix_map
  )
  {}


  void
  ComputeUnitTensorSectorsExplicit(
    const spncci::SpNCCISpace& sp_irrep_vector,
    basis::MatrixVector& lgi_expansion_matrix_vector,
    const u3shell::SpaceU3SPN& space,
    const u3shell::SectorsU3SPN& sectors,
    const u3shell::SectorsU3SPN& arel_sectors,
    basis::MatrixVector& Arel_matrices,
    std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>>& k_matrix_map,
    const u3shell::RelativeUnitTensorLabelsU3ST& unit_tensor,
    basis::MatrixVector& lsu3shell_operator_matrices,
    spncci::LGIUnitTensorSectorCache& unit_tensor_sector_cache_explicit,
    double zero_threshold
  )
  // Compute unit tensor sectors using SpNCCI basis states explicityly constructed in terms of 
  // lsu3shell states. 
  //
  // Arguemnts:
  //  
  {
    // REMOVE 
    int d=0;
    // Get operator labels 
    for(int m=0; m<sp_irrep_vector.size(); ++m)
      for(int n=0; n<sp_irrep_vector.size(); ++n)
        {
          // Get lgi expansion 
          std::pair<int,int> lgi_pair(m,n);
          u3shell::U3SPN bra_lgi_labels=sp_irrep_vector[m];
          u3shell::U3SPN ket_lgi_labels=sp_irrep_vector[n];

          if(not am::AllowedTriangle(ket_lgi_labels.S(),unit_tensor.S0(), bra_lgi_labels.S()))
            continue;

          const HalfInt& Nsigmap=bra_lgi_labels.N();
          const HalfInt& Nsigma=ket_lgi_labels.N();
          Eigen::MatrixXd& bra_lgi_expansion=lgi_expansion_matrix_vector[m];
          Eigen::MatrixXd& ket_lgi_expansion=lgi_expansion_matrix_vector[n];
          // Act A on bra lgi
          for(int s=0; s<sectors.size(); ++s)
            {
              auto& sector=sectors.GetSector(s);
              int bra_index=sector.bra_subspace_index();
              int ket_index=sector.ket_subspace_index();
              u3shell::U3SPN ket_subspace_labels=space.GetSubspace(ket_index).GetSubspaceLabels();
              u3shell::U3SPN bra_subspace_labels=space.GetSubspace(bra_index).GetSubspaceLabels();
              u3::U3 omega=ket_subspace_labels.U3();
              u3::U3 omegap=bra_subspace_labels.U3();
              HalfInt S=ket_subspace_labels.S();
              HalfInt Sp=bra_subspace_labels.S();
              // Checking if omegaS and omegapS are contained in irreps in question
              if(
                  (omegap.N()<Nsigmap)
                  ||(Sp!=bra_lgi_labels.S())
                  ||(bra_subspace_labels.Sp()!=bra_lgi_labels.Sp())
                  ||(bra_subspace_labels.Sn()!=bra_lgi_labels.Sn())
                  ||(omega.N()<Nsigma)
                  ||(ket_subspace_labels.Sp()!=ket_lgi_labels.Sp())
                  ||(ket_subspace_labels.Sn()!=ket_lgi_labels.Sn())
                  ||(ket_subspace_labels.S()!=ket_lgi_labels.S())
                )
                continue;

              if((omegap.N()==Nsigmap)&&(not (bra_subspace_labels==bra_lgi_labels)))
                continue;

              if((omega.N()==Nsigma)&&(not (ket_subspace_labels==ket_lgi_labels)))
                continue;


              int rho0=sector.multiplicity_index();
              spncci::UnitTensorU3Sector unit_U3Sector(omegap,omega,unit_tensor,rho0);
              std::pair<int,int> NnpNn(int(omegap.N()-Nsigmap),int(omega.N()-Nsigma));

              Eigen::MatrixXd unit_rmes;
              Eigen::MatrixXd& operator_matrix=lsu3shell_operator_matrices[s];
              
              // Act A on ket LGI
              if(Nsigmap==(Nsigma+unit_tensor.N0()+2))
                {
                  
                  int arel_index=arel_sectors.LookUpSectorIndex(ket_index,n,1);
                  // Check if there is a valid arel sector that will yield the omegap
                  // subspace
                  if(arel_index==-1)
                    if(u3::OuterMultiplicity(ket_lgi_labels.U3().SU3(),u3::SU3(2,0),omega.SU3()))
                      {
                        std::cout<<unit_tensor.Str()<<std::endl;
                        std::cout<<bra_lgi_labels.Str()<<"  "<<omegap.Str()<<Sp<<"  "<<ket_lgi_labels.Str()<<"  "<<omega.Str()
                          <<S<<"  "<<arel_index<<std::endl;
                        std::cout<<ket_index<<"  "<<n<<std::endl;
                      }
                  if(arel_index!=-1)
                    {
                      Eigen::MatrixXd Kmatrix_inverse=k_matrix_map[ket_lgi_labels.U3()][omega].inverse();
                      double k_inverse=Kmatrix_inverse(0,0);
                      Eigen::MatrixXd omega_expansion
                      =ParitySign(u3::ConjugationGrade(omega.SU3())+u3::ConjugationGrade(ket_lgi_labels.U3().SU3()))
                        *k_inverse*Arel_matrices[arel_index]*ket_lgi_expansion;
                      // std::cout<<"krme "<<krme<<std::endl;
                      unit_rmes
                        =bra_lgi_expansion.transpose()*operator_matrix*omega_expansion;
                    }
                }
              // Act A on both lgi
              if(Nsigmap==(Nsigma+unit_tensor.N0()))
                {

                  int arel_index_bra=arel_sectors.LookUpSectorIndex(bra_index,m,1);
                  int arel_index_ket=arel_sectors.LookUpSectorIndex(ket_index,n,1);
    
                  // if((arel_index_bra==-1)&&(arel_index_ket==-1))
                  //   if(u3::OuterMultiplicity(ket_lgi_labels.U3().SU3(),u3::SU3(2,0),omega.SU3()))
                  //     if(u3::OuterMultiplicity(bra_lgi_labels.U3().SU3(),u3::SU3(2,0),omegap.SU3()))
                  //       {
                  //         std::cout<<unit_tensor.Str()<<std::endl;
                  //         std::cout<<bra_lgi_labels.Str()<<"  "<<omegap.Str()<<Sp<<"  "<<ket_lgi_labels.Str()
                  //         <<"  "<<omega.Str()<<S<<"  "<<arel_index_bra<<"  "<<arel_index_ket<<std::endl;
                  //         std::cout<<bra_index<<"   "<<m<<"  "<<ket_index<<"  "<<n<<std::endl;
                  //       }

                  if((arel_index_bra!=-1)&&(arel_index_ket!=-1))
                    {
                      Eigen::MatrixXd Kmatrixp_inverse=k_matrix_map[bra_lgi_labels.U3()][omegap].inverse();
                      double kp_inverse=Kmatrixp_inverse(0,0);

                      Eigen::MatrixXd Kmatrix_inverse=k_matrix_map[ket_lgi_labels.U3()][omega].inverse();
                      double k_inverse=Kmatrix_inverse(0,0);

                      Eigen::MatrixXd omegap_expansion
                      =ParitySign(u3::ConjugationGrade(omegap.SU3())+u3::ConjugationGrade(bra_lgi_labels.U3().SU3()))
                        *kp_inverse*Arel_matrices[arel_index_bra]*bra_lgi_expansion;
                      Eigen::MatrixXd omega_expansion
                      =ParitySign(u3::ConjugationGrade(omega.SU3())+u3::ConjugationGrade(ket_lgi_labels.U3().SU3()))
                        *k_inverse*Arel_matrices[arel_index_ket]*ket_lgi_expansion;
                      

                      unit_rmes=omegap_expansion.transpose()*operator_matrix*omega_expansion;
                      // if(m==0 && n==0 && d<2)
                      //   {
                      //     std::cout<<omegap.Str()<<"  "<<unit_tensor.Str()<<"  "<<omega.Str()<<std::endl;
                      //     std::cout<<"operator"<<std::endl;
                      //     std::cout<<operator_matrix<<std::endl<<std::endl;
                      //     std::cout<<"bra "<<std::endl;
                      //     std::cout<<(Arel_matrices[arel_index_bra]*bra_lgi_expansion).transpose()<<std::endl<<std::endl;
                      //     std::cout<<"ket "<<std::endl;
                      //     std::cout<<(Arel_matrices[arel_index_ket]*ket_lgi_expansion)<<std::endl<<std::endl;
                      //     std::cout<<"rme"<<std::endl;
                      //     std::cout<<(Arel_matrices[arel_index_bra]*bra_lgi_expansion).transpose()
                      //     *operator_matrix*Arel_matrices[arel_index_ket]*ket_lgi_expansion
                      //     <<std::endl<<"  "<<kp_inverse<<"  "<<k_inverse<<std::endl;
                      //     std::cout<<"unit tensors"<<std::endl;
                      //     std::cout<<unit_rmes<<std::endl<<std::endl;
                      //     ++d;
                      //   }
                    }
                }
              // Act A on ket lgi
              if(Nsigmap==(Nsigma+unit_tensor.N0()-2))
                {

                  int arel_index=arel_sectors.LookUpSectorIndex(bra_index,m,1);
                  if(arel_index!=-1)
                    {
                      Eigen::MatrixXd Kmatrix_inverse=k_matrix_map[bra_lgi_labels.U3()][omegap].inverse();
                      double k_inverse=Kmatrix_inverse(0,0);

                      Eigen::MatrixXd omegap_expansion
                      =ParitySign(u3::ConjugationGrade(omegap.SU3())+u3::ConjugationGrade(bra_lgi_labels.U3().SU3()))
                        *k_inverse*Arel_matrices[arel_index]*bra_lgi_expansion;

                      unit_rmes=omegap_expansion.transpose()
                                *operator_matrix
                                *ket_lgi_expansion;
                    }
                }
                  if(not CheckIfZeroMatrix(unit_rmes,zero_threshold))
                    // If nonzero, save unit tensor rmes to cache
                    unit_tensor_sector_cache_explicit[lgi_pair][NnpNn][unit_U3Sector]
                       =unit_rmes;      
            }
        }

  }
}// end namespace

int main(int argc, char **argv)
{
      // //REMOVE
      // int testb=3;
      // int testk=3;

  u3::U3CoefInit();
  //unit tensor cache 
  u3::UCoefCache u_coef_cache;
  u3::PhiCoefCache phi_coef_cache;

	u3::g_u_cache_enabled = true;
  double zero_threshold=1e-6;
  // For GenerateRelativeUnitTensors
  // should be consistant with lsu3shell tensors
  int T0=0;
  int J0=-1;
  // parse arguments
  if (argc<8)
    {
      std::cout << "Syntax: A twice_Nsigma0 Nsigma0_ex_max N1B Nmax <basis filename> <Nrel filename> <Brel filename> <Arel filename>" 
                << std::endl;
      std::exit(1);
    }
  int A = std::stoi(argv[1]); 
  int twice_Nsigma0= std::stoi(argv[2]);
  int Nsigma0_ex_max=std::stoi(argv[3]);
  int N1b=std::stoi(argv[4]);
  int Nmax = std::stoi(argv[5]);
  std::string lsu3_filename = argv[6];
  std::string nrel_filename = argv[7];
  std::string brel_filename = argv[8];
  std::string arel_filename = argv[9];

  HalfInt Nsigma_0=HalfInt(twice_Nsigma0,2);
  //initializing map that will store map containing unit tensors {SpIrrep pair : {}  
  // inner map is keyed by unit tensor matrix element labels of type UnitTensorRME
  // SpIrrep pair -> NnpNn -> UnitTensorRME -> v'v subsector
  spncci::LGIUnitTensorSectorCache unit_tensor_sector_cache;
  // Texting cache for rme's computed from explicit basis construction
  spncci::LGIUnitTensorSectorCache unit_tensor_sector_cache_explicit;

  //////////////////////////////////////////////////////////////////////////////////////////
  // Get LGI's and populate sector map with LGI rme's
  //////////////////////////////////////////////////////////////////////////////////////////
  // Generating LGI matrix elements 
  lsu3shell::LSU3BasisTable basis_table;
  lsu3shell::U3SPNBasisLSU3Labels basis_provenance;
  u3shell::SpaceU3SPN space;
  // Read in lsu3shell basis 
  lsu3shell::ReadLSU3Basis(Nsigma_0,lsu3_filename, basis_table, basis_provenance, space);

  // // Writing out lsu3shell basis
  // std::cout<<"lsu3shell basis"<<std::endl;
  // for(int subspace_index=0; subspace_index<space.size(); ++subspace_index)
  //   {
  //     const u3shell::SubspaceU3SPN& subspace = space.GetSubspace(subspace_index);
  //     std::cout
  //       << fmt::format("subspace {} labels {} dim {}",
  //                      subspace_index,
  //                      subspace.U3SPN().Str(),
  //                      subspace.size()
  //         )
  //       << std::endl;
  //   }

  //Get LGI expansion by solving for null space of Brel and Ncm
  std::ifstream is_brel(brel_filename.c_str());
  std::ifstream is_nrel(nrel_filename.c_str());
  lgi::MultiplicityTaggedLGIVector lgi_vector;
  basis::MatrixVector lgi_expansion_matrix_vector;
  lgi::GenerateLGIExpansion(A,Nsigma_0,basis_table,space, is_brel,is_nrel,lgi_vector,lgi_expansion_matrix_vector);
  is_brel.close();
  is_nrel.close();


  // Read in Arel for unit tensor rme check
  // TODO: comment out once rme's are validated
  basis::MatrixVector Arel_matrices;
  u3shell::OperatorLabelsU3ST arel_labels(2,u3::SU3(2,0),0,0,0);
  u3shell::SectorsU3SPN arel_sectors(space,arel_labels,true);

  std::ifstream is_arel(arel_filename.c_str());
  if(not is_arel)
    std::cout<<fmt::format("{} not found",arel_filename)<<std::endl;

  lsu3shell::ReadLSU3ShellRMEs(
    is_arel,arel_labels,basis_table,space,
    arel_sectors,Arel_matrices);
  is_arel.close();

  // for(auto arel : Arel_matrices)
  //   std::cout<<arel<<std::endl<<std::endl;

  // Extract LGI labels and store in lgi_vector
  // lgi::GetLGILabels(Nsigma_0,space,lgi_expansion_matrix_vector, lgi_vector);
  int i=0;
  for(auto lgi_tagged : lgi_vector)
  {
    std::cout<<i<<"  "<<lgi_tagged.Str()<<"  "<<lgi_tagged.tag<<std::endl;
    ++i;
  }

  // Setting up the symplectic basis containers
  spncci::SigmaIrrepMap sigma_irrep_map;
  spncci::SpNCCISpace sp_irrep_vector;
  spncci::NmaxTruncator truncator(Nsigma_0,Nmax);
  // Generating sp3r irreps
  spncci::GenerateSpNCCISpace(lgi_vector,truncator,sp_irrep_vector,sigma_irrep_map);

  //////////////////////////////////////////////////////////////////////////////////////////
  //Precomputing Kmatrices 
  //////////////////////////////////////////////////////////////////////////////////////////
  std::unordered_set<u3::U3,boost::hash<u3::U3> >sigma_set;
  for(int l=0; l<sp_irrep_vector.size(); l++)
    {
      sigma_set.insert(sp_irrep_vector[l].sigma());
    }
  std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>> k_matrix_map;
  for( const auto& s : sigma_set)
    {
      vcs::MatrixCache& k_map=k_matrix_map[s];
      // int Nex=int(s.N()-Nsigma_0);
      // vcs::GenerateKMatricesOpenMP(sigma_irrep_map[s], k_map);
      vcs::GenerateKMatrices(sigma_irrep_map[s], k_map);

      std::cout<<"kmap "<<s.Str()<<std::endl;
      for(auto it=k_map.begin(); it!=k_map.end(); ++it)
        std::cout<<it->first.Str()<<"  "<<it->second<<std::endl;
    }

  // Labels of relative unit tensors computed between LGI's
  std::vector<u3shell::RelativeUnitTensorLabelsU3ST> LGI_unit_tensor_labels;  
  std::cout<<"Nsigma0_ex_max "<<Nsigma0_ex_max<<std::endl;
  // -1 is all J0, T0=0 and false->don't restrict N0 to positive (temp)
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(Nsigma0_ex_max+2*N1b, LGI_unit_tensor_labels,J0,T0,false);

  // For each operator, transform from lsu3shell basis to spncci basis
  std::cout<<"Number of unit tensors "<<LGI_unit_tensor_labels.size()<<std::endl;
  
  for(int i=0; i<LGI_unit_tensor_labels.size(); ++i)
    {
      //Get unit tensor labels
      u3shell::RelativeUnitTensorLabelsU3ST unit_tensor(LGI_unit_tensor_labels[i]);
      // std::cout<<"unit tensor "<<i<<"  "<<unit_tensor.Str()<<std::endl;
      u3shell::OperatorLabelsU3ST operator_labels(unit_tensor);
      // Generate sectors from labels
      u3shell::SectorsU3SPN operator_sectors(space,operator_labels,false);
      // Read in lsu3shell rme's of unit tensor
      std::ifstream is_operator(fmt::format("relative_unit_{:06d}.rme",i));
      // If operator is not found, print name and continue;
      if(not is_operator)
        {
          std::cout<<fmt::format("relative_unit_{:06d}.rme not found",i)<<std::endl;
          continue;
        }

      // Read in operator from file
      // std::cout<<fmt::format("Transforming relative_unit_{:06d}.rme",i)<<std::endl;
      // std::cout<< LGI_unit_tensor_labels[i].Str()<<std::endl;
      // std::cout<<"  Reading in rmes"<<std::endl;
      basis::MatrixVector lsu3shell_operator_matrices(operator_sectors.size());
      lsu3shell::ReadLSU3ShellRMEs(
        is_operator,operator_labels, basis_table,space, 
        operator_sectors,lsu3shell_operator_matrices
        );

      // Basis transformation for each sector
      // std::cout<<"  Transforming"<<std::endl;

      // for(auto matrix : lsu3shell_operator_matrices)
      //   std::cout<<matrix<<std::endl<<std::endl;

      basis::MatrixVector spncci_operator_matrices;
      lgi::TransformOperatorToSpBasis(
        operator_sectors,lgi_expansion_matrix_vector,
        lsu3shell_operator_matrices,spncci_operator_matrices
        );

      // explicit basis construction unit tensor matrix elements
      // for comparison with Sp-NCCI

      ComputeUnitTensorSectorsExplicit(
        sp_irrep_vector,lgi_expansion_matrix_vector,space, operator_sectors,
        arel_sectors,Arel_matrices,k_matrix_map,unit_tensor,
        lsu3shell_operator_matrices,unit_tensor_sector_cache_explicit,zero_threshold
      );

      // std::cout<<"traversing the explicit map"<<std::endl;
      //   for(auto it=unit_tensor_sector_cache_explicit.begin(); it!=unit_tensor_sector_cache_explicit.end(); ++it)
      //     {
      //       // if((it->first.first!=2)||(it->first.second!=5))
      //       //   continue;
      //       std::cout<<it->first.first<<"  "<<it->first.second<<std::endl;
      //       for(auto it2=it->second.begin(); it2!=it->second.end(); ++it2)
      //         {
      //           std::cout<<"  "<<it2->first.first<<"  "<<it2->first.second<<std::endl;
      //           for(auto it3=it2->second.begin(); it3!=it2->second.end();++ it3)
      //             {
      //               std::cout<<it3->first.Str()<<std::endl;
      //               std::cout<<it3->second<<std::endl;
      //             }
      //         }
      //     }

      // std::cout<<"lgi expansions"<<std::endl;
      // for(auto matrix :lgi_expansion_matrix_vector)
      //   std::cout<<matrix<<std::endl<<std::endl;

      // Populate unit tensor map with sectors
      // std::cout<<"Populating lgi sectors"<<std::endl;
      std::pair<int,int> N0_pair(0,0);

      for(int s=0; s<operator_sectors.size(); ++s)
        {
          int i=operator_sectors.GetSector(s).bra_subspace_index();
          int j=operator_sectors.GetSector(s).ket_subspace_index();
          int rho0=operator_sectors.GetSector(s).multiplicity_index();
          std::pair<int,int>sp_irrep_pair(i,j);
          u3::U3 sigmap=sp_irrep_vector[i].sigma();
          u3::U3 sigma=sp_irrep_vector[j].sigma();
          spncci::UnitTensorU3Sector u3_sector(sigmap,sigma,unit_tensor,rho0);
          if(not CheckIfZeroMatrix(spncci_operator_matrices[s],zero_threshold))
            {
              unit_tensor_sector_cache[sp_irrep_pair][N0_pair][u3_sector]
                  =spncci_operator_matrices[s];
              
              unit_tensor_sector_cache_explicit[sp_irrep_pair][N0_pair][u3_sector]
                  =spncci_operator_matrices[s];
              
              // if((i==0) && (j==0))
              // {
              // std::cout<<i<<"  "<<j<<std::endl;
              // std::cout<<u3_sector.Str()<<std::endl;
              // std::cout<<spncci_operator_matrices[s]<<std::endl;
              // }
            }
        }
    }

  // for(auto it=unit_tensor_sector_cache.begin(); it!=unit_tensor_sector_cache.end(); ++it)
  //   {
  //     std::cout<<"Irreps "<<it->first.first<<" and "<<it->first.second<<std::endl;
  //   }

  // std::cout<<"traversing the map"<<std::endl;
  // auto it_ending=unit_tensor_sector_cache.begin();
  // for(int i=0; i<unit_tensor_sector_cache.size(); ++i) 
  //   it_ending++;

  // for(auto it=unit_tensor_sector_cache.begin(); it!=it_ending; ++it)
  //   {

  //     // //REMOVE
  //     // if(it->first.first!=0 || it->first.second!=3)
  //     //   continue;
  //     // //

  //     std::cout<<it->first.first<<"  "<<it->first.second<<std::endl;

  //     for(auto it2=it->second.begin(); it2!=it->second.end(); ++it2)
  //       {
  //         std::cout<<"  "<<it2->first.first<<"  "<<it2->first.second<<std::endl;
  //         for(auto it3=it2->second.begin(); it3!=it2->second.end();++ it3)
  //           {
  //             std::cout<<it3->first.Str()<<std::endl;
  //             std::cout<<it3->second<<std::endl;
  //           }
  //       }
  //   }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Computing unit tensor sectors
  //////////////////////////////////////////////////////////////////////////////////////////
  // if true, unit tensors restricted to N0>=0
  // Nrel_max=Nmax+2N1b

  std::map<int,std::vector<u3shell::RelativeUnitTensorLabelsU3ST>> unit_tensor_labels;
  u3shell::GenerateRelativeUnitTensorLabelsU3ST(Nmax+2*N1b, unit_tensor_labels, J0,T0, false);
  if(Nmax!=0)
  {
    std::map<int,double> timing_map;
    std::map<int,std::vector<std::pair<int,int>>> lgi_distribution;
    std::map<int,std::vector<int>>  sector_count_map;
    std::map<int,std::vector<double>> individual_times;
    std::map<int,u3::UCoefCache> u_cache_map;
    std::pair<int,int> N0_pair(0,0);
    int num_nodes=1;
    int counter=0; 
    for(int n=0; n<num_nodes; ++n)
      timing_map[n]=0;

    for(auto it=unit_tensor_sector_cache.begin(); it!=unit_tensor_sector_cache.end(); ++it)
    {
      clock_t start_time=std::clock();

      //REMOVE
      // if(it->first.first!=33 || it->first.second!=3)
      //   continue;
      //

      int node=counter%num_nodes;
      assert(node<num_nodes);
      //Timing data
      spncci::GenerateUnitTensorMatrix(
        N1b,Nmax,it->first,sp_irrep_vector,u_cache_map[node], phi_coef_cache,k_matrix_map,
        unit_tensor_labels,unit_tensor_sector_cache);

      double duration=(std::clock()-start_time)/(double) CLOCKS_PER_SEC;
      int num_sector=unit_tensor_sector_cache[it->first][N0_pair].size();

      timing_map[node]+=duration;
      individual_times[node].push_back(duration);
      lgi_distribution[node].push_back(it->first);
      sector_count_map[node].push_back(num_sector);
      counter++;

    }
    // std::cout<<"individual pairs"<<std::endl;
    // for(int n=0; n<num_nodes; ++n)
    //   {
    //     std::cout<<"node"<<n<<std::endl;
    //     int i_stop=lgi_distribution[n].size();
    //     for(int i=0; i<i_stop; ++i)
    //       {
    //         std::cout<<lgi_distribution[n][i].first<<" "<<lgi_distribution[n][i].second<<"  "
    //         <<sector_count_map[n][i]<<"  "<<individual_times[n][i]<<std::endl;
    //       }
    //     std::cout<<"  "<<std::endl;

    //   }

    // std::cout<<"summarizing"<<std::endl;
    // for(int n=0; n<num_nodes; ++n)
    //   {
    //     std::cout<<"node "<<n<<std::endl;
    //     std::cout<<" time "<<timing_map[n]<<std::endl;
    //     std::cout<<" U cache size "<<u_cache_map[n].size()<<std::endl;
    //   } 
  }
  bool assert_trap=true;
  std::cout<<"traversing the map"<<std::endl;
  for(auto it=unit_tensor_sector_cache.begin(); it!=unit_tensor_sector_cache.end(); ++it)
    {
      // std::cout<<it->first.first<<"  "<<it->first.second<<std::endl;
      for(auto it2=it->second.begin(); it2!=it->second.end(); ++it2)
        {
          // if(it2->first.first!=2)
          //   continue;
          // if(it2->first.second!=2)
          //   continue;
          // // std::cout<<"  "<<it2->first.first<<"  "<<it2->first.second<<std::endl;
          auto& explicit_cache=unit_tensor_sector_cache_explicit[it->first][it2->first];
          for(auto it3=it2->second.begin(); it3!=it2->second.end();++ it3)
            {
              assert(assert_trap);
              // std::cout<<it3->first.Str()<<std::endl;
              int rho0;
              u3shell::RelativeUnitTensorLabelsU3ST tensor;
              std::tie(std::ignore,std::ignore,tensor,rho0)=it3->first.Key();
              // if(tensor.S0()!=1)
              //   continue;
              if(explicit_cache.count(it3->first))
              {
                Eigen::MatrixXd& recurrence_matrix=it3->second;
                
                Eigen::MatrixXd& explicit_matrix=explicit_cache[it3->first];
                if(not mcutils::IsZero(recurrence_matrix-explicit_matrix,1e-3))
                {
                  std::cout<<it->first.first<<"  "<<it->first.second<<std::endl;
                  std::cout<<"  "<<it2->first.first<<"  "<<it2->first.second<<std::endl;
                  std::cout<<it3->first.Str()<<std::endl;
                  // std::cout<<"recurrence"<<std::endl;
                  // std::cout<<recurrence_matrix<<std::endl<<std::endl;
                  // std::cout<<"explicit"<<std::endl;
                  // std::cout<<explicit_matrix<<std::endl<<std::endl;
                  
                  Eigen::MatrixXd ratios(recurrence_matrix.rows(),recurrence_matrix.cols());
                  for(int i=0; i<recurrence_matrix.rows(); ++i)
                    for(int j=0; j<recurrence_matrix.cols(); ++j)
                      ratios(i,j)=recurrence_matrix(i,j)/explicit_matrix(i,j);
                  std::cout<<ratios<<std::endl<<std::endl;

                  // assert_trap=false;
                }
              }
              // else 
              // {
              //   if(mcutils::IsZero(it3->second,1e-3))
              //     continue;
              //   std::cout<<"Explicit sector not found "<<std::endl;
              //   std::cout<<it->first.first<<"  "<<it->first.second<<std::endl;
              //   std::cout<<"  "<<it2->first.first<<"  "<<it2->first.second<<std::endl;
              //   std::cout<<"     "<<it3->first.Str()<<std::endl;
              //   std::cout<<"     "<<it3->second<<std::endl<<std::endl;
              // }
            }
        }
     }

  //////////////////////////////////////////////////////////////////////////////////////////
  // // Getting interaction
  // //////////////////////////////////////////////////////////////////////////////////////////
  // //TODO make input 
  std::string interaction_file="trel_SU3_Nmax06.dat";
  // std::string interaction_file= "Trel_upcouled";
  // std::string interaction_file="id_SU3_Nmax16.dat";
  // std::string interaction_file="unit.dat";
  std::ifstream interaction_stream(interaction_file.c_str());
  assert(interaction_stream);
  
  u3shell::RelativeRMEsU3ST interaction_rme_cache;
  u3shell::ReadRelativeOperatorU3ST(interaction_stream, interaction_rme_cache);

  // for(auto it=interaction_rme_cache.begin(); it!=interaction_rme_cache.end(); ++it)
  //   std::cout<<it->second<<std::endl;

  std::vector<u3shell::IndexedOperatorLabelsU3S> operator_u3s_list;
  u3shell::GetInteractionTensorsU3S(interaction_rme_cache,operator_u3s_list);

  // Get U3S space 
  spncci::SpaceU3S u3s_space(sp_irrep_vector);
  // Storage for sectors, value gives sector index
  
  // spncci::SectorLabelsU3SCache u3s_sectors;
  std::vector<spncci::SectorLabelsU3S> u3s_sector_vector;
  // for(auto tensor : operator_u3s_list)
  //   {
  //     int kappa0, L0;
  //     u3shell::OperatorLabelsU3S tensor_u3st;
  //     std::tie(tensor_u3st,kappa0,L0)=tensor;
  //     std::cout<<tensor_u3st.Str()<<"  "<<kappa0<<"  "<<L0<<std::endl;
  //   }

  spncci::GetSectorsU3S(u3s_space,operator_u3s_list,u3s_sector_vector);

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Contracting
  //////////////////////////////////////////////////////////////////////////////////////////////
  basis::MatrixVector matrix_vector;
  basis::MatrixVector matrix_vector_explicit;
  
  spncci::BabySpNCCISpace baby_spncci_space(sp_irrep_vector);
  
  spncci::ContractAndRegroupU3S(
    Nmax, N1b,u3s_sector_vector,interaction_rme_cache,baby_spncci_space,
    u3s_space,unit_tensor_sector_cache, matrix_vector);

  spncci::ContractAndRegroupU3S(
    Nmax, N1b,u3s_sector_vector,interaction_rme_cache,baby_spncci_space,
    u3s_space,unit_tensor_sector_cache_explicit, matrix_vector_explicit);


  ZeroOutMatrix(matrix_vector,1e-4);
  ZeroOutMatrix(matrix_vector_explicit,1e-4);
  
  // basis::MatrixVector difference_vector;
  // for(int i=0; i<matrix_vector.size();++i)
  //   difference_vector.push_back(matrix_vector_explicit[i]-matrix_vector[i]);
  // ZeroOutMatrix(difference_vector,1e-4);

  std::cout<<"printing"<<std::endl;
  for(int i=0; i<u3s_space.size(); ++i)
    std::cout<<i<<"  "<<u3s_space.GetSubspace(i).GetSubspaceLabels().Str()<<std::endl;
  for(int s=0; s<matrix_vector.size();  ++s)
    {
      if (not CheckIfZeroMatrix(matrix_vector[s], 1e-4))
      {
        std::cout<<u3s_sector_vector[s].Str()<<std::endl;

        std::cout<<matrix_vector[s]<<std::endl<<std::endl;
        // Eigen::MatrixXd difference=matrix_vector_explicit[s]-matrix_vector[s];
        
        // std::cout<<matrix_vector[s]<<std::endl<<std::endl;
        // std::cout<<matrix_vector_explicit[s]<<std::endl<<std::endl;

        // std::cout<<difference_vector[s]<<std::endl<<std::endl;
      }
    }
  HalfInt J=1;
  J0=0;
  spncci::SpaceLS space_LS(u3s_space,J);
  std::cout<<"target_space size "<<space_LS.size()<<std::endl;
  basis::MatrixVector sectors_LS;

  std::vector<spncci::OperatorLabelsLS> tensor_labels_LS;
  spncci::GenerateOperatorLabelsLS(J0, tensor_labels_LS);
  std::vector<spncci::SectorLabelsLS> sector_labels_LS;
  for(auto tensor_labels : tensor_labels_LS)
    std::cout<<"tensor "<<tensor_labels.first<<"  "<<tensor_labels.second<<std::endl;
  GetSectorsLS(space_LS, tensor_labels_LS,sector_labels_LS);

  // std::cout<<"sectors"<<std::endl;
  // for(auto sector :target_sector_labels_LS)
  //   std::cout<<sector.Str()<<std::endl;

  spncci::ContractAndRegroupLSJ(
    J,J0,J,
    u3s_space,u3s_sector_vector,matrix_vector,
    space_LS,sector_labels_LS,sectors_LS
  );

  // ZeroOutMatrix(sectors_LS,1e-4);
  // std::cout<<"printing LS"<<std::endl;
  // for(int i=0; i<space_LS.size(); ++i)
  //   std::cout<<i<<"  "<<space_LS.GetSubspace(i).Str()<<std::endl;
    
  // for(int s=0; s<sectors_LS.size();  ++s)
  //   {
  //     if (not CheckIfZeroMatrix(sectors_LS[s], 1e-4))
  //     {
  //       std::cout<<sector_labels_LS[s].Str()<<std::endl;
  //       std::cout<<sectors_LS[s]<<std::endl<<std::endl;
  //     }
  //   }

  Eigen::MatrixXd operator_matrix;

  ConstructOperatorMatrix(space_LS,sector_labels_LS,
    sectors_LS,operator_matrix);

  // for(auto it=unit_tensor_labels.begin(); it!=unit_tensor_labels.end(); ++it)
  //   for(auto tensor : it->second)
  //     std::cout<<tensor.Str()<<std::endl;
  // std::cout<<operator_matrix<<std::endl;
}
