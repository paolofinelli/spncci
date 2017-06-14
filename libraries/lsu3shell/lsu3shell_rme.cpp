/****************************************************************
  lsu3shell_rme.cpp

  Anna E. McCoy and Mark A. Caprio
  University of Notre Dame

****************************************************************/

#include "lsu3shell/lsu3shell_rme.h"


#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>
//#include <functional>

#include "cppformat/format.h"
#include "mcutils/io.h"
#include "mcutils/parsing.h"

namespace lsu3shell
{

  // global mode setting for rme I/O
  bool g_rme_binary_format = true;

  void 
  ReadLSU3ShellRMEsText(
      const std::string& filename,
      const LSU3BasisTable& lsu3_basis_table,
      const u3shell::SpaceU3SPN& space, 
      const u3shell::OperatorLabelsU3ST& operator_labels,
      const u3shell::SectorsU3SPN& sectors,
      basis::MatrixVector& matrix_vector,
      double scale_factor
    )
  {
    // open file
    std::ifstream in_stream(filename);
    StreamCheck(bool(in_stream),filename,"Failure opening lsu3shell rme file");

    // process stream
    basis::SetOperatorToZero(sectors,matrix_vector);
    std::string line;
    while(std::getline(in_stream,line))
      {
        // skip initial header line
        if(not std::isdigit(line[0]))
          continue;

        // read bra/ket lsu3shell basis multiplicity group indices
        int i,j;
        std::istringstream line_stream(line);
        line_stream >> i >> j;
        // std::cout<<i<<" "<<j<<std::endl;

        // retrieve lsu3shell basis multiplicity group information
        u3shell::U3SPN omegaSPNi, omegaSPNj;
        // std::tie(omegaSPNi,group_size_i,start_index_i)=lsu3_basis_table[i];
        // std::tie(omegaSPNj,group_size_j,start_index_j)=lsu3_basis_table[j];
        const LSU3BasisGroupData& group_i = lsu3_basis_table[i];
        const LSU3BasisGroupData& group_j = lsu3_basis_table[j];

        u3::SU3 xi(group_i.omegaSPN.SU3());
        u3::SU3 xj(group_j.omegaSPN.SU3());
        // std::cout<<fmt::format("{}  {}  {}", group_i.omegaSPN.Str(), operator_labels.Str(),group_j.omegaSPN.Str())<<std::endl;
        int rho0_max=u3::OuterMultiplicity(xj,operator_labels.x0(),xi);
        // std::cout<<group_i.dim<<"  "<<group_j.dim<<"  "<<rho0_max<<std::endl;
        // extract and store matrix elements
        int i_space=space.LookUpSubspaceIndex(group_i.omegaSPN);
        int j_space=space.LookUpSubspaceIndex(group_j.omegaSPN);

        // warn if file provides an rme of multiplicity zero
        //
        // This still falls short of verifying the the multiplicity
        // given in the file, but at least it's something...
        if (rho0_max==0)
          {
            std::cout
              << "WARN: input rme entry present for rme with expected zero rho0_max"
              << std::endl
              << fmt::format("i {} j {}:  {}  {}  {}",i,j,group_i.omegaSPN.Str(),operator_labels.Str(),group_j.omegaSPN.Str())
              << std::endl
              << fmt::format("{}: {}",filename,line)
              <<std::endl;
          }

        // extract and store matrix elements
        for(int gi=0; gi<group_i.dim; ++gi)
          for(int gj=0; gj<group_j.dim; ++gj)
            for(int rho0=1; rho0<=rho0_max; ++rho0)
              {
                double rme;
                // std::cout<<"getting rme"<<std::endl;
                line_stream >> rme;

                // suppress zero values -- no longer appropriate/necessary
                // if(fabs(rme)<zero_threshold)
                //   continue;

                // std::cout<<fmt::format("{} {}  {} {} {}  {}",i,j,i_space,j_space,rho0,rme)<<std::endl;
                // Note: Since rho0 is most rapidly varying index in sector enumeration, we could just 
                // calculate the sector_index by offsetting from the sector with rho0=1.
                int sector_index=sectors.LookUpSectorIndex(i_space,j_space,rho0);
                assert(sector_index!=-1);
                int row_index=group_i.start_index+gi;
                int column_index=group_j.start_index+gj;
                // std::cout<<fmt::format("sector {} row {} column {} matrix ({},{})  {}",
                //   sector_index, row_index,column_index, matrix_vector[sector_index].rows(),
                //   matrix_vector[sector_index].cols(),rme)<<std::endl;
                // std::cout<<"sector index "<<sector_index<<std::endl;
                matrix_vector[sector_index](row_index,column_index)=scale_factor*rme;
                // std::cout<<matrix_vector[sector_index]<<std::endl;
              }
        // std::cout<<"finished reading in "<<std::endl;
        // for(int i=0; i<matrix_vector.size(); ++i)
        //   std::cout<<matrix_vector[i]<<std::endl;
        
      }

    // close file
    in_stream.close();
  };

