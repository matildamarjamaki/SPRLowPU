#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"
#include "TrackingTools/GeomPropagators/interface/AnalyticalPropagator.h"

#include "DataFormats/GeometrySurface/interface/Plane.h"
#include "DataFormats/GeometrySurface/interface/Cylinder.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"

#include "Calibration/IsolatedParticles/interface/CaloConstants.h"
#include "Calibration/IsolatedParticles/interface/CaloPropagateTrack.h"

#include "SPRLowPU/ScoutingAnalysis/interface/CaloPropagateTrack.h"

namespace sprScouting {

  //
  // Shamelessly stolen from
  // https://cmssdt.cern.ch/lxr/source/Calibration/IsolatedParticles/src/CaloPropagateTrack.cc#0140
  // and adapted for Run3ScoutingTrack
  //
  void propagateCALO(
      edm::Handle<Run3ScoutingTrackCollection>& trkCollection,
      const CaloGeometry* geo,
      const MagneticField* bField,
      std::vector<sprScouting::propagatedRun3ScoutingTrackID>& vdets){

    const CaloSubdetectorGeometry* barrelGeom = geo->getSubdetectorGeometry(DetId::Ecal, EcalBarrel);
    const CaloSubdetectorGeometry* endcapGeom = geo->getSubdetectorGeometry(DetId::Ecal, EcalEndcap);
    const CaloSubdetectorGeometry* gHB = geo->getSubdetectorGeometry(DetId::Hcal, HcalBarrel);

    unsigned int indx;
    Run3ScoutingTrackCollection::const_iterator trkItr;

    for (trkItr = trkCollection->begin(), indx = 0; trkItr != trkCollection->end(); ++trkItr, indx++) {
      const Run3ScoutingTrack* pTrack = &(*trkItr);

      sprScouting::propagatedRun3ScoutingTrackID vdet;
      vdet.trkItr = trkItr;
      vdet.ok = true;
      vdet.detIdECAL  = DetId(0);
      vdet.detIdHCAL  = DetId(0);
      vdet.detIdEHCAL = DetId(0);

      //
      // ECAL
      //
      std::pair<math::XYZPoint, bool> info = sprScouting::propagateECAL(pTrack, bField);
      vdet.okECAL = info.second;
      if (vdet.okECAL) {
        const GlobalPoint point(info.first.x(), info.first.y(), info.first.z());
        vdet.etaECAL = point.eta();
        vdet.phiECAL = point.phi();
        if (std::abs(point.eta()) < spr::etaBEEcal) {
          vdet.detIdECAL = barrelGeom->getClosestCell(point);
        }
        else {
          if (endcapGeom)
            vdet.detIdECAL = endcapGeom->getClosestCell(point);
          else
            vdet.okECAL = false;
        }
        vdet.detIdEHCAL = gHB->getClosestCell(point);
      }

      //
      // HCAL
      //
      info = sprScouting::propagateHCAL(pTrack, bField);
      vdet.okHCAL = info.second;
      if (vdet.okHCAL) {
        const GlobalPoint point(info.first.x(), info.first.y(), info.first.z());
        vdet.etaHCAL = point.eta();
        vdet.phiHCAL = point.phi();
        vdet.detIdHCAL = gHB->getClosestCell(point);
      }
      vdets.push_back(vdet);
    }
  }
  void propagateCALO(edm::Handle<Run3ScoutingTrackCollection>& trkCollection,
      const CaloGeometry* geo,
      const MagneticField* bField,
      std::vector<sprScouting::propagatedRun3ScoutingTrackDirection>& trkDir) {
    const CaloSubdetectorGeometry* barrelGeom = geo->getSubdetectorGeometry(DetId::Ecal, EcalBarrel);
    const CaloSubdetectorGeometry* endcapGeom = geo->getSubdetectorGeometry(DetId::Ecal, EcalEndcap);
    const CaloSubdetectorGeometry* gHB = geo->getSubdetectorGeometry(DetId::Hcal, HcalBarrel);

    unsigned int indx;
    Run3ScoutingTrackCollection::const_iterator trkItr;

    for (trkItr = trkCollection->begin(), indx = 0; trkItr != trkCollection->end(); ++trkItr, indx++) {
      const Run3ScoutingTrack* pTrack = &(*trkItr);

      sprScouting::propagatedRun3ScoutingTrackDirection trkD;
      trkD.trkItr = trkItr;
      trkD.detIdECAL = DetId(0);
      trkD.detIdHCAL = DetId(0);
      trkD.detIdEHCAL = DetId(0);

      //
      // ECAL
      //
      sprScouting::propagatedRun3ScoutingTrack info = sprScouting::propagateTrackToECAL(pTrack, bField);

      GlobalPoint point(info.point.x(), info.point.y(), info.point.z());
      trkD.okECAL = info.ok;
      trkD.pointECAL = point;
      trkD.directionECAL = info.direction;

      if (trkD.okECAL) {
        if (std::abs(info.point.eta()) < spr::etaBEEcal) {
          trkD.detIdECAL = barrelGeom->getClosestCell(point);
        } else {
          if (endcapGeom)
            trkD.detIdECAL = endcapGeom->getClosestCell(point);
          else
            trkD.okECAL = false;
        }
        trkD.detIdEHCAL = gHB->getClosestCell(point);
      }

      //
      // HCAL
      //
      info = sprScouting::propagateTrackToHCAL(pTrack, bField);

      point = GlobalPoint(info.point.x(), info.point.y(), info.point.z());
      trkD.okHCAL = info.ok;
      trkD.pointHCAL = point;
      trkD.directionHCAL = info.direction;

      if (trkD.okHCAL) {
        trkD.detIdHCAL = gHB->getClosestCell(point);
      }
      trkDir.push_back(trkD);
    }
  }
  //
  // Function for a single track input
  // https://cmssdt.cern.ch/lxr/source/Calibration/IsolatedParticles/src/CaloPropagateTrack.cc#0289
  //
  sprScouting::propagatedRun3ScoutingTrackID propagateCALO(const Run3ScoutingTrack* pTrack,
      const CaloGeometry* geo,
      const MagneticField* bField) {
    const CaloSubdetectorGeometry* barrelGeom = geo->getSubdetectorGeometry(DetId::Ecal, EcalBarrel);
    const CaloSubdetectorGeometry* endcapGeom = geo->getSubdetectorGeometry(DetId::Ecal, EcalEndcap);
    const CaloSubdetectorGeometry* gHB = geo->getSubdetectorGeometry(DetId::Hcal, HcalBarrel);

    sprScouting::propagatedRun3ScoutingTrackID vdet;
    vdet.ok = true;
    vdet.detIdECAL = DetId(0);
    vdet.detIdHCAL = DetId(0);
    vdet.detIdEHCAL = DetId(0);
    std::pair<math::XYZPoint, bool> info = sprScouting::propagateECAL(pTrack, bField);
    vdet.okECAL = info.second;
    if (vdet.okECAL) {
      const GlobalPoint point(info.first.x(), info.first.y(), info.first.z());
      vdet.etaECAL = point.eta();
      vdet.phiECAL = point.phi();
      if (std::abs(point.eta()) < spr::etaBEEcal) {
        vdet.detIdECAL = barrelGeom->getClosestCell(point);
      } else {
        if (endcapGeom)
          vdet.detIdECAL = endcapGeom->getClosestCell(point);
        else
          vdet.okECAL = false;
      }
      vdet.detIdEHCAL = gHB->getClosestCell(point);
    }
    info = sprScouting::propagateHCAL(pTrack, bField);
    vdet.okHCAL = info.second;
    if (vdet.okHCAL) {
      const GlobalPoint point(info.first.x(), info.first.y(), info.first.z());
      vdet.etaHCAL = point.eta();
      vdet.phiHCAL = point.phi();
      vdet.detIdHCAL = gHB->getClosestCell(point);
    }
    return vdet;
  }

