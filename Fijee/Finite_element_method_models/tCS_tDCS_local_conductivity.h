//  Copyright (c) 2014, Yann Cobigo 
//  All rights reserved.     
//   
//  Redistribution and use in source and binary forms, with or without       
//  modification, are permitted provided that the following conditions are met:   
//   
//  1. Redistributions of source code must retain the above copyright notice, this   
//     list of conditions and the following disclaimer.    
//  2. Redistributions in binary form must reproduce the above copyright notice,   
//     this list of conditions and the following disclaimer in the documentation   
//     and/or other materials provided with the distribution.   
//   
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE   
//  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR   
//  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES   
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;   
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND   
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   
//     
//  The views and conclusions contained in the software and documentation are those   
//  of the authors and should not be interpreted as representing official policies,    
//  either expressed or implied, of the FreeBSD Project.  
#ifndef TCS_TDCS__LOCAL_CONDUCTIVITY_H
#define TCS_TDCS__LOCAL_CONDUCTIVITY_H
#include <list>
#include <memory>
#include <string>
#include <mutex>
#include <stdexcept>      // std::logic_error
#include <map>
#include <thread>         // std::thread
#include <random>
//
// Eigen
//
#include <Eigen/Dense>
//
// FEniCS
//
#include <dolfin.h>
// transcranial current stimulation
#include "tCS_model.h"
#include "tCS_current_density_model.h"
//
// pugixml
// same resources than Dolfin
//
#include "Utils/Third_party/pugi/pugixml.hpp"
//
// UCSF project
//
#include "Utils/Minimizers/Minimizer.h"
#include "Utils/Minimizers/Iterative_minimizer.h"
#include "Utils/Minimizers/Downhill_simplex.h"
// 
#include "Physics.h"
#include "Conductivity.h"
#include "Boundaries.h"
#include "Sub_domaines.h"
#include "PDE_solver_parameters.h"
// Validation
#include "Spheres_electric_monopole.h"
//
//
//
typedef std::vector<std::vector<std::pair<dolfin::la_index, dolfin::la_index> > > Global_dof_to_cell_dof;
//
//
//
/*!
 * \file tCS_tDCS_local_conductivity.h
 * \brief brief describe 
 * \author Yann Cobigo
 * \version 0.1
 */

/*! \namespace Solver
 * 
 * Name space for our new package
 *
 */
namespace Solver
{
  /*! \class tCS_tDCS_local_conductivity
   * \brief classe representing tDCS simulation.
   *
   *  This class representing the Physical model for the estimation of skinn/skull conductivity tensors using the transcranial Direct Current Stimulation (tDCS) simulation.
   */
  class tCS_tDCS_local_conductivity : Physics
  {
    typedef std::tuple< 
      double,          /* - 0 - estimation */
      Eigen::Vector3d  /* - 1 - sigma (0) skin, (1) skull compacta, (2) skull spongiosa */
      > Estimation_tuple;
      
  private:
    //! Function space
    std::shared_ptr< tCS_model::FunctionSpace > V_;
    //! Function space
    std::shared_ptr< tCS_current_density_model::FunctionSpace > V_field_;
    //! Sample studied
    int sample_;
    // Head time series potential output file
    std::shared_ptr< File > file_potential_time_series_;
    // Brain time series potential output file
    std::shared_ptr< File > file_brain_potential_time_series_;
    // Time series potential field output file
    std::shared_ptr< File > file_current_density_time_series_;

    //
    // Local conductivity estimation
    // 
    //! Simplex for downhill simplex estimation
    std::vector< Estimation_tuple > simplex_;
    //! vector of conductivity boundary values for each tissues
    //! - 0 - sigma skin
    //! - 1 - sigma skull compacta
    //! - 2 - sigma skull spongiosa
    std::vector< std::tuple<double, double> > conductivity_boundaries_;
    //! Minimizer:
    //!  - Downhill simplex: Utils::Minimizers::Downhill_simplex
    typedef Utils::Minimizers::Downhill_simplex Algorithm;
    std::shared_ptr< Utils::Minimizers::Iterative_minimizer< Algorithm >  > minimizer_algo_;
    
  private:
    std::mutex critical_zone_;

  public:
    /*!
     *  \brief Default Constructor
     *
     *  Constructor of the class tCS_tDCS_local_conductivity
     *
     */
    tCS_tDCS_local_conductivity();
    /*!
     *  \brief Copy Constructor
     *
     *  Constructor is a copy constructor
     *
     */
    tCS_tDCS_local_conductivity( const tCS_tDCS_local_conductivity& ){};
    /*!
     *  \brief Destructeur
     *
     *  Destructor of the class tCS_tDCS_local_conductivity
     */
    virtual ~tCS_tDCS_local_conductivity(){/* Do nothing */};
    /*!
     *  \brief Operator =
     *
     *  Operator = of the class tCS_tDCS_local_conductivity
     *
     */
    tCS_tDCS_local_conductivity& operator = ( const tCS_tDCS_local_conductivity& ){return *this;};
    /*!
     *  \brief Operator ()
     *
     *  Operator () of the class tCS_tDCS_local_conductivity
     *
     */
    virtual void operator ()();
    
  public:
    /*!
     *  \brief Operator ()
     *
     *  Operator () of the class tCS_tDCS_local_conductivity
     *
     */
    double solve( const Eigen::Vector3d& );
    double operator ()( const Eigen::Vector3d& A);
    /*!
     *  \brief Get number of physical events
     *
     *  This method return the number of parallel process for the Physics solver. 
     *
     */
    virtual inline
      int get_number_of_physical_events(){return 1;};
    /*!
     *  \brief XML output
     *
     *  This method generates XML output.
     *
     */
    virtual void XML_output(){};
   /*!
     *  \brief Solution domain extraction
     *
     *  This method extract from the Function solution U the sub solution covering the sub-domains Sub_domains.
     *  The result is a file with the name tDCS_{Sub_domains}.vtu
     *
     *  \param U: Function solution of the Partial Differential Equation.
     *  \param Sub_domains: array of sub-domain we want to extract from U.
     *
     */
    void regulation_factor(const Function& , std::list<std::size_t>& );
  };
}
#endif