  void 
  ReadLSU3ShellRMEsBinary(
      const std::string& filename,
      const LSU3BasisTable& lsu3_basis_table,
      const u3shell::SpaceU3SPN& space, 
      const u3shell::OperatorLabelsU3ST& operator_labels,
      const u3shell::SectorsU3SPN& sectors,
      basis::MatrixVector& matrix_vector,
      double scale_factor
    )
  {
    // open file
    std::ifstream in_stream(filename,std::ios_base::in|std::ios_base::binary);
    StreamCheck(bool(in_stream),filename,"Failure opening lsu3shell rme file");

    // process stream
    basis::SetOperatorToZero(sectors,matrix_vector);
    while(in_stream)
      {
        // read bra/ket lsu3shell basis multiplicity group indices
        int i,j;
        mcutils::ReadBinary<int>(in_stream,i);
        mcutils::ReadBinary<int>(in_stream,j);
        // std::cout<<i<<" "<<j<<std::endl;

        // quit if this has brought us past end of file
        if (!in_stream)
          break;

        // retrieve lsu3shell basis multiplicity group information
        u3shell::U3SPN omegaSPNi, omegaSPNj;
        assert((i<lsu3_basis_table.size())&&(j<lsu3_basis_table.size()));
        const LSU3BasisGroupData& group_i = lsu3_basis_table[i];
        const LSU3BasisGroupData& group_j = lsu3_basis_table[j];
        u3::SU3 xi(group_i.omegaSPN.SU3());
        u3::SU3 xj(group_j.omegaSPN.SU3());
        // std::cout<<fmt::format("{}  {}  {}", group_i.omegaSPN.Str(), operator_labels.Str(),group_j.omegaSPN.Str())<<std::endl;
        int rho0_max=u3::OuterMultiplicity(xj,operator_labels.x0(),xi);
        // std::cout<<group_i.dim<<"  "<<group_j.dim<<"  "<<rho0_max<<std::endl;
        int i_subspace_index=space.LookUpSubspaceIndex(group_i.omegaSPN);
        int j_subspace_index=space.LookUpSubspaceIndex(group_j.omegaSPN);
        assert((i_subspace_index!=basis::kNone)&&(j_subspace_index!=basis::kNone));

        // verify multiplicity given in file
        mcutils::VerifyBinary<int>(
            in_stream,rho0_max,
            fmt::format("Unexpected value encountered reading binary rme file {}",filename),"rho0_max"
          );
        
        // extract and store matrix elements
        for(int gi=0; gi<group_i.dim; ++gi)
          for(int gj=0; gj<group_j.dim; ++gj)
            for(int rho0=1; rho0<=rho0_max; ++rho0)
              {
                // read rme
                float rme;
                mcutils::ReadBinary<float>(in_stream,rme);
                // std::cout<<fmt::format("{} {}  {} {} {}  {}",i,j,i_subspace_index,j_subspace_index,rho0,rme)<<std::endl;

                // suppress zero values -- no longer appropriate/necessary
                // if(fabs(rme)<zero_threshold)
                //   continue;

                // store rme

                // Note: Since rho0 is most rapidly varying index in sector enumeration, we could just 
                // calculate the sector_index by offsetting from the sector with rho0=1.
                int sector_index=sectors.LookUpSectorIndex(i_subspace_index,j_subspace_index,rho0);
                assert(sector_index!=basis::kNone);
                int row_index=group_i.start_index+gi;
                int column_index=group_j.start_index+gj;
                // std::cout<<fmt::format("sector {} row {} column {} matrix ({},{})  {}",
                //   sector_index, row_index,column_index, matrix_vector[sector_index].rows(),
                //   matrix_vector[sector_index].cols(),rme)<<std::endl;
                // std::cout<<"sector index "<<sector_index<<std::endl;
                matrix_vector[sector_index](row_index,column_index)=scale_factor*rme;
                // std::cout<<matrix_vector[sector_index]<<std::endl;
              }
        // std::cout<<"finished reading in "<<std::endl;
        // for(int i=0; i<matrix_vector.size(); ++i)
        //   std::cout<<matrix_vector[i]<<std::endl;
        
      }
    // std::cout<<"finished reading in "<<std::endl;
    // for(int i=0; i<matrix_vector.size(); ++i)
    //   std::cout<<matrix_vector[i]<<std::endl;

    // close file
    in_stream.close();
  };

