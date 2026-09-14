#ifndef SPRLowPUScoutingAnalysisCaloPropagateTrack_h
#define SPRLowPUScoutingAnalysisCaloPropagateTrack_h

#include <cmath>
#include <vector>
#include <string>

#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/Scouting/interface/Run3ScoutingHBHERecHit.h"
#include "DataFormats/Scouting/interface/Run3ScoutingEBRecHit.h"
#include "DataFormats/Scouting/interface/Run3ScoutingEERecHit.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalVector.h"

#include "DataFormats/GeometrySurface/interface/Plane.h"
#include "DataFormats/GeometrySurface/interface/Cylinder.h"
#include "Geometry/HcalTowerAlgo/interface/HcalGeometry.h"

#include "Calibration/IsolatedParticles/interface/MatrixHCALDetIds.h"
#include "Calibration/IsolatedParticles/interface/MatrixECALDetIds.h"

#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/Records/interface/CaloTopologyRecord.h"
#include "Geometry/Records/interface/HcalRecNumberingRecord.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "Geometry/CaloTopology/interface/HcalTopology.h"

namespace sprScouting {

  //
  // https://cmssdt.cern.ch/lxr/source/Calibration/IsolatedParticles/interface/CaloPropagateTrack.h#0031
  //
  struct propagatedRun3ScoutingTrack {
    propagatedRun3ScoutingTrack() { ok = false; }
    bool ok;
    math::XYZPoint point;
    GlobalVector direction;
  };
  struct propagatedRun3ScoutingTrackID {
    propagatedRun3ScoutingTrackID() {
      ok = false;
      okECAL = false;
      okHCAL = false;
    }
    bool ok, okECAL, okHCAL;
    DetId detIdECAL, detIdHCAL, detIdEHCAL;
    double etaECAL, etaHCAL;
    double phiECAL, phiHCAL;
    Run3ScoutingTrackCollection::const_iterator trkItr;
  };
  struct propagatedRun3ScoutingTrackDirection {
    propagatedRun3ScoutingTrackDirection() {
      ok = false;
      okECAL = false;
      okHCAL = false;
    }
    bool ok, okECAL, okHCAL;
    DetId detIdECAL, detIdHCAL, detIdEHCAL;
    GlobalPoint pointECAL, pointHCAL;
    GlobalVector directionECAL, directionHCAL;
    Run3ScoutingTrackCollection::const_iterator trkItr;
  };

  sprScouting::propagatedRun3ScoutingTrack propagateCalo(const GlobalPoint& tpVertex,const GlobalVector& tpMomentum,int tpCharge,const MagneticField* bField,float zdist,float radius,float corner);
  sprScouting::propagatedRun3ScoutingTrack propagateTrackToECAL(const Run3ScoutingTrack* track, const MagneticField* bfield);
  sprScouting::propagatedRun3ScoutingTrack propagateTrackToHCAL(const Run3ScoutingTrack* track,  const MagneticField* bfield);

  void propagateCALO(edm::Handle<Run3ScoutingTrackCollection>& trkCollection,const CaloGeometry* geo,const MagneticField* bField,std::vector<sprScouting::propagatedRun3ScoutingTrackID>& vdets);
  void propagateCALO(edm::Handle<Run3ScoutingTrackCollection>& trkCollection, const CaloGeometry* geo, const MagneticField* bField,std::vector<sprScouting::propagatedRun3ScoutingTrackDirection>& trkDir);
  sprScouting::propagatedRun3ScoutingTrackID propagateCALO(const Run3ScoutingTrack* pTrack,const CaloGeometry* geo,const MagneticField* bField);

  std::pair<math::XYZPoint, bool> propagateECAL(const Run3ScoutingTrack* track, const MagneticField* bfield);
  std::pair<math::XYZPoint, bool> propagateECAL(const GlobalPoint& vertex, const GlobalVector& momentum,int charge, const MagneticField* bfield);
  std::pair<math::XYZPoint, bool> propagateHCAL(const Run3ScoutingTrack* track, const MagneticField* bfield);
  std::pair<math::XYZPoint, bool> propagateHCAL(const GlobalPoint& vertex, const GlobalVector& momentum, int charge, const MagneticField* bfield);

}// namespace sprScouting
#endif
