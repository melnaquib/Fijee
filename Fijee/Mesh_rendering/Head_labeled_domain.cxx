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
#include <strings.h>
#include <thread>
#include <omp.h>
//
// UCSF
//
#include "Fijee/Fijee_enum.h"
#include "Head_labeled_domain.h"
#include "CGAL_image_filtering.h"
#include "Labeled_domain.h"
#include "VTK_implicite_domain.h"
#include "Access_parameters.h"
#include "Point_vector.h"
#include "Build_electrodes_list.h"
//
// VTK
//
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>
//
// CGAL
//
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_with_normal_3.h>
#include <CGAL/Image_3.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Mesh_3/Image_to_labeled_function_wrapper.h>
#include <CGAL/Labeled_image_mesh_domain_3.h>
// Implicite functions
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Point_with_normal_3<Kernel> Point_with_normal;
typedef CGAL::Surface_mesh_default_triangulation_3 Triangle_surface;
typedef Triangle_surface::Geom_traits GT;
typedef CGAL::Mesh_3::Image_to_labeled_function_wrapper<CGAL::Image_3, Kernel > Image_wrapper;
typedef Domains::CGAL_image_filtering<CGAL::Image_3, Kernel > Image_filter;
//
// We give a comprehensive type name
//
typedef Domains::Head_labeled_domain Domains_Head_labeled;
typedef Domains::Access_parameters DAp;

#ifdef DEBUG_UCSF
// Time log
vtkSmartPointer<vtkTimerLog> timerLog = 
  vtkSmartPointer<vtkTimerLog>::New();
#endif