  void 
  ReadLSU3ShellRMEs(
      const std::string& filename,
      const LSU3BasisTable& lsu3_basis_table,
      const u3shell::SpaceU3SPN& space, 
      const u3shell::OperatorLabelsU3ST& operator_labels,
      const u3shell::SectorsU3SPN& sectors,
      basis::MatrixVector& matrix_vector,
      double scale_factor
    )
  {
    if (g_rme_binary_format)
      // binary format
      ReadLSU3ShellRMEsBinary(filename,lsu3_basis_table,space,operator_labels,sectors,matrix_vector,scale_factor);
    else
      // text format
      ReadLSU3ShellRMEsText(filename,lsu3_basis_table,space,operator_labels,sectors,matrix_vector,scale_factor);
  }



  bool 
  CompareLSU3ShellRMEs(
      std::ostream& log_stream,
      const U3SPNBasisLSU3Labels& basis_provenance,
      const u3shell::SpaceU3SPN& space, 
      const u3shell::SectorsU3SPN& sectors,
      const basis::MatrixVector& matrices1,
      const basis::MatrixVector& matrices2,
      double tolerance,
      bool verbose
    )
  {  

    // initialize statistics
    int entries_compared = 0;
    double max_residual = 0.;
    double total_sqr_residual = 0.;
    bool success = true;

    // iterate over sectors
    for (int sector_index=0; sector_index<sectors.size(); ++sector_index)
      {

        // retrieve sector
        const u3shell::SectorsU3SPN::SectorType& sector = sectors.GetSector(sector_index);
        if (verbose)
          log_stream
            << fmt::format(
                "sector {} bra {} ket {} dim {}x{}",
                sector_index,
                sector.bra_subspace().LabelStr(),
                sector.ket_subspace().LabelStr(),
                sector.bra_subspace().size(),
                sector.ket_subspace().size()
              )
            << std::endl;

        // iterate over matrix elements
        for (int bra_index=0; bra_index<sector.bra_subspace().size(); ++bra_index)
          for (int ket_index=0; ket_index<sector.ket_subspace().size(); ++ket_index)
            {
              // retrieve matrix elements
              double rme1 = matrices1[sector_index](bra_index,ket_index);
              double rme2 = matrices2[sector_index](bra_index,ket_index);

              // compare matrix elements
              double residual = std::fabs(rme2-rme1);
              ++entries_compared;
              max_residual = std::max(max_residual,residual);
              total_sqr_residual += sqr(residual);
              bool entries_agree = (residual <= tolerance);
              // std::cout<<std::endl<<fmt::format("residual {}  tolerance {}  bool {}", residual,tolerance,entries_agree)<<std::endl<<std::endl;
              success &= entries_agree;

              // write entry diagnostics
              if (!entries_agree)
                {
                  log_stream
                    << fmt::format(
                        "  FAIL: {}:({},{}) rme1 {:e} rme2 {:e} residual {:e}",
                        sector_index,bra_index,ket_index,
                        rme1,rme2,residual
                      )
                    << std::endl;
                  const lsu3shell::LSU3BasisGroupLabels& bra_labels = basis_provenance[sector.bra_subspace_index()][bra_index];
                  const lsu3shell::LSU3BasisGroupLabels& ket_labels = basis_provenance[sector.ket_subspace_index()][ket_index];
                  log_stream
                    << fmt::format(
                        "  bra ip {} in {} Np {} Nn {} ; ket ip {} in {} Np {} Nn {}",
                        bra_labels.ip,bra_labels.in,bra_labels.Np,bra_labels.Nn,
                        ket_labels.ip,ket_labels.in,ket_labels.Np,ket_labels.Nn
                      )
                    << std::endl;
                }
            }
      }

    // generate global diagnostics
    double rms_residual = std::sqrt(total_sqr_residual/entries_compared);
    log_stream
      << fmt::format(
          "entries {} rms_residual {:e} max_residual {:e} total_sqr_residual {:e}",
          entries_compared,rms_residual,max_residual,total_sqr_residual
        )
      << std::endl<<std::endl;
        

    return success;
  }

