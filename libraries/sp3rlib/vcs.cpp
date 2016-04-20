/****************************************************************
  vcs.cpp                       

  Define vector coherent state methods for Sp(3,R).

  Anna E. McCoy
  University of Notre Dame

  Created by Anna E. McCoy on 3/9/16.   
 
  3/9/16 (aem): Created based on prototype vcs.py.

****************************************************************/

#include "sp3rlib/vcs.h"

namespace vcs{
  double BosonCreationRME(const u3::U3& np, const u3::U3& n)
  //  SU(3) Reduced matrix element of a^\dagger boson creation operator
  {
    double rme=0;
    int n1=int(n.f1());
    int n2=int(n.f2());
    int n3=int(n.f3());

    if((np.f1()==(n.f1()+2))&&(np.f2()==n.f2())&&(np.f3()==n.f3()))
      rme=sqrt(
               (n1+4)*(n1-n2+2)*(n1-n3+3)
               /(2.*(n1-n2+3)*(n1-n3+4))
               );

    if ((np.f1()==n.f1())&&(np.f2()==(n.f2()+2))&&(np.f3()==n.f3()))
      rme=sqrt(
               (n2+3)*(n1-n2)*(n2-n3+2)
               /(2.*(n1-n2-1)*(n2-n3+3))
               );
    if ((np.f1()==n.f1())&&(np.f2()==n.f2())&&(np.f3()==(n.f3()+2)))
      rme=sqrt(
               (n3+2)*(n2-n3)*(n1-n3+1)/(2.*(n1-n3)*(n2-n3-1))
               );
    return rme;

  }

  double U3BosonCreationRME(
                            const u3::U3& sigmap, const MultiplicityTagged<u3::U3>np_rhop,  const u3::U3& omegap,
                            const u3::U3& sigma, const MultiplicityTagged<u3::U3> n_rho, const u3::U3& omega)
  {
    if (sigmap==sigma)
      return (
              u3::U(sigma.SU3(), n_rho.irrep.SU3(), omegap.SU3(), u3::SU3(2,0), omega.SU3(),n_rho.tag,1,np_rhop.irrep.SU3(),1,np_rhop.tag)
              *BosonCreationRME(np_rhop.irrep,n_rho.irrep)
              );
    else
      return 0.0;
  }


  void GenerateKMatrices(const sp3r::Sp3RSpace& irrep, std::map<u3::U3,Eigen::MatrixXd>& K_matrix_map)
  {
    u3::U3 sigma=irrep.sigma();
    std::map<u3::U3,Eigen::MatrixXd> S_matrix_map;
    for(int i=0; i<irrep.size(); i++)
      {
      
        // Generate S_matrix = K_matrix^2
        sp3r::U3Subspace u3_subspace_p=irrep.GetSubspace(i);
        u3::U3 omega_p=u3_subspace_p.GetSubspaceLabels();

        int dimension_p=u3_subspace_p.size();
        Eigen::MatrixXd S_matrix_p=Eigen::MatrixXd::Zero(dimension_p,dimension_p);
        Eigen::MatrixXd K_matrix_p(dimension_p,dimension_p);
        if (sigma==omega_p)
          {
            S_matrix_p(0,0)=1.0;
            K_matrix_p(0,0)=1.0;
          }
        // general case 
        else 
          {
          
            MultiplicityTagged<u3::U3>::vector omega_set=KroneckerProduct(omega_p, u3::U3(0,0,-2));
            // sum over omega 
          
            for (int w=0; w<omega_set.size(); w++)
              {
                u3::U3 omega(omega_set[w].irrep);
                if (not irrep.ContainsSubspace(omega))
                  continue;

                const sp3r::U3Subspace& u3_subspace=irrep.LookUpSubspace(omega);//GetSubspace().LookUpSubspaceIndex(omega);

                int dimension=u3_subspace.size();
                // OPTCHECK: Try doing mat-mat-mat by hand 
                Eigen::MatrixXd coef1_matrix(dimension_p,dimension);
                Eigen::MatrixXd coef2_matrix(dimension,dimension_p);
              
                // iterate over n1p,rho1p and n2p rho2p
                for (int i1=0; i1<dimension_p; i1++)
                  {
                  
                    MultiplicityTagged<u3::U3> n1p_rho1p=u3_subspace_p.GetStateLabels(i1);
                    u3::U3 n1p(n1p_rho1p.irrep);
                    for (int i2=0; i2<dimension_p; i2++)
                      {
                        MultiplicityTagged<u3::U3> n2p_rho2p=u3_subspace_p.GetStateLabels(i2);
                        u3::U3 n2p(n2p_rho2p.irrep);

                        // Filling out coef1_matrix 
                        for (int j1=0; j1<dimension; j1++)
                          {
                            MultiplicityTagged<u3::U3> n1_rho1=u3_subspace.GetStateLabels(j1);
                            u3::U3 n1(n1_rho1.irrep);
                            if (u3::OuterMultiplicity(n1.SU3(), u3::SU3(2,0), n1p.SU3())>0)
                              coef1_matrix(i1,j1)=(
                                                   2./int(n1p.N())
                                                   *(Omega(n1p, omega_p)-Omega(n1,omega))
                                                   *U3BosonCreationRME(sigma,n1p_rho1p,omega_p,sigma, n1_rho1,omega)
                                                   );
                            else
                              coef1_matrix(i1,j1)=0.0;
                          }

                        // Filling out coef2_matrix                      
                        for (int j2=0; j2<dimension; j2++)
                          {
                            MultiplicityTagged<u3::U3> n2_rho2=u3_subspace.GetStateLabels(j2);
                            u3::U3 n2(n2_rho2.irrep);
                            if (u3::OuterMultiplicity(n2.SU3(),u3::SU3(2,0), n2p.SU3())>0)
                              coef2_matrix(j2,i2)=U3BosonCreationRME(sigma, n2p_rho2p, omega_p, sigma, n2_rho2, omega);
                            else
                              coef2_matrix(j2,i2)=0.0;
                          }
                      }                  
                  }
                S_matrix_p+=coef1_matrix*S_matrix_map[omega]*coef2_matrix;


              }
            //calculate K matrix 
            Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigen_system(S_matrix_p);
            K_matrix_p=eigen_system.operatorSqrt();
            //std::cout<<omega_p.Str()<<K_matrix_p<<std::endl;

          }
        S_matrix_map[omega_p]=S_matrix_p;
        K_matrix_map[omega_p]=K_matrix_p;
    
      } // end for (i)  
  }








}  //  namespace 
