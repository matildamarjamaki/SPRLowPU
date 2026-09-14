// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/Scouting/interface/Run3ScoutingTrack.h"
#include "DataFormats/Scouting/interface/Run3ScoutingHBHERecHit.h"
#include "DataFormats/Scouting/interface/Run3ScoutingEBRecHit.h"
#include "DataFormats/Scouting/interface/Run3ScoutingEERecHit.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalVector.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"

#include "DataFormats/NanoAOD/interface/FlatTable.h"
#include "DataFormats/Math/interface/deltaR.h"

#include "Geometry/HcalTowerAlgo/interface/HcalGeometry.h"

#include "Calibration/IsolatedParticles/interface/CaloConstants.h"
#include "Calibration/IsolatedParticles/interface/CaloPropagateTrack.h"
#include "Calibration/IsolatedParticles/interface/MatrixHCALDetIds.h"
#include "Calibration/IsolatedParticles/interface/MatrixECALDetIds.h"

#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/Records/interface/CaloTopologyRecord.h"
#include "Geometry/Records/interface/HcalRecNumberingRecord.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "Geometry/CaloTopology/interface/HcalTopology.h"

#include "SPRLowPU/ScoutingAnalysis/interface/CaloPropagateTrack.h"

class IsoTrackScoutingTableProducer : public edm::stream::EDProducer<> {

public:
  explicit IsoTrackScoutingTableProducer(const edm::ParameterSet&);
  ~IsoTrackScoutingTableProducer() override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void produce(edm::Event &, const edm::EventSetup &) override;

  int indexOf(const std::vector<unsigned int>&, const unsigned int&);
  // void beginJob() override;
  // void endJob() override;

  // ----------member data ---------------------------
  edm::EDGetTokenT<Run3ScoutingTrackCollection>      m_token_scouting_tracks;
  edm::EDGetTokenT<Run3ScoutingHBHERecHitCollection> m_token_scouting_rechitsHBHE;
  edm::EDGetTokenT<Run3ScoutingEBRecHitCollection>   m_token_scouting_rechitsEB;
  edm::EDGetTokenT<Run3ScoutingEERecHitCollection>   m_token_scouting_rechitsEE;

  edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> magneticFieldToken_;
  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> geometryToken_;
  edm::ESGetToken<CaloTopology, CaloTopologyRecord> caloTopologyToken_;
  edm::ESGetToken<HcalTopology, HcalRecNumberingRecord> hcalTopologyToken_;

  const bool m_saveAllRecHitsEB;
  const bool m_saveAllRecHitsEE;
  const bool m_saveAllRecHitsHBHE;
};

IsoTrackScoutingTableProducer::IsoTrackScoutingTableProducer(const edm::ParameterSet& iConfig):
  m_token_scouting_tracks(consumes<Run3ScoutingTrackCollection>(iConfig.getParameter<edm::InputTag>("tracks"))),
  m_token_scouting_rechitsHBHE(consumes<Run3ScoutingHBHERecHitCollection>(iConfig.getParameter<edm::InputTag>("rechitsHBHE"))),
  m_token_scouting_rechitsEB(consumes<Run3ScoutingEBRecHitCollection>(iConfig.getParameter<edm::InputTag>("rechitsEB"))),
  m_token_scouting_rechitsEE(consumes<Run3ScoutingEERecHitCollection>(iConfig.getParameter<edm::InputTag>("rechitsEE"))),
  m_saveAllRecHitsEB(iConfig.getParameter<bool>("saveAllRecHitsEB")),
  m_saveAllRecHitsEE(iConfig.getParameter<bool>("saveAllRecHitsEE")),
  m_saveAllRecHitsHBHE(iConfig.getParameter<bool>("saveAllRecHitsHBHE")){

  magneticFieldToken_ = esConsumes<MagneticField, IdealMagneticFieldRecord>();
  geometryToken_      = esConsumes<CaloGeometry, CaloGeometryRecord>();
  caloTopologyToken_  = esConsumes<CaloTopology, CaloTopologyRecord>();
  hcalTopologyToken_  = esConsumes<HcalTopology, HcalRecNumberingRecord>();

  produces<nanoaod::FlatTable>("ScoutingTrack");
  produces<nanoaod::FlatTable>("ScoutingRecHitEB");
  produces<nanoaod::FlatTable>("ScoutingRecHitEE");
  produces<nanoaod::FlatTable>("ScoutingRecHitHBHE");
  produces<nanoaod::FlatTable>("ScoutingTrackToScoutingRecHitEB");
  produces<nanoaod::FlatTable>("ScoutingTrackToScoutingRecHitEE");
  produces<nanoaod::FlatTable>("ScoutingTrackToScoutingRecHitHBHE");
}

