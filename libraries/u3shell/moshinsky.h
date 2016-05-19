/****************************************************************
  moshinsky.h

  U(3) Moshinsky coefficient.
                                  
  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  3/7/16 (aem,mac): Created based on prototype u3states.py, u3.py, and so3.py.
  3/8/16 (aem,mac): Add U3ST structure and rename U3S structure.
  3/9/16 (aem,mac): Add KeyType typedefs.  Extract MultiplicityTagged.
  5/11/16 (aem,mac): Move to u3shell namespace.

****************************************************************/

#ifndef MOSHINSKY_H_
#define MOSHINSKY_H_

#include "sp3rlib/u3.h"
#include "sp3rlib/u3coef.h"
#include "u3shell/indexing_u3st.h"
#include <eigen3/Eigen/Eigen>
#include <eigen3/Eigen/Eigenvalues>  
#include "u3shell/tensor.h"
namespace u3shell
{
  // double MoshinskyCoefficient(const u3::SU3& x1, const u3::SU3& x2, const u3::SU3& xr,const u3::SU3& xc,const u3::SU3& x);
  // SU(3) Moshinsky Coefficient which is equivalent to a Wigner little d function evaluated at pi/2

  // double MoshinskyCoefficient(const u3::U3& w1, const u3::U3& w2, const u3::U3& wr,const u3::U3& wc,const u3::U3& w);
  //Overleading for U3 arguements 

  double MoshinskyCoefficient(int r1, int r2, int r, int R, const u3::SU3& x);
  //Overloading Moshinsky to take integer arguements for two-body and relative-center of mass arguments
  // and SU(3) total symmetry (lambda,mu)

  double MoshinskyCoefficient(int r1, int r2, int r, int R, const u3::U3& w);
  // Overloading Moshinsky to take integers and U3 for total symmetry

  void MoshinskyTransformation(const u3shell::RelativeUnitTensorLabelsU3ST& tensor, int Nmax);
  // Moshinsky transform of relative unit tensor operator to twobody space.  

  





} //namespace

#endif 