  sprScouting::propagatedRun3ScoutingTrack propagateTrackToECAL(const Run3ScoutingTrack* track, const MagneticField* bfield) {
    const GlobalPoint vertex(track->tk_vx(), track->tk_vy(), track->tk_vz());

    math::PtEtaPhiMLorentzVector track_p4(track->tk_pt(), track->tk_eta(), track->tk_phi(), 0.f);
    const GlobalVector momentum(track_p4.px(), track_p4.py(), track_p4.pz());
    int charge(track->tk_charge());

    return sprScouting::propagateCalo(vertex, momentum, charge, bfield, spr::zFrontEE, spr::rFrontEB, spr::etaBEEcal);
  }
  sprScouting::propagatedRun3ScoutingTrack propagateTrackToHCAL(const Run3ScoutingTrack* track,  const MagneticField* bfield) {
    const GlobalPoint vertex(track->tk_vx(), track->tk_vy(), track->tk_vz());

    math::PtEtaPhiMLorentzVector track_p4(track->tk_pt(), track->tk_eta(), track->tk_phi(), 0.f);
    const GlobalVector momentum(track_p4.px(), track_p4.py(), track_p4.pz());
    int charge(track->tk_charge());

    return sprScouting::propagateCalo(vertex, momentum, charge, bfield, spr::zFrontHE, spr::rFrontHB, spr::etaBEHcal);
  }

