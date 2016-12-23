/****************************************************************
  unit_tensor.cpp

  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

****************************************************************/
#include <omp.h>

#include "sp3rlib/u3coef.h"
#include "sp3rlib/vcs.h"
#include "spncci/unit_tensor.h"

namespace spncci
{
  typedef std::pair<UnitTensorU3Sector, Eigen::MatrixXd> UnitTensorU3SectorPair;

  std::string UnitTensorU3Sector::Str() const
  {
    std::ostringstream ss;
    ss << omegap_.Str() << " " << omega_.Str() << " " << tensor_.Str() << " " << rho0_;
    return ss.str();
  }

  std::pair<int,int> GetNSectorIndices(const int Nmax, const int irrep_size, const int Nn, std::vector<int>& NPartition)
  // 
  {
    int i_min, i_max;
    // Starting index
    if (Nn==0)
      i_min=0;
    else
      i_min=NPartition[Nn/2];
    // Ending index+1.   
    if (NPartition.size()==(Nn+2)/2)
      i_max=irrep_size-1;
    else 
      i_max=NPartition[(Nn+2)/2]-1;
    return std::pair<int,int>(i_min,i_max);
  }

  ////////////////////////////////////////////////////////////////////////////////////
  void 
  GenerateUnitTensorU3SectorLabels(
    int N1b,
    int Nmax,
    std::pair<int,int>  sp_irrep_pair,
    const spncci::SpIrrepVector& sp_irrep_vector,
    std::map< int,std::vector<u3shell::RelativeUnitTensorLabelsU3ST>>& unit_tensor_labels_map,
    std::map<std::pair<int,int>,std::vector<spncci::UnitTensorU3Sector>>& unit_tensor_NpN_sector_map
    )
  {   
    #ifdef VERBOSE
    std::cout<<"Entering GenerateU3SectorLabels"<<std::endl;
    #endif


    // Extracting SpIrrep labels from pair
    const spncci::SpIrrep& sp_irrepp=sp_irrep_vector[sp_irrep_pair.first].irrep;
    const spncci::SpIrrep& sp_irrep=sp_irrep_vector[sp_irrep_pair.second].irrep;
    u3::U3 sigmap=sp_irrepp.sigma();
    u3::U3 sigma=sp_irrep.sigma(); 

    const sp3r::Sp3RSpace& irrepp=sp_irrepp.Sp3RSpace();
    const sp3r::Sp3RSpace& irrep=sp_irrep.Sp3RSpace();

    int irrep_size=irrep.size();
    int irrepp_size=irrepp.size();

    // partition irreps by Nn and Nnp.  Each int in vector corresponds to the start of the next N space 
    std::vector<int> NpPartition=sp3r::PartitionIrrepByNn(irrepp, Nmax);
    std::vector<int> NPartition=sp3r::PartitionIrrepByNn(irrep, Nmax);
    ////////////////////////////////////////////////////////////////////////////////////
    // Looping over omega' and omega subspaces 
    ////////////////////////////////////////////////////////////////////////////////////
    // Loop over Nnp+Nn starting from 2, (Nnp+Nn=0 accounted for elsewhere)
    std::cout<<"Loop over omega subspaces"<<std::endl;
    for (int Nsum=2; Nsum<=2*Nmax; Nsum+=2)
      for (int Nnp=0; Nnp<=std::min(Nsum,Nmax); Nnp+=2)
        {
          int Nn=Nsum-Nnp;
          if((Nnp+sp_irrepp.Nex())>Nmax)
            continue;
          
          if ((Nn+sp_irrep.Nex())>Nmax)
            continue; 
          
          // Only computing unit tensors with N0>=0 
          // The rest are obtain by conjustation.
          int N0=sp_irrepp.Nex()+Nnp-sp_irrep.Nex()-Nn;
          if(N0<0)
            continue;

          std::pair<int,int> NpN_pair(Nnp,Nn);
          // Selecting section of spaces to iterate over
          int ip_min, ip_max, i_min, i_max;
          std::tie (i_min,i_max)=GetNSectorIndices(Nmax, irrep_size, Nn, NPartition);
          std::tie (ip_min,ip_max)=GetNSectorIndices(Nmax, irrepp_size, Nnp, NpPartition);
          // Get set of operator labels for given omega'omega sector
          // taking into account different Nsigmas 
          std::vector<u3shell::RelativeUnitTensorLabelsU3ST>& N0_operator_set
            =unit_tensor_labels_map[N0];
          // iterate over omega' subspace
          std::cout<<"loop through subspaces"<<std::endl;
          for(int ip=ip_min; ip<=ip_max; ip++ )
            {
              u3::U3 omegap=irrepp.GetSubspace(ip).GetSubspaceLabels();    
              // iterate over omega subspace
              for(int i=i_min; i<=i_max; i++ )
                {
                  u3::U3 omega=irrep.GetSubspace(i).GetSubspaceLabels();
                  // Iterating over the operator labels             
                  for (auto unit_tensor : N0_operator_set)
                    {                     
                      //unpack unit_tensor labels 
                      u3::SU3 x0(unit_tensor.x0());
                      HalfInt S0(unit_tensor.S0());
                      HalfInt T0(unit_tensor.T0());
                      int rbp=unit_tensor.bra().eta();
                      int rb=unit_tensor.ket().eta();

                      //Checking angular momentum constraint 
                      if (not am::AllowedTriangle(sp_irrep.S(),sp_irrepp.S(),S0))
                        continue;

                      // The max bosons a single relative particle can carry is 
                      // 2*max_one_body_value+num_excitation_quanta
                      if(rb>(2*N1b+Nn))
                        continue;
                      if(rbp>(2*N1b+Nnp))
                        continue;

                      int rho0_max=OuterMultiplicity(omega.SU3(),x0,omegap.SU3());
                      ///////////////////////////////////////////////////////////////////////////////////////
                      // Iterating over outer multiplicity
                      for (int rho0=1; rho0<=rho0_max; rho0++)
                        {                   
                          spncci::UnitTensorU3Sector unit_U3Sector(omegap,omega,unit_tensor,rho0);
                          unit_tensor_NpN_sector_map[NpN_pair].push_back(unit_U3Sector);
                        }
                    }
                }
            }
        }    

    #ifdef VERBOSE
    std::cout<<"Exiting GenerateU3SectorLabels"<<std::endl;
    #endif
  }

////////////////////////////////////////////////////////////////////////////////////
  Eigen::MatrixXd 
    UnitTensorMatrix(
    u3::UCoefCache& u_coef_cache,
    std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>> k_matrix_map,
     // SpIrrep pair sector 
    const spncci::SpIrrep& sp_irrepp,
    const spncci::SpIrrep& sp_irrep,
    const std::pair<int,int>& lgi_mult,
     // vector of addresses to relevant Np,N sectors of unit tensor matrix
     spncci::UnitTensorSectorsCache& sector_NpN2,
     spncci::UnitTensorSectorsCache& sector_NpN4,
     spncci::UnitTensorU3Sector unit_labels
     )
  {
    #ifdef VERBOSE
    std::cout<<"Entering UnitTensorMatrix"<<std::endl;
    #endif
    // v',v
    int N1b=2;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  Set up for calculation 
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Extracting labels 
    u3::U3 omegap, omega;
    u3shell::RelativeUnitTensorLabelsU3ST tensor;
    int rho0;
    std::tie(omegap,omega,tensor,rho0) = unit_labels.Key();
    int multp=lgi_mult.first;
    int mult=lgi_mult.second;

    const sp3r::Sp3RSpace& irrepp=sp_irrepp.Sp3RSpace();
    const sp3r::Sp3RSpace& irrep=sp_irrep.Sp3RSpace();
    sp3r::U3Subspace u3_subspacep = irrepp.LookUpSubspace(omegap);
    sp3r::U3Subspace u3_subspace  = irrep.LookUpSubspace(omega);

    int Nn=int(omega.N()-sp_irrep.sigma().N());
    int Nnp=int(omegap.N()-sp_irrepp.sigma().N());

    int dimp=u3_subspacep.size();
    int dim=u3_subspace.size();
    assert(dimp!=0 && dim!=0);

    // unpacking the unit tensor labels 
    u3::SU3 x0=tensor.x0();
    HalfInt S0=tensor.S0();
    HalfInt T0=tensor.T0();
    int rbp=tensor.bra().eta();
    int rb=tensor.ket().eta();

    int rho0_max=u3::OuterMultiplicity(omega.SU3(),x0,omegap.SU3());
 
    // Extracting K matrices for sp_irrep and sp_irrepp from the K_matrix_maps 
    vcs::MatrixCache& K_matrix_map_sp_irrep=k_matrix_map[sp_irrep.sigma()];
    vcs::MatrixCache& K_matrix_map_sp_irrepp=k_matrix_map[sp_irrepp.sigma()];

    Eigen::MatrixXd Kp=K_matrix_map_sp_irrepp[omegap];
    Eigen::MatrixXd K_inv=K_matrix_map_sp_irrep[omega].inverse();

    // Precalculating kronecker products used in sum to calculate unit tensor matrix
    MultiplicityTagged<u3::U3>::vector omegapp_set=KroneckerProduct(omegap, u3::U3(0,0,-2)); 
    MultiplicityTagged<u3::U3>::vector omega1_set=KroneckerProduct(omega, u3::U3(0,0,-2));
    MultiplicityTagged<u3::SU3>::vector x0p_set=KroneckerProduct(x0, u3::SU3(2,0));

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  Calculate unit tensor matrix
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Eigen::MatrixXd unit_tensor_matrix=Eigen::MatrixXd::Zero(dimp*multp,dim*mult);

    // summing over omega1
    for (auto omega1_tagged : omega1_set)
      {	
        u3::U3 omega1(omega1_tagged.irrep);			
        //check that omega1 in irrep  
        if (not irrep.ContainsSubspace(omega1))
          continue;
        // omega1 sector
        sp3r::U3Subspace u3_subspace1=irrep.LookUpSubspace(omega1);
        int dim1=u3_subspace1.size();
        // Look up K1 matrix (dim v1, v1)
        Eigen::MatrixXd K1=K_matrix_map_sp_irrep[omega1];
        // Initializing unit tensor matrix with dim. v' v1
        Eigen::MatrixXd unit_matrix
          =Eigen::MatrixXd::Zero(dimp*multp,dim1*mult);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Matrix of B*U coefs with dim v1 and v
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////
        Eigen::MatrixXd BU(dim1,dim);
        //iterating over (n,rho)
        for (int m=0; m<dim; m++)
          {
            MultiplicityTagged<u3::U3> n_rho=u3_subspace.GetStateLabels(m);
            u3::U3 n(n_rho.irrep);
            // iterate over (n1,rho1)
            for (int m1=0; m1<dim1; m1++)
              {
                MultiplicityTagged<u3::U3> n1_rho1=u3_subspace1.GetStateLabels(m1);
                u3::U3 n1(n1_rho1.irrep);
                // check allowed couping
                if (u3::OuterMultiplicity(n1.SU3(), u3::SU3(2,0),n.SU3())>0)
                    BU(m1,m)=vcs::BosonCreationRME(n,n1)
                             *u3::UCached(u_coef_cache,u3::SU3(2,0),n1.SU3(),omega.SU3(),sp_irrep.sigma().SU3(),
                                          n.SU3(),1,n_rho.tag,omega1.SU3(),n1_rho1.tag,1);
                else
                  BU(m1,m)=0;
              }	
          }								        
        Eigen::MatrixXd KBUK(dim1,dim);
        KBUK.noalias()=K1*BU*K_inv;

        // std::cout<<"KBUK "<<KBUK<<std::endl;
        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        //summing over x0'
        for (auto x0p_tagged :x0p_set)
          {
            u3::SU3 x0p(x0p_tagged.irrep);
            int rho0p_max=OuterMultiplicity(omega1.SU3(),x0p,omegap.SU3());
				  
            // summing over rho0'
            for (int rho0p=1; rho0p<=rho0p_max; rho0p++)
              {
                double coef=0;
                for(int rho0b=1; rho0b<=rho0_max; ++rho0b)
                  coef+=u3::Phi(omega.SU3(),x0,omegap.SU3(),rho0,rho0b)
                         *u3::UCached(u_coef_cache,x0,u3::SU3(2,0),omegap.SU3(), omega1.SU3(),x0p,1,rho0p,omega.SU3(),1,rho0b);
                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                // third term
                // sum over omega'', v'' and rho0''
                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                //Initilize 3rd-term-unit-tensor matrix
                Eigen::MatrixXd unit3_matrix=Eigen::MatrixXd::Zero(dimp*multp,dim1*mult);

                // Summing over omega''
                for ( auto omegapp_tagged : omegapp_set)
                  {
                    u3::U3 omegapp(omegapp_tagged.irrep);
                    if (not irrepp.ContainsSubspace(omegapp))
                      continue;
                    
                    // In term three, if omega'=sigma', then <omega'|A|omega''> will be strictly zero.
                    // won't need to check for Nn_zero cases in term 3 since they will automatically be zero.  
                    if(Nnp==0)
                      continue;

                   // quick trap to check if sectors are found in NpN4, if not found for rho0b'=1, then continue      
                   spncci::UnitTensorU3Sector 
                      unit_tensor_u3_sector_1(omegapp,omega1,tensor,1);
                    if ((sector_NpN4).count(unit_tensor_u3_sector_1)==0)
                      continue;	

                    // omega'' subspace (v'')
                    sp3r::U3Subspace u3_subspacepp=irrepp.LookUpSubspace(omegapp);
                    int dimpp=u3_subspacepp.size();
                    // Obtaining K matrix for omega''
                    Eigen::MatrixXd Kpp_inv=K_matrix_map_sp_irrepp[omegapp].inverse();
                    //Constructing a^\dagger U(3) boson matrix for A matrix
                    Eigen::MatrixXd boson_matrix(dimp,dimpp);
                    for(int vpp=0; vpp<dimpp; vpp++)
                      {
                        MultiplicityTagged<u3::U3> npp_rhopp=u3_subspacepp.GetStateLabels(vpp);
                        const u3::U3& npp=npp_rhopp.irrep;
                        const int& rhopp=npp_rhopp.tag;
                        for(int vp=0; vp<dimp; vp++)
                          {
                            MultiplicityTagged<u3::U3> np_rhop=u3_subspacep.GetStateLabels(vp);
                            const u3::U3& np=np_rhop.irrep;
                            const int& rhop=np_rhop.tag; 
                            
                            if (u3::OuterMultiplicity(npp.SU3(), u3::SU3(2,0),np.SU3())>0)
                              boson_matrix(vp,vpp)=
                                vcs::BosonCreationRME(np,npp)
                                *u3::UCached(u_coef_cache,sp_irrepp.sigma().SU3(),npp.SU3(),omegap.SU3(),u3::SU3(2,0), 
                                              omegapp.SU3(),rhopp,1,np.SU3(),1,rhop);
                            else
                              boson_matrix(vp,vpp)=0;
                          } //end vp
                      } //end vpp

                    //std::cout<<"boson matrix "<<boson_matrix<<std::endl;
                    Eigen::MatrixXd A=Kp*boson_matrix*Kpp_inv;
                    // Unit tensor matrix 
                    Eigen::MatrixXd unit3pp_matrix=Eigen::MatrixXd::Zero(dimpp*multp,dim1*mult);
                    int rho0pp_max=u3::OuterMultiplicity(omega1.SU3(),x0,omegapp.SU3());
                    // Summing over rho0''
                    for(int rho0pp=1; rho0pp<=rho0pp_max; ++rho0pp)
                      {
                        double coef3=0;
                        for (int rho0bp=1; rho0bp<=rho0p_max; rho0bp++)
                            coef3+=u3::Phi(x0p,omega1.SU3(),omegap.SU3(),rho0p,rho0bp)
                                        *u3::UCached(u_coef_cache,
                                          omega.SU3(),x0,omegap.SU3(),u3::SU3(2,0),
                                          omegapp.SU3(),rho0pp,1,x0p,1,rho0bp
                                          );    

                        // Retriving unit tensor matrix 
                        spncci::UnitTensorU3Sector unit3_labels(omegapp,omega1,tensor,rho0pp);
                        assert(sector_NpN4.count(unit3_labels)>0);
                        unit3pp_matrix+=coef3*sector_NpN4[unit3_labels];
                      } //end rho0pp
                    // matrix product (v',v'')*(v'',v1)

                    for(int i=0; i<multp; ++i)
                      for(int j=0; j<mult; ++j)
                        {
                          // Get target indices 
                          int it=i*dimp;
                          int jt=j*dim1;
                          // Get source indices
                          int is=i*dimpp;
                          int js=j*dim1;

                          unit3_matrix.block(it,jt,dimp,dim1)+=A*unit3pp_matrix.block(is,js,dimpp,dim1);
                        }
                        // unit3_matrix+=Kp*boson_matrix*Kpp_inv*unit3pp_matrix;
                  } // end omegapp
                // std::cout<<"unit3_matrix "<<unit3_matrix<<std::endl;
                unit_matrix+=unit3_matrix;
                // std::cout<<"term 3  "<<unit_matrix<<std::endl;							
                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                //first term 
                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                bool N0_negative=(rbp-rb+2)<0;
                if(u3::OuterMultiplicity(u3::SU3(rbp,0),u3::SU3(0,rb-2),x0p)>0)
                {
                  double 
                  coef1=u3::UCached(u_coef_cache,u3::SU3(rbp,0),u3::SU3(0,rb),x0p, u3::SU3(2,0),x0,1,1,u3::SU3(0,rb-2),1,1)
                        *sqrt((rb+2)*(rb+1.)*u3::dim(x0p)/(2.*u3::dim(x0)));                      

                  if(N0_negative)
                  // TODO: check conjugation coefficiet is correct. 
                    coef1*=ParitySign(rbp+rb-2+ConjugationGrade(omega1)+ConjugationGrade(omegap))
                           *sqrt(1.*u3::dim(u3::SU3(rbp,0))*u3::dim(omega1)/(u3::dim(u3::SU3(rb-2,0))*u3::dim(omegap)));
                  
                  u3shell::RelativeStateLabelsU3ST ket(tensor.ket().eta()-2,tensor.ket().S(),tensor.ket().T());
                  
                  // zero initialize unit1_matrix depending on N0 sign
                  Eigen::MatrixXd unit1_matrix;
                  if(N0_negative)
                    unit1_matrix=Eigen::MatrixXd::Zero(dim1*mult,dimp*multp);
                  else
                    unit1_matrix=Eigen::MatrixXd::Zero(dimp*multp,dim1*mult);

                  // summing over rho0bp and accumulating sectors in unit1_matrix. 
                  for(int rho0bp=1; rho0bp<=rho0p_max; ++rho0bp)
                    {
                      spncci::UnitTensorU3Sector unit1_labels;
                      if(N0_negative)
                        unit1_labels=spncci::UnitTensorU3Sector(omega1,omegap,u3shell::RelativeUnitTensorLabelsU3ST(u3::Conjugate(x0p),S0,T0,ket,tensor.bra()),rho0bp);
                      else
                        unit1_labels=spncci::UnitTensorU3Sector(omegap,omega1,u3shell::RelativeUnitTensorLabelsU3ST(x0p,S0,T0,tensor.bra(),ket),rho0bp);
                      // Accumulate
                      if (sector_NpN2.count(unit1_labels)!=0)  
                          unit1_matrix+=u3::Phi(x0p,omega1.SU3(),omegap.SU3(),rho0p,rho0bp)*sector_NpN2[unit1_labels];
                    } //end rho0bp

                  // accumulate term 1 sectors in unit matrix sector
                  // if N0 negative, transpose omega1,omegap sector to omegap,omega1 sector
                  if(N0_negative)
                    unit_matrix+=coef1*unit1_matrix.transpose();    
                  else
                    unit_matrix+=coef1*unit1_matrix;
                }               
                // std::cout<< "term 1  "<<unit_matrix<<std::endl;
                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                // second term 
                //////////////////////////////////////////////////////////////////////////////////////////////////////////	
                if (u3::OuterMultiplicity(u3::SU3(rbp+2,0),u3::SU3(0,rb),x0p)==0)
                  {
                    double 
                    coef2=-1*(rbp+2)*(rbp+1)*sqrt(u3::dim(x0p)/(2.*(rbp+4)*(rbp+3)*u3::dim(x0)))
                            *u3::UCached(u_coef_cache,u3::SU3(2,0),u3::SU3(rbp,0),x0p,u3::SU3(0,rb),
                                          u3::SU3(rbp+2,0),1,1,x0,1,1);

                    if(N0_negative)
                    // TODO: check conjugation coefficiet is correct. 
                      coef2*=ParitySign(rbp+rb-2+ConjugationGrade(omega1)+ConjugationGrade(omegap))
                             *sqrt(1.*u3::dim(u3::SU3(rbp,0))*u3::dim(omega1)/(u3::dim(u3::SU3(rb-2,0))*u3::dim(omegap)));
                    spncci::UnitTensorU3Sector unit2_labels;
                    u3shell::RelativeStateLabelsU3ST bra(tensor.bra().eta()+2,tensor.bra().S(),tensor.bra().T());

                    // zero initialize unit1_matrix depending on N0 sign
                    Eigen::MatrixXd unit2_matrix;
                    if(N0_negative)
                      unit2_matrix=Eigen::MatrixXd::Zero(dim1*mult,dimp*multp);
                    else
                      unit2_matrix=Eigen::MatrixXd::Zero(dimp*multp,dim1*mult);
               
                    for(int rho0bp=1; rho0bp<=rho0p_max; ++rho0bp)
                      {
                        if(N0_negative)
                          unit2_labels=spncci::UnitTensorU3Sector(omega1,omegap,u3shell::RelativeUnitTensorLabelsU3ST(u3::Conjugate(x0p),S0,T0,tensor.ket(),bra),rho0p);
                        else
                          unit2_labels=spncci::UnitTensorU3Sector(omegap,omega1,u3shell::RelativeUnitTensorLabelsU3ST(x0p,S0,T0,bra,tensor.ket()),rho0bp);

                        if(sector_NpN2.count(unit2_labels)>0)
                          unit2_matrix+=u3::Phi(x0p,omega1.SU3(),omegap.SU3(),rho0p,rho0bp)*sector_NpN2[unit2_labels];
                      }

                    // accumulate term 2 sectors in unit matrix sector
                    // if N0 negative, transpose omega1,omegap sector to omegap,omega1 sector
                    if(N0_negative)
                      unit_matrix+=coef2*unit2_matrix.transpose();    
                    else
                      unit_matrix+=coef2*unit2_matrix;
                  }
                  // std::cout<<"term 2 "<<unit_matrix<<std::endl;
                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                unit_matrix=coef*unit_matrix;
              } //end rho0p
          } //end sum over x0p
        // summing over n, rho, n1, rho1, v1
        unit_tensor_matrix+=unit_matrix*KBUK;
      }// end sum over omega1
    assert(unit_tensor_matrix.cols()!=0 && unit_tensor_matrix.rows()!=0);
    // std::cout<<unit_tensor_matrix <<std::endl;
    #ifdef VERBOSE
    std::cout<<"Exiting UnitTensorMatrix"<<std::endl;
    #endif
    return unit_tensor_matrix;
  } // End function


