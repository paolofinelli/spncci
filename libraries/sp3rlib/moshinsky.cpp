/****************************************************************
  u3.h

  U(3) and SU(3) labeling, branching, and Kronecker product.
                                  
  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  3/7/16 (aem,mac): Created based on prototype u3states.py, u3.py, and so3.py.
  3/8/16 (aem,mac): Add U3ST structure and rename U3S structure.
  3/9/16 (aem,mac): Add KeyType typedefs.  Extract MultiplicityTagged.

****************************************************************/

#include <cmath>
#include "gsl/gsl_sf.h"

#include "am/halfint.h"
#include "u3.h"
namespace u3 
{
	double MoshinskyCoefficient(const u3::SU3& x1, const u3::SU3& x2, const u3::SU3& xr,const u3::SU3& xc,const u3::SU3& x)
	// SU(3) Moshinsky Coefficient which is equivalent to a Wigner little d function evaluated at pi/2
	{
		
		HalfInt J(x.lambda,2);
		HalfInt Mp(x1.lambda-x2.lambda,2);
		HalfInt M(xr.lambda-xc.lambda,2); 
		gsl_sf_result result;

		double moshinsky_coef=0;
		int Kmax=std::max(
			std::max(int(J+M),int(J-M)),int(J-Mp));
		
		for(int K=0; K<=Kmax; K++)
			moshinsky_coef=moshinsky_coef+ParitySign(K)*gsl_sf_choose_e(int(J+M),int(J-Mp-K),&result)*gsl_sf_choose_e(int(J-M),K,&result);
		moshinsky_coef=moshinsky_coef*(
			ParitySign(J-Mp)
			*sqrt(
				gsl_sf_fact(int(J+Mp))*gsl_sf_fact(int(J-Mp))
				/(pow(2.,TwiceValue(J))*gsl_sf_fact(int(J+M))*gsl_sf_fact(int(J-M)))
				)
			);
		return moshinsky_coef;
	}

	double MoshinskyCoefficient(const u3::U3& w1, const u3::U3& w2, const u3::U3& wr,const u3::U3& wc,const u3::U3& w)
	//Overleading for U3 arguements 
	{
		return MoshinskyCoefficient(w1.SU3(), w2.SU3(), wr.SU3(), wc.SU3(), w.SU3());

	}

	double MoshinskyCoefficient(int r1, int r2, int r, int R, const u3::SU3& x)
	//Overloading Moshinsky to take integer arguements for two-body and relative-center of mass arguments
	// and SU(3) total symmetry (lambda,mu)
	{

		HalfInt J(x.lambda,2);
		HalfInt Mp(r1-r2,2);
		HalfInt M(r-R,2) ;
		gsl_sf_result result;

		double moshinsky_coef=0;
		int Kmax=std::max(std::max(int(J+M),int(J-M)),int(J-Mp));

		for(int K=0; K<=Kmax; K++)
			moshinsky_coef=moshinsky_coef+ParitySign(K)*gsl_sf_choose_e(int(J+M),int(J-Mp-K),&result)*gsl_sf_choose_e(int(J-M),K,&result);
		moshinsky_coef=moshinsky_coef*(
			ParitySign(J-Mp)
			*sqrt(
				gsl_sf_fact(int(J+Mp))*gsl_sf_fact(int(J-Mp))
				/(pow(2.,TwiceValue(J))*gsl_sf_fact(int(J+M))*gsl_sf_fact(int(J-M)))
				)
			);
		return moshinsky_coef;	
	}

	double MoshinskyCoefficient(int r1, int r2, int r, int R, const u3::U3& w)
	// Overloading Moshinsky to take integers and U3 for total symmetry
	{
		return MoshinskyCoefficient(r1, r2, r, R, w.SU3());
	}		



} //namespace









