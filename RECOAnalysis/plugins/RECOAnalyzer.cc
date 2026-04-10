// system include files
#include <memory>
#include <utility>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"

#include "DataFormats/Provenance/interface/StableProvenance.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockFwd.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlock.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElement.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElementTrack.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElementCluster.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElementBrem.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElementSuperCluster.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElementGsfTrack.h"
#include "DataFormats/ParticleFlowReco/interface/PFClusterFwd.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"

#include "TH1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TProfile.h"
#include "TProfile2D.h"


class RECOAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit RECOAnalyzer(const edm::ParameterSet&);
  ~RECOAnalyzer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override;

  edm::Service<TFileService> fs_;

  std::map<std::string, TH1F*>       m_mapH1D;
  std::map<std::string, TH2F*>       m_mapH2D;
  std::map<std::string, TH3F*>       m_mapH3D;
  std::map<std::string, TProfile*>   m_mapProf;
  std::map<std::string, TProfile2D*> m_mapProf2D;

  template <typename T, typename... Args>
  T* book(Args&&... args) const {
    T* t = fs_->make<T>(std::forward<Args>(args)...);
    return t;
  }

  template <typename... Args>
  void bookTH1F(const std::string& hName, const std::string& hTitle, Args&&... args){
    m_mapH1D[hName] = book<TH1F>(hName.c_str(),hTitle.c_str(),std::forward<Args>(args)...);
  }
  template <typename... Args>
  void bookTH2F(const std::string& hName, const std::string& hTitle, Args&&... args){
    m_mapH2D[hName] = book<TH2F>(hName.c_str(),hTitle.c_str(),std::forward<Args>(args)...);
  }
  template <typename... Args>
  void bookTH3F(const std::string& hName, const std::string& hTitle, Args&&... args){
    m_mapH3D[hName] = book<TH3F>(hName.c_str(),hTitle.c_str(),std::forward<Args>(args)...);
  }
  template <typename... Args>
  void bookProfile(const std::string& pName,const std::string& pTitle, Args&&... args){
    m_mapProf[pName] = book<TProfile>(pName.c_str(),pTitle.c_str(),std::forward<Args>(args)...);
  }
  template <typename... Args>
  void bookProfile2D(const std::string& pName,const std::string& pTitle, Args&&... args){
    m_mapProf2D[pName] = book<TProfile2D>(pName.c_str(),pTitle.c_str(),std::forward<Args>(args)...);
  }

  // ----------member data ---------------------------
  edm::EDGetTokenT<std::vector<reco::PFBlock>> pfBlocksToken_;
  edm::EDGetTokenT<reco::VertexCollection> primaryVerticesToken_;
};



RECOAnalyzer::RECOAnalyzer(const edm::ParameterSet& iConfig):
  pfBlocksToken_(consumes<std::vector<reco::PFBlock>>(iConfig.getParameter<edm::InputTag>("pfBlock"))),
  primaryVerticesToken_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("primaryVertices"))){
  //now do what ever initialization is needed

  usesResource(TFileService::kSharedResource);

  bookTH1F("h_nPV",";nPV",         80, 0, 80);
  bookTH1F("h_nPVGood",";nPVGood", 80, 0, 80);

  bookTH1F("h_pftrack_pt",";PF track pT [GeV]", 50, 0., 50.);
  bookTH1F("h_pftrack_eta",";PF track #eta",   100, -5., 5.);

  bookTH2F("h2_pftrack_eta_vs_pt",";PF track #eta;PF track pT [GeV]", 100, -5., 5., 50, 0., 50.);
}

RECOAnalyzer::~RECOAnalyzer() {
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
  //
  // please remove this method altogether if it would be left empty
}