  void 
  GenerateUnitTensorU3Sector(
      u3::UCoefCache& u_coef_cache,
      std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>> k_matrix_map,
      const spncci::UnitTensorU3Sector& unit_tensor_u3_sector, 
      const spncci::SpIrrep& sp_irrepp,
      const spncci::SpIrrep& sp_irrep,
      const  std::pair<int,int>& lgi_multiplicities,                                 
      // Eigen doesn't like const 
      spncci::UnitTensorSectorsCache& sector_NpN2,
      spncci::UnitTensorSectorsCache& sector_NpN4,
      bool Nn_zero,
      std::vector< UnitTensorU3SectorPair>& unit_tensor_u3_sector_pairs
    )
  {
    //calculate unit tensor matrix.   
    int zerocout=0;
    /////////////////////////////////////////////////////////////////////////////////////
    #ifdef VERBOSE
    std::cout<<"Entering GenerateUnitTensorU3Sector"<<std::endl;
    #endif
    Eigen::MatrixXd temp_matrix;
    // In the special case that omegap.N()!=sigmap.N() but omega.N()==sigma.N(), then to calculate we
    // need to calculate the conjugate transpose of the unit tensor matrix and then invert and multiply 
    // by factor to obtain desired matrix
    //
    // Case 1: Nn=zero and Nnp>0 
    if (Nn_zero)
      {
        u3::U3 omegap,omega;
        u3::SU3 x0;
        int rp, r,rho0;
        HalfInt S0, T0, Sp, Tp, S, T;
        u3shell::RelativeUnitTensorLabelsU3ST unit_tensor;

        std::tie (omegap,omega,unit_tensor,rho0)=unit_tensor_u3_sector.Key();
        
        // std::tie (x0,S0,T0,rp,Sp,Tp,r,S,T)=unit_tensor.Key();
        spncci::UnitTensorU3Sector unit_tensor_calc_u3_sector
          =spncci::UnitTensorU3Sector(omega,omegap,u3shell::Conjugate(unit_tensor),rho0);

        //  Call UnitTensorMatrix function to calculate the Unit Tensor sub matrix for the vv' 
        //  corresponding to omega and omega'
        // Note: 
        temp_matrix=spncci::UnitTensorMatrix(
                      u_coef_cache,k_matrix_map,sp_irrep,sp_irrepp,lgi_multiplicities,
                      sector_NpN2,sector_NpN4,unit_tensor_calc_u3_sector
                      );
        // Conjugation phase        
        double coef=ParitySign(rp+r+ConjugationGrade(omega)+ConjugationGrade(omegap))
              *sqrt(1.*dim(u3::SU3(rp,0))*dim(omega)/(dim(u3::SU3(r,0))*dim(omegap)));
        // if the matrix has non-zero entries,
        if (temp_matrix.any())
          // apply symmtry factors, transpose the matrix and 
          unit_tensor_u3_sector_pairs.push_back(UnitTensorU3SectorPair(unit_tensor_u3_sector,coef*temp_matrix.transpose()));
      }
    // case 2: Nn>0.
    else 
      {
        //  Call UnitTensorMatrix function to calculate the Unit Tensor sub matrix for the v'v 
        //  corresponding to omega' and omega
        temp_matrix=spncci::UnitTensorMatrix(
                      u_coef_cache,k_matrix_map,sp_irrepp,sp_irrep, lgi_multiplicities,
                      sector_NpN2,sector_NpN4,unit_tensor_u3_sector
                      );
      
        // If temp_matrix is non-zero, add unit tensor sub matrix into the unit_tensor_rme_map
        if (temp_matrix.any())
            unit_tensor_u3_sector_pairs.push_back(UnitTensorU3SectorPair(unit_tensor_u3_sector,temp_matrix));
      }
      // std::cout<<"zero count  "<<zerocout<<std::endl;
  #ifdef VERBOSE
  std::cout<<"Number of pairs  "<<unit_tensor_u3_sector_pairs.size()<<std::endl;
  std::cout<<"Exiting GenerateUnitTensorU3Sector"<<std::endl;
  #endif
  }


void 
GenerateNpNSector(
  const std::pair<int,int> NpN_pair, 
  const spncci::SpIrrep& sp_irrepp,
  const spncci::SpIrrep& sp_irrep,
  const std::pair<int,int>& lgi_multiplicities,
  u3::UCoefCache& u_coef_cache,
  std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>> k_matrix_map,
  std::map<std::pair<int,int>,spncci::UnitTensorSectorsCache>& unit_tensor_rme_map,
  std::map<std::pair<int,int>,std::vector<spncci::UnitTensorU3Sector> >& unit_tensor_NpN_sector_map
  )
{
  ////////////////////// NSectors///////////////////////////////////////////////////////
 const std::vector<spncci::UnitTensorU3Sector>& unit_U3Sector_vector
          =unit_tensor_NpN_sector_map[NpN_pair];
  // Checking for special case that Nnp!=0 but that Nn=0. In this case, sector is computed for inverse
  // TODO confirm that we don't have problems with not computing N0<0 rmes. 
  int Nnp=NpN_pair.first;
  int Nn=NpN_pair.second;
  bool Nn_zero=(Nnp!=0 && Nn==0);

  std::pair<int,int> NpN2=Nn_zero?std::pair<int,int>(Nn,Nnp-2):std::pair<int,int>(Nnp,Nn-2);
  std::pair<int,int> NpN4=Nn_zero?NpN4=std::pair<int,int>(Nn-2,Nnp-2):std::pair<int,int>(Nnp-2,Nn-2);

  UnitTensorSectorsCache& sector_NpN2=unit_tensor_rme_map[NpN2];
  UnitTensorSectorsCache& sector_NpN4=unit_tensor_rme_map[NpN4];

  // debugging variable
  int sector_count = 0;  
  #ifdef VERBOSE
  std::cout<<"Begin generating sectors "<< unit_U3Sector_vector.size()<<std::endl;
  #endif

  #pragma omp parallel reduction(+:sector_count)
  {
    
    #ifdef VERBOSE_OMP
    #pragma omp single
    std::cout << "omp_get_num_threads " << omp_get_num_threads() << std::endl;
    #endif

    // private storage of generated sectors
    std::vector<spncci::UnitTensorU3SectorPair> u3sector_pairs;
    #pragma omp for schedule(runtime)
    for (int i=0; i<unit_U3Sector_vector.size(); i++)
      {
        const spncci::UnitTensorU3Sector& unit_tensor_u3_sector=unit_U3Sector_vector[i];
        GenerateUnitTensorU3Sector(
          u_coef_cache, k_matrix_map,
          unit_tensor_u3_sector, sp_irrepp, sp_irrep,
          lgi_multiplicities,
          sector_NpN2, sector_NpN4, Nn_zero, u3sector_pairs
         );
      }
    // save out sectors
    #pragma omp critical
    {
      #ifdef VERBOSE_OMP
      std::cout << "  Saving sectors from thread " << omp_get_thread_num() << std::endl;
      #endif
      unit_tensor_rme_map[NpN_pair].insert(u3sector_pairs.begin(),u3sector_pairs.end());
      // sector_count += u3sector_pairs.size();
      // for (int j=0; j<u3sector_pairs.size(); j++)
      //   {
      //     std::cout << " " << u3sector_pairs[j].second<<std::endl;
      //     unit_tensor_rme_map[NpN_pair].insert(u3sector_pairs[j]);
      //   }
    }
  }  // omp parallel
  #ifdef VERBOSE
  std::cout<<"Finish generating sectors "<<std::endl;
  #endif
  // remove invalid NpN4 key that was inserted into map.          
  if ((Nnp+Nn)==2)
    unit_tensor_rme_map.erase(NpN4);
}

