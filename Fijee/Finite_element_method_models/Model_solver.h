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
#ifndef _MODEL_SOLVER_H
#define _MODEL_SOLVER_H
#include <memory>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
//
// FEniCS
//
#include <dolfin.h>
//
//
// UCSF project
//
#include "PDE_solver_parameters.h"
#include "Fijee/Fijee_thread_dispatching.h"
//
//
//
typedef Solver::PDE_solver_parameters SDEsp;
//
//
//
//using namespace dolfin;
//
/*!
 * \file Model_solver.h
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
  /*! \class Model_solver
   * \brief class representing the model used in the study
   *
   *  This class is a template for models used in a finite element study.
   */
  template < typename Physical_model, int num_of_threads = 1 >
  class Model_solver
  {
  private:
    Physical_model model_;

  public:
    /*!
     *  \brief Default Constructor
     *
     *  Constructor of the class Model_solver
     *
     */
  Model_solver(){};
    /*!
     *  \brief Copy Constructor
     *
     *  Constructor is a copy constructor
     *
     */
    Model_solver( const Model_solver& ) = delete;
    /*!
     *  \brief Operator =
     *
     *  Operator = of the class Model_solver
     *
     */
  Model_solver& operator = ( const Model_solver& ) = delete;

  public:
    /*!
     *   \brief Solver loop
     * 
     * This method loops over time series, leadfield matrix, ... files and process the specified model
     * 
     */
    void solver_loop()
    {
      
      //
      // Define the number of threads in the pool of threads
      Fijee::Thread_dispatching pool( num_of_threads );
      
      
      //
      //
//      int tempo = 0;
      for( int physical_event = 0 ;
	   physical_event != model_.get_number_of_physical_events() ; 
	   physical_event++ )
//	if( tempo++ < 10 )
	  {
	    //
	    // Enqueue tasks
	    pool.enqueue( std::ref(model_) );
	    // 
	    // pospone the next launch 0.5 second
	    std::this_thread::sleep_for( std::chrono::microseconds(5000) );
	  }
//	else {break;}
    }
    /*!
     *   \brief XML output
     * 
     * This method generates XML output.
     *
     */
    void XML_output()
    {
      model_.XML_output();
    }
  };
}
#endif