  // Shamelessly stolen from
  // https://cmssdt.cern.ch/lxr/source/Calibration/IsolatedParticles/src/CaloPropagateTrack.cc#0660
  // and adapted for Run3ScoutingTrack
  //
  std::pair<math::XYZPoint, bool> propagateECAL(
    const Run3ScoutingTrack* track, const MagneticField* bfield) {

    const GlobalPoint vertex(track->tk_vx(), track->tk_vy(), track->tk_vz());

    math::PtEtaPhiMLorentzVector track_p4(track->tk_pt(), track->tk_eta(), track->tk_phi(), 0.f);
    const GlobalVector momentum(track_p4.px(), track_p4.py(), track_p4.pz());
    int charge(track->tk_charge());

    return sprScouting::propagateECAL(vertex, momentum, charge, bfield);
  }
  std::pair<math::XYZPoint, bool> propagateECAL(
    const GlobalPoint& vertex, const GlobalVector& momentum,
    int charge,
    const MagneticField* bfield) {

    sprScouting::propagatedRun3ScoutingTrack track = sprScouting::propagateCalo(
      vertex, momentum, charge, bfield,
      spr::zFrontEE, spr::rFrontEB, spr::etaBEEcal
    );
    return std::pair<math::XYZPoint, bool>(track.point, track.ok);
  }

  // Shamelessly stolen from
  // https://cmssdt.cern.ch/lxr/source/Calibration/IsolatedParticles/src/CaloPropagateTrack.cc#0720
  // and adapted for Run3ScoutingTrack
  //
  std::pair<math::XYZPoint, bool> propagateHCAL(const Run3ScoutingTrack* track, const MagneticField* bfield) {
    const GlobalPoint vertex(track->tk_vx(), track->tk_vy(), track->tk_vz());

    math::PtEtaPhiMLorentzVector track_p4(track->tk_pt(), track->tk_eta(), track->tk_phi(), 0.f);
    const GlobalVector momentum(track_p4.px(), track_p4.py(), track_p4.pz());
    int charge(track->tk_charge());

    return sprScouting::propagateHCAL(vertex, momentum, charge, bfield);
  }

  std::pair<math::XYZPoint, bool> propagateHCAL(
    const GlobalPoint& vertex, const GlobalVector& momentum, int charge, const MagneticField* bfield) {
    sprScouting::propagatedRun3ScoutingTrack track = sprScouting::propagateCalo(
      vertex, momentum, charge, bfield,
      spr::zFrontHE, spr::rFrontHB, spr::etaBEHcal
    );
    return std::pair<math::XYZPoint, bool>(track.point, track.ok);
  }

  // Shamelessly stolen from
  // https://cmssdt.cern.ch/lxr/source/Calibration/IsolatedParticles/src/CaloPropagateTrack.cc#0796
  // and adapted for Run3ScoutingTrack
  //
  sprScouting::propagatedRun3ScoutingTrack propagateCalo(
    const GlobalPoint& tpVertex,const GlobalVector& tpMomentum,
    int tpCharge,
    const MagneticField* bField,
    float zdist,float radius,float corner) {
    sprScouting::propagatedRun3ScoutingTrack track;
    FreeTrajectoryState fts(tpVertex, tpMomentum, tpCharge, bField);

    Plane::PlanePointer lendcap = Plane::build(Plane::PositionType(0, 0, -zdist), Plane::RotationType());
    Plane::PlanePointer rendcap = Plane::build(Plane::PositionType(0, 0, zdist), Plane::RotationType());
    Cylinder::CylinderPointer barrel = Cylinder::build(Cylinder::PositionType(0, 0, 0), Cylinder::RotationType(), radius);
    AnalyticalPropagator myAP(bField, alongMomentum, 2 * M_PI);

    TrajectoryStateOnSurface tsose;
    if (tpMomentum.eta() < 0) {
      tsose = myAP.propagate(fts, *lendcap);
    } else {
       tsose = myAP.propagate(fts, *rendcap);
    }
    TrajectoryStateOnSurface tsosb = myAP.propagate(fts, *barrel);
    track.ok = true;
    if (tsose.isValid() && tsosb.isValid()) {
      float absEta = std::abs(tsosb.globalPosition().eta());
      if (absEta < corner) {
        track.point.SetXYZ(tsosb.globalPosition().x(), tsosb.globalPosition().y(), tsosb.globalPosition().z());
        track.direction = tsosb.globalDirection();
      } else {
         track.point.SetXYZ(tsose.globalPosition().x(), tsose.globalPosition().y(), tsose.globalPosition().z());
         track.direction = tsose.globalDirection();
       }
    }
    else if (tsose.isValid()) {
     track.point.SetXYZ(tsose.globalPosition().x(), tsose.globalPosition().y(), tsose.globalPosition().z());
     track.direction = tsose.globalDirection();
    }
    else if (tsosb.isValid()) {
      track.point.SetXYZ(tsosb.globalPosition().x(), tsosb.globalPosition().y(), tsosb.globalPosition().z());
      track.direction = tsosb.globalDirection();
    }
    else {
      track.point.SetXYZ(-999., -999., -999.);
      track.direction = GlobalVector(0, 0, 1);
      track.ok = false;
    }
     return track;
  }
}  // namespace sprScouting