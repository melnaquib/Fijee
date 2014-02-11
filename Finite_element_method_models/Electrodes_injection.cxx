#include "Electrodes_injection.h"
//
//
//
Solver::Electrodes_injection::Electrodes_injection()
{}
//
//
//
Solver::Electrodes_injection::Electrodes_injection(const Electrodes_injection& that)
{
  //
  electrodes_vector_.resize( that.electrodes_vector_.size() );
  std::copy( that.electrodes_vector_.begin(), 
	     that.electrodes_vector_.end(), 
	     electrodes_vector_.begin() );
}
//
//
//
void 
Solver::Electrodes_injection::eval(Array<double>& values, const Array<double>& x, const ufc::cell& cell) const
{

  Point vertex( x[0], x[1], x[2] );
//  std::cout << vertex << std::endl;

  //
  // Copy the value
  double tempo_val = 0.;
  // go through all electrodes and check if we have a value.
  for ( int intensity = 0 ; intensity < electrodes_vector_.size() ; intensity++ )
    tempo_val += electrodes_vector_[intensity].eval( vertex, cell );

  //
  //
  values[0] = tempo_val;
}
//
//
//
Solver::Electrodes_injection&
Solver::Electrodes_injection::operator =( const Electrodes_injection& that )
{
  //
  electrodes_vector_.resize( that.electrodes_vector_.size() );
  std::copy( that.electrodes_vector_.begin(), 
	     that.electrodes_vector_.end(), 
	     electrodes_vector_.begin() );
 
  //
  //
  return *this;
}
//
//
//
void 
Solver::Electrodes_injection::add_electrode( std::string Electric_variable, int Index, 
					     std::string Label, double I,
					     Point X, Point V, double Re_z_l, double Im_z_l, 
					     double Surface, double Radius  )
{
  electrodes_vector_.push_back( Intensity(Electric_variable, Index, Label, I, 
					  X, V, Re_z_l, Im_z_l, Surface, Radius) );
}
//
//
//
std::ostream& 
Solver::operator << ( std::ostream& stream, 
		      const Solver::Electrodes_injection& that)
{
  //  //
  //  //
  //  stream 
  //    << "Dipole source -- index: " << that.get_index_() << " -- " 
  //    << "index cell: " << that.get_index_cell_() << "\n"
  //    << "Position:"
  //    << " (" << that.get_X_() << ", " << that.get_Y_() << ", " << that.get_Z_()  << ") " 
  //    << " -- direction: " 
  //    << " (" << that.get_VX_()  << ", " << that.get_VY_() << ", " << that.get_VZ_()  << ") \n"
  //    << "Intensity: " << that.get_Q_() << std::endl;
  
  //
  //
  return stream;
};
//
//
//
bool 
Solver::Electrodes_injection::inside( const Point& Vertex ) const
{
  //
  //
  bool inside_electrode = false;
  //
  for( auto electrode : electrodes_vector_ )
    if ( electrode.get_I_() != 0. )
      if( electrode.get_r0_values_().distance(Vertex) < electrode.get_radius_() + 3. /* mm */ )
	{
	  inside_electrode = true;
	}

  //
  //
  return inside_electrode;
}
//
//
//
std::tuple<std::string, bool> 
Solver::Electrodes_injection::inside_probe( const Point& Vertex ) const
{
  //
  //
  bool inside_electrode = false;
  std::string label;
  //
  for( auto electrode : electrodes_vector_ )
    if ( electrode.get_I_() != 0. )
      if( electrode.get_r0_values_().distance(Vertex) < electrode.get_radius_() + 3. /* mm */ )
	{
	  inside_electrode = true;
	  label =  electrode.get_label_();
	}

  //
  //
  return std::make_tuple (label, inside_electrode);
}