  void 
  GenerateUnitTensorMatrix(
    int N1b,
    int Nmax, 
    std::pair<int,int> sp_irrep_pair,
    const spncci::SpIrrepVector& sp_irrep_vector,
    u3::UCoefCache u_coef_cache,
    std::unordered_map<u3::U3,vcs::MatrixCache, boost::hash<u3::U3>> k_matrix_map,
    std::map<std::pair<int,int>,std::vector<spncci::UnitTensorU3Sector>>& unit_tensor_NpN_sector_map,
    std::map<std::pair<int,int>,spncci::UnitTensorSectorsCache>& unit_tensor_rme_map
    )
  // Generates all unit tensor matrix matrices between states in the irreps of sp_irrep_pair
  // The unit tensors are stored in the map of a map unit_tensor_rme_map which has key 
  // sp_irrep_pair to a map with key std::pair<Nnp,Nn> and value map(matrix labels for 
  // w'w sector, matrix) 
  { 
    #ifdef VERBOSE
    std::cout<<"Entering GenerateUnitTensorMatrix"<<std::endl;
    #endif

    // extract SpIrrep labels from pair
    const spncci::SpIrrep& sp_irrepp=sp_irrep_vector[sp_irrep_pair.first].irrep;
    const spncci::SpIrrep& sp_irrep=sp_irrep_vector[sp_irrep_pair.second].irrep;
    const sp3r::Sp3RSpace& irrepp=sp_irrepp.Sp3RSpace();
    const sp3r::Sp3RSpace& irrep=sp_irrep.Sp3RSpace();
    std::pair<int,int> lgi_multiplicities(
      sp_irrep_vector[sp_irrep_pair.first].tag,
      sp_irrep_vector[sp_irrep_pair.second].tag
      );

    ////////////////////////////////////////////////////////////////////////////////////
    // Looping over NpN subspaces 
    ////////////////////////////////////////////////////////////////////////////////////    
    int num_unit_tensor_sectors=0;
    int Np_truncate=Nmax-sp_irrepp.Nex();
    int N_truncate=Nmax-sp_irrep.Nex();
    for (int Nsum=2; Nsum<=2*Nmax; Nsum+=2)
      for (int Nnp=0; Nnp<=std::min(Nsum,Np_truncate); Nnp+=2)
        {
          // Check Nmax constrain
          int Nn=Nsum-Nnp;
          if (Nn>N_truncate)
            continue;
          // Check N0>=0 constraint.
          if((Nnp+sp_irrepp.Nex()-Nn-sp_irrep.Nex())<0)
            continue;
          //////////////////////////////////////////////////////////////////////////////     
          //  Compute rmes for NSectors
          //////////////////////////////////////////////////////////////////////////////    
          std::pair<int,int> NpN_pair(Nnp,Nn);              
          GenerateNpNSector(
            NpN_pair,sp_irrepp,sp_irrep,lgi_multiplicities,
            u_coef_cache, k_matrix_map,
            unit_tensor_rme_map,unit_tensor_NpN_sector_map
            ); 
          // diagnostic output
          int num=unit_tensor_rme_map[NpN_pair].size();
          num_unit_tensor_sectors+=num;
          // std::cout<<Nnp<<"  "<<Nn<<"  "<<"num  "<< num_unit_tensor_sectors<<std::endl;
        } 
    ////////////////////////////////////////////////////////////////////////////////////    
    std::cout<<"number of unit tensor sectors "<< num_unit_tensor_sectors<<std::endl;

  #ifdef VERBOSE
  std::cout<<"Exiting GenerateUnitTensorMatrix"<<std::endl; 
  #endif     
}  // end function


} // End namespace 
  
          
