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
#ifndef EM_ALGORITHM_H
#define EM_ALGORITHM_H
/*!
 * \file EM_algorithm.h
 * \brief brief describe 
 * \author Yann Cobigo
 * \version 0.1
 */
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>    // std::sort
#include <cmath> 
#include <ctgmath>
//
// UCSF
//
#include "Minimizer.h"
#include "Fijee/Fijee_log_management.h"
#include "Fijee/Fijee_exception_handler.h"
#include "Utils/Data_structure/Graph_abstract_data_type.h"
//
//
//
/*! \namespace Utils
 * 
 * Name space for our new package
 *
 */
namespace Utils
{
  /*! \namespace Minimizers
   * 
   * Name space for our new package
   *
   */
  namespace Minimizers
  {
    /*! \class Expectation_maximization
     * \brief classe expectation–maximization (EM) algorithm.
     *
     *  This class is an expectation–maximization (EM) algorithm. EM is an iterative method for finding maximum likelihood or maximum a posteriori (MAP) estimates of parameters in statistical models, where the model depends on unobserved latent variables. The EM iteration alternates between performing an expectation (E) step, which creates a function for the expectation of the log-likelihood evaluated using the current estimate for the parameters, and a maximization (M) step, which computes parameters maximizing the expected log-likelihood found on the E step. These parameter-estimates are then used to determine the distribution of the latent variables in the next E step.(wiki).
     * 
     */
    class Expectation_maximization : public It_minimizer
    {
    private:
      //! Number of sites
      int num_sites_;
       //! Number of sub sites
      int num_sub_sites_;
      //! Measure
      const unsigned char* measure_;
      //! Measure
      const int* bg_mask_;
      //! Clusters
       int* clusters_;


      //
      // Sufficient statistics
      // 

      //! Number of clusters
      int num_clusters_;
      //! Mu
      double* mu_;
      //! Sigma squared
      double* sigma_2_;
      //! Number of sites per cluster
      int* nk_;
      

      // 
      // Graph neigborhood system
      // 
      
      //! Graph neigborhood system
      Utils::Data_structure::Graph_abstract_data_type<int> neighborhood_system_;
    
      
    public:
      /*!
       *  \brief Default Constructor
       *
       *  Constructor of the class Expectation_maximization_sphere
       *
       */
      Expectation_maximization( const int, const unsigned char*, const int*, int*,
				const int, double*,
				const Utils::Data_structure::Graph_abstract_data_type<int>& );
      /*!
       *  \brief Destructeur
       *
       *  Destructor of the class Expectation_maximization_sphere
       */
      virtual ~Expectation_maximization(){/* Do nothing*/};
//      /*!
//       *  \brief Copy Constructor
//       *
//       *  Constructor is a copy constructor
//       *
//       */
//    Expectation_maximization( const Expectation_maximization& that):It_minimizer(that){};
//      /*!
//       *  \brief Operator =
//       *
//       *  Operator = of the class Expectation_maximization_sphere
//       *
//       */
//      Expectation_maximization& operator = ( const Expectation_maximization& that)
//	{
//	  It_minimizer::operator=(that);
//	  return *this;
//	};

      
    public:
      /*!
       *  \brief minimize function
       *
       *  This method initialize the minimizer
       */
      virtual void initialization( Function,  
				   const std::vector< Estimation_tuple >&,
				   const std::vector< std::tuple<double, double> >& ){};
      /*!
       *  \brief minimize function
       *
       *  This method launch the minimization algorithm
       */
      virtual void minimize();

    private:
      /*!
       *  \brief Expectation–maximization for Gaussian Mixture Model
       *
       *  This method for Expectation–maximization for Gaussian Mixture Model
       */
      void GMM_EM();
      void GMM_EM2();
      /*!
       *  \brief Expectation–maximization for Hidden Markov Random Field Model
       *
       *  This method for Expectation–maximization for Hidden Markov Random Field Model
       */
      void HMRF_EM();
    };
    /*!
     *  \brief Dump values for Electrode
     *
     *  This method overload "<<" operator for a customuzed output.
     *
     */
    std::ostream& operator << ( std::ostream&, const Expectation_maximization& );
  }
}
#endif
