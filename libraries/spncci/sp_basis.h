/****************************************************************
  sp_basis.h

  Sp(3,R) basis construction, indexing, and branching.
                                  
  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  3/10/16 (aem,mac): Created.
  3/11/16 (aem,mac): Implement basis iteration.

****************************************************************/

#ifndef SP_BASIS_H_
#define SP_BASIS_H_

#include <vector>

#include "sp3rlib/sp3r.h"
#include "sp3rlib/u3.h"

namespace spncci
{

  ////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////
  // Sp(3,R) LGI enumeration
  ////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////
  // LGI data structure
  ////////////////////////////////////////////////////////////////

  // Stored for each LGI:
  //   Nex sigma Sp Sn S
  //
  // Note that Sp and Sn are spectators for most conceivable
  // calculations but are retained for informational value.
  //
  // Although sigma x S could be encoded together as a single
  // u3::U3S object, it is not clear that there is any benefit
  // (conceptual or practical) to adding an extra layer of packaging
  // at this stage.
  
  struct LGI
  {
    ////////////////////////////////////////////////////////////////
    // constructors
    ////////////////////////////////////////////////////////////////

    // copy constructor: synthesized copy constructor since only data
    // member needs copying

    inline
    LGI(int Nex_, const u3::U3& sigma_, const HalfInt& Sp_, const HalfInt& Sn_, const HalfInt& S_)
    : Nex(Nex_), sigma(sigma_), Sp(Sp_), Sn(Sn_), S(S_) {}


    ////////////////////////////////////////////////////////////////
    // string conversion
    ////////////////////////////////////////////////////////////////
    
    std::string Str() const;

    ////////////////////////////////////////////////////////////////
    // labels
    ////////////////////////////////////////////////////////////////
    
    int Nex;
    u3::U3 sigma;
    HalfInt Sp, Sn, S;
  };

  ////////////////////////////////////////////////////////////////
  // enumeration of LGI set based on input table
  ////////////////////////////////////////////////////////////////

  // LGI input tabulation format:
  //
  //   Nex lambda mu 2Sp 2Sn 2S count
  //   ...
  //
  // Each input table line results in multiple stored LGIs based on
  // the given count.
  //
  // In calculating sigma, we also need Nsigma_0 for this nucleus, since
  //
  //   Nsigma = Nsigma_0 + Nex

  void GenerateLGIVector(std::vector<spncci::LGI>& lgi_vec, const std::string& lgi_filename, const HalfInt& Nsigma_0);
  // Generates vector of LGIs based on LGI input tabulation.
  //
  // Arguments:
  //   lgi_vec (vector<LGI>) : container for LGI list (OUTPUT)
  //   filename (string) : filename for LGI table file
  //   Nsigma_0 (HalfInt) : Nsigma_0 for nucleus

  ////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////
  // Sp(3,R) LGI -> U(3) branching
  ////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////
  // truncation scheme definition
  ////////////////////////////////////////////////////////////////

  // The Nn_max for each sigma might be determined by any number of
  // creative truncation schemes, and we put in place a general mechanism for this through the TruncatorInterface.
  //
  // The truncation is most simply defined based on an Nmax cutoff:
  //
  //   Nn_max = Nmax - Nex
  //          = Nmax - (Nsigma - Nsigma_0)

  class TruncatorInterface
  // Define generic interface for defining truncations by (sigma,S).
  {
  public:
    virtual int Nn_max(const u3::U3S& sigmaS) const
      = 0; //pure virtual
    // Calculate Nn_max to use for given LGI (sigma,S) labels.
    //
    // Arguments:
    //   sigmaS (u3::U3S) : (sigma,S) labels of the LGI
    //
    // Returns:
    //   (int) : the truncation Nn_max to use for this LGI
  };

  class NmaxTruncator 
    : public TruncatorInterface
  {
  public:
    ////////////////////////////////////////////////////////////////
    // constructors
    ////////////////////////////////////////////////////////////////
    
    // construct by given Nmax
    NmaxTruncator(const HalfInt& Nsigma_0, int Nmax) : Nmax_(Nmax) {}

    ////////////////////////////////////////////////////////////////
    // truncator calculation
    ////////////////////////////////////////////////////////////////

    int Nn_max(const u3::U3S& sigmaS) const;

    ////////////////////////////////////////////////////////////////
    // truncation information
    ////////////////////////////////////////////////////////////////
    
  private:
    HalfInt Nsigma_0_;
    int Nmax_;

  };

  ////////////////////////////////////////////////////////////////
  // storage of S(3,R) -> U(3) branchings
  ////////////////////////////////////////////////////////////////

  void GenerateSp3RSpaces(
			  const std::vector<spncci::LGI>& lgi_vec,
			  std::map<u3::U3S,sp3r::Sp3RSpace>& sigma_map,
			  const TruncatorInterface& truncator
			  );
  // Generate Sp(3,R) space branching information required for given set of LGIs.
  //
  // Arguments:
  //   lgi_vec (vector<spncci::LGI>) : vector of LGIs for which we
  //     must generate branchings
  //   sigma_map (map<u3::U3S,sp3r::Sp3RSpace>) : dictionary of
  //     mappings (sigma,S) -> Sp3RSpace key-value pairs (OUTPUT)
  //   truncator (TruncatorInterface) : truncator instance providing
  //     the means of determining the Nn_max needed for each LGI

}  // namespace

#endif