//
//
//
Domains_Head_labeled::Head_labeled_domain()
{  
  //
  // Header image's information MUST be the same for eigen values et vectores
  number_of_pixels_x_ = (DAp::get_instance())->get_number_of_pixels_x_();
  number_of_pixels_y_ = (DAp::get_instance())->get_number_of_pixels_y_();
  number_of_pixels_z_ = (DAp::get_instance())->get_number_of_pixels_z_();
  //
  size_of_pixel_size_x_ = (DAp::get_instance())->get_number_of_pixels_x_();
  size_of_pixel_size_y_ = (DAp::get_instance())->get_number_of_pixels_y_();
  size_of_pixel_size_z_ = (DAp::get_instance())->get_number_of_pixels_z_();
  //
  rotation_    = (DAp::get_instance())->get_rotation_();
  translation_ = (DAp::get_instance())->get_translation_();
  //
  qfac_ = (int)rotation_.determinant();
  if ( !(qfac_ == 1 || qfac_ == -1) )
    {
      std::cerr << "qfac = " << qfac_ << endl;
      std::cerr << "qfac value must be -1 or 1." << endl;
      abort();
    }

  //
  // Data initialization
  //
  std::string head_model_inr = (Domains::Access_parameters::get_instance())->get_files_path_output_();
  head_model_inr += std::string("head_model.inr");
  file_inrimage_ = new std::ofstream(head_model_inr.c_str(), std::ios::out | std::ios::binary);
  data_label_    = new char[ 256 * 256 * 256 ];
  data_position_ = new double*[ 256 * 256 * 256 ];

  //
  // initialisation 256 * 256 * 256 voxels set at 0
  for ( int k = 0; k < 256; k++ )
    for ( int j = 0; j < 256; j++ )
      for ( int i = 0; i < 256; i++ )
	{
	  int idx = i + j*256 + k*256*256;
	  data_label_[idx] = NO_SEGMENTATION;
	  // center of the voxel centered into the MRI framework
	  data_position_[idx] = new double[3];
	  data_position_[idx][0] = i + 0.5; 
	  data_position_[idx][1] = j + 0.5;  
	  data_position_[idx][2] = k + 0.5;
	}

  // 
  //Create the INRIMAGE
  //
  std::string header  = "#INRIMAGE-4#{\n";
  header +="XDIM=256\n"; // x dimension
  header +="YDIM=256\n"; // y dimension
  header +="ZDIM=256\n"; // z dimension
  header +="VDIM=1\n";   // number of scalar per voxel 
                         // (1 = scalar, 3 = 3D image of vectors)
  header +="TYPE=unsigned fixed\n";
  header +="PIXSIZE=8 bits\n"; // 8, 16, 32, or 64
  header +="CPU=decm\n";
  header +="VX=1\n"; // voxel size in x
  header +="VY=1\n"; // voxel size in y
  header +="VZ=1\n"; // voxel size in z
  std::string headerEND = "##}\n";
  
  //
  // output .inr header
  int hlen = 256 - header.length() - headerEND.length();
  for (int i = 0 ; i < hlen ; i++)
    header += '\n';
  //
  header += headerEND;
  file_inrimage_->write(  header.c_str(), header.size() );
}
////
////
////
//Domains_Head_labeled::Head_labeled_domain( const Domains_Head_labeled& that )
//{
//}
////
////
////
//Domains_Head_labeled::Head_labeled_domain( Domains_Head_labeled&& that )
//{
//}
//
//
//
Domains_Head_labeled::~Head_labeled_domain()
{  
  // close INRIMAGE file
  file_inrimage_->close();
  delete file_inrimage_;

  //
  // clean-up the arrays
  delete [] data_label_;
  //
  for ( int k = 0; k < 256; k++ )
    for ( int j = 0; j < 256; j++ )
      for ( int i = 0; i < 256; i++ )
	delete [] data_position_[i + j*256 + k*256*256];
  delete [] data_position_;

}
////
////
////
//Domains_Head_labeled& 
//Domains_Head_labeled::operator = ( const Domains_Head_labeled& that )
//{
//  if ( this != &that ) 
//    {
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
//    }
//  //
//  return *this;
//}
////
////
////
//Domains_Head_labeled& 
//Domains_Head_labeled::operator = ( Domains_Head_labeled&& that )
//{
//  if( this != &that )
//    {
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
//    }
//  //
//  return *this;
//}
//
//
//
void
Domains_Head_labeled::operator()()
{
  model_segmentation();
  write_inrimage_file();
}
//
//
//
void
Domains_Head_labeled::model_segmentation()
{
#ifdef DEBUG_UCSF
  timerLog->GetUniversalTime();
#endif
  
  //
  // Skull and scalp
#ifdef DEBUG_UCSF
  timerLog->MarkEvent("Skull and scalp");
#endif
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    outside_scalp( (DAp::get_instance())->get_outer_skin_surface_() );
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    outside_skull( (DAp::get_instance())->get_outer_skull_surface_() );
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    inside_skull( (DAp::get_instance())->get_inner_skull_surface_() );
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    inside_brain( (DAp::get_instance())->get_inner_brain_surface_() );
  //  
  //  outside_scalp( data_position_ );
  //  outside_skull( data_position_ );
  //  inside_skull ( data_position_ );
  std::thread outside_scalp_thread(std::ref(outside_scalp), data_position_);
  std::thread outside_skull_thread(std::ref(outside_skull), data_position_);
  std::thread inside_skull_thread (std::ref(inside_skull), data_position_);
  std::thread inside_brain_thread (std::ref(inside_brain), data_position_);
  outside_scalp_thread.join();
  outside_skull_thread.join();
  inside_skull_thread.join();
  inside_brain_thread.join();

  //
  // Cortical segmentation
#ifdef DEBUG_UCSF
  timerLog->MarkEvent("Cortical segmentation");
#endif
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    left_gray_matter ( (DAp::get_instance())->get_lh_pial_() );
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    right_gray_matter ( (DAp::get_instance())->get_rh_pial_() );
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    left_white_matter ( (DAp::get_instance())->get_lh_smoothwm_() );
  Labeled_domain< VTK_implicite_domain, GT::Point_3, std::list< Point_vector > > 
    right_white_matter ( (DAp::get_instance())->get_rh_smoothwm_() );
  //
  //  left_gray_matter( data_position_ );
  //  right_gray_matter( data_position_ );
  //  left_white_matter( data_position_ );
  //  right_white_matter( data_position_ );
  std::thread left_gray_matter_thread(std::ref(left_gray_matter), data_position_);
  std::thread right_gray_matter_thread(std::ref(right_gray_matter), data_position_);
  std::thread left_white_matter_thread(std::ref(left_white_matter), data_position_);
  std::thread right_white_matter_thread(std::ref(right_white_matter), data_position_);

  //
  // Electrodes localization
  Domains::Build_electrodes_list electrodes;
  electrodes.adjust_cap_positions_on( outside_scalp, inside_skull );
  // Write electrodes XML file
  electrodes.Output_electrodes_list_xml();

  //
  // End of cortical segmentation
  left_gray_matter_thread.join();
  right_gray_matter_thread.join();
  left_white_matter_thread.join();
  right_white_matter_thread.join();
  

  //
  // Subcortical segmenation
  // FreeSurfer output aseg.mgz
#ifdef DEBUG_UCSF
  timerLog->MarkEvent("Subcortical segmenation");
#endif
  CGAL::Image_3 aseg;
  aseg.read( (DAp::get_instance())->get_aseg_hdr_() );
  Image_wrapper subcortical_brain ( aseg );

  //
  // SPM volumes
  //

  // Skull
  CGAL::Image_3 SPM_bones;
  SPM_bones.read( (DAp::get_instance())->get_sc4T1_() );
  Image_filter spm_bones( SPM_bones, data_position_ );
  spm_bones.init( 5 /* % of outliers to remove */,
          85 /* % of the signal */);
  //  spm_bones.holes_detection();
  std::thread bones_thread( std::ref(spm_bones) );


  // Skin
  CGAL::Image_3 SPM_skin;
  SPM_skin.read( (DAp::get_instance())->get_sc5T1_() );
  Image_filter  spm_skin( SPM_skin, data_position_ );
  spm_skin.init( 10 /* % of outliers to remove */,
		 80 /* % of the signal */);

  // Air
  CGAL::Image_3 SPM_air;
  SPM_air.read( (DAp::get_instance())->get_sc6T1_() );
  //  Image_wrapper spm_air( SPM_air );
  Image_filter  air( SPM_air, data_position_ );
  air.init( 10 /* % of outliers to remove */,
	    70 /* % of the signal */);

  // CSF
  CGAL::Image_3 SPM_csf;
  SPM_csf.read( (DAp::get_instance())->get_sc3T1_() );
  Image_filter spm_csf( SPM_csf, data_position_ );
  spm_csf.init( 10 /* % of outliers to remove */,
		70 /* % of the signal */);
  spm_csf.eyes_detection();

  //
  // 
  bones_thread.join();
  

  //
  // main loop building inrimage data
  // 

#ifdef DEBUG_UCSF
  timerLog->MarkEvent("building inrimage data");
#endif
  Eigen::Matrix< float, 3, 1 > position;
  // speed-up
  bool is_in_Scalp = false;
  bool is_in_Skull = false;
  bool is_in_Bone  = false;
  bool is_in_CSF   = false;
  //
  // create a data_label_tmp private in the different
  for ( int k = 0; k < 256; k++ )
    for ( int j = 0; j < 256; j++ )
      for ( int i = 0; i < 256; i++ )
	{
	  //
	  // 
	  is_in_Scalp = false;
	  is_in_Skull = false;
	  is_in_Bone  = false;
	  is_in_CSF   = false;

	  // 
	  // 
 	  int idx = i + j*256 + k*256*256;
	  //
	  position <<
	    /* size_of_pixel_size_x_ * */ data_position_[idx][0],
	    /* size_of_pixel_size_y_ * */ data_position_[idx][1],
	    /* size_of_pixel_size_z_ * */ data_position_[idx][2];
	  position = rotation_ * position + translation_;
	  //
	  GT::Point_3 cell_center(position(0,0),
				  position(1,0),
				  position(2,0));
	  //	  std::cout << position << std::endl << std::endl;
	  //
	  GT::Point_3 cell_center_aseg(data_position_[idx][0],
				       data_position_[idx][1],
				       data_position_[idx][2]);

	  //
	  // Electrodes position
	  //
	  if( electrodes.inside_domain( cell_center ) ) 
	    data_label_[ idx ] = ELECTRODE; 
	  
	  //
	  // Head segmentation
	  //
	  
	  //
	  // Scalp
	  if( outside_scalp.inside_domain( cell_center ) )
	    {
	      data_label_[ idx ] = OUTSIDE_SCALP; 
	      is_in_Scalp = true;
	    }
	  //
	  if (!is_in_Scalp && spm_skin.inside(cell_center_aseg) )
	    {
	      data_label_[ idx ] = OUTSIDE_SCALP;
	      is_in_Scalp = true;
	    }
	  //
	  if( outside_skull.inside_domain( cell_center ) )
	    {
	      is_in_Skull = true;
	    }

	  // 
	  // Skull compacta and songiosa
	  if ( is_in_Skull )
	    {
	      if( spm_bones.inside(cell_center_aseg) )
		{
		  data_label_[ idx ] = OUTSIDE_SKULL;
		  is_in_Bone = true;
		}
	      else if( spm_bones.in_hole(cell_center_aseg) )
		{
		  data_label_[ idx ] = SPONGIOSA_SKULL; 
		  is_in_Bone = true;
		}
	    }

	  //
	  // CSF
	  if( is_in_Scalp )
	    {
	      if( (inside_brain.inside_domain(cell_center) && !is_in_Bone ) || 
		  spm_csf.inside(cell_center_aseg) )
		{
		  data_label_[ idx ] = CEREBROSPINAL_FLUID; 
		  is_in_CSF = true;
		}
	      else
		is_in_CSF = false;
	    }

//AIR	  // 
//AIR	  // Air
//AIR	  if( is_in_Skull && !is_in_Bone  )
//AIR	    if( air.inside(cell_center_aseg) )
//AIR	      {
//AIR		data_label_[ idx ] = AIR_IN_SKULL; 
//AIR	      }

	  //
	  // Eyes
	  if ( is_in_Scalp && !is_in_CSF  )
	    if ( spm_csf.in_hole(cell_center_aseg) ) 
	      {
		data_label_[ idx ] = EYE;
	      }

	  //
	  // Brain segmentation
	  //
	  
	  if ( is_in_CSF ) 
	    {
	      //
	      // Gray-matter
	      if( left_gray_matter.inside_domain( cell_center ) )
		data_label_[ idx ] = LEFT_GRAY_MATTER;
	      //
	      if( right_gray_matter.inside_domain( cell_center ) )
		data_label_[ idx ] = RIGHT_GRAY_MATTER;
	      
	      //
	      //	  if( subcortical_brain(cell_center_aseg) == RIGHT_CEREBRAL_CORTEX || 
	      //	      subcortical_brain(cell_center_aseg) == LEFT_CEREBRAL_CORTEX  )
	      //	    data_label[ idx ] = GRAY_MATTER;
	      
	      //
	      // White-matter
	      if( right_white_matter.inside_domain( cell_center ) || 
		  left_white_matter.inside_domain( cell_center ) )
		data_label_[ idx ] = WHITE_MATTER;

	      //
	      //	  if( subcortical_brain(cell_center_aseg) == WM_HYPOINTENSITIES          || 
	      //	      subcortical_brain(cell_center_aseg) == RIGHT_CEREBRAL_WHITE_MATTER || 
	      //	      subcortical_brain(cell_center_aseg) ==  LEFT_CEREBRAL_WHITE_MATTER )
	      //	    data_label[ idx ] = WHITE_MATTER;

	      //
	      // Cerebellum
	      if( subcortical_brain(cell_center_aseg) == RIGHT_CEREBELLUM_CORTEX || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_CEREBELLUM_CORTEX )
		data_label_[ idx ] = CEREBELLUM_GRAY_MATTER;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_CEREBELLUM_WHITE_MATTER || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_CEREBELLUM_WHITE_MATTER )
		data_label_[ idx ] = CEREBELLUM_WHITE_MATTER;

	      //
	      // Subcortex
	      if( subcortical_brain(cell_center_aseg) == BRAIN_STEM )
		data_label_[ idx ] = BRAIN_STEM_SUBCORTICAL;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_HIPPOCAMPUS || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_HIPPOCAMPUS )
		data_label_[ idx ] = HIPPOCAMPUS;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_AMYGDALA || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_AMYGDALA )
		data_label_[ idx ] = AMYGDALA_SUBCORTICAL;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_CAUDATE || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_CAUDATE )
		data_label_[ idx ] = CAUDATE;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_PUTAMEN || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_PUTAMEN )
		data_label_[ idx ] = PUTAMEN;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_THALAMUS_PROPER || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_THALAMUS_PROPER )
		data_label_[ idx ] = THALAMUS;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_ACCUMBENS_AREA || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_ACCUMBENS_AREA )
		data_label_[ idx ] = ACCUMBENS;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_PALLIDUM || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_PALLIDUM )
		data_label_[ idx ] = PALLIDUM;
	      //
	      if( subcortical_brain(cell_center_aseg) == CSF                     || 
		  subcortical_brain(cell_center_aseg) == RIGHT_LATERAL_VENTRICLE || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_LATERAL_VENTRICLE || 
		  subcortical_brain(cell_center_aseg) == RIGHT_INF_LAT_VENT      || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_INF_LAT_VENT      ||
		  subcortical_brain(cell_center_aseg) == FOURTH_VENTRICLE        || 
		  subcortical_brain(cell_center_aseg) == THIRD_VENTRICLE         || 
		  subcortical_brain(cell_center_aseg) == FIFTH_VENTRICLE         )
		data_label_[ idx ] = CEREBROSPINAL_FLUID;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_CHOROID_PLEXUS || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_CHOROID_PLEXUS )
		data_label_[ idx ] = PLEXUS;
	      //
	      if( subcortical_brain(cell_center) ==  FORNIX )
		data_label_[ idx ] = FORNIX_SUBCORTICAL;
	      //
	      if( subcortical_brain(cell_center_aseg) == CC_POSTERIOR     || 
		  subcortical_brain(cell_center_aseg) == CC_MID_POSTERIOR || 
		  subcortical_brain(cell_center_aseg) == CC_CENTRAL       || 
		  subcortical_brain(cell_center_aseg) == CC_MID_ANTERIOR  || 
		  subcortical_brain(cell_center_aseg) == CC_ANTERIOR      )
		data_label_[ idx ] = CORPUS_COLLOSUM;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_VESSEL || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_VESSEL )
		data_label_[ idx ] = VESSEL;
	      //
	      if( subcortical_brain(cell_center_aseg) == RIGHT_VENTRALDC || 
		  subcortical_brain(cell_center_aseg) ==  LEFT_VENTRALDC )
		data_label_[ idx ] = VENTRAL_DIENCEPHALON;
	      //
	      if( subcortical_brain(cell_center_aseg) == OPTIC_CHIASM )
		data_label_[ idx ] = OPTIC_CHIASM_SUBCORTICAL;
	    }
	}

  //
  // Save the surfaces information
  (DAp::get_instance())->set_lh_gray_matter_surface_point_normal_( left_gray_matter.get_point_normal() );
  (DAp::get_instance())->set_rh_gray_matter_surface_point_normal_( right_gray_matter.get_point_normal() );
  (DAp::get_instance())->set_lh_white_matter_surface_point_normal_( left_white_matter.get_point_normal() );
  (DAp::get_instance())->set_rh_white_matter_surface_point_normal_( right_white_matter.get_point_normal() );
  // match white matter vertices with gray matter vertices
  (DAp::get_instance())->epitaxy_growth();
 
  //
  //
#ifdef DEBUG_UCSF
  //
  // Time log 
  std::cout << "Head_labeled_domain - event log:" << *timerLog << std::endl;
#endif
}
//
//
//
std::ostream& 
Domains::operator << ( std::ostream& stream, 
		       const Domains_Head_labeled& that)
{
  // std::for_each( that.get_list_position().begin(),
  //		 that.get_list_position().end(),
  //		 [&stream]( int Val )
  //		 {
  //		   stream << "list pos = " << Val << "\n";
  //		 });
  // //
  // stream << "position x = " <<    that.get_pos_x() << "\n";
  // stream << "position y = " <<    that.get_pos_y() << "\n";
  // if ( &that.get_tab() )
  //   {
  //     stream << "tab[0] = "     << ( &that.get_tab() )[0] << "\n";
  //     stream << "tab[1] = "     << ( &that.get_tab() )[1] << "\n";
  //     stream << "tab[2] = "     << ( &that.get_tab() )[2] << "\n";
  //     stream << "tab[3] = "     << ( &that.get_tab() )[3] << "\n";
  //   }
  //
  return stream;
};