IsoTrackScoutingTableProducer::~IsoTrackScoutingTableProducer() {}

int IsoTrackScoutingTableProducer::indexOf(const std::vector<unsigned int>& v, const unsigned int& value) {
  auto it = std::find(v.begin(), v.end(), value);
  if (it == v.end()) return -1;
  return static_cast<int>(std::distance(v.begin(), it));
}

void IsoTrackScoutingTableProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;

  const CaloGeometry*  geometry = &iSetup.getData(geometryToken_);
  const MagneticField* bField = &iSetup.getData(magneticFieldToken_);
  const CaloTopology*  caloTopology = &iSetup.getData(caloTopologyToken_);
  const HcalTopology*  theHBHETopology = &iSetup.getData(hcalTopologyToken_);

  const HcalGeometry* cellGeometryHB = dynamic_cast<const HcalGeometry *>(geometry->getSubdetectorGeometry(DetId::Hcal, HcalSubdetector::HcalBarrel));
  const HcalGeometry* cellGeometryHE = dynamic_cast<const HcalGeometry *>(geometry->getSubdetectorGeometry(DetId::Hcal, HcalSubdetector::HcalEndcap));

  edm::Handle<Run3ScoutingTrackCollection> handle_scouting_tracks;
  iEvent.getByToken(m_token_scouting_tracks, handle_scouting_tracks);

  edm::Handle<Run3ScoutingEBRecHitCollection> handle_scouting_rechitsEB;
  iEvent.getByToken(m_token_scouting_rechitsEB, handle_scouting_rechitsEB);

  edm::Handle<Run3ScoutingEERecHitCollection> handle_scouting_rechitsEE;
  iEvent.getByToken(m_token_scouting_rechitsEE, handle_scouting_rechitsEE);

  edm::Handle<Run3ScoutingHBHERecHitCollection> handle_scouting_rechitsHBHE;
  iEvent.getByToken(m_token_scouting_rechitsHBHE, handle_scouting_rechitsHBHE);

  //
  // Make pointers
  //
  std::vector<const Run3ScoutingTrack*> all_tracks_ptrs;
  for (const Run3ScoutingTrack& track : *handle_scouting_tracks) {
    all_tracks_ptrs.push_back(&track);
  }

  //
  // Select tracks
  //
  std::vector<const Run3ScoutingTrack*> selected_tracks;
  for (const Run3ScoutingTrack* const& track : all_tracks_ptrs) {
    if (track->tk_pt() * std::cosh(track->tk_eta()) < 3.f) continue;// Cut on momentum
    if (abs(track->tk_eta()) > 1.3f) continue;// Do barrel tracks first
    selected_tracks.push_back(track);
  }
  std::sort(selected_tracks.begin(), selected_tracks.end(), [](const Run3ScoutingTrack* a, const Run3ScoutingTrack* b) {
    return a->tk_pt() * std::cosh(a->tk_eta()) > b->tk_pt() * std::cosh(b->tk_eta());
  });

  //
  // Loop over selected tracks
  //
  unsigned int nTrack = 0;
  std::vector<float>          Track_pt;
  std::vector<float>          Track_eta;
  std::vector<float>          Track_phi;
  std::vector<float>          Track_p;
  std::vector<int>            Track_charge;
  std::vector<float>          Track_closestECAL_eta;
  std::vector<float>          Track_closestECAL_phi;
  std::vector<unsigned int>   Track_closestECAL_detId;
  std::vector<float>          Track_closestECAL_EBieta;
  std::vector<float>          Track_closestECAL_EBiphi;
  std::vector<float>          Track_closestECAL_EEix;
  std::vector<float>          Track_closestECAL_EEiy;
  std::vector<float>          Track_closestHCAL_eta;
  std::vector<float>          Track_closestHCAL_phi;
  std::vector<unsigned int>   Track_closestHCAL_detId;
  std::vector<unsigned int>   Track_closestEHCAL_detId;
  std::vector<float>          Track_closestHCAL_ieta;
  std::vector<float>          Track_closestHCAL_iphi;

  std::vector<float>          Track_trkIsoDR0p4_ptsum;
  std::vector<float>          Track_trkIsoDR0p3_ptsum;
  std::vector<float>          Track_trkIsoDR0p2_ptsum;
  std::vector<float>          Track_trkIsoDR0p1_ptsum;
  std::vector<float>          Track_trkIsoDR0p05_ptsum;

  std::vector<std::vector<unsigned int>> Track_ECALDetIds;
  std::vector<std::vector<unsigned int>> Track_HBHEDetIds;

  int trackIdx = 0;
  for (const Run3ScoutingTrack* const& track : selected_tracks) {

    Track_pt.push_back(track->tk_pt());
    Track_eta.push_back(track->tk_eta());
    Track_phi.push_back(track->tk_phi());
    Track_p.push_back(track->tk_pt() * std::cosh(track->tk_eta()));
    Track_charge.push_back(track->tk_charge());

    float trkIsoDR0p4_ptsum = 0.f;
    float trkIsoDR0p3_ptsum = 0.f;
    float trkIsoDR0p2_ptsum = 0.f;
    float trkIsoDR0p1_ptsum = 0.f;
    float trkIsoDR0p05_ptsum = 0.f;

    for (const Run3ScoutingTrack* const& trackForIso : all_tracks_ptrs) {
      if (track == trackForIso)continue; // Same track. skip
      float dR = reco::deltaR(track->tk_eta(), track->tk_phi(), trackForIso->tk_eta(), trackForIso->tk_phi());
      if (dR < 0.4) trkIsoDR0p4_ptsum += trackForIso->tk_pt();
      if (dR < 0.3) trkIsoDR0p3_ptsum += trackForIso->tk_pt();
      if (dR < 0.2) trkIsoDR0p2_ptsum += trackForIso->tk_pt();
      if (dR < 0.1) trkIsoDR0p1_ptsum += trackForIso->tk_pt();
      if (dR < 0.05) trkIsoDR0p05_ptsum += trackForIso->tk_pt();
    }
    Track_trkIsoDR0p4_ptsum.push_back(trkIsoDR0p4_ptsum);
    Track_trkIsoDR0p3_ptsum.push_back(trkIsoDR0p3_ptsum);
    Track_trkIsoDR0p2_ptsum.push_back(trkIsoDR0p2_ptsum);
    Track_trkIsoDR0p1_ptsum.push_back(trkIsoDR0p1_ptsum);
    Track_trkIsoDR0p05_ptsum.push_back(trkIsoDR0p05_ptsum);

    // sprScouting::propagatedRun3ScoutingTrackID trackID
    //   = sprScouting::propagateCALO(track, geometry, bField);

    sprScouting::propagatedRun3ScoutingTrackID trackID;
    //   = sprScouting::propagateCALO(track, geometry, bField);

    unsigned int closestECAL_detId = 0;
    unsigned int closestHCAL_detId = 0;
    unsigned int closestEHCAL_detId = 0;

    int   closestECAL_EBieta = 0;
    int   closestECAL_EBiphi = 0;
    int   closestECAL_EEix = 0;
    int   closestECAL_EEiy = 0;
    float closestECAL_eta = -9.f;
    float closestECAL_phi = -9.f;

    int closestHCAL_ieta = 0;
    int closestHCAL_iphi = 0;
    float closestHCAL_eta = -9.f;
    float closestHCAL_phi = -9.f;

    std::vector<DetId> vdets_ECAL;
    std::vector<DetId> vdets_HBHE;

    if (trackID.okECAL) {
      const DetId closestCell(trackID.detIdECAL);
      closestECAL_detId = trackID.detIdECAL;
      closestEHCAL_detId = trackID.detIdEHCAL;
      vdets_ECAL = spr::matrixECALIds(closestCell,/*ieta=*/1,/*iphi=*/1,
        geometry,caloTopology,/*debug=*/false, /*igNoreTransition=*/true
      );

      if (closestCell.det() == DetId::Ecal && closestCell.subdetId() == EcalSubdetector::EcalBarrel){
        EBDetId theECalEBDetId(closestECAL_detId);
        closestECAL_EBieta = theECalEBDetId.ieta();
        closestECAL_EBiphi = theECalEBDetId.iphi();
        auto cellGeometry = geometry->getSubdetectorGeometry(theECalEBDetId)->getGeometry(theECalEBDetId);
        closestECAL_eta = cellGeometry->getPosition().eta();
        closestECAL_phi = cellGeometry->getPosition().phi();
      }
      if (closestCell.det() == DetId::Ecal && closestCell.subdetId() == EcalSubdetector::EcalEndcap){
        EEDetId theECalEEDetId(closestECAL_detId);
        closestECAL_EEix = theECalEEDetId.ix();
        closestECAL_EEiy = theECalEEDetId.iy();
        auto cellGeometry = geometry->getSubdetectorGeometry(theECalEEDetId)->getGeometry(theECalEEDetId);
        closestECAL_eta = cellGeometry->getPosition().eta();
        closestECAL_phi = cellGeometry->getPosition().phi();
      }
    }

    if (trackID.okHCAL) {
      const DetId closestCell(trackID.detIdHCAL);
      HcalDetId theHcalDetId(closestCell);

      std::vector<DetId> dets(1, closestCell);
      vdets_HBHE = spr::matrixHCALIds(dets, theHBHETopology,/*ieta=*/2,/*iphi=*/2,/*includeHO*/false,/*debug=*/false);

      closestHCAL_detId = trackID.detIdHCAL;
      closestHCAL_ieta = theHcalDetId.ieta();
      closestHCAL_iphi = theHcalDetId.iphi();
      if(closestCell.subdetId() == HcalSubdetector::HcalBarrel){
        closestHCAL_eta = cellGeometryHB->getPosition(theHcalDetId).eta();
        closestHCAL_phi = cellGeometryHB->getPosition(theHcalDetId).phi();
      }
      else if(closestCell.subdetId() == HcalSubdetector::HcalEndcap){
        closestHCAL_eta = cellGeometryHE->getPosition(theHcalDetId).eta();
        closestHCAL_phi = cellGeometryHE->getPosition(theHcalDetId).phi();
      }
    }

    Track_closestECAL_detId.push_back(closestECAL_detId);
    Track_closestEHCAL_detId.push_back(closestEHCAL_detId);
    Track_closestHCAL_detId.push_back(closestHCAL_detId);

    Track_closestECAL_eta.push_back(closestECAL_eta);
    Track_closestECAL_phi.push_back(closestECAL_phi);
    Track_closestECAL_EBieta.push_back(closestECAL_EBieta);
    Track_closestECAL_EBiphi.push_back(closestECAL_EBiphi);
    Track_closestECAL_EEix.push_back(closestECAL_EEix);
    Track_closestECAL_EEiy.push_back(closestECAL_EEiy);

    Track_closestHCAL_ieta.push_back(closestHCAL_ieta);
    Track_closestHCAL_iphi.push_back(closestHCAL_iphi);
    Track_closestHCAL_eta.push_back(closestHCAL_eta);
    Track_closestHCAL_phi.push_back(closestHCAL_phi);

    std::vector<unsigned int> vdets_ECAL_rawId;
    std::vector<unsigned int> vdets_HBHE_rawId;

    // std::cout << "tk_pt = " << track.tk_pt() << ", tk_eta() = " << track.tk_eta()<< ", tk_phi() = " << track.tk_phi() << std::endl;
    // std::cout << "  closestHCAL_eta = " << closestHCAL_eta << ", closestHCAL_phi = " << closestHCAL_phi << std::endl;
    // std::cout << "  trackID.okECAL = " << trackID.okECAL << ", vdets_ECAL.size() = " << vdets_ECAL.size() << std::endl;
    // std::cout << "  trackID.okHCAL = " << trackID.okHCAL << ", vdets_HBHE.size() = " << vdets_HBHE.size() << std::endl;

    for (const auto& det: vdets_ECAL){
      vdets_ECAL_rawId.push_back(det.rawId());
    }
    Track_ECALDetIds.push_back(vdets_ECAL_rawId);

    for (const auto& det: vdets_HBHE){
      vdets_HBHE_rawId.push_back(det.rawId());
    }
    Track_HBHEDetIds.push_back(vdets_HBHE_rawId);

    nTrack++;
    trackIdx++;
  }

  //=================================
  //
  // Save RecHitEB
  //
  //=================================
  unsigned int nRecHitEB = 0;
  std::vector<float>        RecHitEB_energy;
  std::vector<int>          RecHitEB_ieta;
  std::vector<int>          RecHitEB_iphi;
  std::vector<unsigned int> RecHitEB_detId;
  std::vector<int>          RecHitEB_tower_ieta;
  std::vector<int>          RecHitEB_tower_iphi;
  std::vector<float>        RecHitEB_eta;
  std::vector<float>        RecHitEB_phi;

  for (const Run3ScoutingEBRecHit& rechit : *handle_scouting_rechitsEB) {
    DetId detIdRaw(rechit.detId());

    if (detIdRaw.det() == DetId::Ecal && detIdRaw.subdetId() == EcalSubdetector::EcalBarrel){
      if(!m_saveAllRecHitsEB){
        bool hasRechit = std::any_of(Track_ECALDetIds.begin(), Track_ECALDetIds.end(),
          [&rechit](const std::vector<unsigned int>& vec){
            return std::find(vec.begin(), vec.end(), rechit.detId()) != vec.end();
          }
        );
        if (!hasRechit) continue;
      }

      EBDetId theECalEBDetId(rechit.detId());

      RecHitEB_energy.push_back(rechit.energy());
      RecHitEB_ieta.push_back(theECalEBDetId.ieta());
      RecHitEB_iphi.push_back(theECalEBDetId.iphi());
      RecHitEB_detId.push_back(rechit.detId());

      RecHitEB_tower_ieta.push_back(theECalEBDetId.tower_ieta());
      RecHitEB_tower_iphi.push_back(theECalEBDetId.tower_iphi());

      auto cellGeometry = geometry->getSubdetectorGeometry(theECalEBDetId)->getGeometry(theECalEBDetId);
      double eta = cellGeometry->getPosition().eta();
      double phi = cellGeometry->getPosition().phi();
      RecHitEB_eta.push_back(eta);
      RecHitEB_phi.push_back(phi);

      nRecHitEB++;
    }
  }

  //=================================
  //
  // Save RecHitEE
  //
  //=================================
  unsigned int nRecHitEE = 0;
  std::vector<float>        RecHitEE_energy;
  std::vector<int>          RecHitEE_ix;
  std::vector<int>          RecHitEE_iy;
  std::vector<unsigned int> RecHitEE_detId;
  std::vector<float>        RecHitEE_eta;
  std::vector<float>        RecHitEE_phi;

  for (const Run3ScoutingEERecHit& rechit : *handle_scouting_rechitsEE) {
    DetId detIdRaw(rechit.detId());
    if(detIdRaw.det() == DetId::Ecal && detIdRaw.subdetId() == EcalSubdetector::EcalEndcap){
      if(!m_saveAllRecHitsEE){
        bool hasRechit = std::any_of(Track_ECALDetIds.begin(), Track_ECALDetIds.end(),
          [&rechit](const std::vector<unsigned int>& vec){
            return std::find(vec.begin(), vec.end(), rechit.detId()) != vec.end();
          }
        );
        if (!hasRechit) continue;
      }

      EEDetId theECalEEDetId(rechit.detId());

      RecHitEE_energy.push_back(rechit.energy());
      RecHitEE_ix.push_back(theECalEEDetId.ix());
      RecHitEE_iy.push_back(theECalEEDetId.iy());
      RecHitEE_detId.push_back(rechit.detId());

      auto cellGeometry = geometry->getSubdetectorGeometry(theECalEEDetId)->getGeometry(theECalEEDetId);
      double eta = cellGeometry->getPosition().eta();
      double phi = cellGeometry->getPosition().phi();
      RecHitEE_eta.push_back(eta);
      RecHitEE_phi.push_back(phi);
      nRecHitEE++;
    }
  }

  //=================================
  //
  // Save RecHitHBHE
  //
  //=================================
  unsigned int nRecHitHBHE = 0;
  std::vector<float>          RecHitHBHE_energy;
  std::vector<int>            RecHitHBHE_ieta;
  std::vector<int>            RecHitHBHE_iphi;
  std::vector<int>            RecHitHBHE_depth;
  std::vector<unsigned int>   RecHitHBHE_detId;
  std::vector<float>          RecHitHBHE_eta;
  std::vector<float>          RecHitHBHE_phi;

  for (const Run3ScoutingHBHERecHit& rechit : *handle_scouting_rechitsHBHE) {
    DetId detIdRaw(rechit.detId());

    if(detIdRaw.subdetId() == HcalSubdetector::HcalBarrel || detIdRaw.subdetId() == HcalSubdetector::HcalEndcap){
      HcalDetId theHcalDetId(detIdRaw);

      if(!m_saveAllRecHitsHBHE){
        bool hasRechit = std::any_of(Track_HBHEDetIds.begin(), Track_HBHEDetIds.end(),
          [&rechit](const std::vector<unsigned int>& vec){
            return std::find(vec.begin(), vec.end(), rechit.detId()) != vec.end();
          }
        );
        if (!hasRechit) continue;
      }

      RecHitHBHE_energy.push_back(rechit.energy());
      RecHitHBHE_ieta.push_back(theHcalDetId.ieta());
      RecHitHBHE_iphi.push_back(theHcalDetId.iphi());
      RecHitHBHE_depth.push_back(theHcalDetId.depth());
      RecHitHBHE_detId.push_back(rechit.detId());

      float eta = -9.f;
      float phi = -9.f;
      if(detIdRaw.subdetId() == HcalSubdetector::HcalBarrel){
        eta = cellGeometryHB->getPosition(theHcalDetId).eta();
        phi = cellGeometryHB->getPosition(theHcalDetId).phi();
      }
      else if(detIdRaw.subdetId() == HcalSubdetector::HcalEndcap){
        eta = cellGeometryHE->getPosition(theHcalDetId).eta();
        phi = cellGeometryHE->getPosition(theHcalDetId).phi();
      }
      RecHitHBHE_eta.push_back(eta);
      RecHitHBHE_phi.push_back(phi);
      nRecHitHBHE++;
    }
  }

  //=================================
  //
  //
  //
  //=================================
  int nTrackToRecHitEB = 0;
  std::vector<int> TrackToRecHitEB_TrackIdx;
  std::vector<int> TrackToRecHitEB_RecHitIdx;
  std::vector<int> TrackToRecHitEB_detId;

  int nTrackToRecHitEE = 0;
  std::vector<int> TrackToRecHitEE_TrackIdx;
  std::vector<int> TrackToRecHitEE_RecHitIdx;
  std::vector<int> TrackToRecHitEE_detId;

  for (int trkIdx = 0; trkIdx < (int)Track_ECALDetIds.size(); trkIdx++){
    for (int detIdIdx = 0; detIdIdx < (int) Track_ECALDetIds[trkIdx].size(); detIdIdx++){
      DetId detIdRaw(Track_ECALDetIds[trkIdx][detIdIdx]);
      //
      // EB
      //
      if (detIdRaw.det() == DetId::Ecal && detIdRaw.subdetId() == EcalSubdetector::EcalBarrel){
        TrackToRecHitEB_TrackIdx.push_back(trkIdx);
        int RecHitIdx = indexOf(RecHitEB_detId,Track_ECALDetIds[trkIdx][detIdIdx]);
        TrackToRecHitEB_RecHitIdx.push_back(RecHitIdx);
        TrackToRecHitEB_detId.push_back(Track_ECALDetIds[trkIdx][detIdIdx]);
        nTrackToRecHitEB++;
      }
      //
      // EE
      //
      else if (detIdRaw.det() == DetId::Ecal && detIdRaw.subdetId() == EcalSubdetector::EcalEndcap){
        TrackToRecHitEE_TrackIdx.push_back(trkIdx);
        int RecHitIdx = indexOf(RecHitEE_detId,Track_ECALDetIds[trkIdx][detIdIdx]);
        TrackToRecHitEE_RecHitIdx.push_back(RecHitIdx);
        TrackToRecHitEE_detId.push_back(Track_ECALDetIds[trkIdx][detIdIdx]);
        nTrackToRecHitEE++;
      }
    }
  }

  int nTrackToRecHitHBHE = 0;
  std::vector<int> TrackToRecHitHBHE_TrackIdx;
  std::vector<int> TrackToRecHitHBHE_RecHitIdx;
  std::vector<int> TrackToRecHitHBHE_detId;

  for (int trkIdx = 0; trkIdx < (int)Track_HBHEDetIds.size(); trkIdx++){
    for (int detIdIdx = 0; detIdIdx < (int) Track_HBHEDetIds[trkIdx].size(); detIdIdx++){
      DetId detIdRaw(Track_HBHEDetIds[trkIdx][detIdIdx]);
      TrackToRecHitHBHE_TrackIdx.push_back(trkIdx);
      int RecHitIdx = indexOf(RecHitHBHE_detId,Track_HBHEDetIds[trkIdx][detIdIdx]);
      TrackToRecHitHBHE_RecHitIdx.push_back(RecHitIdx);
      TrackToRecHitHBHE_detId.push_back(Track_HBHEDetIds[trkIdx][detIdIdx]);
      nTrackToRecHitHBHE++;
    }
  }

  auto trackTable = std::make_unique<nanoaod::FlatTable>(nTrack, "ScoutingTrack", false, false);
  trackTable->addColumn<float>("pt",Track_pt,"pt",-1);
  trackTable->addColumn<float>("eta",Track_eta,"eta",-1);
  trackTable->addColumn<float>("phi",Track_phi,"phi",-1);
  trackTable->addColumn<float>("p",Track_p,"p",-1);
  trackTable->addColumn<int>("charge",Track_charge,"charge");
  trackTable->addColumn<float>("trkIsoDR0p05_ptsum",Track_trkIsoDR0p05_ptsum,"pT sum of tracks within dR < 0.05",-1);
  trackTable->addColumn<float>("trkIsoDR0p1_ptsum",Track_trkIsoDR0p1_ptsum,"pT sum of tracks within dR < 0.1",-1);
  trackTable->addColumn<float>("trkIsoDR0p2_ptsum",Track_trkIsoDR0p2_ptsum,"pT sum of tracks within dR < 0.2",-1);
  trackTable->addColumn<float>("trkIsoDR0p3_ptsum",Track_trkIsoDR0p3_ptsum,"pT sum of tracks within dR < 0.3",-1);
  trackTable->addColumn<float>("trkIsoDR0p4_ptsum",Track_trkIsoDR0p4_ptsum,"pT sum of tracks within dR < 0.4",-1);
  trackTable->addColumn<unsigned int>("closestECAL_detId",Track_closestECAL_detId,"closestECAL detId");
  trackTable->addColumn<unsigned int>("closestHCAL_detId",Track_closestHCAL_detId,"closestHCAL detId");
  trackTable->addColumn<unsigned int>("closestEHCAL_detId",Track_closestEHCAL_detId,"closestEHCAL detId");
  trackTable->addColumn<float>("closestECAL_eta",Track_closestECAL_eta,"closestECAL eta",-1);
  trackTable->addColumn<float>("closestECAL_phi",Track_closestECAL_phi,"closestECAL phi",-1);
  trackTable->addColumn<int>("closestECAL_EBieta",Track_closestECAL_EBieta,"closestECAL EBieta");
  trackTable->addColumn<int>("closestECAL_EBiphi",Track_closestECAL_EBiphi,"closestECAL EBiphi");
  trackTable->addColumn<int>("closestECAL_EEix",Track_closestECAL_EEix,"closestECAL EEix");
  trackTable->addColumn<int>("closestECAL_EEiy",Track_closestECAL_EEiy,"closestECAL EEiy");
  trackTable->addColumn<int>("closestHCAL_ieta",Track_closestHCAL_ieta,"closestHCAL ieta");
  trackTable->addColumn<int>("closestHCAL_iphi",Track_closestHCAL_iphi,"closestHCAL iphi");
  trackTable->addColumn<float>("closestHCAL_eta",Track_closestHCAL_eta,"closestHCAL eta",-1);
  trackTable->addColumn<float>("closestHCAL_phi",Track_closestHCAL_phi,"closestHCAL phi",-1);
  iEvent.put(std::move(trackTable),"ScoutingTrack");

  auto recHitEBTable = std::make_unique<nanoaod::FlatTable>(nRecHitEB, "ScoutingRecHitEB", false, false);
  recHitEBTable->addColumn<float>("energy",RecHitEB_energy,"energy",-1);
  recHitEBTable->addColumn<int>("ieta",RecHitEB_ieta,"ieta");
  recHitEBTable->addColumn<int>("iphi",RecHitEB_iphi,"iphi");
  recHitEBTable->addColumn<unsigned int>("detId",RecHitEB_detId,"detId");
  recHitEBTable->addColumn<int>("tower_ieta",RecHitEB_tower_ieta,"tower_ieta");
  recHitEBTable->addColumn<int>("tower_iphi",RecHitEB_tower_iphi,"tower_iphi");
  recHitEBTable->addColumn<float>("eta",RecHitEB_eta,"eta",-1);
  recHitEBTable->addColumn<float>("phi",RecHitEB_phi,"phi",-1);
  iEvent.put(std::move(recHitEBTable),"ScoutingRecHitEB");

  auto recHitEETable = std::make_unique<nanoaod::FlatTable>(nRecHitEE, "ScoutingRecHitEE", false, false);
  recHitEETable->addColumn<float>("energy",RecHitEE_energy,"energy",-1);
  recHitEETable->addColumn<int>("ix",RecHitEE_ix,"ix");
  recHitEETable->addColumn<int>("iy",RecHitEE_iy,"iy");
  recHitEETable->addColumn<unsigned int>("detId",RecHitEE_detId,"detId");
  recHitEETable->addColumn<float>("eta",RecHitEE_eta,"tower_eta",-1);
  recHitEETable->addColumn<float>("phi",RecHitEE_phi,"tower_phi",-1);
  iEvent.put(std::move(recHitEETable),"ScoutingRecHitEE");

  auto recHitHBHETable = std::make_unique<nanoaod::FlatTable>(nRecHitHBHE, "ScoutingRecHitHBHE", false, false);
  recHitHBHETable->addColumn<float>("energy",RecHitHBHE_energy,"energy",-1);
  recHitHBHETable->addColumn<int>("ieta",RecHitHBHE_ieta,"ieta");
  recHitHBHETable->addColumn<int>("iphi",RecHitHBHE_iphi,"iphi");
  recHitHBHETable->addColumn<int>("depth",RecHitHBHE_depth,"depth");
  recHitHBHETable->addColumn<unsigned int>("detId",RecHitHBHE_detId,"detId");
  recHitHBHETable->addColumn<float>("eta",RecHitHBHE_eta,"eta",-1);
  recHitHBHETable->addColumn<float>("phi",RecHitHBHE_phi,"phi",-1);
  iEvent.put(std::move(recHitHBHETable),"ScoutingRecHitHBHE");

  auto trackToRecHitEBTable = std::make_unique<nanoaod::FlatTable>(nTrackToRecHitEB, "ScoutingTrackToScoutingRecHitEB", false, false);
  trackToRecHitEBTable->addColumn<int>("TrackIdx",TrackToRecHitEB_TrackIdx,"TrackIdx");
  trackToRecHitEBTable->addColumn<int>("RecHitIdx",TrackToRecHitEB_RecHitIdx,"RecHitIdx");
  trackToRecHitEBTable->addColumn<int>("detId",TrackToRecHitEB_detId,"detId");
  iEvent.put(std::move(trackToRecHitEBTable),"ScoutingTrackToScoutingRecHitEB");

  auto trackToRecHitEETable = std::make_unique<nanoaod::FlatTable>(nTrackToRecHitEE, "ScoutingTrackToScoutingRecHitEE", false, false);
  trackToRecHitEETable->addColumn<int>("TrackIdx",TrackToRecHitEE_TrackIdx,"TrackIdx");
  trackToRecHitEETable->addColumn<int>("RecHitIdx",TrackToRecHitEE_RecHitIdx,"RecHitIdx");
  trackToRecHitEETable->addColumn<int>("detId",TrackToRecHitEE_detId,"detId");
  iEvent.put(std::move(trackToRecHitEETable),"ScoutingTrackToScoutingRecHitEE");

  auto trackToRecHitHBHETable = std::make_unique<nanoaod::FlatTable>(nTrackToRecHitHBHE, "ScoutingTrackToScoutingRecHitHBHE", false, false);
  trackToRecHitHBHETable->addColumn<int>("TrackIdx",TrackToRecHitHBHE_TrackIdx,"TrackIdx");
  trackToRecHitHBHETable->addColumn<int>("RecHitIdx",TrackToRecHitHBHE_RecHitIdx,"RecHitIdx");
  trackToRecHitHBHETable->addColumn<int>("detId",TrackToRecHitHBHE_detId,"detId");
  iEvent.put(std::move(trackToRecHitHBHETable),"ScoutingTrackToScoutingRecHitHBHE");
}
// void IsoTrackScoutingTableProducer::beginJob() {}
// void IsoTrackScoutingTableProducer::endJob() {}
void IsoTrackScoutingTableProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(IsoTrackScoutingTableProducer);
