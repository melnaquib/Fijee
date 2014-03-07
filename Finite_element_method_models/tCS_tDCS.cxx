#include <iostream>
#include "tCS_tDCS.h"

typedef Solver::PDE_solver_parameters SDEsp;

//
//
//
Solver::tCS_tDCS::tCS_tDCS():Physics()
{
  //
  // Define the function space
  V_.reset( new tCS_model::FunctionSpace(mesh_) );
  V_field_.reset( new tCS_field_model::FunctionSpace(mesh_) );

  //
  // Read the electrodes xml file
  electrodes_.reset( new Electrodes_setup() );

  //
  // Boundary marking
  //
  
  //
  // Boundary conditions
  std::cout << "Load boundaries" << std::endl;

  //
  // Load the facets collection
  std::cout << "Load facets collection" << std::endl;
  std::string facets_collection_xml = (SDEsp::get_instance())->get_files_path_output_();
  facets_collection_xml += "mesh_facets_subdomains.xml";
  //
  mesh_facets_collection_.reset( new MeshValueCollection< std::size_t > (*mesh_, facets_collection_xml) );

  //
  // MeshDataCollection methode
  // Recreate the connectivity
  // Get mesh connectivity D --> d
  const std::size_t d = mesh_facets_collection_->dim();
  const std::size_t D = mesh_->topology().dim();
  dolfin_assert(d == 2);
  dolfin_assert(D == 3);

  //
  // Generate connectivity if it does not excist
  mesh_->init(D, d);
  const MeshConnectivity& connectivity = mesh_->topology()(D, d);
  dolfin_assert(!connectivity.empty());
  
  //
  // Map the facet index with cell index
  std::map< std::size_t, std::size_t > map_index_cell;
  typename std::map<std::pair<std::size_t, std::size_t>, std::size_t>::const_iterator it;
  const std::map<std::pair<std::size_t, std::size_t>, std::size_t>& values
    = mesh_facets_collection_->values();
  // Iterate over all values
  for ( it = values.begin() ; it != values.end() ; ++it )
    {
      // Get value collection entry data
      const std::size_t cell_index = it->first.first;
      const std::size_t local_entity = it->first.second;
      const std::size_t value = it->second;

      std::size_t entity_index = 0;
      // Get global (local to to process) entity index
      //      dolfin_assert(cell_index < mesh_->num_cells());
      map_index_cell[connectivity(cell_index)[local_entity]] = cell_index;
 
      // Set value for entity
      //  dolfin_assert(entity_index < _size);
    }

  
  //
  // Define boundary condition
  boundaries_.reset( new MeshFunction< std::size_t >(mesh_) );
  *boundaries_ = *mesh_facets_collection_;
  //
  boundaries_->rename( mesh_facets_collection_->name(),
		       mesh_facets_collection_->label() );
  //
  mesh_facets_collection_.reset();

  //
  // Boundary definition
  Electrodes_surface electrodes_101( electrodes_, boundaries_, map_index_cell );
  //
  electrodes_101.mark( *boundaries_, 101 );
  electrodes_101.surface_vertices_per_electrodes( 101 );
  // write boundaries
  std::string boundaries_file_name = (SDEsp::get_instance())->get_files_path_result_();
  boundaries_file_name            += std::string("boundaries.pvd");
  File boundaries_file( boundaries_file_name.c_str() );
  boundaries_file << *boundaries_;
}
//
//
//
void 
Solver::tCS_tDCS::operator () ( /*Solver::Phi& source,
				  SLD_model::FunctionSpace& V,
				  FacetFunction< size_t >& boundaries*/)
{
//  //
//  // Mutex the electrodes vector poping process
//  //
//  Solver::Current_density source;
//    try {
//      // lock the electrode list
//      std::lock_guard< std::mutex > lock_critical_zone ( critical_zone_ );
//      source = electrodes_list_.front();
//      electrodes_list_.pop_front();
//    }
//    catch (std::logic_error&) {
//      std::cout << "[exception caught]\n";
//    }
//
//  //
//  //
//  std::cout << source.get_name_() << std::endl;
//  
////  //
////  // Define Dirichlet boundary conditions 
////  DirichletBC boundary_conditions(*V, source, perifery);


  //////////////////////////////////////////////////////
  // Transcranial direct current stimulation equation //
  //////////////////////////////////////////////////////
      

  //
  // tDCS electric potential u
  //

//  //
//  // PDE boundary conditions
//  DirichletBC bc(*V_, *(electrodes_->get_current()), *boundaries_, 101);

  //
  // Define variational forms
  tCS_model::BilinearForm a(V_, V_);
  tCS_model::LinearForm L(V_);
      
  //
  // Anisotropy
  // Bilinear
  a.a_sigma  = *sigma_;
  // a.dx       = *domains_;
  
  
  // Linear
  L.I  = *(electrodes_->get_current());
  L.ds = *boundaries_;

  //
  // Compute solution
  Function u(*V_);
  LinearVariationalProblem problem(a, L, u/*, bc*/);
  LinearVariationalSolver  solver(problem);
  // krylov
  solver.parameters["linear_solver"]  
    = (SDEsp::get_instance())->get_linear_solver_();
  solver.parameters("krylov_solver")["maximum_iterations"] 
    = (SDEsp::get_instance())->get_maximum_iterations_();
  solver.parameters("krylov_solver")["relative_tolerance"] 
    = (SDEsp::get_instance())->get_relative_tolerance_();
  solver.parameters["preconditioner"] 
    = (SDEsp::get_instance())->get_preconditioner_();
  //
  solver.solve();


 //
 // Regulation terme:  \int u dx = 0
 double old_u_bar = 0.;
 double u_bar = 1.e+6;
 double U_bar = 0.;
 double N = u.vector()->size();
 int iteration = 0;
 double Sum = 1.e+6;
 //
 //  while ( abs( u_bar - old_u_bar ) > 0.1 )
 while ( fabs(Sum) > 1.e-3 )
   {
     old_u_bar = u_bar;
     u_bar  = u.vector()->sum();
     u_bar /= N;
     (*u.vector()) -= u_bar;
     //
     U_bar += u_bar;
     Sum = u.vector()->sum();
     std::cout << ++iteration << " ~ " << Sum  << std::endl;
   }
 
 std::cout << "int u dx = " << Sum << std::endl;

 //
 // tDCS electric current density field \vec{J}
 // 
 
 //
 // Define variational forms
 tCS_field_model::BilinearForm a_field(V_field_, V_field_);
 tCS_field_model::LinearForm L_field(V_field_);
 
 //
 // Anisotropy
 // Bilinear
 // a.dx       = *domains_;
 
 
 // Linear
 L_field.u       = u;
 L_field.a_sigma = *sigma_;
 //  L.ds = *boundaries_;

 //
 // Compute solution
 Function J(*V_field_);
 LinearVariationalProblem problem_field(a_field, L_field, J/*, bc*/);
 LinearVariationalSolver  solver_field(problem_field);
 // krylov
 solver_field.parameters["linear_solver"]  
   = (SDEsp::get_instance())->get_linear_solver_();
 solver_field.parameters("krylov_solver")["maximum_iterations"] 
   = (SDEsp::get_instance())->get_maximum_iterations_();
 solver_field.parameters("krylov_solver")["relative_tolerance"] 
   = (SDEsp::get_instance())->get_relative_tolerance_();
 solver_field.parameters["preconditioner"] 
   = (SDEsp::get_instance())->get_preconditioner_();
 //
 solver_field.solve();


 //
 // Filter function over a subdomain
 std::list<std::size_t> test_sub_domains{4,5};
 solution_domain_extraction(J, test_sub_domains, "tDCS_Current_density");
 solution_domain_extraction(u, test_sub_domains, "tDCS_potential");
 
 //
 // Validation process
 // !! WARNING !!
 // This process must be pluged only for spheres model
 if( true )
   {
     std::cout << "Validation processing" << std::endl;
     //
     //
     std::cout << "je passe create T7" << std::endl;
     Solver::Spheres_electric_monopole mono_T7( (*electrodes_)["T7"].get_I_(),  
						(*electrodes_)["T7"].get_r0_projection_() );
     //
     std::cout << "je passe create T8" << std::endl;
     Solver::Spheres_electric_monopole mono_T8( (*electrodes_)["T8"].get_I_(),  
						(*electrodes_)["T8"].get_r0_projection_() );
     //
     //
     Function 
       Injection_T7(V_),
       Injection_T8(V_);
     //Function & Injection = Injection_T7;

     //
     std::cout << "je passe T7" << std::endl;
     Injection_T7.interpolate( mono_T7 );
     // std::thread T7(Injection_T7.interpolate, mono_T7);
     std::cout << "je passe T8" << std::endl;
     Injection_T8.interpolate( mono_T8 );
     //
     std::cout << "je passe T7+T8" << std::endl;
     *(Injection_T7.vector()) += *(Injection_T8.vector());
     std::cout << "je passe T7+T8 end" << std::endl;

     //
     // Produce outputs
     std::string file_validation = (SDEsp::get_instance())->get_files_path_result_() + 
       std::string("validation.pvd");
     File validation( file_validation.c_str() );
     //
     validation << Injection_T7;
     std::cout << "je passe" << std::endl;
   }


//  //
//  // Extract subfunctions
//  Function potential = u[0];

  //
  // Save solution in VTK format
  //  * Binary (.bin)
  //  * RAW    (.raw)
  //  * SVG    (.svg)
  //  * XD3    (.xd3)
  //  * XDMF   (.xdmf)
  //  * XML    (.xml) // FEniCS xml
  //  * XYZ    (.xyz)
  //  * VTK    (.pvd) // paraview
  std::string file_name = (SDEsp::get_instance())->get_files_path_result_() + 
    std::string("tDCS.pvd");
  File file( file_name.c_str() );
  //
  file << u;
  //
  std::string field_name = (SDEsp::get_instance())->get_files_path_result_() + 
    std::string("tDCS_field.pvd");
  File field( field_name.c_str() );
  //
  field << J;
};
//
//
//
void
Solver::tCS_tDCS::regulation_factor(const Function& u, std::list<std::size_t>& Sub_domains)
{
  // 
  const std::size_t num_vertices = mesh_->num_vertices();
  
  // Get number of components
  const std::size_t dim = u.value_size();
  
  // Open file
  std::string sub_dom("tDCS");
  for ( auto sub : Sub_domains )
    sub_dom += std::string("_") + std::to_string(sub);
  sub_dom += std::string("_bis.vtu");
  //
  std::string extracted_solution = (SDEsp::get_instance())->get_files_path_result_();
  extracted_solution            += sub_dom;
  //
  std::ofstream VTU_xml_file(extracted_solution);
  VTU_xml_file.precision(16);

  // Allocate memory for function values at vertices
  const std::size_t size = num_vertices * dim; // dim = 1
  std::vector<double> values(size);
  u.compute_vertex_values(values, *mesh_);
 
  //
  // 
  std::vector<int> V(num_vertices, -1);

  //
  // int u dx = 0
  double old_u_bar = 0.;
  double u_bar = 1.e+6;
  double U_bar = 0.;
  double N = size;
  int iteration = 0;
  double Sum = 1.e+6;
  //
  //  while ( abs( u_bar - old_u_bar ) > 0.1 )
  while ( abs(Sum) > .01 || abs((old_u_bar - u_bar) / old_u_bar) > 1. /* % */ )
    {
      old_u_bar = u_bar;
      u_bar = 0.;
      for ( double val : values ) u_bar += val;
      u_bar /= N;
      std::for_each(values.begin(), values.end(), [&u_bar](double& val){val -= u_bar;});
      //
      U_bar += u_bar;
      Sum = 0;
      for ( double val : values ) Sum += val;
      std::cout << ++iteration << " ~ " << Sum  << " ~ " << u_bar << std::endl;
    }

  std::cout << "int u dx = " << Sum << " " << U_bar << std::endl;
  std::cout << "Size = " << Sub_domains.size()  << std::endl;
 

  //
  //
  int 
    num_tetrahedra = 0,
    offset = 0,
    inum = 0;
  //
  std::string 
    vertices_position_string,
    vertices_associated_to_tetra_string,
    offsets_string,
    cells_type_string,
    point_data;
  // loop over mesh cells
  for ( CellIterator cell(*mesh_) ; !cell.end() ; ++cell )
    // loop over extraction sub-domains
//    for( auto sub_domain : Sub_domains ) 
//     if ( (*domains_)[cell->index()] == sub_domain || Sub_domains.size() == 0 )
	{
	  //  vertex id
	  for ( VertexIterator vertex(*cell) ; !vertex.end() ; ++vertex )
	    {
	      if( V[ vertex->index() ] == -1 )
		{
		  //
		  V[ vertex->index() ] = inum++;
		  vertices_position_string += 
		    std::to_string( vertex->point().x() ) + " " + 
		    std::to_string( vertex->point().y() ) + " " +
		    std::to_string( vertex->point().z() ) + " " ;
		  point_data += std::to_string( values[vertex->index()] ) + " ";
		}

	      //
	      // Volume associated
	      vertices_associated_to_tetra_string += 
		std::to_string( V[vertex->index()] ) + " " ;
	    }
      
	  //
	  // Offset for each volumes
	  offset += 4;
	  offsets_string += std::to_string( offset ) + " ";
	  //
	  cells_type_string += "10 ";
	  //
	  num_tetrahedra++;
	}

  //
  // header
  VTU_xml_file << "<?xml version=\"1.0\"?>" << std::endl;
  VTU_xml_file << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">" << std::endl;
  VTU_xml_file << "  <UnstructuredGrid>" << std::endl;

  // 
  // vertices and values
  VTU_xml_file << "    <Piece NumberOfPoints=\"" << inum
	       << "\" NumberOfCells=\"" << num_tetrahedra << "\">" << std::endl;
  VTU_xml_file << "      <Points>" << std::endl;
  VTU_xml_file << "        <DataArray type = \"Float32\" NumberOfComponents=\"3\" format=\"ascii\">" << std::endl;
  VTU_xml_file << vertices_position_string << std::endl;
  VTU_xml_file << "        </DataArray>" << std::endl;
  VTU_xml_file << "      </Points>" << std::endl;
  
  //
  // Point data
  VTU_xml_file << "      <PointData Scalars=\"scalars\">" << std::endl;
  VTU_xml_file << "        <DataArray type=\"Float32\" Name=\"scalars\" format=\"ascii\">" << std::endl; 
  VTU_xml_file << point_data << std::endl; 
  VTU_xml_file << "         </DataArray>" << std::endl; 
  VTU_xml_file << "      </PointData>" << std::endl; 
 
  //
  // Tetrahedra
  VTU_xml_file << "      <Cells>" << std::endl;
  VTU_xml_file << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" << std::endl;
  VTU_xml_file << vertices_associated_to_tetra_string << std::endl;
  VTU_xml_file << "        </DataArray>" << std::endl;
  VTU_xml_file << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" << std::endl;
  VTU_xml_file << offsets_string << std::endl;
  VTU_xml_file << "        </DataArray>" << std::endl;
  VTU_xml_file << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" << std::endl;
  VTU_xml_file << cells_type_string << std::endl;
  VTU_xml_file << "        </DataArray>" << std::endl;
  VTU_xml_file << "      </Cells>" << std::endl;
  VTU_xml_file << "    </Piece>" << std::endl;

  //
  // Tail
  VTU_xml_file << "  </UnstructuredGrid>" << std::endl;
  VTU_xml_file << "</VTKFile>" << std::endl;
}