// ------------ method called for each event  ------------
void RECOAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {

  edm::Handle<reco::VertexCollection> primaryVerticesHandle;
  iEvent.getByToken(primaryVerticesToken_, primaryVerticesHandle);
  int nPV = 0;
  int nPVGood = 0;
  for (unsigned int ind = 0; ind < primaryVerticesHandle->size(); ind++) {
    const reco::Vertex& pv =  (*primaryVerticesHandle)[ind];
    nPV++;
    bool pass = (!pv.isFake()) && pv.ndof() > 4 && abs(pv.z()) <= 24 && pv.position().Rho() <= 2;
    if (pass) {
      nPVGood++;
    }
  }
  m_mapH1D["h_nPV"]->Fill(nPV);
  m_mapH1D["h_nPVGood"]->Fill(nPVGood);

  //
  // Get PFBlocks
  //
  edm::Handle<std::vector<reco::PFBlock>> pfBlocksHandle;
  iEvent.getByToken(pfBlocksToken_, pfBlocksHandle);
  std::vector<reco::PFBlock> pfBlocks = *pfBlocksHandle;

  //
  // Loop over PFBlocks
  //
  std::list<reco::PFBlockRef> singleTrackWithHCALBlockRefs;

  for (unsigned iBlock = 0; iBlock < pfBlocks.size(); ++iBlock) {
    reco::PFBlockRef blockref = reco::PFBlockRef(pfBlocksHandle, iBlock);
    const reco::PFBlock& block = *blockref;
    const edm::OwnVector<reco::PFBlockElement>& elements = block.elements();

    int nTrack = 0;
    int nPS1 = 0;
    int nPS2 = 0;
    int nECAL = 0;
    int nHCAL = 0;
    int nHFEM = 0;
    int nHFHAD = 0;
    int nHO = 0;
    //
    // Loop over elements in PFBlock
    //
    for (unsigned iElement = 0; iElement < elements.size(); ++iElement) {
      reco::PFBlockElement::Type type = elements[iElement].type();
      if (type == reco::PFBlockElement::TRACK) {
        nTrack++;
      }
      else if (type == reco::PFBlockElement::PS1) {
        nPS1++;
      }
      else if (type == reco::PFBlockElement::PS2) {
        nPS2++;
      }
      else if (type == reco::PFBlockElement::ECAL) {
        nECAL++;
      }
      else if (type == reco::PFBlockElement::HCAL) {
        nHCAL++;
      }
      else if (type == reco::PFBlockElement::HFEM) {
        nHFEM++;
      }
      else if (type == reco::PFBlockElement::HFHAD) {
        nHFHAD++;
      }
      else if (type == reco::PFBlockElement::HO) {
        nHO++;
      }
    }
    if(nTrack == 1 && nHCAL >= 1){
      singleTrackWithHCALBlockRefs.push_back(blockref);
    }
  }

  //
  // Loop over the singleTrackWithHCALBlockRefs
  //
  for (const reco::PFBlockRef& blockRef : singleTrackWithHCALBlockRefs) {
    if (blockRef.isNull())
      continue;
    //
    // Get the elements and loop over them
    //
    const reco::PFBlock& block = *blockRef;
    const auto& elements = block.elements();

    for (unsigned i = 0; i < elements.size(); ++i) {
      //
      // Find the tracks. Should be only one track
      //
      if (elements[i].type() == reco::PFBlockElement::TRACK) {
        reco::TrackRef trk = elements[i].trackRef();
        if (trk.isNonnull()) {
          float trk_pt = trk->pt();
          float trk_eta = trk->eta();
          if (trk_pt < 3.) continue;
          m_mapH1D["h_pftrack_pt"]->Fill(trk_pt);
          m_mapH1D["h_pftrack_eta"]->Fill(trk_eta);
          m_mapH2D["h2_pftrack_eta_vs_pt"]->Fill(trk_eta,trk_pt);
        }
      }
    }
  }

}

// ------------ method called once each job just before starting event loop  ------------
void RECOAnalyzer::beginJob() {
  // please remove this method if not needed
}

// ------------ method called once each job just after ending the event loop  ------------
void RECOAnalyzer::endJob() {
  // please remove this method if not needed
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void RECOAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(RECOAnalyzer);
