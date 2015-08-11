/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if CERES_FOUND

#include <opencv2/sfm/simple_pipeline.hpp>
#include <opencv2/xfeatures2d.hpp>

#include "libmv/simple_pipeline/bundle.h"
#include "libmv/simple_pipeline/initialize_reconstruction.h"
#include "libmv/simple_pipeline/uncalibrated_reconstructor.h"

#include "libmv/simple_pipeline/tracks.h"


using namespace cv;
using namespace std;


namespace cv
{

void
libmv_solveReconstruction( const libmv::Tracks &tracks,
                           int keyframe1, int keyframe2,
                           double focal_length,
                           double principal_x, double principal_y,
                           double k1, double k2, double k3,
                           libmv_EuclideanReconstruction &libmv_reconstruction,
                           int refine_intrinsics )
{
    /* Invert the camera intrinsics. */
    libmv::vector<libmv::Marker> markers = tracks.AllMarkers();
    libmv::EuclideanReconstruction *reconstruction = &libmv_reconstruction.reconstruction;
    libmv::CameraIntrinsics *intrinsics = &libmv_reconstruction.intrinsics;

    intrinsics->SetFocalLength(focal_length, focal_length);
    intrinsics->SetPrincipalPoint(principal_x, principal_y);
    intrinsics->SetRadialDistortion(k1, k2, k3);

    cout << "\tNumber of markers: " << markers.size() << endl;
    for (int i = 0; i < markers.size(); ++i)
    {
        intrinsics->InvertIntrinsics(markers[i].x,
                                     markers[i].y,
                                     &(markers[i].x),
                                     &(markers[i].y));
    }

    libmv::Tracks normalized_tracks(markers);

    cout << "\tframes to init from: " << keyframe1 << " " << keyframe2 << endl;
    libmv::vector<libmv::Marker> keyframe_markers =
        normalized_tracks.MarkersForTracksInBothImages(keyframe1, keyframe2);
    cout << "\tNumber of markers for init: " << keyframe_markers.size() << endl;

    libmv::EuclideanReconstructTwoFrames(keyframe_markers, reconstruction);
    libmv::EuclideanBundle(normalized_tracks, reconstruction);
    libmv::EuclideanCompleteReconstruction(libmv::ReconstructionOptions(), normalized_tracks, reconstruction);

    if (refine_intrinsics)
    {
      libmv::EuclideanBundleCommonIntrinsics( tracks, refine_intrinsics, libmv::BUNDLE_NO_CONSTRAINTS, reconstruction, intrinsics);
    }

    libmv_reconstruction.tracks = tracks;
    libmv_reconstruction.error = libmv::EuclideanReprojectionError(tracks, *reconstruction, *intrinsics);

}


void
libmv_solveReconstruction( const libmv::Tracks &tracks,
                           int keyframe1, int keyframe2,
                           double focal_length,
                           double principal_x, double principal_y,
                           double k1, double k2, double k3,
                           libmv_ProjectiveReconstruction &libmv_reconstruction,
                           int refine_intrinsics )
{

  /* Invert the camera intrinsics. */
  libmv::vector<libmv::Marker> markers = tracks.AllMarkers();
  libmv::ProjectiveReconstruction *reconstruction = &libmv_reconstruction.reconstruction;
  libmv::CameraIntrinsics *intrinsics = &libmv_reconstruction.intrinsics;

  intrinsics->SetFocalLength(focal_length, focal_length);
  intrinsics->SetPrincipalPoint(principal_x, principal_y);
  intrinsics->SetRadialDistortion(k1, k2, k3);

  cout << "\tNumber of markers: " << markers.size() << endl;
  for (int i = 0; i < markers.size(); ++i)
  {
      intrinsics->InvertIntrinsics(markers[i].x,
                                   markers[i].y,
                                   &(markers[i].x),
                                   &(markers[i].y));
  }

  libmv::Tracks normalized_tracks(markers);

  cout << "\tframes to init from: " << keyframe1 << " " << keyframe2 << endl;
  libmv::vector<libmv::Marker> keyframe_markers =
      normalized_tracks.MarkersForTracksInBothImages(keyframe1, keyframe2);
  cout << "\tNumber of markers for init: " << keyframe_markers.size() << endl;

  libmv::ProjectiveReconstructTwoFrames(keyframe_markers, reconstruction);
  libmv::ProjectiveBundle(normalized_tracks, reconstruction);
  libmv::ProjectiveCompleteReconstruction(libmv::ReconstructionOptions(), normalized_tracks, reconstruction);

//  if (refine_intrinsics)
//  {
//    libmv::ProjectiveBundleCommonIntrinsics( tracks, refine_intrinsics, libmv::BUNDLE_NO_CONSTRAINTS, reconstruction, intrinsics);
//  }

  libmv_reconstruction.tracks = tracks;
  libmv_reconstruction.error = libmv::ProjectiveReprojectionError(tracks, *reconstruction, *intrinsics);

}


void
libmv_solveReconstruction( const libmv::Tracks &tracks,
                           int keyframe1, int keyframe2,
                           double focal_length,
                           double principal_x, double principal_y,
                           double k1, double k2, double k3,
                           libmv_UncalibratedReconstruction &libmv_reconstruction,
                           int refine_intrinsics )
{
  UncalibratedReconstructor uncalibrated_reconstructor( principal_x, principal_y, keyframe1, keyframe2, tracks);

  libmv_reconstruction.tracks = uncalibrated_reconstructor.calibrated_tracks();
  libmv_reconstruction.intrinsics = uncalibrated_reconstructor.camera_intrinsics();

  libmv_reconstruction.euclidean_reconstruction = uncalibrated_reconstructor.euclidean_reconstruction();
  libmv_reconstruction.projective_reconstruction = uncalibrated_reconstructor.projective_reconstruction();

  libmv_reconstruction.error =
    ProjectiveReprojectionError( libmv_reconstruction.tracks,
                                 libmv_reconstruction.projective_reconstruction,
                                 libmv_reconstruction.intrinsics );
}


template <class T>
void
libmv_solveReconstructionImpl( const std::vector<std::string> &images,
                               const cv::Matx33d &K,
                               T &libmv_reconstruction)
{
  Ptr<Feature2D> edetector = ORB::create(10000);
  Ptr<Feature2D> edescriber = xfeatures2d::DAISY::create();
  //Ptr<Feature2D> edescriber = xfeatures2d::LATCH::create(64, true, 4);

  cout << "Initialize nViewMatcher ... ";
  libmv::correspondence::nRobustViewMatching nViewMatcher(edetector, edescriber);

  cout << "OK" << endl << "Performing Cross Matching ... ";
  nViewMatcher.computeCrossMatch(images); cout << "OK" << endl;

  // Building tracks
  libmv::Tracks tracks;
  libmv::Matches matches = nViewMatcher.getMatches();

  parser_2D_tracks( matches, tracks );


  // Initial reconstruction
  //const int keyframe1 = 1, keyframe2 = matches.NumImages();
  const int keyframe1 = 1, keyframe2 = 2;

  const double focal_length = K(0,0);
  const double principal_x = K(0,2), principal_y = K(1,2), k1 = 0, k2 = 0, k3 = 0;

  // Refinement parameters
  int refine_intrinsics = SFM_BUNDLE_FOCAL_LENGTH | SFM_BUNDLE_PRINCIPAL_POINT | SFM_BUNDLE_RADIAL_K1 | SFM_BUNDLE_RADIAL_K2; // | SFM_BUNDLE_TANGENTIAL;  /* (see libmv::EuclideanBundleCommonIntrinsics) */

  // Perform reconstruction
  libmv_solveReconstruction( tracks, keyframe1, keyframe2,
                             focal_length, principal_x, principal_y, k1, k2, k3,
                             libmv_reconstruction, refine_intrinsics );

}


void
parser_2D_tracks( const std::vector<cv::Mat> &points2d, libmv::Tracks &tracks )
{
  const int nframes = static_cast<int>(points2d.size());

  for (int frame = 1; frame <= nframes; ++frame)
  {
    const int ntracks = points2d[frame-1].cols;

    for (int track = 1; track <= ntracks; ++track)
    {
      const Vec2d track_pt = points2d[frame-1].col(track-1);
      if ( ( track_pt[0] != 0 && track_pt[1] != 0 ) &&
           ( track_pt[0] != -1 && track_pt[1] != -1 ) )
      {
        //cout << frame << " " << track << " " << track_pt << endl;
        tracks.Insert(frame, track, track_pt[0], track_pt[1]);
      }

    }

  }
}

void
parser_2D_tracks( const libmv::Matches &matches, libmv::Tracks &tracks )
{
  std::set<Matches::ImageID>::const_iterator iter_image =
      matches.get_images().begin();

  bool is_first_time = true;

  for (; iter_image != matches.get_images().end(); ++iter_image) {
    // Exports points
    Matches::Features<PointFeature> pfeatures =
        matches.InImage<PointFeature>(*iter_image);

    while(pfeatures) {

      double x = pfeatures.feature()->x(),
             y = pfeatures.feature()->y();

      // valid marker
      if ( x > 0 && y > 0 )
      {
          tracks.Insert(*iter_image+1, pfeatures.track()+1, x, y);

          if ( is_first_time )
              is_first_time = false;
      }

      // lost track
      else if ( x < 0 && y < 0 )
      {
          is_first_time = true;
      }

      pfeatures.operator++();
    }
  }
}


template void libmv_solveReconstructionImpl<libmv_EuclideanReconstruction>(
  const std::vector<std::string> &images,
  const cv::Matx33d &K,
  libmv_EuclideanReconstruction &libmv_reconstruction);

template void libmv_solveReconstructionImpl<libmv_ProjectiveReconstruction>(
  const std::vector<std::string> &images,
  const cv::Matx33d &K,
  libmv_ProjectiveReconstruction &libmv_reconstruction);

template void libmv_solveReconstructionImpl<libmv_UncalibratedReconstruction>(
  const std::vector<std::string> &images,
  const cv::Matx33d &K,
  libmv_UncalibratedReconstruction &libmv_reconstruction);

} // namespace cv

#endif
