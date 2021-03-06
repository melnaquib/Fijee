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
//
// Project
//
#include "Spheres_implicite_domain.h"
//
// CGAL
//
//typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
//typedef CGAL::Point_with_normal_3<Kernel> Point_with_normal;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef CGAL::Poisson_reconstruction_function<Kernel> Poisson_reconstruction_function;
typedef CGAL::Implicit_surface_3<Kernel, Poisson_reconstruction_function> Surface_3;
//
// Eigen
//
#include <Eigen/Dense>
//
// VTK
//
typedef Domains::Spheres_implicite_domain Domain;
//
//
//
Domain::Spheres_implicite_domain():
  vtk_mesh_("")
{
  poly_data_bounds_[0] = 0.;
  poly_data_bounds_[1] = 0.;
  poly_data_bounds_[2] = 0.;
  poly_data_bounds_[3] = 0.;
  poly_data_bounds_[4] = 0.;
  poly_data_bounds_[5] = 0.;
}
//
//
//
Domain::Spheres_implicite_domain( const char* Vtk_Mesh ):
  vtk_mesh_( Vtk_Mesh )
{
  //
  // create the stream for CGAL applications
  std::stringstream stream;

  // Delta vectors
  Eigen::Matrix3f delta_rotation;
  delta_rotation <<
    0., 0., 0.,
    0., 0., 0.,
    0., 0., 0.;
  //
  Eigen::Vector3f delta_traslation;
  delta_traslation << 0., 0., 0.;
  // Set the delta transformation
  (Domains::Access_parameters::get_instance())->set_delta_rotation_(delta_rotation);
  (Domains::Access_parameters::get_instance())->set_delta_translation_(delta_traslation);

  //
  // Boundary data
  poly_data_bounds_[0] = 0.;
  poly_data_bounds_[1] = 0.;
  poly_data_bounds_[2] = 0.;
  poly_data_bounds_[3] = 0.;
  poly_data_bounds_[4] = 0.;
  poly_data_bounds_[5] = 0.;
	
  //
  //
  std::string line;
  std::ifstream stream_file( Vtk_Mesh );
  //
  if ( stream_file.is_open() )
    {
      //
      //
      while ( stream_file.good() )
	{
	  // X Y Z n_X n_Y n_Z
	  if ( getline (stream_file,line) )
	    {
	      stream << line << std::endl;
	    }
	}
    }
  else
    {
      std::cerr << "Problem opening the file: " << Vtk_Mesh << std::endl;
      exit(0);
    }

  //
  //
  std::vector<Point_with_normal> Point_normal;
  if (!stream ||
      !CGAL::read_xyz_points_and_normals( stream,
					  std::back_inserter(Point_normal),
					  CGAL::make_normal_of_point_with_normal_pmap( std::vector<Point_with_normal>::value_type() )))
    {
      std::cerr << "Reading error with the stream of point with normal" << std::endl;
      exit(-1);
    }

  //
  // Fill up our own point vector container
  for ( auto pv : Point_normal )
    point_normal_.push_back(Domains::Point_vector( pv.position().x(), pv.position().y(), pv.position().z(),
						   pv.normal().x(), pv.normal().y(), pv.normal().z() ));
  
  //
  // Creates implicit function from the read points using the default solver.
  function_.reset( new Poisson_reconstruction_function( Point_normal.begin(), 
							Point_normal.end(),
							CGAL::make_normal_of_point_with_normal_pmap( std::vector<Point_with_normal>::value_type() )));
}
//
//
//
void
Domain::operator ()( double** Space_Points )
{
  // Computes the Poisson indicator function f()
  // at each vertex of the triangulation.
  // smoother_hole_filling = true: controls if the Delaunay refinement is done for the input 
  // points
  if ( !function_->compute_implicit_function() )
    exit(-1);
  // Check if the surface is inside out
  if( inside_domain( CGAL::Point_3< Kernel > (256., 256., 256.) ) )
    {
      //    function_->flip_f();
      ((*function_).*Fijee::result<Poisson_recon_func_spheres>::ptr)();
    }

}
//
//
//
Domain::Spheres_implicite_domain( const Domain& that )
{
}
//
//
//
Domain::Spheres_implicite_domain( Domain&& that )
{
}
//
//
//
Domain& 
Domain::operator = ( const Domain& that )
{
  if ( this != &that ) 
    {
//      // free existing ressources
//      if( tab_ )
//	{
//	  delete [] tab_;
//	  tab_ = nullptr;
//	}
//      // allocating new ressources
//      pos_x_ = that.get_pos_x();
//      pos_y_ = that.get_pos_y();
//      list_position_ = that.get_list_position();
//      //
//      tab_ = new int[4];
//      std::copy( &that.get_tab(),  &that.get_tab() + 4, tab_ );
    }
  //
  return *this;
}
//
//
//
Domain& 
Domain::operator = ( Domain&& that )
{
  if( this != &that )
    {
//      // initialisation
//      pos_x_ = 0;
//      pos_y_ = 0;
//      delete [] tab_;
//      tab_   = nullptr;
//      // pilfer the source
//      list_position_ = std::move( that.list_position_ );
//      pos_x_ =  that.get_pos_x();
//      pos_y_ =  that.get_pos_y();
//      tab_   = &that.get_tab();
//      // reset that
//      that.set_pos_x( 0 );
//      that.set_pos_y( 0 );
//      that.set_tab( nullptr );
    }
  //
  return *this;
}
//
//
//
std::ostream& 
Domains::operator << ( std::ostream& stream, 
		       const Domain& that)
{
//  std::for_each( that.get_list_position().begin(),
//		 that.get_list_position().end(),
//		 [&stream]( int Val )
//		 {
//		   stream << "list pos = " << Val << "\n";
//		 });
//  //
//  stream << "positions minimum = " 
//	 << that.get_min_x() << " "
//	 << that.get_min_y() << " "
//	 << that.get_min_z() << "\n";
//  stream << "position y = " <<    that.get_pos_y() << "\n";
//  if ( &that.get_tab() )
//    {
//      stream << "tab[0] = "     << ( &that.get_tab() )[0] << "\n";
//      stream << "tab[1] = "     << ( &that.get_tab() )[1] << "\n";
//      stream << "tab[2] = "     << ( &that.get_tab() )[2] << "\n";
//      stream << "tab[3] = "     << ( &that.get_tab() )[3] << "\n";
//    }
  //
  return stream;
};
