/****************************************************************
  results_output.h

  Code to generate results tabulations.
                                  
  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

  6/17/17 (mac): Created.
****************************************************************/

#ifndef SPNCCI_SPNCCI_RESULTS_OUTPUT_H_
#define SPNCCI_SPNCCI_RESULTS_OUTPUT_H_

#include <iostream>
#include <string>

#include "cppformat/format.h"
#include "spncci/branching.h"
#include "spncci/parameters.h"

namespace spncci
{

  ////////////////////////////////////////////////////////////////
  // version stamp
  ////////////////////////////////////////////////////////////////
  const int g_results_version = 201706200; // "yyyymmddv" (v=version w/in day)

  ////////////////////////////////////////////////////////////////
  // output utilities
  ////////////////////////////////////////////////////////////////

  void StartNewSection(std::ostream& out_stream, const std::string& title);
  // Start new file section.
  //
  // Arguments:
  //   out_stream (input): output stream
  //   title (input): section title

  template <typename tValue>
    void WriteKeyValue(std::ostream& out_stream, const std::string& keyword, const std::string& format, tValue value)
  // Write key-value pair.
  //
  // Arguments:
  //   out_stream (input): output stream
  //   title (input): keyword
  //   format (input): format code for value (no braces!)
  //   value (input): value to write
  {
    std::string full_format = fmt::format("{{{}}}",format);  // encapsulate format code in braces
    out_stream << fmt::format("{} = {}",keyword,fmt::format(full_format,value)) << std::endl;
  }

  template <typename tValueIterable>
    void WriteKeyValueList(
        std::ostream& out_stream, const std::string& keyword, const std::string& format,
        tValueIterable data
      )
    // Write key-value-list pair.
    //
    // The data may be any iterable container of values, such as std::vector or std::array.
    //
    // Arguments:
    //   out_stream (input): output stream
    //   title (input): keyword
    //   format (input): format code for value (no braces!)
    //   date (input): values to write
    {
    std::string full_format = fmt::format("{{{}}}",format);  // encapsulate format code in braces
    out_stream << fmt::format("{} =",keyword);
    for (const auto& value : data)
      out_stream << " " << fmt::format(full_format,value);
    out_stream << std::endl;
      
  }


  ////////////////////////////////////////////////////////////////
  // output code
  ////////////////////////////////////////////////////////////////

  void WriteCodeInformation(std::ostream& out_stream, const spncci::RunParameters& run_parameters);

  void WriteRunParameters(std::ostream& out_stream, const spncci::RunParameters& run_parameters);

  void WriteBasisStatistics(
      std::ostream& out_stream,
      const spncci::SpNCCISpace& spncci_space,
      const spncci::BabySpNCCISpace& baby_spncci_space,
      const spncci::SpaceSpU3S& spu3s_space,
      const spncci::SpaceSpLS spls_space
    );

  void WriteCalculationParameters(
      std::ostream& out_stream,
      double hw
    );

  void WriteEigenvalues(
      std::ostream& out_stream,
      const std::map<HalfInt,Eigen::VectorXd>& eigenvalues
    );
  // Write table of eigenvalues.
  //
  // CAVEAT: parity quantum number in output is hard-coded to natural
  // parity
  //
  // TODO: revise to take SpJSpace and std::vector<Eigen::VectorXd>,
  // and then take J and gex from SpJSubspace labels
  //
  // Arguments:
  //   out_stream (input): output stream
  //   eigenvalues (input): map of J -> eigenvalues

}  // namespace

#endif