  // void GenerateNcmMatrixVector_DEPRECATED_TakesStream(
  //     int A,
  //     std::ifstream& is_Nrel,
  //     const lsu3shell::LSU3BasisTable& lsu3_basis_table,
  //     const u3shell::SpaceU3SPN& space,
  //     basis::MatrixVector& matrix_vector
  //   )
  // // DEPRECATED
  // {
  // 
  //   assert(false);  // this deprecated version will fail on binary rme file since only uses ReadLSU3ShellRMEsText_DEPRECATED_TakesStream
  //   assert(is_Nrel.is_open());
  // 
  //   // Read in Nrel matrix elements and populate sectors
  //   u3shell::OperatorLabelsU3ST Nrel_labels(0,u3::SU3(0,0),0,0,0);
  //   basis::MatrixVector Nrel_matrix_vector;
  //   u3shell::SectorsU3SPN Nrel_sectors(space,Nrel_labels,true);
  //   lsu3shell::ReadLSU3ShellRMEsText_DEPRECATED_TakesStream(
  //       is_Nrel,Nrel_labels,lsu3_basis_table,space, 
  //       Nrel_sectors,Nrel_matrix_vector
  //     );
  // 
  //   // Resize vector
  //   matrix_vector.resize(Nrel_matrix_vector.size());
  // 
  //   // Iterate over Nrel subspaces and populate Ncm sectors in matrix_vector
  //   for(int i=0; i<Nrel_matrix_vector.size(); ++i)
  //     {
  //       auto subspace=space.GetSubspace(i);
  //       // eigenvalue of N is given by total number of oscilator quanta minus
  //       // the zero point energy boson 3A/2. 
  //       HalfInt N=subspace.N()-3.*A/2;
  //       // std::cout<<N<<std::endl<<Nrel_matrix_vector[i]<<std::endl;
  //       int dim=subspace.size();
  //       // std::cout<< Eigen::MatrixXd::Identity(dim,dim)*double(N) <<"     "<<Nrel_matrix_vector[i]<<std::endl;
  //       // std::cout<<"Nrel"<<std::endl<<Nrel_matrix_vector[i]<<std::endl;
  //   
  //       // Ncm=N-Nrel
  //       matrix_vector[i]=Eigen::MatrixXd::Identity(dim,dim)*double(N)-Nrel_matrix_vector[i];
  //     }
  // }

  void GenerateLSU3ShellNcmRMEs(
      const u3shell::SpaceU3SPN& space,
      const u3shell::SectorsU3SPN& Nrel_sectors,
      const basis::MatrixVector& Nrel_matrices,
      int A,
      basis::MatrixVector& Ncm_matrices
    )
  {

    // populate matrices for Ncm
    Ncm_matrices.resize(Nrel_sectors.size());
    for(int sector_index=0; sector_index<Nrel_sectors.size(); ++sector_index)
      {
        // define alias for subspace
        //
        // Note: Sectors here are diagonal and in 1-1 correspondence
        // with subspaces.  We do not actually need the space as an
        // argument, but keep it for uniformity.
        assert(Nrel_sectors.GetSector(sector_index).IsDiagonal());
        const auto& subspace = Nrel_sectors.GetSector(sector_index).ket_subspace();

        // calculate Ncm
        //
        // Obtain as Ncm=N-Nrel, where eigenvalue of N is given by
        // total number of oscilator quanta minus the zero point
        // energy 3A/2.
        HalfInt N = subspace.N()-3.*A/2;
        int dim = subspace.size();
        Ncm_matrices[sector_index]
          = Eigen::MatrixXd::Identity(dim,dim)*double(N)-Nrel_matrices[sector_index];
      }

  }



  ////////////////////////////////////////////////////////////////
}// end namespace
