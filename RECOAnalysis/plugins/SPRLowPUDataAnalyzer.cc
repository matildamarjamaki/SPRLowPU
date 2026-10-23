#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "CondFormats/DataRecord/interface/PFCalibrationRcd.h"
#include "CondFormats/PhysicsToolsObjects/interface/PerformancePayloadFromTFormula.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlock.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockFwd.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"
#include "DataFormats/Candidate/interface/VertexCompositeCandidate.h"
#include "DataFormats/RecoCandidate/interface/RecoChargedCandidate.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/ESInputTag.h"
#include "RecoParticleFlow/PFClusterTools/interface/PFEnergyCalibration.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TProfile.h"

#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {
const double trkPBins[] = {3.0,  3.3,  3.6,  3.9,  4.2,  4.6,  5.0,  5.5,  6.0,  6.6,  7.2,
                           7.9,  8.6,  9.3,  10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.5, 18.0,
                           20.0, 22.0, 25.0, 30.0, 35.0, 40.0, 45.0, 55.0, 65.0, 80.0, 100.0};
constexpr int nTrkPBins = sizeof(trkPBins) / sizeof(trkPBins[0]) - 1;
}  // namespace

class SPRLowPUDataAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources, edm::one::WatchRuns> {
public:
  explicit SPRLowPUDataAnalyzer(edm::ParameterSet const& config);
  ~SPRLowPUDataAnalyzer() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  struct MuonMatchResult {
    unsigned int multiplicity = 0;
    bool passesSoft = false;
    bool passesLoose = false;
    bool passesMedium = false;
    bool passesTight = false;

    bool hasExactMatch() const { return multiplicity > 0; }
    bool isMuonTagged() const { return passesSoft || passesLoose; }
  };

  void beginRun(edm::Run const& run, edm::EventSetup const& setup) override;
  void endRun(edm::Run const& run, edm::EventSetup const& setup) override {}
  void analyze(edm::Event const& event, edm::EventSetup const& setup) override;
  bool isGoodVertex(reco::Vertex const& vertex) const;
  bool isIsolationNeighbor(reco::Track const& track) const;
  std::pair<double, double> chargedIsolation(std::size_t candidateIndex,
                                             reco::TrackCollection const& tracks,
                                             reco::Vertex const* leadingVertex) const;
  MuonMatchResult matchMuon(reco::TrackRef const& trackRef,
                            reco::MuonCollection const& muons,
                            reco::Vertex const* leadingVertex) const;
  void fillPFLinkingDiagnostics(reco::TrackRef const& trackRef,
                                reco::Track const& track,
                                reco::PFBlockCollection const& pfBlocks,
                                unsigned int numberOfGoodVertices,
                                MuonMatchResult const& muonMatch,
                                reco::PFCandidate const* matchedPFCandidate);

  edm::EDGetTokenT<reco::VertexCompositeCandidateCollection> lambdaToken_;
  edm::EDGetTokenT<reco::VertexCompositeCandidateCollection> kshortToken_;
  edm::EDGetTokenT<reco::TrackCollection> tracksToken_;
  edm::EDGetTokenT<reco::VertexCollection> verticesToken_;
  edm::EDGetTokenT<reco::PFBlockCollection> pfBlocksToken_;
  edm::EDGetTokenT<reco::PFCandidateCollection> pfCandidatesToken_;
  edm::EDGetTokenT<reco::MuonCollection> muonsToken_;
  edm::ESGetToken<PerformancePayload, PFCalibrationRcd> pfCalibrationToken_;

  PFEnergyCalibration pfEnergyCalibration_;

  bool doV0Tagging_;
  bool requirePrimaryVertex_;
  double minVertexNdof_;
  double maxVertexAbsZ_;
  double maxVertexRho_;
  reco::TrackBase::TrackQuality trackQuality_;
  double minTrackPt_;
  double minTrackP_;
  double minV0TrackPt_;
  double minV0TrackP_;
  double maxTrackAbsEta_;
  double maxTrackNormalizedChi2_;
  double maxTrackRelativePError_;
  unsigned int minTrackerLayers_;
  unsigned int minPixelLayers_;
  double maxTrackAbsDxy_;
  double maxTrackAbsDz_;
  double isolationCone_;
  double minIsolationTrackPt_;
  double maxIsolationTrackAbsEta_;
  double maxRelativeIsolation_;

  TH1D* v0CandidateCounts_;
  TH1D* v0DaughterCutflow_;
  TH1D* v0TrackSelectionCutflow_;
  TH1D* v0WithoutPVCutsCutflow_;
  TH1D* v0SpeciesCounts_;
  TH1D* v0SpeciesPFMatches_;
  TH1D* v0SpeciesMultiplicity_;
  TH1D* v0SpeciesCombination_;
  TH2D* v0SpeciesCutflow_;
  TH2D* v0SpeciesPt_;
  TH2D* v0SpeciesP_;
  TH2D* v0SpeciesEta_;
  TH2D* v0SpeciesPFResponse_;
  TH2D* v0SpeciesResponseLinkCategory_;
  TH2D* v0PFBlockLinkStatus_;
  TH2D* v0PFVsLinkedResponse_;
  TH2D* v0PFVsLinkedResponseAnyCluster_;
  TH2D* v0PFVsLinkedResponseHCAL_;
  TH2D* v0PFVsLinkedResponseNoCluster_;
  TProfile* v0SpeciesResponseVsP_[4];
  TProfile* v0SpeciesPositiveResponseVsP_[4];
  TH1D* eventCount_;
  TH1D* eventCutflow_;
  TH1D* trackCutflow_;
  TH1D* numberOfVertices_;
  TH1D* numberOfGoodVertices_;
  TH1D* numberOfTracks_;
  TH1D* numberOfPFBlocks_;
  TH1D* numberOfSelectedTracks_;
  TH1D* leadingVertexZ_;
  TH1D* leadingVertexRho_;
  TH1D* leadingVertexNdof_;
  TH1D* selectedTrackP_;
  TH1D* selectedTrackPVarBins_;
  TH1D* selectedTrackPt_;
  TH1D* selectedTrackPtVarBins_;
  TH1D* selectedTrackEta_;
  TH1D* selectedTrackPhi_;
  TH1D* selectedTrackNormalizedChi2_;
  TH1D* selectedTrackRelativePError_;
  TH1D* selectedTrackTrackerLayers_;
  TH1D* selectedTrackPixelLayers_;
  TH1D* selectedTrackDxy_;
  TH1D* selectedTrackDz_;
  TH1D* selectedTrackSamePVRelativeIsolation_;
  TH1D* selectedTrackAllVertexRelativeIsolation_;
  TH1D* muonMatchMultiplicity_;
  TH1D* muonIDSummary_;
  TH1D* selectedTrackMuonCategory_;
  TH1D* exactMuonMatchedTrackP_;
  TH1D* exactMuonMatchedTrackPVarBins_;
  TH1D* muonTaggedTrackP_;
  TH1D* muonTaggedTrackPVarBins_;
  TH1D* nonMuonTrackP_;
  TH1D* nonMuonTrackPVarBins_;
  TH1D* neutralHadronMultiplicityLinkedHCAL_;
  TH1D* neutralHadronEnergyFractionLinkedHCAL_;
  TH1D* neutralHadronMultiplicitySameHCAL_;
  TH1D* neutralHadronEnergyFractionSameHCAL_;
  TH1D* neutralHadronMultiplicitySameBlock_;
  TH1D* neutralHadronEnergyFractionSameBlock_;
  TProfile* neutralSameBlockFractionVsP_;
  TProfile* neutralSameHCALFractionVsP_;
  TProfile* neutralLinkedHCALFractionVsP_;
  TProfile* meanNeutralHadronFractionVsP_;
  TProfile* meanNeutralHadronMultiplicityVsP_;
  TH1D* pfCandidateMatchMultiplicity_;
  TH1D* pfCandidateParticleId_;
  TH2D* pfCandidateResponseVsParticleId_;
  TH1D* pfCandidateECALPlusHCALEnergy_;
  TH1D* pfCandidateECALPlusHCALResponse_;
  TH2D* pfCandidateECALPlusHCALResponseVsPVarBins_;
  TProfile* pfCandidateMatchEfficiencyVsPVarBins_;
  TProfile* meanPFCandidateECALPlusHCALResponseVsPVarBins_;
  TProfile* meanPFCandidateECALResponseVsPVarBins_;
  TProfile* meanPFCandidateHCALResponseVsPVarBins_;
  TH1D* pfCandidateToLinkedEnergyRatio_;
  TProfile* meanPFCandidateToLinkedEnergyRatioVsPVarBins_;
  TProfile* meanSameTrackPFCandidateResponseVsPVarBins_;
  TProfile* meanSameTrackLinkedResponseVsPVarBins_;
  TProfile* meanHCALLinkedPFCandidateResponseVsPVarBins_;
  TProfile* meanHCALLinkedRawPFCandidateResponseVsPVarBins_;
  TProfile* meanDeltaResponseSharedHCALVsP_;
  TProfile* meanDeltaResponseNonSharedHCALVsP_;
  TProfile* meanDeltaResponseMultiTrackBlockVsP_;
  TProfile* meanDeltaResponseMultiHCALBlockVsP_;
  TProfile* meanDeltaResponseMultiECALBlockVsP_;
  TProfile* meanDeltaResponseSimpleBlockVsP_;
  TProfile* meanDeltaResponseComplexBlockVsP_;
  TProfile* meanPFResponseSimpleBlockVsP_;
  TProfile* meanLinkedResponseSimpleBlockVsP_;
  TProfile* meanPFResponseComplexBlockVsP_;
  TProfile* meanLinkedResponseComplexBlockVsP_;
  TH2D* hcalLinkedPFCandidateResponseVsPVarBins_;
  TH2D* hcalLinkedRawPFCandidateResponseVsPVarBins_;
  TProfile* hcalLinkedPFCandidateMatchEfficiencyVsPVarBins_;
  TProfile* hcalLinkedChargedHadronEfficiencyVsPVarBins_;
  TH2D* pfCandidateECALVsLinkedECAL_;
  TH2D* pfCandidateHCALVsLinkedHCAL_;
  TH2D* rawPFCandidateECALVsLinkedECAL_;
  TH2D* rawPFCandidateHCALVsLinkedHCAL_;
  TH1D* pfLinkingCutflow_;
  TH1D* pfBlockMatchMultiplicity_;
  TH1D* matchedPFBlockNumberOfElements_;
  TH1D* matchedPFBlockNumberOfTrackElements_;
  TH1D* matchedPFBlockNumberOfECALElements_;
  TH1D* matchedPFBlockNumberOfHCALElements_;
  TH1D* matchedPFBlockNumberOfHOElements_;
  TH1D* matchedPFBlockNumberOfOtherElements_;
  TH1D* linkedECALClusterMultiplicity_;
  TH1D* linkedHCALClusterMultiplicity_;
  TH1D* linkedHOClusterMultiplicity_;
  TH1D* trackECALLinkDistance_;
  TH1D* trackHCALLinkDistance_;
  TH1D* trackHOLinkDistance_;
  TH1D* noPFBlockMatchTrackP_;
  TH1D* noPFBlockMatchTrackPVarBins_;
  TH1D* multiplePFBlockMatchesTrackP_;
  TH1D* multiplePFBlockMatchesTrackPVarBins_;
  TH1D* noLinkedECALTrackP_;
  TH1D* noLinkedECALTrackPVarBins_;
  TH1D* noLinkedHCALTrackP_;
  TH1D* noLinkedHCALTrackPVarBins_;
  TH1D* linkedECALEnergy_;
  TH1D* linkedHCALEnergy_;
  TH1D* linkedHOEnergy_;
  TH1D* linkedECALPlusHCALEnergy_;
  TH1D* linkedECALResponse_;
  TH1D* linkedHCALResponse_;
  TH1D* linkedECALPlusHCALResponse_;
  TH1D* linkedECALPlusHCALPlusHOResponse_;
  TH1D* linkedECALEnergyFraction_;
  TH1D* prePFHadronCalibrationLinkedECALEnergy_;
  TH1D* prePFHadronCalibrationLinkedHCALEnergy_;
  TH1D* prePFHadronCalibrationLinkedECALPlusHCALEnergy_;
  TH1D* prePFHadronCalibrationLinkedECALResponse_;
  TH1D* prePFHadronCalibrationLinkedHCALResponse_;
  TH1D* prePFHadronCalibrationLinkedECALPlusHCALResponse_;
  TH1D* pfHadronCalibratedLinkedECALEnergy_;
  TH1D* pfHadronCalibratedLinkedHCALEnergy_;
  TH1D* pfHadronCalibratedLinkedECALPlusHCALEnergy_;
  TH1D* pfHadronCalibratedLinkedECALResponse_;
  TH1D* pfHadronCalibratedLinkedHCALResponse_;
  TH1D* pfHadronCalibratedLinkedECALPlusHCALResponse_;
  TH1D* pfHadronCalibrationFactor_;
  TH1D* pfHadronCalibrationResponseChange_;
  TH1D* muonTaggedLinkedECALPlusHCALResponse_;
  TH1D* nonMuonLinkedECALPlusHCALResponse_;
  TH2D* linkedECALEnergyVsLinkedHCALEnergy_;
  TH2D* linkedECALPlusHCALResponseVsP_;
  TH2D* linkedECALPlusHCALResponseVsPVarBins_;
  TH2D* linkedECALPlusHCALResponseVsEta_;
  TH2D* linkedECALPlusHCALResponseVsGoodVertices_;
  TH2D* linkedECALPlusHCALResponseVsPFBlockTracks_;
  TH2D* prePFHadronCalibrationLinkedECALPlusHCALResponseVsP_;
  TH2D* prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins_;
  TH2D* prePFHadronCalibrationLinkedECALPlusHCALResponseVsEta_;
  TH2D* prePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices_;
  TH2D* prePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks_;
  TH2D* pfHadronCalibratedLinkedECALPlusHCALResponseVsP_;
  TH2D* pfHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins_;
  TH2D* pfHadronCalibratedLinkedECALPlusHCALResponseVsEta_;
  TH2D* pfHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices_;
  TH2D* pfHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks_;
  TH2D* rawVsPFHadronCalibratedLinkedECALPlusHCALResponse_;
  TH2D* muonTaggedLinkedECALPlusHCALResponseVsP_;
  TH2D* muonTaggedLinkedECALPlusHCALResponseVsPVarBins_;
  TH2D* nonMuonLinkedECALPlusHCALResponseVsP_;
  TH2D* nonMuonLinkedECALPlusHCALResponseVsPVarBins_;
  TProfile* meanTrackPtVsEta_;
  TProfile* meanSelectedTracksVsGoodVertices_;
  TProfile* exactMuonMatchEfficiencyVsP_;
  TProfile* exactMuonMatchEfficiencyVsPVarBins_;
  TProfile* exactMuonMatchEfficiencyVsEta_;
  TProfile* muonTagEfficiencyVsP_;
  TProfile* muonTagEfficiencyVsPVarBins_;
  TProfile* muonTagEfficiencyVsEta_;
  TProfile* pfBlockMatchEfficiencyVsP_;
  TProfile* pfBlockMatchEfficiencyVsPVarBins_;
  TProfile* pfBlockMatchEfficiencyVsEta_;
  TProfile* pfBlockMatchEfficiencyVsGoodVertices_;
  TProfile* uniquePFBlockMatchEfficiencyVsP_;
  TProfile* uniquePFBlockMatchEfficiencyVsPVarBins_;
  TProfile* uniquePFBlockMatchEfficiencyVsEta_;
  TProfile* uniquePFBlockMatchEfficiencyVsGoodVertices_;
  TProfile* linkedECALEfficiencyVsP_;
  TProfile* linkedECALEfficiencyVsPVarBins_;
  TProfile* linkedECALEfficiencyVsEta_;
  TProfile* linkedECALEfficiencyVsGoodVertices_;
  TProfile* linkedHCALEfficiencyVsP_;
  TProfile* linkedHCALEfficiencyVsPVarBins_;
  TProfile* linkedHCALEfficiencyVsEta_;
  TProfile* linkedHCALEfficiencyVsGoodVertices_;
  TProfile* meanLinkedECALPlusHCALResponseVsP_;
  TProfile* meanLinkedECALPlusHCALResponseVsPVarBins_;
  TProfile* meanLinkedECALPlusHCALResponseVsEta_;
  TProfile* meanLinkedECALPlusHCALResponseVsGoodVertices_;
  TProfile* meanLinkedECALPlusHCALResponseVsPFBlockTracks_;
  TProfile* meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsP_;
  TProfile* meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins_;
  TProfile* meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsEta_;
  TProfile* meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices_;
  TProfile* meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks_;
  TProfile* meanPFHadronCalibratedLinkedECALPlusHCALResponseVsP_;
  TProfile* meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins_;
  TProfile* meanPFHadronCalibratedLinkedECALPlusHCALResponseVsEta_;
  TProfile* meanPFHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices_;
  TProfile* meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks_;
  TProfile* meanMuonTaggedLinkedECALPlusHCALResponseVsP_;
  TProfile* meanMuonTaggedLinkedECALPlusHCALResponseVsPVarBins_;
  TProfile* meanNonMuonLinkedECALPlusHCALResponseVsP_;
  TProfile* meanNonMuonLinkedECALPlusHCALResponseVsPVarBins_;
};

SPRLowPUDataAnalyzer::SPRLowPUDataAnalyzer(edm::ParameterSet const& config)
    : lambdaToken_(consumes<reco::VertexCompositeCandidateCollection>(
          config.getParameter<edm::InputTag>("lambdas"))),
      kshortToken_(consumes<reco::VertexCompositeCandidateCollection>(
          config.getParameter<edm::InputTag>("kshorts"))),
      tracksToken_(consumes<reco::TrackCollection>(config.getParameter<edm::InputTag>("tracks"))),
      verticesToken_(consumes<reco::VertexCollection>(config.getParameter<edm::InputTag>("vertices"))),
      pfBlocksToken_(consumes<reco::PFBlockCollection>(config.getParameter<edm::InputTag>("pfBlocks"))),
      pfCandidatesToken_(consumes<reco::PFCandidateCollection>(
          config.getParameter<edm::InputTag>("pfCandidates"))),
      muonsToken_(consumes<reco::MuonCollection>(config.getParameter<edm::InputTag>("muons"))),
      pfCalibrationToken_(esConsumes<edm::Transition::BeginRun>(edm::ESInputTag("", ""))),
      doV0Tagging_(config.getParameter<bool>("doV0Tagging")),
      requirePrimaryVertex_(config.getParameter<bool>("requirePrimaryVertex")),
      minVertexNdof_(config.getParameter<double>("minVertexNdof")),
      maxVertexAbsZ_(config.getParameter<double>("maxVertexAbsZ")),
      maxVertexRho_(config.getParameter<double>("maxVertexRho")),
      trackQuality_(reco::TrackBase::qualityByName(config.getParameter<std::string>("trackQuality"))),
      minTrackPt_(config.getParameter<double>("minTrackPt")),
      minTrackP_(config.getParameter<double>("minTrackP")),
      minV0TrackPt_(config.getParameter<double>("minV0TrackPt")),
      minV0TrackP_(config.getParameter<double>("minV0TrackP")),
      maxTrackAbsEta_(config.getParameter<double>("maxTrackAbsEta")),
      maxTrackNormalizedChi2_(config.getParameter<double>("maxTrackNormalizedChi2")),
      maxTrackRelativePError_(config.getParameter<double>("maxTrackRelativePError")),
      minTrackerLayers_(config.getParameter<unsigned int>("minTrackerLayers")),
      minPixelLayers_(config.getParameter<unsigned int>("minPixelLayers")),
      maxTrackAbsDxy_(config.getParameter<double>("maxTrackAbsDxy")),
      maxTrackAbsDz_(config.getParameter<double>("maxTrackAbsDz")),
      isolationCone_(config.getParameter<double>("isolationCone")),
      minIsolationTrackPt_(config.getParameter<double>("minIsolationTrackPt")),
      maxIsolationTrackAbsEta_(config.getParameter<double>("maxIsolationTrackAbsEta")),
      maxRelativeIsolation_(config.getParameter<double>("maxRelativeIsolation")) {
  usesResource(TFileService::kSharedResource);

  edm::Service<TFileService> fileService;
  v0CandidateCounts_ = fileService->make<TH1D>(
      "v0CandidateCounts", ";V0 candidate type;Candidates", 2, 0.5, 2.5);
  v0CandidateCounts_->GetXaxis()->SetBinLabel(1, "Lambda / anti-Lambda");
  v0CandidateCounts_->GetXaxis()->SetBinLabel(2, "Kshort");

  v0DaughterCutflow_ = fileService->make<TH1D>(
      "v0DaughterCutflow", ";V0 daughter selection;Tracks", 3, 0.5, 3.5);
  v0DaughterCutflow_->GetXaxis()->SetBinLabel(1, "Valid V0 daughter");
  v0DaughterCutflow_->GetXaxis()->SetBinLabel(2, "In generalTracks");
  v0DaughterCutflow_->GetXaxis()->SetBinLabel(3, "Passed SPR selection");


  v0SpeciesCounts_ = fileService->make<TH1D>(
      "v0SpeciesCounts", ";V0 daughter hypothesis;Tracks",
      4, 0.5, 4.5);

  v0SpeciesPFMatches_ = fileService->make<TH1D>(
      "v0SpeciesPFMatches",
      ";V0 daughter hypothesis;Unique charged-hadron PF matches",
      4, 0.5, 4.5);

  v0SpeciesPFResponse_ = fileService->make<TH2D>(
      "v0SpeciesPFResponse",
      ";V0 daughter hypothesis;(ECAL+HCAL)/p",
      4, 0.5, 4.5, 200, 0.0, 5.0);


  v0SpeciesResponseLinkCategory_ = fileService->make<TH2D>(
      "v0SpeciesResponseLinkCategory",
      ";V0 daughter hypothesis;PF response and cluster-link category",
      4, 0.5, 4.5, 6, 0.5, 6.5);

  const char* v0ResponseLinkLabels[] = {
      "zero, no cluster",
      "low, no cluster",
      "positive, no cluster",
      "zero, linked",
      "low, linked",
      "positive, linked"};

  for (int i = 1; i <= 6; ++i) {
    v0SpeciesResponseLinkCategory_->GetYaxis()->SetBinLabel(
        i, v0ResponseLinkLabels[i - 1]);
  }

  v0PFBlockLinkStatus_ = fileService->make<TH2D>(
      "v0PFBlockLinkStatus",
      ";V0 daughter hypothesis;PFBlock linking status",
      4, 0.5, 4.5, 6, 0.5, 6.5);

  v0PFVsLinkedResponse_ = fileService->make<TH2D>(
      "v0PFVsLinkedResponse",
      ";PF candidate (ECAL+HCAL)/p;closest linked clusters (ECAL+HCAL)/p",
      100, 0.0, 5.0, 100, 0.0, 5.0);

  v0PFVsLinkedResponseAnyCluster_ = fileService->make<TH2D>(
      "v0PFVsLinkedResponse_anyCluster",
      ";PF candidate (ECAL+HCAL)/p;linked clusters (ECAL+HCAL)/p",
      100, 0.0, 5.0, 100, 0.0, 5.0);

  v0PFVsLinkedResponseHCAL_ = fileService->make<TH2D>(
      "v0PFVsLinkedResponse_HCAL",
      ";PF candidate (ECAL+HCAL)/p;linked clusters (ECAL+HCAL)/p",
      100, 0.0, 5.0, 100, 0.0, 5.0);

  v0PFVsLinkedResponseNoCluster_ = fileService->make<TH2D>(
      "v0PFVsLinkedResponse_noCluster",
      ";PF candidate (ECAL+HCAL)/p;linked clusters (ECAL+HCAL)/p",
      100, 0.0, 5.0, 100, 0.0, 5.0);

  const char* v0LinkLabels[] = {
      "PF matched", "No PFBlock", "Unique PFBlock",
      "Multiple PFBlocks", "Linked ECAL", "Linked HCAL"};

  for (int i = 1; i <= 6; ++i) {
    v0PFBlockLinkStatus_->GetYaxis()->SetBinLabel(
        i, v0LinkLabels[i - 1]);
  }

  const char* v0ProfileNames[] = {
      "v0LambdaProtonResponseVsP",
      "v0AntiLambdaAntiprotonResponseVsP",
      "v0LambdaPionResponseVsP",
      "v0KshortPionResponseVsP"};

  // Extend only the V0 response profiles below 3 GeV.
  // Keep the original SPR momentum bin edges unchanged.
  const double v0LowPBins[] = {
      1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.3, 2.6
  };

  std::vector<double> v0PBins(
      std::begin(v0LowPBins), std::end(v0LowPBins));
  v0PBins.insert(
      v0PBins.end(), trkPBins, trkPBins + nTrkPBins + 1);

  for (int i = 0; i < 4; ++i) {
    v0SpeciesResponseVsP_[i] = fileService->make<TProfile>(
        v0ProfileNames[i],
        ";track p [GeV];mean PF (ECAL+HCAL)/p",
        static_cast<int>(v0PBins.size()) - 1,
        v0PBins.data());
  }

  const char* v0PositiveProfileNames[] = {
      "v0LambdaProtonPositiveResponseVsP",
      "v0AntiLambdaAntiprotonPositiveResponseVsP",
      "v0LambdaPionPositiveResponseVsP",
      "v0KshortPionPositiveResponseVsP"};

  for (int i = 0; i < 4; ++i) {
    v0SpeciesPositiveResponseVsP_[i] = fileService->make<TProfile>(
        v0PositiveProfileNames[i],
        ";track p [GeV];mean PF (ECAL+HCAL)/p, E/p > 0",
        static_cast<int>(v0PBins.size()) - 1,
        v0PBins.data());
  }

  const char* v0SpeciesLabels[] = {
      "Lambda proton", "anti-Lambda antiproton",
      "Lambda pion", "Kshort pion"};

  for (int i = 1; i <= 4; ++i) {
    v0SpeciesCounts_->GetXaxis()->SetBinLabel(
        i, v0SpeciesLabels[i - 1]);
    v0SpeciesPFMatches_->GetXaxis()->SetBinLabel(
        i, v0SpeciesLabels[i - 1]);
    v0SpeciesPFResponse_->GetXaxis()->SetBinLabel(
        i, v0SpeciesLabels[i - 1]);
    v0SpeciesResponseLinkCategory_->GetXaxis()->SetBinLabel(
        i, v0SpeciesLabels[i - 1]);
    v0PFBlockLinkStatus_->GetXaxis()->SetBinLabel(
        i, v0SpeciesLabels[i - 1]);
  }


  v0SpeciesMultiplicity_ = fileService->make<TH1D>(
      "v0SpeciesMultiplicity",
      ";Number of species hypotheses per selected V0 track;Tracks",
      5, -0.5, 4.5);


  v0SpeciesCombination_ = fileService->make<TH1D>(
      "v0SpeciesCombination",
      ";Species hypothesis bitmask;Tracks",
      16, -0.5, 15.5);


  v0SpeciesCutflow_ = fileService->make<TH2D>(
      "v0SpeciesCutflow",
      ";Selection step;Species hypothesis",
      10, 0.5, 10.5,
      4, 0.5, 4.5);

  const char* v0Steps[] = {
      "Matched track",
      "High purity",
      "pT",
      "p",
      "eta",
      "normalized chi2",
      "relative p error",
      "tracker layers",
      "pixel layers",
      "PF match"
  };

  for (int i = 0; i < 10; ++i) {
    v0SpeciesCutflow_->GetXaxis()->SetBinLabel(i + 1, v0Steps[i]);
  }

  v0SpeciesCutflow_->GetYaxis()->SetBinLabel(1, "Lambda proton");
  v0SpeciesCutflow_->GetYaxis()->SetBinLabel(2, "anti-Lambda antiproton");
  v0SpeciesCutflow_->GetYaxis()->SetBinLabel(3, "Lambda pion");
  v0SpeciesCutflow_->GetYaxis()->SetBinLabel(4, "Kshort pion");


  v0SpeciesPt_ = fileService->make<TH2D>(
      "v0SpeciesPt",
      ";Track p_{T} [GeV];Species hypothesis",
      200, 0.0, 10.0,
      4, 0.5, 4.5);

  v0SpeciesP_ = fileService->make<TH2D>(
      "v0SpeciesP",
      ";Track p [GeV];Species hypothesis",
      200, 0.0, 20.0,
      4, 0.5, 4.5);

  v0SpeciesEta_ = fileService->make<TH2D>(
      "v0SpeciesEta",
      ";Track #eta;Species hypothesis",
      120, -3.0, 3.0,
      4, 0.5, 4.5);

  const char* speciesNames[] = {
      "Lambda proton",
      "anti-Lambda antiproton",
      "Lambda pion",
      "Kshort pion"
  };

  for (int i = 0; i < 4; ++i) {
    v0SpeciesPt_->GetYaxis()->SetBinLabel(i + 1, speciesNames[i]);
    v0SpeciesP_->GetYaxis()->SetBinLabel(i + 1, speciesNames[i]);
    v0SpeciesEta_->GetYaxis()->SetBinLabel(i + 1, speciesNames[i]);
  }

  eventCount_ = fileService->make<TH1D>("eventCount", ";;events", 1, 0.0, 1.0);
  eventCutflow_ = fileService->make<TH1D>("eventCutflow", ";event selection;events", 4, 0.5, 4.5);
  eventCutflow_->GetXaxis()->SetBinLabel(1, "All events");
  eventCutflow_->GetXaxis()->SetBinLabel(2, requirePrimaryVertex_ ? "Has vertex" : "Vertex check bypassed");
  eventCutflow_->GetXaxis()->SetBinLabel(3, requirePrimaryVertex_ ? "Good leading vertex" : "PV selection bypassed");
  eventCutflow_->GetXaxis()->SetBinLabel(4, "Has selected track");

  trackCutflow_ = fileService->make<TH1D>("trackCutflow", ";track selection;tracks", 13, 0.5, 13.5);
  trackCutflow_->GetXaxis()->SetBinLabel(1, "All tracks");
  trackCutflow_->GetXaxis()->SetBinLabel(2, "High purity");
  trackCutflow_->GetXaxis()->SetBinLabel(3, "pT");
  trackCutflow_->GetXaxis()->SetBinLabel(4, "p");
  trackCutflow_->GetXaxis()->SetBinLabel(5, "eta");
  trackCutflow_->GetXaxis()->SetBinLabel(6, "normalized chi2");
  trackCutflow_->GetXaxis()->SetBinLabel(7, "relative p error");
  trackCutflow_->GetXaxis()->SetBinLabel(8, "tracker layers");
  trackCutflow_->GetXaxis()->SetBinLabel(9, "pixel layers");
  trackCutflow_->GetXaxis()->SetBinLabel(10, requirePrimaryVertex_ ? "leading-PV dxy" : "PV dxy bypassed");
  trackCutflow_->GetXaxis()->SetBinLabel(11, requirePrimaryVertex_ ? "leading-PV dz" : "PV dz bypassed");
  trackCutflow_->GetXaxis()->SetBinLabel(12, "isolation");
  trackCutflow_->GetXaxis()->SetBinLabel(13, "Selected tracks");


  v0TrackSelectionCutflow_ = fileService->make<TH1D>(
      "v0TrackSelectionCutflow", ";V0 daughter track selection;tracks",
      13, 0.5, 13.5);

  for (int bin = 1; bin <= 13; ++bin) {
    v0TrackSelectionCutflow_->GetXaxis()->SetBinLabel(
        bin, trackCutflow_->GetXaxis()->GetBinLabel(bin));
  }


  v0WithoutPVCutsCutflow_ = fileService->make<TH1D>(
      "v0WithoutPVCutsCutflow",
      ";V0 selection without PV impact-parameter cuts;tracks",
      4, 0.5, 4.5);
  v0WithoutPVCutsCutflow_->GetXaxis()->SetBinLabel(1, "Passed track quality and kinematics");
  v0WithoutPVCutsCutflow_->GetXaxis()->SetBinLabel(2, "Passed tracker layers");
  v0WithoutPVCutsCutflow_->GetXaxis()->SetBinLabel(3, "Passed pixel layers");
  v0WithoutPVCutsCutflow_->GetXaxis()->SetBinLabel(4, "Passed isolation");

  numberOfVertices_ =
      fileService->make<TH1D>("numberOfVertices", ";number of primary vertices;events", 100, 0.0, 100.0);
  numberOfGoodVertices_ =
      fileService->make<TH1D>("numberOfGoodVertices", ";number of good primary vertices;events", 100, 0.0, 100.0);
  numberOfTracks_ = fileService->make<TH1D>("numberOfTracks", ";number of tracks;events", 500, 0.0, 5000.0);
  numberOfPFBlocks_ = fileService->make<TH1D>("numberOfPFBlocks", ";number of PF blocks;events", 500, 0.0, 5000.0);
  numberOfSelectedTracks_ =
      fileService->make<TH1D>("numberOfSelectedTracks", ";number of selected tracks;events", 100, 0.0, 100.0);
  leadingVertexZ_ =
      fileService->make<TH1D>("leadingVertexZ", ";leading primary vertex z [cm];events", 120, -30.0, 30.0);
  leadingVertexRho_ =
      fileService->make<TH1D>("leadingVertexRho", ";leading primary vertex #rho [cm];events", 100, 0.0, 2.0);
  leadingVertexNdof_ =
      fileService->make<TH1D>("leadingVertexNdof", ";leading primary vertex ndof;events", 100, 0.0, 200.0);

  selectedTrackP_ = fileService->make<TH1D>("selectedTrackP", ";selected-track p [GeV];tracks", 100, 0.0, 100.0);
  selectedTrackPVarBins_ =
      fileService->make<TH1D>("selectedTrackPVarBins", ";selected-track p [GeV];tracks", nTrkPBins, trkPBins);
  selectedTrackPt_ = fileService->make<TH1D>("selectedTrackPt", ";selected-track p_{T} [GeV];tracks", 100, 0.0, 100.0);
  selectedTrackPtVarBins_ = fileService->make<TH1D>(
      "selectedTrackPtVarBins", ";selected-track p_{T} [GeV];tracks", nTrkPBins, trkPBins);
  selectedTrackEta_ = fileService->make<TH1D>("selectedTrackEta", ";selected-track #eta;tracks", 60, -3.0, 3.0);
  selectedTrackPhi_ = fileService->make<TH1D>("selectedTrackPhi", ";selected-track #phi;tracks", 64, -3.2, 3.2);
  selectedTrackNormalizedChi2_ = fileService->make<TH1D>(
      "selectedTrackNormalizedChi2", ";selected-track normalized #chi^{2};tracks", 100, 0.0, 10.0);
  selectedTrackRelativePError_ =
      fileService->make<TH1D>("selectedTrackRelativePError", ";selected-track #sigma_{p}/p;tracks", 100, 0.0, 0.5);
  selectedTrackTrackerLayers_ = fileService->make<TH1D>(
      "selectedTrackTrackerLayers", ";selected-track tracker layers with measurements;tracks", 30, -0.5, 29.5);
  selectedTrackPixelLayers_ = fileService->make<TH1D>(
      "selectedTrackPixelLayers", ";selected-track pixel layers with measurements;tracks", 10, -0.5, 9.5);
  selectedTrackDxy_ =
      fileService->make<TH1D>("selectedTrackDxy", ";selected-track d_{xy}(leading PV) [cm];tracks", 100, -0.1, 0.1);
  selectedTrackDz_ =
      fileService->make<TH1D>("selectedTrackDz", ";selected-track d_{z}(leading PV) [cm];tracks", 100, -0.2, 0.2);
  selectedTrackSamePVRelativeIsolation_ = fileService->make<TH1D>(
      "selectedTrackSamePVRelativeIsolation", ";selected-track same-PV relative isolation;tracks", 100, 0.0, 2.0);
  selectedTrackAllVertexRelativeIsolation_ = fileService->make<TH1D>(
      "selectedTrackAllVertexRelativeIsolation", ";selected-track all-vertex relative isolation;tracks", 100, 0.0, 2.0);

  muonMatchMultiplicity_ = fileService->make<TH1D>(
      "muonMatchMultiplicity", ";exact reco::Muon inner-track matches;selected tracks", 6, -0.5, 5.5);
  muonIDSummary_ = fileService->make<TH1D>("muonIDSummary", ";muon matching and ID;selected tracks", 7, 0.5, 7.5);
  muonIDSummary_->GetXaxis()->SetBinLabel(1, "Selected tracks");
  muonIDSummary_->GetXaxis()->SetBinLabel(2, "Exact match");
  muonIDSummary_->GetXaxis()->SetBinLabel(3, "Soft");
  muonIDSummary_->GetXaxis()->SetBinLabel(4, "Loose");
  muonIDSummary_->GetXaxis()->SetBinLabel(5, "Soft or Loose");
  muonIDSummary_->GetXaxis()->SetBinLabel(6, "Medium");
  muonIDSummary_->GetXaxis()->SetBinLabel(7, "Tight");
  selectedTrackMuonCategory_ =
      fileService->make<TH1D>("selectedTrackMuonCategory", ";muon category;selected tracks", 5, 0.5, 5.5);
  selectedTrackMuonCategory_->GetXaxis()->SetBinLabel(1, "No exact match");
  selectedTrackMuonCategory_->GetXaxis()->SetBinLabel(2, "Exact, fail Soft/Loose");
  selectedTrackMuonCategory_->GetXaxis()->SetBinLabel(3, "Soft only");
  selectedTrackMuonCategory_->GetXaxis()->SetBinLabel(4, "Loose only");
  selectedTrackMuonCategory_->GetXaxis()->SetBinLabel(5, "Soft and Loose");
  exactMuonMatchedTrackP_ =
      fileService->make<TH1D>("exactMuonMatchedTrackP", ";selected-track p [GeV];exact muon matches", 100, 0.0, 100.0);
  exactMuonMatchedTrackPVarBins_ = fileService->make<TH1D>(
      "exactMuonMatchedTrackPVarBins", ";selected-track p [GeV];exact muon matches", nTrkPBins, trkPBins);
  muonTaggedTrackP_ =
      fileService->make<TH1D>("muonTaggedTrackP", ";selected-track p [GeV];muon-tagged tracks", 100, 0.0, 100.0);
  muonTaggedTrackPVarBins_ = fileService->make<TH1D>(
      "muonTaggedTrackPVarBins", ";selected-track p [GeV];muon-tagged tracks", nTrkPBins, trkPBins);
  nonMuonTrackP_ = fileService->make<TH1D>("nonMuonTrackP", ";selected-track p [GeV];non-muon tracks", 100, 0.0, 100.0);
  nonMuonTrackPVarBins_ = fileService->make<TH1D>(
      "nonMuonTrackPVarBins", ";selected-track p [GeV];non-muon tracks", nTrkPBins, trkPBins);

  pfCandidateMatchMultiplicity_ =
      fileService->make<TH1D>("pfCandidateMatchMultiplicity",
                              ";number of exact PF-candidate matches;selected tracks",
                              5, -0.5, 4.5);

  pfCandidateParticleId_ =
      fileService->make<TH1D>("pfCandidateParticleId",
                              ";PF-candidate particle type;uniquely matched selected tracks",
                              8, -0.5, 7.5);
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(1, "X");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(2, "h");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(3, "e");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(4, "mu");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(5, "gamma");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(6, "h0");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(7, "h_HF");
  pfCandidateParticleId_->GetXaxis()->SetBinLabel(8, "egamma_HF");

  pfCandidateResponseVsParticleId_ =
      fileService->make<TH2D>("pfCandidateResponseVsParticleId",
                              ";PF-candidate particle type;PF-candidate ECAL+HCAL energy / p",
                              8, -0.5, 7.5, 200, 0.0, 5.0);
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(1, "X");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(2, "h");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(3, "e");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(4, "mu");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(5, "gamma");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(6, "h0");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(7, "h_HF");
  pfCandidateResponseVsParticleId_->GetXaxis()->SetBinLabel(8, "egamma_HF");

  pfCandidateECALPlusHCALEnergy_ =
      fileService->make<TH1D>("pfCandidateECALPlusHCALEnergy",
                              ";PF-candidate ECAL+HCAL energy [GeV];matched selected tracks",
                              200, 0.0, 200.0);

  pfCandidateECALPlusHCALResponse_ =
      fileService->make<TH1D>("pfCandidateECALPlusHCALResponse",
                              ";PF-candidate ECAL+HCAL energy / p;matched selected tracks",
                              200, 0.0, 5.0);

  pfCandidateECALPlusHCALResponseVsPVarBins_ =
      fileService->make<TH2D>("pfCandidateECALPlusHCALResponseVsPVarBins",
                              ";selected-track p [GeV];PF-candidate ECAL+HCAL energy / p",
                              nTrkPBins, trkPBins, 200, 0.0, 5.0);

  pfCandidateMatchEfficiencyVsPVarBins_ =
      fileService->make<TProfile>("pfCandidateMatchEfficiencyVsPVarBins",
                                  ";selected-track p [GeV];exact PF-candidate match efficiency",
                                  nTrkPBins, trkPBins);

  meanPFCandidateECALPlusHCALResponseVsPVarBins_ =
      fileService->make<TProfile>("meanPFCandidateECALPlusHCALResponseVsPVarBins",
                                  ";selected-track p [GeV];mean PF-candidate ECAL+HCAL energy / p",
                                  nTrkPBins, trkPBins);

  meanPFCandidateECALResponseVsPVarBins_ =
      fileService->make<TProfile>("meanPFCandidateECALResponseVsPVarBins",
                                  ";selected-track p [GeV];mean PF-candidate ECAL energy / p",
                                  nTrkPBins, trkPBins);

  meanPFCandidateHCALResponseVsPVarBins_ =
      fileService->make<TProfile>("meanPFCandidateHCALResponseVsPVarBins",
                                  ";selected-track p [GeV];mean PF-candidate HCAL energy / p",
                                  nTrkPBins, trkPBins);

  pfCandidateToLinkedEnergyRatio_ =
      fileService->make<TH1D>(
          "pfCandidateToLinkedEnergyRatio",
          ";PF-candidate (ECAL+HCAL) / linked-cluster (ECAL+HCAL);selected tracks",
          200, 0.0, 5.0);

  meanPFCandidateToLinkedEnergyRatioVsPVarBins_ =
      fileService->make<TProfile>(
          "meanPFCandidateToLinkedEnergyRatioVsPVarBins",
          ";selected-track p [GeV];<PF-candidate (ECAL+HCAL) / linked-cluster (ECAL+HCAL)>",
          nTrkPBins, trkPBins);

  meanSameTrackPFCandidateResponseVsPVarBins_ =
      fileService->make<TProfile>(
          "meanSameTrackPFCandidateResponseVsPVarBins",
          ";selected-track p [GeV];mean PF-candidate (ECAL+HCAL) / p",
          nTrkPBins, trkPBins);

  meanSameTrackLinkedResponseVsPVarBins_ =
      fileService->make<TProfile>(
          "meanSameTrackLinkedResponseVsPVarBins",
          ";selected-track p [GeV];mean linked-cluster (ECAL+HCAL) / p",
          nTrkPBins, trkPBins);

  meanHCALLinkedPFCandidateResponseVsPVarBins_ =
      fileService->make<TProfile>(
          "meanHCALLinkedPFCandidateResponseVsPVarBins",
          ";selected-track p [GeV];mean PF-candidate (ECAL+HCAL) / p",
          nTrkPBins, trkPBins);

  meanHCALLinkedRawPFCandidateResponseVsPVarBins_ =
      fileService->make<TProfile>(
          "meanHCALLinkedRawPFCandidateResponseVsPVarBins",
          ";selected-track p [GeV];mean raw PF-candidate (ECAL+HCAL) / p",
          nTrkPBins, trkPBins);

  meanDeltaResponseSharedHCALVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseSharedHCALVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanDeltaResponseNonSharedHCALVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseNonSharedHCALVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanDeltaResponseMultiTrackBlockVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseMultiTrackBlockVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanDeltaResponseMultiHCALBlockVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseMultiHCALBlockVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanDeltaResponseMultiECALBlockVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseMultiECALBlockVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanDeltaResponseSimpleBlockVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseSimpleBlockVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanDeltaResponseComplexBlockVsP_ =
      fileService->make<TProfile>(
          "meanDeltaResponseComplexBlockVsP",
          ";selected-track p [GeV];mean PF - linked calibrated response",
          nTrkPBins, trkPBins);

  meanPFResponseSimpleBlockVsP_ =
      fileService->make<TProfile>(
          "meanPFResponseSimpleBlockVsP",
          ";selected-track p [GeV];mean (ECAL+HCAL)/p",
          nTrkPBins, trkPBins);

  meanLinkedResponseSimpleBlockVsP_ =
      fileService->make<TProfile>(
          "meanLinkedResponseSimpleBlockVsP",
          ";selected-track p [GeV];mean (ECAL+HCAL)/p",
          nTrkPBins, trkPBins);

  meanPFResponseComplexBlockVsP_ =
      fileService->make<TProfile>(
          "meanPFResponseComplexBlockVsP",
          ";selected-track p [GeV];mean (ECAL+HCAL)/p",
          nTrkPBins, trkPBins);

  meanLinkedResponseComplexBlockVsP_ =
      fileService->make<TProfile>(
          "meanLinkedResponseComplexBlockVsP",
          ";selected-track p [GeV];mean (ECAL+HCAL)/p",
          nTrkPBins, trkPBins);

  hcalLinkedPFCandidateResponseVsPVarBins_ =
      fileService->make<TH2D>(
          "hcalLinkedPFCandidateResponseVsPVarBins",
          ";selected-track p [GeV];PF-candidate (ECAL+HCAL) / p",
          nTrkPBins, trkPBins,
          200, 0.0, 5.0);

  hcalLinkedRawPFCandidateResponseVsPVarBins_ =
      fileService->make<TH2D>(
          "hcalLinkedRawPFCandidateResponseVsPVarBins",
          ";selected-track p [GeV];raw PF-candidate (ECAL+HCAL) / p",
          nTrkPBins, trkPBins,
          200, 0.0, 5.0);

  hcalLinkedPFCandidateMatchEfficiencyVsPVarBins_ =
      fileService->make<TProfile>(
          "hcalLinkedPFCandidateMatchEfficiencyVsPVarBins",
          ";selected-track p [GeV];unique PF-candidate match efficiency",
          nTrkPBins, trkPBins);

  hcalLinkedChargedHadronEfficiencyVsPVarBins_ =
      fileService->make<TProfile>(
          "hcalLinkedChargedHadronEfficiencyVsPVarBins",
          ";selected-track p [GeV];charged-hadron PF-candidate efficiency",
          nTrkPBins, trkPBins);

  pfCandidateECALVsLinkedECAL_ =
      fileService->make<TH2D>(
          "pfCandidateECALVsLinkedECAL",
          ";linked ECAL energy [GeV];PF-candidate ECAL energy [GeV]",
          200, 0.0, 20.0, 200, 0.0, 20.0);

  pfCandidateHCALVsLinkedHCAL_ =
      fileService->make<TH2D>(
          "pfCandidateHCALVsLinkedHCAL",
          ";linked HCAL energy [GeV];PF-candidate HCAL energy [GeV]",
          200, 0.0, 20.0, 200, 0.0, 20.0);

  rawPFCandidateECALVsLinkedECAL_ =
      fileService->make<TH2D>(
          "rawPFCandidateECALVsLinkedECAL",
          ";linked ECAL energy [GeV];raw PF-candidate ECAL energy [GeV]",
          200, 0.0, 20.0, 200, 0.0, 20.0);

  rawPFCandidateHCALVsLinkedHCAL_ =
      fileService->make<TH2D>(
          "rawPFCandidateHCALVsLinkedHCAL",
          ";linked HCAL energy [GeV];raw PF-candidate HCAL energy [GeV]",
          200, 0.0, 20.0, 200, 0.0, 20.0);

  pfLinkingCutflow_ = fileService->make<TH1D>("pfLinkingCutflow", ";PF linking;selected tracks", 7, 0.5, 7.5);
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(1, "Selected tracks");
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(2, "Found PF block");
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(3, "Exactly one PF block");
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(4, "Linked ECAL");
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(5, "Linked HCAL");
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(6, "Linked ECAL or HCAL");
  pfLinkingCutflow_->GetXaxis()->SetBinLabel(7, "Linked ECAL and HCAL");

  pfBlockMatchMultiplicity_ = fileService->make<TH1D>(
      "pfBlockMatchMultiplicity", ";number of PF blocks containing selected track;selected tracks", 6, -0.5, 5.5);
  neutralHadronMultiplicityLinkedHCAL_ =
      fileService->make<TH1D>(
          "neutralHadronMultiplicityLinkedHCAL",
          ";neutral hadrons with HCAL association;charged hadrons",
          20, -0.5, 19.5);

  neutralHadronEnergyFractionLinkedHCAL_ =
      fileService->make<TH1D>(
          "neutralHadronEnergyFractionLinkedHCAL",
          ";neutral energy fraction (HCAL association);charged hadrons",
          50, 0.0, 1.0);

  neutralHadronMultiplicitySameHCAL_ =
      fileService->make<TH1D>(
          "neutralHadronMultiplicitySameHCAL",
          ";neutral hadrons sharing HCAL element;charged hadrons",
          20, -0.5, 19.5);

  neutralHadronEnergyFractionSameHCAL_ =
      fileService->make<TH1D>(
          "neutralHadronEnergyFractionSameHCAL",
          ";neutral energy fraction (shared HCAL element);charged hadrons",
          50, 0.0, 1.0);

  neutralHadronMultiplicitySameBlock_ =
      fileService->make<TH1D>(
          "neutralHadronMultiplicitySameBlock",
          ";neutral hadrons in same PFBlock;charged hadron candidates",
          20, -0.5, 19.5);

  neutralHadronEnergyFractionSameBlock_ =
      fileService->make<TH1D>(
          "neutralHadronEnergyFractionSameBlock",
          ";neutral hadron energy fraction;charged hadron candidates",
          50, 0.0, 1.0);

  neutralSameBlockFractionVsP_ =
      fileService->make<TProfile>(
          "neutralSameBlockFractionVsP",
          ";track p [GeV];fraction with neutral hadron in same PFBlock",
          nTrkPBins, trkPBins);

  neutralSameHCALFractionVsP_ =
      fileService->make<TProfile>(
          "neutralSameHCALFractionVsP",
          ";track p [GeV];fraction sharing HCAL element",
          nTrkPBins, trkPBins);

  neutralLinkedHCALFractionVsP_ =
      fileService->make<TProfile>(
          "neutralLinkedHCALFractionVsP",
          ";track p [GeV];fraction with linked HCAL association",
          nTrkPBins, trkPBins);

  meanNeutralHadronFractionVsP_ =
      fileService->make<TProfile>(
          "meanNeutralHadronFractionVsP",
          ";track p [GeV];mean neutral hadron energy fraction",
          nTrkPBins, trkPBins);

  meanNeutralHadronMultiplicityVsP_ =
      fileService->make<TProfile>(
          "meanNeutralHadronMultiplicityVsP",
          ";track p [GeV];mean neutral hadron multiplicity",
          nTrkPBins, trkPBins);

  matchedPFBlockNumberOfElements_ =
      fileService->make<TH1D>("matchedPFBlockNumberOfElements",
                              ";number of elements in matched PF block;uniquely matched selected tracks",
                              100,
                              -0.5,
                              99.5);
  matchedPFBlockNumberOfTrackElements_ =
      fileService->make<TH1D>("matchedPFBlockNumberOfTrackElements",
                              ";TRACK elements in matched PF block;uniquely matched selected tracks",
                              50,
                              -0.5,
                              49.5);
  matchedPFBlockNumberOfECALElements_ =
      fileService->make<TH1D>("matchedPFBlockNumberOfECALElements",
                              ";ECAL elements in matched PF block;uniquely matched selected tracks",
                              50,
                              -0.5,
                              49.5);
  matchedPFBlockNumberOfHCALElements_ =
      fileService->make<TH1D>("matchedPFBlockNumberOfHCALElements",
                              ";HCAL elements in matched PF block;uniquely matched selected tracks",
                              50,
                              -0.5,
                              49.5);
  matchedPFBlockNumberOfHOElements_ =
      fileService->make<TH1D>("matchedPFBlockNumberOfHOElements",
                              ";HO elements in matched PF block;uniquely matched selected tracks",
                              20,
                              -0.5,
                              19.5);
  matchedPFBlockNumberOfOtherElements_ =
      fileService->make<TH1D>("matchedPFBlockNumberOfOtherElements",
                              ";other elements in matched PF block;uniquely matched selected tracks",
                              50,
                              -0.5,
                              49.5);
  linkedECALClusterMultiplicity_ =
      fileService->make<TH1D>("linkedECALClusterMultiplicity",
                              ";directly linked ECAL PF clusters;uniquely matched selected tracks",
                              20,
                              -0.5,
                              19.5);
  linkedHCALClusterMultiplicity_ =
      fileService->make<TH1D>("linkedHCALClusterMultiplicity",
                              ";directly linked HCAL PF clusters;uniquely matched selected tracks",
                              20,
                              -0.5,
                              19.5);
  linkedHOClusterMultiplicity_ = fileService->make<TH1D>(
      "linkedHOClusterMultiplicity", ";directly linked HO PF clusters;uniquely matched selected tracks", 20, -0.5, 19.5);
  trackECALLinkDistance_ =
      fileService->make<TH1D>("trackECALLinkDistance", ";closest track-ECAL PF link distance;tracks", 200, 0.0, 2.0);
  trackHCALLinkDistance_ =
      fileService->make<TH1D>("trackHCALLinkDistance", ";closest track-HCAL PF link distance;tracks", 200, 0.0, 2.0);
  trackHOLinkDistance_ =
      fileService->make<TH1D>("trackHOLinkDistance", ";closest track-HO PF link distance;tracks", 200, 0.0, 2.0);
  noPFBlockMatchTrackP_ = fileService->make<TH1D>(
      "noPFBlockMatchTrackP", ";p [GeV];selected tracks without a PF-block match", 100, 0.0, 100.0);
  noPFBlockMatchTrackPVarBins_ = fileService->make<TH1D>(
      "noPFBlockMatchTrackPVarBins", ";p [GeV];selected tracks without a PF-block match", nTrkPBins, trkPBins);
  multiplePFBlockMatchesTrackP_ = fileService->make<TH1D>(
      "multiplePFBlockMatchesTrackP", ";p [GeV];selected tracks with multiple PF-block matches", 100, 0.0, 100.0);
  multiplePFBlockMatchesTrackPVarBins_ = fileService->make<TH1D>("multiplePFBlockMatchesTrackPVarBins",
                                                                 ";p [GeV];selected tracks with multiple PF-block matches",
                                                                 nTrkPBins,
                                                                 trkPBins);
  noLinkedECALTrackP_ = fileService->make<TH1D>(
      "noLinkedECALTrackP", ";p [GeV];uniquely matched selected tracks without a linked ECAL cluster", 100, 0.0, 100.0);
  noLinkedECALTrackPVarBins_ = fileService->make<TH1D>(
      "noLinkedECALTrackPVarBins",
      ";p [GeV];uniquely matched selected tracks without a linked ECAL cluster",
      nTrkPBins,
      trkPBins);
  noLinkedHCALTrackP_ = fileService->make<TH1D>(
      "noLinkedHCALTrackP", ";p [GeV];uniquely matched selected tracks without a linked HCAL cluster", 100, 0.0, 100.0);
  noLinkedHCALTrackPVarBins_ = fileService->make<TH1D>(
      "noLinkedHCALTrackPVarBins",
      ";p [GeV];uniquely matched selected tracks without a linked HCAL cluster",
      nTrkPBins,
      trkPBins);

  linkedECALEnergy_ =
      fileService->make<TH1D>("linkedECALEnergy",
                              ";closest linked ECAL PF-cluster energy [GeV];uniquely matched selected tracks",
                              200,
                              0.0,
                              200.0);
  linkedHCALEnergy_ =
      fileService->make<TH1D>("linkedHCALEnergy",
                              ";closest linked HCAL PF-cluster energy [GeV];uniquely matched selected tracks",
                              200,
                              0.0,
                              200.0);
  linkedHOEnergy_ = fileService->make<TH1D>(
      "linkedHOEnergy", ";closest linked HO PF-cluster energy [GeV];uniquely matched selected tracks", 100, 0.0, 50.0);
  linkedECALPlusHCALEnergy_ =
      fileService->make<TH1D>("linkedECALPlusHCALEnergy",
                              ";closest linked ECAL+HCAL PF-cluster energy [GeV];uniquely matched selected tracks",
                              200,
                              0.0,
                              200.0);
  linkedECALResponse_ =
      fileService->make<TH1D>("linkedECALResponse",
                              ";closest linked ECAL PF-cluster energy / p;uniquely matched selected tracks",
                              200,
                              0.0,
                              5.0);
  linkedHCALResponse_ =
      fileService->make<TH1D>("linkedHCALResponse",
                              ";closest linked HCAL PF-cluster energy / p;uniquely matched selected tracks",
                              200,
                              0.0,
                              5.0);
  linkedECALPlusHCALResponse_ =
      fileService->make<TH1D>("linkedECALPlusHCALResponse",
                              ";closest linked ECAL+HCAL PF-cluster energy / p;uniquely matched selected tracks",
                              200,
                              0.0,
                              5.0);
  linkedECALPlusHCALPlusHOResponse_ =
      fileService->make<TH1D>("linkedECALPlusHCALPlusHOResponse",
                              ";closest linked ECAL+HCAL+HO PF-cluster energy / p;uniquely matched selected tracks",
                              200,
                              0.0,
                              5.0);
  linkedECALEnergyFraction_ =
      fileService->make<TH1D>("linkedECALEnergyFraction",
                              ";closest linked ECAL / (ECAL+HCAL) energy;tracks with ECAL+HCAL energy > 0",
                              100,
                              0.0,
                              1.0);
  prePFHadronCalibrationLinkedECALEnergy_ = fileService->make<TH1D>(
      "prePFHadronCalibrationLinkedECALEnergy",
      ";pre-PF-hadron-calibration closest linked ECAL energy [GeV];tracks with closest linked HCAL",
      200,
      0.0,
      200.0);
  prePFHadronCalibrationLinkedHCALEnergy_ = fileService->make<TH1D>(
      "prePFHadronCalibrationLinkedHCALEnergy",
      ";pre-PF-hadron-calibration closest linked HCAL energy [GeV];tracks with closest linked HCAL",
      200,
      0.0,
      200.0);
  prePFHadronCalibrationLinkedECALPlusHCALEnergy_ = fileService->make<TH1D>(
      "prePFHadronCalibrationLinkedECALPlusHCALEnergy",
      ";pre-PF-hadron-calibration closest linked ECAL+HCAL energy [GeV];tracks with closest linked HCAL",
      200,
      0.0,
      200.0);
  prePFHadronCalibrationLinkedECALResponse_ = fileService->make<TH1D>(
      "prePFHadronCalibrationLinkedECALResponse",
      ";pre-PF-hadron-calibration closest linked ECAL energy / p;tracks with closest linked HCAL",
      200,
      0.0,
      5.0);
  prePFHadronCalibrationLinkedHCALResponse_ = fileService->make<TH1D>(
      "prePFHadronCalibrationLinkedHCALResponse",
      ";pre-PF-hadron-calibration closest linked HCAL energy / p;tracks with closest linked HCAL",
      200,
      0.0,
      5.0);
  prePFHadronCalibrationLinkedECALPlusHCALResponse_ = fileService->make<TH1D>(
      "prePFHadronCalibrationLinkedECALPlusHCALResponse",
      ";pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
      200,
      0.0,
      5.0);
  pfHadronCalibratedLinkedECALEnergy_ =
      fileService->make<TH1D>("pfHadronCalibratedLinkedECALEnergy",
                              ";PF hadron-calibrated closest linked ECAL energy [GeV];tracks with closest linked HCAL",
                              200,
                              0.0,
                              200.0);
  pfHadronCalibratedLinkedHCALEnergy_ =
      fileService->make<TH1D>("pfHadronCalibratedLinkedHCALEnergy",
                              ";PF hadron-calibrated closest linked HCAL energy [GeV];tracks with closest linked HCAL",
                              200,
                              0.0,
                              200.0);
  pfHadronCalibratedLinkedECALPlusHCALEnergy_ =
      fileService->make<TH1D>("pfHadronCalibratedLinkedECALPlusHCALEnergy",
                              ";PF hadron-calibrated closest linked ECAL+HCAL energy [GeV];tracks with closest linked HCAL",
                              200,
                              0.0,
                              200.0);
  pfHadronCalibratedLinkedECALResponse_ =
      fileService->make<TH1D>("pfHadronCalibratedLinkedECALResponse",
                              ";PF hadron-calibrated closest linked ECAL energy / p;tracks with closest linked HCAL",
                              200,
                              0.0,
                              5.0);
  pfHadronCalibratedLinkedHCALResponse_ =
      fileService->make<TH1D>("pfHadronCalibratedLinkedHCALResponse",
                              ";PF hadron-calibrated closest linked HCAL energy / p;tracks with closest linked HCAL",
                              200,
                              0.0,
                              5.0);
  pfHadronCalibratedLinkedECALPlusHCALResponse_ =
      fileService->make<TH1D>("pfHadronCalibratedLinkedECALPlusHCALResponse",
                              ";PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              200,
                              0.0,
                              5.0);
  pfHadronCalibrationFactor_ =
      fileService->make<TH1D>("pfHadronCalibrationFactor",
                              ";PF hadron-calibrated energy / raw energy;tracks with closest linked HCAL and raw energy > 0",
                              1000,
                              0.0,
                              50.0);
  pfHadronCalibrationResponseChange_ =
      fileService->make<TH1D>("pfHadronCalibrationResponseChange",
                              ";PF hadron-calibrated response - raw response;tracks with closest linked HCAL",
                              400,
                              -5.0,
                              5.0);
  muonTaggedLinkedECALPlusHCALResponse_ =
      fileService->make<TH1D>("muonTaggedLinkedECALPlusHCALResponse",
                              ";closest linked ECAL+HCAL PF-cluster energy / p;muon-tagged tracks",
                              200,
                              0.0,
                              5.0);
  nonMuonLinkedECALPlusHCALResponse_ =
      fileService->make<TH1D>("nonMuonLinkedECALPlusHCALResponse",
                              ";closest linked ECAL+HCAL PF-cluster energy / p;non-muon tracks",
                              200,
                              0.0,
                              5.0);

  linkedECALEnergyVsLinkedHCALEnergy_ = fileService->make<TH2D>(
      "linkedECALEnergyVsLinkedHCALEnergy",
      ";closest linked ECAL PF-cluster energy [GeV];closest linked HCAL PF-cluster energy [GeV]",
      200,
      0.0,
      200.0,
      200,
      0.0,
      200.0);
  linkedECALPlusHCALResponseVsP_ =
      fileService->make<TH2D>("linkedECALPlusHCALResponseVsP",
                              ";selected-track p [GeV];closest linked ECAL+HCAL PF-cluster energy / p",
                              100,
                              0.0,
                              100.0,
                              200,
                              0.0,
                              5.0);
  linkedECALPlusHCALResponseVsPVarBins_ =
      fileService->make<TH2D>("linkedECALPlusHCALResponseVsPVarBins",
                              ";selected-track p [GeV];closest linked ECAL+HCAL PF-cluster energy / p",
                              nTrkPBins,
                              trkPBins,
                              200,
                              0.0,
                              5.0);
  linkedECALPlusHCALResponseVsEta_ =
      fileService->make<TH2D>("linkedECALPlusHCALResponseVsEta",
                              ";selected-track #eta;closest linked ECAL+HCAL PF-cluster energy / p",
                              40,
                              -maxTrackAbsEta_,
                              maxTrackAbsEta_,
                              200,
                              0.0,
                              5.0);
  linkedECALPlusHCALResponseVsGoodVertices_ =
      fileService->make<TH2D>("linkedECALPlusHCALResponseVsGoodVertices",
                              ";number of good primary vertices;closest linked ECAL+HCAL PF-cluster energy / p",
                              20,
                              -0.5,
                              19.5,
                              200,
                              0.0,
                              5.0);
  linkedECALPlusHCALResponseVsPFBlockTracks_ =
      fileService->make<TH2D>("linkedECALPlusHCALResponseVsPFBlockTracks",
                              ";TRACK elements in matched PF block;closest linked ECAL+HCAL PF-cluster energy / p",
                              20,
                              -0.5,
                              19.5,
                              200,
                              0.0,
                              5.0);
  prePFHadronCalibrationLinkedECALPlusHCALResponseVsP_ = fileService->make<TH2D>(
      "prePFHadronCalibrationLinkedECALPlusHCALResponseVsP",
      ";selected-track p [GeV];pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
      100,
      0.0,
      100.0,
      200,
      0.0,
      5.0);
  prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins_ = fileService->make<TH2D>(
      "prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins",
      ";selected-track p [GeV];pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
      nTrkPBins,
      trkPBins,
      200,
      0.0,
      5.0);
  prePFHadronCalibrationLinkedECALPlusHCALResponseVsEta_ = fileService->make<TH2D>(
      "prePFHadronCalibrationLinkedECALPlusHCALResponseVsEta",
      ";selected-track #eta;pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
      40,
      -maxTrackAbsEta_,
      maxTrackAbsEta_,
      200,
      0.0,
      5.0);
  prePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices_ = fileService->make<TH2D>(
      "prePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices",
      ";number of good primary vertices;pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
      20,
      -0.5,
      19.5,
      200,
      0.0,
      5.0);
  prePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks_ = fileService->make<TH2D>(
      "prePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks",
      ";TRACK elements in matched PF block;pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
      20,
      -0.5,
      19.5,
      200,
      0.0,
      5.0);
  pfHadronCalibratedLinkedECALPlusHCALResponseVsP_ =
      fileService->make<TH2D>("pfHadronCalibratedLinkedECALPlusHCALResponseVsP",
                              ";selected-track p [GeV];PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              100,
                              0.0,
                              100.0,
                              200,
                              0.0,
                              5.0);
  pfHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins_ =
      fileService->make<TH2D>("pfHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins",
                              ";selected-track p [GeV];PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              nTrkPBins,
                              trkPBins,
                              200,
                              0.0,
                              5.0);
  pfHadronCalibratedLinkedECALPlusHCALResponseVsEta_ =
      fileService->make<TH2D>("pfHadronCalibratedLinkedECALPlusHCALResponseVsEta",
                              ";selected-track #eta;PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              40,
                              -maxTrackAbsEta_,
                              maxTrackAbsEta_,
                              200,
                              0.0,
                              5.0);
  pfHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices_ =
      fileService->make<TH2D>("pfHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices",
                              ";number of good primary vertices;PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              20,
                              -0.5,
                              19.5,
                              200,
                              0.0,
                              5.0);
  pfHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks_ =
      fileService->make<TH2D>("pfHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks",
                              ";TRACK elements in matched PF block;PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              20,
                              -0.5,
                              19.5,
                              200,
                              0.0,
                              5.0);
  rawVsPFHadronCalibratedLinkedECALPlusHCALResponse_ =
      fileService->make<TH2D>("rawVsPFHadronCalibratedLinkedECALPlusHCALResponse",
                              ";raw closest linked ECAL+HCAL energy / p;PF hadron-calibrated closest linked ECAL+HCAL energy / p;tracks with closest linked HCAL",
                              200,
                              0.0,
                              5.0,
                              200,
                              0.0,
                              5.0);
  muonTaggedLinkedECALPlusHCALResponseVsP_ =
      fileService->make<TH2D>("muonTaggedLinkedECALPlusHCALResponseVsP",
                              ";selected-track p [GeV];closest linked ECAL+HCAL PF-cluster energy / p",
                              100,
                              0.0,
                              100.0,
                              200,
                              0.0,
                              5.0);
  muonTaggedLinkedECALPlusHCALResponseVsPVarBins_ =
      fileService->make<TH2D>("muonTaggedLinkedECALPlusHCALResponseVsPVarBins",
                              ";selected-track p [GeV];closest linked ECAL+HCAL PF-cluster energy / p",
                              nTrkPBins,
                              trkPBins,
                              200,
                              0.0,
                              5.0);
  nonMuonLinkedECALPlusHCALResponseVsP_ =
      fileService->make<TH2D>("nonMuonLinkedECALPlusHCALResponseVsP",
                              ";selected-track p [GeV];closest linked ECAL+HCAL PF-cluster energy / p",
                              100,
                              0.0,
                              100.0,
                              200,
                              0.0,
                              5.0);
  nonMuonLinkedECALPlusHCALResponseVsPVarBins_ =
      fileService->make<TH2D>("nonMuonLinkedECALPlusHCALResponseVsPVarBins",
                              ";selected-track p [GeV];closest linked ECAL+HCAL PF-cluster energy / p",
                              nTrkPBins,
                              trkPBins,
                              200,
                              0.0,
                              5.0);

  meanTrackPtVsEta_ =
      fileService->make<TProfile>("meanTrackPtVsEta", ";track #eta;mean track p_{T} [GeV]", 60, -3.0, 3.0);
  meanTrackPtVsEta_->SetMinimum(0.0);
  meanTrackPtVsEta_->SetMaximum(100.0);
  meanSelectedTracksVsGoodVertices_ = fileService->make<TProfile>(
      "meanSelectedTracksVsGoodVertices", ";number of good primary vertices;mean selected tracks", 20, -0.5, 19.5);

  exactMuonMatchEfficiencyVsP_ = fileService->make<TProfile>(
      "exactMuonMatchEfficiencyVsP", ";selected-track p [GeV];fraction with an exact muon match", 100, 0.0, 100.0);
  exactMuonMatchEfficiencyVsPVarBins_ = fileService->make<TProfile>(
      "exactMuonMatchEfficiencyVsPVarBins",
      ";selected-track p [GeV];fraction with an exact muon match",
      nTrkPBins,
      trkPBins);
  exactMuonMatchEfficiencyVsEta_ = fileService->make<TProfile>("exactMuonMatchEfficiencyVsEta",
                                                               ";selected-track #eta;fraction with an exact muon match",
                                                               40,
                                                               -maxTrackAbsEta_,
                                                               maxTrackAbsEta_);
  muonTagEfficiencyVsP_ = fileService->make<TProfile>(
      "muonTagEfficiencyVsP", ";selected-track p [GeV];fraction tagged as Soft or Loose muon", 100, 0.0, 100.0);
  muonTagEfficiencyVsPVarBins_ = fileService->make<TProfile>(
      "muonTagEfficiencyVsPVarBins",
      ";selected-track p [GeV];fraction tagged as Soft or Loose muon",
      nTrkPBins,
      trkPBins);
  muonTagEfficiencyVsEta_ = fileService->make<TProfile>("muonTagEfficiencyVsEta",
                                                        ";selected-track #eta;fraction tagged as Soft or Loose muon",
                                                        40,
                                                        -maxTrackAbsEta_,
                                                        maxTrackAbsEta_);

  pfBlockMatchEfficiencyVsP_ = fileService->make<TProfile>(
      "pfBlockMatchEfficiencyVsP", ";selected-track p [GeV];fraction with a PF-block match", 100, 0.0, 100.0);
  pfBlockMatchEfficiencyVsPVarBins_ = fileService->make<TProfile>(
      "pfBlockMatchEfficiencyVsPVarBins",
      ";selected-track p [GeV];fraction with a PF-block match",
      nTrkPBins,
      trkPBins);
  pfBlockMatchEfficiencyVsEta_ = fileService->make<TProfile>("pfBlockMatchEfficiencyVsEta",
                                                             ";selected-track #eta;fraction with a PF-block match",
                                                             40,
                                                             -maxTrackAbsEta_,
                                                             maxTrackAbsEta_);
  pfBlockMatchEfficiencyVsGoodVertices_ =
      fileService->make<TProfile>("pfBlockMatchEfficiencyVsGoodVertices",
                                  ";number of good primary vertices;fraction with a PF-block match",
                                  20,
                                  -0.5,
                                  19.5);
  uniquePFBlockMatchEfficiencyVsP_ =
      fileService->make<TProfile>("uniquePFBlockMatchEfficiencyVsP",
                                  ";selected-track p [GeV];fraction with exactly one PF-block match",
                                  100,
                                  0.0,
                                  100.0);
  uniquePFBlockMatchEfficiencyVsPVarBins_ = fileService->make<TProfile>(
      "uniquePFBlockMatchEfficiencyVsPVarBins",
      ";selected-track p [GeV];fraction with exactly one PF-block match",
      nTrkPBins,
      trkPBins);
  uniquePFBlockMatchEfficiencyVsEta_ =
      fileService->make<TProfile>("uniquePFBlockMatchEfficiencyVsEta",
                                  ";selected-track #eta;fraction with exactly one PF-block match",
                                  40,
                                  -maxTrackAbsEta_,
                                  maxTrackAbsEta_);
  uniquePFBlockMatchEfficiencyVsGoodVertices_ =
      fileService->make<TProfile>("uniquePFBlockMatchEfficiencyVsGoodVertices",
                                  ";number of good primary vertices;fraction with exactly one PF-block match",
                                  20,
                                  -0.5,
                                  19.5);
  linkedECALEfficiencyVsP_ = fileService->make<TProfile>(
      "linkedECALEfficiencyVsP", ";selected-track p [GeV];fraction with a direct ECAL PF link", 100, 0.0, 100.0);
  linkedECALEfficiencyVsPVarBins_ = fileService->make<TProfile>(
      "linkedECALEfficiencyVsPVarBins",
      ";selected-track p [GeV];fraction with a direct ECAL PF link",
      nTrkPBins,
      trkPBins);
  linkedECALEfficiencyVsEta_ = fileService->make<TProfile>("linkedECALEfficiencyVsEta",
                                                           ";selected-track #eta;fraction with a direct ECAL PF link",
                                                           40,
                                                           -maxTrackAbsEta_,
                                                           maxTrackAbsEta_);
  linkedECALEfficiencyVsGoodVertices_ =
      fileService->make<TProfile>("linkedECALEfficiencyVsGoodVertices",
                                  ";number of good primary vertices;fraction with a direct ECAL PF link",
                                  20,
                                  -0.5,
                                  19.5);
  linkedHCALEfficiencyVsP_ = fileService->make<TProfile>(
      "linkedHCALEfficiencyVsP", ";selected-track p [GeV];fraction with a direct HCAL PF link", 100, 0.0, 100.0);
  linkedHCALEfficiencyVsPVarBins_ = fileService->make<TProfile>(
      "linkedHCALEfficiencyVsPVarBins",
      ";selected-track p [GeV];fraction with a direct HCAL PF link",
      nTrkPBins,
      trkPBins);
  linkedHCALEfficiencyVsEta_ = fileService->make<TProfile>("linkedHCALEfficiencyVsEta",
                                                           ";selected-track #eta;fraction with a direct HCAL PF link",
                                                           40,
                                                           -maxTrackAbsEta_,
                                                           maxTrackAbsEta_);
  linkedHCALEfficiencyVsGoodVertices_ =
      fileService->make<TProfile>("linkedHCALEfficiencyVsGoodVertices",
                                  ";number of good primary vertices;fraction with a direct HCAL PF link",
                                  20,
                                  -0.5,
                                  19.5);
  meanLinkedECALPlusHCALResponseVsP_ =
      fileService->make<TProfile>("meanLinkedECALPlusHCALResponseVsP",
                                  ";selected-track p [GeV];mean closest linked ECAL+HCAL PF-cluster energy / p",
                                  100,
                                  0.0,
                                  100.0);
  meanLinkedECALPlusHCALResponseVsPVarBins_ = fileService->make<TProfile>(
      "meanLinkedECALPlusHCALResponseVsPVarBins",
      ";selected-track p [GeV];mean closest linked ECAL+HCAL PF-cluster energy / p",
      nTrkPBins,
      trkPBins);
  meanLinkedECALPlusHCALResponseVsEta_ =
      fileService->make<TProfile>("meanLinkedECALPlusHCALResponseVsEta",
                                  ";selected-track #eta;mean closest linked ECAL+HCAL PF-cluster energy / p",
                                  40,
                                  -maxTrackAbsEta_,
                                  maxTrackAbsEta_);
  meanLinkedECALPlusHCALResponseVsGoodVertices_ = fileService->make<TProfile>(
      "meanLinkedECALPlusHCALResponseVsGoodVertices",
      ";number of good primary vertices;mean closest linked ECAL+HCAL PF-cluster energy / p",
      20,
      -0.5,
      19.5);
  meanLinkedECALPlusHCALResponseVsPFBlockTracks_ = fileService->make<TProfile>(
      "meanLinkedECALPlusHCALResponseVsPFBlockTracks",
      ";TRACK elements in matched PF block;mean closest linked ECAL+HCAL PF-cluster energy / p",
      20,
      -0.5,
      19.5);
  meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsP_ = fileService->make<TProfile>(
      "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsP",
      ";selected-track p [GeV];mean pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      100,
      0.0,
      100.0);
  meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins_ = fileService->make<TProfile>(
      "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins",
      ";selected-track p [GeV];mean pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      nTrkPBins,
      trkPBins);
  meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsEta_ = fileService->make<TProfile>(
      "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsEta",
      ";selected-track #eta;mean pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      40,
      -maxTrackAbsEta_,
      maxTrackAbsEta_);
  meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices_ = fileService->make<TProfile>(
      "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices",
      ";number of good primary vertices;mean pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      20,
      -0.5,
      19.5);
  meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks_ = fileService->make<TProfile>(
      "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks",
      ";TRACK elements in matched PF block;mean pre-PF-hadron-calibration closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      20,
      -0.5,
      19.5);
  meanPFHadronCalibratedLinkedECALPlusHCALResponseVsP_ =
      fileService->make<TProfile>("meanPFHadronCalibratedLinkedECALPlusHCALResponseVsP",
                                  ";selected-track p [GeV];mean PF hadron-calibrated closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
                                  100,
                                  0.0,
                                  100.0);
  meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins_ = fileService->make<TProfile>(
      "meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins",
      ";selected-track p [GeV];mean PF hadron-calibrated closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      nTrkPBins,
      trkPBins);
  meanPFHadronCalibratedLinkedECALPlusHCALResponseVsEta_ =
      fileService->make<TProfile>("meanPFHadronCalibratedLinkedECALPlusHCALResponseVsEta",
                                  ";selected-track #eta;mean PF hadron-calibrated closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
                                  40,
                                  -maxTrackAbsEta_,
                                  maxTrackAbsEta_);
  meanPFHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices_ = fileService->make<TProfile>(
      "meanPFHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices",
      ";number of good primary vertices;mean PF hadron-calibrated closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      20,
      -0.5,
      19.5);
  meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks_ = fileService->make<TProfile>(
      "meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks",
      ";TRACK elements in matched PF block;mean PF hadron-calibrated closest linked ECAL+HCAL energy / p for tracks with closest linked HCAL",
      20,
      -0.5,
      19.5);
  meanMuonTaggedLinkedECALPlusHCALResponseVsP_ = fileService->make<TProfile>(
      "meanMuonTaggedLinkedECALPlusHCALResponseVsP",
      ";selected-track p [GeV];mean closest linked ECAL+HCAL PF-cluster energy / p for muon-tagged tracks",
      100,
      0.0,
      100.0);
  meanMuonTaggedLinkedECALPlusHCALResponseVsPVarBins_ = fileService->make<TProfile>(
      "meanMuonTaggedLinkedECALPlusHCALResponseVsPVarBins",
      ";selected-track p [GeV];mean closest linked ECAL+HCAL PF-cluster energy / p for muon-tagged tracks",
      nTrkPBins,
      trkPBins);
  meanNonMuonLinkedECALPlusHCALResponseVsP_ = fileService->make<TProfile>(
      "meanNonMuonLinkedECALPlusHCALResponseVsP",
      ";selected-track p [GeV];mean closest linked ECAL+HCAL PF-cluster energy / p for non-muon tracks",
      100,
      0.0,
      100.0);
  meanNonMuonLinkedECALPlusHCALResponseVsPVarBins_ = fileService->make<TProfile>(
      "meanNonMuonLinkedECALPlusHCALResponseVsPVarBins",
      ";selected-track p [GeV];mean closest linked ECAL+HCAL PF-cluster energy / p for non-muon tracks",
      nTrkPBins,
      trkPBins);
}

void SPRLowPUDataAnalyzer::beginRun(edm::Run const&, edm::EventSetup const& setup) {
  auto const payloadHandle = setup.getHandle(pfCalibrationToken_);
  auto const* calibrationFunctions = static_cast<PerformancePayloadFromTFormula const*>(payloadHandle.product());
  pfEnergyCalibration_.setCalibrationFunctions(calibrationFunctions);
}

void SPRLowPUDataAnalyzer::analyze(edm::Event const& event, edm::EventSetup const&) {
  auto const tracksHandle = event.getHandle(tracksToken_);
  auto const& tracks = *tracksHandle;
  auto const& pfBlocks = event.get(pfBlocksToken_);
  auto const& pfCandidates = event.get(pfCandidatesToken_);
  auto const& muons = event.get(muonsToken_);

  std::set<reco::TrackRef> v0DaughterTracks;
  std::map<reco::TrackRef, std::set<int>> v0TrackSpecies;

  if (doV0Tagging_) {
    auto collectDaughters = [&](auto const& candidates, double category) {
      for (auto const& candidate : candidates) {
        v0CandidateCounts_->Fill(category);

        if (candidate.numberOfDaughters() != 2)
          continue;

        for (unsigned int i = 0; i < 2; ++i) {
          auto const* daughter =
              dynamic_cast<reco::RecoChargedCandidate const*>(
                  candidate.daughter(i));

          if (!daughter || daughter->track().isNull())
            continue;

          v0DaughterCutflow_->Fill(1.0);
          v0DaughterTracks.insert(daughter->track());

          int species = 0;

          // Daughter ordering follows CMSSW V0Fitter:
          // Lambda:      proton, pion-
          // anti-Lambda: anti-proton, pion+
          // Kshort:      pion+, pion-
          if (candidate.pdgId() == 3122) {
            species = (i == 0) ? 1 : 3;
          } else if (candidate.pdgId() == -3122) {
            species = (i == 0) ? 2 : 3;
          } else if (candidate.pdgId() == 310) {
            species = 4;
          }

          if (species > 0)
            v0TrackSpecies[daughter->track()].insert(species);
        }
      }
    };

    auto const lambdas = event.getHandle(lambdaToken_);
    auto const kshorts = event.getHandle(kshortToken_);

    if (lambdas.isValid())
      collectDaughters(*lambdas, 1.0);

    if (kshorts.isValid())
      collectDaughters(*kshorts, 2.0);
  }

  std::set<reco::TrackRef> matchedV0Tracks;

  if (doV0Tagging_) {
    for (std::size_t i = 0; i < tracks.size(); ++i) {
      reco::TrackRef const ref(tracksHandle, i);
      if (v0DaughterTracks.count(ref)) {
        matchedV0Tracks.insert(ref);
        v0DaughterCutflow_->Fill(2.0);
      }
    }
  }

  eventCount_->Fill(0.5);
  numberOfTracks_->Fill(tracks.size());
  numberOfPFBlocks_->Fill(pfBlocks.size());

  for (auto const& track : tracks) {
    meanTrackPtVsEta_->Fill(track.eta(), track.pt());
  }

  eventCutflow_->Fill(1.0);

  unsigned int numberOfGoodVertices = 0;
  reco::Vertex const* leadingVertex = nullptr;
  if (requirePrimaryVertex_) {
    auto const& vertices = event.get(verticesToken_);
    numberOfVertices_->Fill(vertices.size());

    for (auto const& vertex : vertices) {
      if (isGoodVertex(vertex)) {
        ++numberOfGoodVertices;
      }
    }
    numberOfGoodVertices_->Fill(numberOfGoodVertices);

    if (vertices.empty()) {
      return;
    }
    eventCutflow_->Fill(2.0);

    leadingVertex = &vertices.front();
    leadingVertexZ_->Fill(leadingVertex->z());
    leadingVertexRho_->Fill(leadingVertex->position().Rho());
    leadingVertexNdof_->Fill(leadingVertex->ndof());

    if (!isGoodVertex(*leadingVertex)) {
      return;
    }
    eventCutflow_->Fill(3.0);
  } else {
    eventCutflow_->Fill(2.0);
    eventCutflow_->Fill(3.0);
  }

  unsigned int numberOfSelectedTracks = 0;
  for (std::size_t trackIndex = 0; trackIndex < tracks.size(); ++trackIndex) {
    reco::Track const& track = tracks[trackIndex];
    reco::TrackRef const v0TrackRef(tracksHandle, trackIndex);
    bool const isV0Daughter =
        doV0Tagging_ && matchedV0Tracks.count(v0TrackRef) != 0;

    if (isV0Daughter) {
      auto const speciesForCutflow = v0TrackSpecies.find(v0TrackRef);

      if (speciesForCutflow != v0TrackSpecies.end()) {
        auto const fillSpeciesStep = [&](int step) {
          for (int species : speciesForCutflow->second) {
            v0SpeciesCutflow_->Fill(step, species);
          }
        };

        fillSpeciesStep(1);

        if (track.quality(trackQuality_)) {
          fillSpeciesStep(2);

          for (int species : speciesForCutflow->second) {
            v0SpeciesPt_->Fill(track.pt(), species);
            v0SpeciesP_->Fill(track.p(), species);
            v0SpeciesEta_->Fill(track.eta(), species);
          }

          if (minV0TrackPt_ < 0.0 || track.pt() >= minV0TrackPt_) {
            fillSpeciesStep(3);

            if (track.p() >= minV0TrackP_) {
              fillSpeciesStep(4);

              if (std::abs(track.eta()) <= maxTrackAbsEta_) {
                fillSpeciesStep(5);

                if (track.normalizedChi2() <= maxTrackNormalizedChi2_) {
                  fillSpeciesStep(6);

                  if (track.qoverp() != 0.0 &&
                      track.qoverpError() / std::abs(track.qoverp()) <=
                          maxTrackRelativePError_) {
                    fillSpeciesStep(7);

                    if (track.hitPattern().trackerLayersWithMeasurement() >=
                        static_cast<int>(minTrackerLayers_)) {
                      fillSpeciesStep(8);

                      if (track.hitPattern().pixelLayersWithMeasurement() >=
                          static_cast<int>(minPixelLayers_)) {
                        fillSpeciesStep(9);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      bool const passesV0TrackQuality =
          track.quality(trackQuality_) &&
          (minV0TrackPt_ < 0.0 || track.pt() >= minV0TrackPt_) &&
          track.p() >= minV0TrackP_ &&
          std::abs(track.eta()) <= maxTrackAbsEta_ &&
          track.normalizedChi2() <= maxTrackNormalizedChi2_ &&
          track.qoverp() != 0.0 &&
          track.qoverpError() / std::abs(track.qoverp()) <=
              maxTrackRelativePError_ &&
          track.hitPattern().trackerLayersWithMeasurement() >=
              static_cast<int>(minTrackerLayers_) &&
          track.hitPattern().pixelLayersWithMeasurement() >=
              static_cast<int>(minPixelLayers_);

      if (passesV0TrackQuality) {
        auto const speciesIt = v0TrackSpecies.find(v0TrackRef);

        if (speciesIt != v0TrackSpecies.end()) {
          v0SpeciesMultiplicity_->Fill(speciesIt->second.size());

          int speciesMask = 0;
          for (int species : speciesIt->second) {
            speciesMask |= (1 << (species - 1));
          }
          v0SpeciesCombination_->Fill(speciesMask);

          unsigned int matchCount = 0;
          reco::PFCandidate const* matched = nullptr;

          for (auto const& candidate : pfCandidates) {
            if (candidate.trackRef().isNonnull() &&
                candidate.trackRef() == v0TrackRef) {
              ++matchCount;
              matched = &candidate;
            }
          }

          for (int species : speciesIt->second) {
            v0SpeciesCounts_->Fill(species);

            if (matchCount == 1 && matched != nullptr &&
                matched->particleId() == reco::PFCandidate::h) {
              v0SpeciesPFMatches_->Fill(species);
              v0SpeciesCutflow_->Fill(10, species);
              double const response =
                  (matched->ecalEnergy() + matched->hcalEnergy()) /
                  track.p();

              v0SpeciesPFResponse_->Fill(species, response);
              v0SpeciesResponseVsP_[species - 1]->Fill(
                  track.p(), response);

              if (response > 0.0) {
                v0SpeciesPositiveResponseVsP_[species - 1]->Fill(
                    track.p(), response);
              }

              v0PFBlockLinkStatus_->Fill(species, 1);

              unsigned int blockMatches = 0;
              reco::PFBlock const* matchedBlock = nullptr;
              unsigned int trackElementIndex = 0;

              for (auto const& block : pfBlocks) {
                auto const& elements = block.elements();
                for (unsigned int j = 0; j < elements.size(); ++j) {
                  if (elements[j].type() == reco::PFBlockElement::TRACK &&
                      elements[j].trackRef() == v0TrackRef) {
                    ++blockMatches;
                    matchedBlock = &block;
                    trackElementIndex = j;
                    break;
                  }
                }
              }

              if (blockMatches == 0) {
                v0PFBlockLinkStatus_->Fill(species, 2);
              } else if (blockMatches > 1) {
                v0PFBlockLinkStatus_->Fill(species, 4);
              } else {
                v0PFBlockLinkStatus_->Fill(species, 3);

                auto const& elements = matchedBlock->elements();

                auto closestEnergy = [&](reco::PFBlockElement::Type type,
                                         bool& found) {
                  std::multimap<double, unsigned int> links;
                  matchedBlock->associatedElements(
                      trackElementIndex, matchedBlock->linkData(),
                      links, type, reco::PFBlock::LINKTEST_ALL);

                  for (auto const& link : links) {
                    auto const& cluster = elements[link.second].clusterRef();
                    if (cluster.isNonnull() && cluster.isAvailable()) {
                      found = true;
                      return static_cast<double>(cluster->energy());
                    }
                  }
                  return 0.0;
                };

                bool hasECAL = false;
                bool hasHCAL = false;

                double const ecal = closestEnergy(
                    reco::PFBlockElement::ECAL, hasECAL);
                double const hcal = closestEnergy(
                    reco::PFBlockElement::HCAL, hasHCAL);

                if (hasECAL)
                  v0PFBlockLinkStatus_->Fill(species, 5);
                if (hasHCAL)
                  v0PFBlockLinkStatus_->Fill(species, 6);

                double const linkedResponse =
                    (ecal + hcal) / track.p();

                v0PFVsLinkedResponse_->Fill(
                    response, linkedResponse);

                bool const hasCluster = hasECAL || hasHCAL;

                int const responseCategory =
                    response == 0.0 ? 0 :
                    response < 0.05 ? 1 : 2;

                v0SpeciesResponseLinkCategory_->Fill(
                    species,
                    1 + responseCategory + (hasCluster ? 3 : 0));

                if (hasECAL || hasHCAL) {
                  v0PFVsLinkedResponseAnyCluster_->Fill(
                      response, linkedResponse);
                } else {
                  v0PFVsLinkedResponseNoCluster_->Fill(
                      response, linkedResponse);
                }

                if (hasHCAL) {
                  v0PFVsLinkedResponseHCAL_->Fill(
                      response, linkedResponse);
                }
              }
            }
          }
        }
      }
    }

    trackCutflow_->Fill(1.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(1.0);

    if (!track.quality(trackQuality_)) {
      continue;
    }
    trackCutflow_->Fill(2.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(2.0);

    if (minTrackPt_ >= 0.0 && track.pt() < minTrackPt_) {
      continue;
    }
    trackCutflow_->Fill(3.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(3.0);

    if (track.p() < minTrackP_) {
      continue;
    }
    trackCutflow_->Fill(4.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(4.0);

    if (std::abs(track.eta()) > maxTrackAbsEta_) {
      continue;
    }
    trackCutflow_->Fill(5.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(5.0);

    if (track.normalizedChi2() > maxTrackNormalizedChi2_) {
      continue;
    }
    trackCutflow_->Fill(6.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(6.0);

    double const relativePError = track.qoverpError() / std::abs(track.qoverp());
    if (relativePError > maxTrackRelativePError_) {
      continue;
    }
    trackCutflow_->Fill(7.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(7.0);

    unsigned int const trackerLayers = track.hitPattern().trackerLayersWithMeasurement();
    if (trackerLayers < minTrackerLayers_) {
      continue;
    }
    trackCutflow_->Fill(8.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(8.0);

    unsigned int const pixelLayers = track.hitPattern().pixelLayersWithMeasurement();
    if (pixelLayers < minPixelLayers_) {
      continue;
    }
    trackCutflow_->Fill(9.0);
    if (isV0Daughter) {
      v0TrackSelectionCutflow_->Fill(9.0);
      v0WithoutPVCutsCutflow_->Fill(1.0);
      v0WithoutPVCutsCutflow_->Fill(2.0);
      v0WithoutPVCutsCutflow_->Fill(3.0);

      auto const [v0SamePVIsolation, v0AllVertexIsolation] =
          chargedIsolation(trackIndex, tracks, leadingVertex);
      if (v0AllVertexIsolation / track.pt() < maxRelativeIsolation_)
        v0WithoutPVCutsCutflow_->Fill(4.0);
    }

    double dxy = 0.0;
    if (leadingVertex != nullptr) {
      dxy = track.dxy(leadingVertex->position());
      if (std::abs(dxy) > maxTrackAbsDxy_) {
        continue;
      }
    }
    trackCutflow_->Fill(10.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(10.0);

    double dz = 0.0;
    if (leadingVertex != nullptr) {
      dz = track.dz(leadingVertex->position());
      if (std::abs(dz) > maxTrackAbsDz_) {
        continue;
      }
    }
    trackCutflow_->Fill(11.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(11.0);

    auto const [samePVIsolation, allVertexIsolation] = chargedIsolation(trackIndex, tracks, leadingVertex);
    double const samePVRelativeIsolation = samePVIsolation / track.pt();
    double const allVertexRelativeIsolation = allVertexIsolation / track.pt();
    if (allVertexRelativeIsolation >= maxRelativeIsolation_) {
      continue;
    }
    trackCutflow_->Fill(12.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(12.0);
    trackCutflow_->Fill(13.0);
    if (isV0Daughter)
      v0TrackSelectionCutflow_->Fill(13.0);

    ++numberOfSelectedTracks;
    selectedTrackP_->Fill(track.p());
    selectedTrackPVarBins_->Fill(track.p());
    selectedTrackPt_->Fill(track.pt());
    selectedTrackPtVarBins_->Fill(track.pt());
    selectedTrackEta_->Fill(track.eta());
    selectedTrackPhi_->Fill(track.phi());
    selectedTrackNormalizedChi2_->Fill(track.normalizedChi2());
    selectedTrackRelativePError_->Fill(relativePError);
    selectedTrackTrackerLayers_->Fill(trackerLayers);
    selectedTrackPixelLayers_->Fill(pixelLayers);
    if (leadingVertex != nullptr) {
      selectedTrackDxy_->Fill(dxy);
      selectedTrackDz_->Fill(dz);
      selectedTrackSamePVRelativeIsolation_->Fill(samePVRelativeIsolation);
    }
    selectedTrackAllVertexRelativeIsolation_->Fill(allVertexRelativeIsolation);

    reco::TrackRef const trackRef(tracksHandle, trackIndex);

    if (doV0Tagging_ && matchedV0Tracks.count(trackRef))
      v0DaughterCutflow_->Fill(3.0);

    MuonMatchResult const muonMatch = matchMuon(trackRef, muons, leadingVertex);
    muonMatchMultiplicity_->Fill(muonMatch.multiplicity);
    muonIDSummary_->Fill(1.0);
    if (muonMatch.hasExactMatch()) {
      muonIDSummary_->Fill(2.0);
      exactMuonMatchedTrackP_->Fill(track.p());
      exactMuonMatchedTrackPVarBins_->Fill(track.p());
    }
    if (muonMatch.passesSoft) {
      muonIDSummary_->Fill(3.0);
    }
    if (muonMatch.passesLoose) {
      muonIDSummary_->Fill(4.0);
    }
    if (muonMatch.isMuonTagged()) {
      muonIDSummary_->Fill(5.0);
      muonTaggedTrackP_->Fill(track.p());
      muonTaggedTrackPVarBins_->Fill(track.p());
    } else {
      nonMuonTrackP_->Fill(track.p());
      nonMuonTrackPVarBins_->Fill(track.p());
    }
    if (muonMatch.passesMedium) {
      muonIDSummary_->Fill(6.0);
    }
    if (muonMatch.passesTight) {
      muonIDSummary_->Fill(7.0);
    }

    if (!muonMatch.hasExactMatch()) {
      selectedTrackMuonCategory_->Fill(1.0);
    } else if (!muonMatch.isMuonTagged()) {
      selectedTrackMuonCategory_->Fill(2.0);
    } else if (muonMatch.passesSoft && !muonMatch.passesLoose) {
      selectedTrackMuonCategory_->Fill(3.0);
    } else if (!muonMatch.passesSoft && muonMatch.passesLoose) {
      selectedTrackMuonCategory_->Fill(4.0);
    } else {
      selectedTrackMuonCategory_->Fill(5.0);
    }

    exactMuonMatchEfficiencyVsP_->Fill(track.p(), muonMatch.hasExactMatch());
    exactMuonMatchEfficiencyVsPVarBins_->Fill(track.p(), muonMatch.hasExactMatch());
    exactMuonMatchEfficiencyVsEta_->Fill(track.eta(), muonMatch.hasExactMatch());
    muonTagEfficiencyVsP_->Fill(track.p(), muonMatch.isMuonTagged());
    muonTagEfficiencyVsPVarBins_->Fill(track.p(), muonMatch.isMuonTagged());
    muonTagEfficiencyVsEta_->Fill(track.eta(), muonMatch.isMuonTagged());

    unsigned int pfCandidateMatchMultiplicity = 0;
    reco::PFCandidate const* matchedPFCandidate = nullptr;

    for (auto const& pfCandidate : pfCandidates) {
      reco::TrackRef const candidateTrackRef = pfCandidate.trackRef();

      if (candidateTrackRef.isNonnull() && candidateTrackRef == trackRef) {
        ++pfCandidateMatchMultiplicity;

        if (matchedPFCandidate == nullptr) {
          matchedPFCandidate = &pfCandidate;
        }
      }
    }

    if (pfCandidateMatchMultiplicity == 1 &&
        matchedPFCandidate != nullptr &&
        matchedPFCandidate->particleId() == reco::PFCandidate::h) {

      unsigned int neutralLinkedHCALMultiplicity = 0;
      double neutralLinkedHCALEnergy = 0.0;
      unsigned int neutralSameHCALMultiplicity = 0;
      double neutralSameHCALEnergy = 0.0;
      unsigned int neutralMultiplicity = 0;
      double neutralEnergy = 0.0;

      auto const& chargedElements =
          matchedPFCandidate->elementsInBlocks();

      for (auto const& candidate : pfCandidates) {
        if (candidate.particleId() != reco::PFCandidate::h0) {
          continue;
        }

        bool sameBlock = false;

        for (auto const& chargedElement : chargedElements) {
          if (chargedElement.first.isNull()) {
            continue;
          }

          for (auto const& neutralElement :
               candidate.elementsInBlocks()) {

            if (neutralElement.first.isNonnull() &&
                neutralElement.first == chargedElement.first) {
              sameBlock = true;
              break;
            }
          }

          if (sameBlock) break;
        }

        bool sameHCALElement = false;

        for (auto const& chargedElement : chargedElements) {
          if (chargedElement.first.isNull()) continue;

          auto const& block = *chargedElement.first;
          auto const& elements = block.elements();

          if (chargedElement.second >= elements.size() ||
              elements[chargedElement.second].type() !=
                  reco::PFBlockElement::HCAL) {
            continue;
          }

          for (auto const& neutralElement :
               candidate.elementsInBlocks()) {
            if (neutralElement.first == chargedElement.first &&
                neutralElement.second == chargedElement.second) {
              sameHCALElement = true;
              break;
            }
          }

          if (sameHCALElement) break;
        }

        bool linkedHCAL = false;

        for (auto const& chargedElement : chargedElements) {
          if (chargedElement.first.isNull()) continue;

          auto const& block = *chargedElement.first;
          auto const& elements = block.elements();

          if (chargedElement.second >= elements.size()) continue;

          for (auto const& neutralElement :
               candidate.elementsInBlocks()) {

            if (neutralElement.first != chargedElement.first ||
                neutralElement.second >= elements.size()) {
              continue;
            }

            unsigned int const chargedIndex = chargedElement.second;
            unsigned int const neutralIndex = neutralElement.second;

            bool const chargedIsHCAL =
                elements[chargedIndex].type() == reco::PFBlockElement::HCAL;

            bool const neutralIsHCAL =
                elements[neutralIndex].type() == reco::PFBlockElement::HCAL;

            if (!chargedIsHCAL && !neutralIsHCAL) continue;

            if (chargedIndex == neutralIndex) {
              linkedHCAL = true;
            } else {
              double const distance =
                  block.dist(chargedIndex, neutralIndex, block.linkData());

              if (distance >= 0.0) {
                linkedHCAL = true;
              }
            }

            if (linkedHCAL) break;
          }

          if (linkedHCAL) break;
        }

        if (linkedHCAL) {
          ++neutralLinkedHCALMultiplicity;
          neutralLinkedHCALEnergy += candidate.energy();
        }

        if (sameHCALElement) {
          ++neutralSameHCALMultiplicity;
          neutralSameHCALEnergy += candidate.energy();
        }

        if (sameBlock) {
          ++neutralMultiplicity;
          neutralEnergy += candidate.energy();
        }
      }

      double const linkedHCALTotalEnergy =
          matchedPFCandidate->energy() + neutralLinkedHCALEnergy;

      neutralHadronMultiplicityLinkedHCAL_->Fill(
          neutralLinkedHCALMultiplicity);

      if (linkedHCALTotalEnergy > 0.0) {
        neutralHadronEnergyFractionLinkedHCAL_->Fill(
            neutralLinkedHCALEnergy / linkedHCALTotalEnergy);
      }

      double const sameHCALTotalEnergy =
          matchedPFCandidate->energy() + neutralSameHCALEnergy;

      neutralHadronMultiplicitySameHCAL_->Fill(
          neutralSameHCALMultiplicity);

      if (sameHCALTotalEnergy > 0.0) {
        neutralHadronEnergyFractionSameHCAL_->Fill(
            neutralSameHCALEnergy / sameHCALTotalEnergy);
      }

      double const totalEnergy =
          matchedPFCandidate->energy() + neutralEnergy;

      double const neutralFraction =
          totalEnergy > 0.0 ? neutralEnergy / totalEnergy : 0.0;

      neutralHadronMultiplicitySameBlock_->Fill(
          neutralMultiplicity);

      neutralSameBlockFractionVsP_->Fill(
          track.p(), neutralMultiplicity > 0);
      neutralSameHCALFractionVsP_->Fill(
          track.p(), neutralSameHCALMultiplicity > 0);
      neutralLinkedHCALFractionVsP_->Fill(
          track.p(), neutralLinkedHCALMultiplicity > 0);

      if (totalEnergy > 0.0) {
        neutralHadronEnergyFractionSameBlock_->Fill(
            neutralFraction);

        meanNeutralHadronFractionVsP_->Fill(
            track.p(), neutralFraction);
      }

      meanNeutralHadronMultiplicityVsP_->Fill(
          track.p(), neutralMultiplicity);
    }

    pfCandidateMatchMultiplicity_->Fill(pfCandidateMatchMultiplicity);
    pfCandidateMatchEfficiencyVsPVarBins_->Fill(
        track.p(), pfCandidateMatchMultiplicity == 1);

    if (pfCandidateMatchMultiplicity == 1 && matchedPFCandidate != nullptr) {
      pfCandidateParticleId_->Fill(matchedPFCandidate->particleId());

      double const pfCandidateECALEnergy =
          matchedPFCandidate->ecalEnergy();
      double const pfCandidateHCALEnergy =
          matchedPFCandidate->hcalEnergy();

      double const pfCandidateECALPlusHCALEnergy =
          pfCandidateECALEnergy + pfCandidateHCALEnergy;

      double const pfCandidateECALResponse =
          pfCandidateECALEnergy / track.p();
      double const pfCandidateHCALResponse =
          pfCandidateHCALEnergy / track.p();
      double const pfCandidateECALPlusHCALResponse =
          pfCandidateECALPlusHCALEnergy / track.p();

      pfCandidateResponseVsParticleId_->Fill(
          matchedPFCandidate->particleId(), pfCandidateECALPlusHCALResponse);

      pfCandidateECALPlusHCALEnergy_->Fill(
          pfCandidateECALPlusHCALEnergy);

      pfCandidateECALPlusHCALResponse_->Fill(
          pfCandidateECALPlusHCALResponse);

      pfCandidateECALPlusHCALResponseVsPVarBins_->Fill(
          track.p(), pfCandidateECALPlusHCALResponse);

      meanPFCandidateECALPlusHCALResponseVsPVarBins_->Fill(
          track.p(), pfCandidateECALPlusHCALResponse);
      meanPFCandidateECALResponseVsPVarBins_->Fill(
          track.p(), pfCandidateECALResponse);
      meanPFCandidateHCALResponseVsPVarBins_->Fill(
          track.p(), pfCandidateHCALResponse);
    }

    fillPFLinkingDiagnostics(
        trackRef,
        track,
        pfBlocks,
        numberOfGoodVertices,
        muonMatch,
        pfCandidateMatchMultiplicity == 1 ? matchedPFCandidate : nullptr);
  }

  numberOfSelectedTracks_->Fill(numberOfSelectedTracks);
  if (requirePrimaryVertex_) {
    meanSelectedTracksVsGoodVertices_->Fill(numberOfGoodVertices, numberOfSelectedTracks);
  }
  if (numberOfSelectedTracks > 0) {
    eventCutflow_->Fill(4.0);
  }
}

bool SPRLowPUDataAnalyzer::isGoodVertex(reco::Vertex const& vertex) const {
  return !vertex.isFake() && vertex.ndof() > minVertexNdof_ && std::abs(vertex.z()) < maxVertexAbsZ_ &&
         vertex.position().Rho() < maxVertexRho_;
}

bool SPRLowPUDataAnalyzer::isIsolationNeighbor(reco::Track const& track) const {
  return track.quality(trackQuality_) && track.pt() > minIsolationTrackPt_ &&
         std::abs(track.eta()) < maxIsolationTrackAbsEta_;
}

std::pair<double, double> SPRLowPUDataAnalyzer::chargedIsolation(std::size_t candidateIndex,
                                                                 reco::TrackCollection const& tracks,
                                                                 reco::Vertex const* leadingVertex) const {
  double samePVIsolation = 0.0;
  double allVertexIsolation = 0.0;
  double const isolationCone2 = isolationCone_ * isolationCone_;
  reco::Track const& candidate = tracks[candidateIndex];

  for (std::size_t neighborIndex = 0; neighborIndex < tracks.size(); ++neighborIndex) {
    if (neighborIndex == candidateIndex) {
      continue;
    }

    reco::Track const& neighbor = tracks[neighborIndex];
    if (!isIsolationNeighbor(neighbor)) {
      continue;
    }
    if (reco::deltaR2(candidate.eta(), candidate.phi(), neighbor.eta(), neighbor.phi()) >= isolationCone2) {
      continue;
    }

    allVertexIsolation += neighbor.pt();
    if (leadingVertex != nullptr && std::abs(neighbor.dxy(leadingVertex->position())) < maxTrackAbsDxy_ &&
        std::abs(neighbor.dz(leadingVertex->position())) < maxTrackAbsDz_) {
      samePVIsolation += neighbor.pt();
    }
  }

  return {samePVIsolation, allVertexIsolation};
}

SPRLowPUDataAnalyzer::MuonMatchResult SPRLowPUDataAnalyzer::matchMuon(reco::TrackRef const& trackRef,
                                                                      reco::MuonCollection const& muons,
                                                                      reco::Vertex const* leadingVertex) const {
  MuonMatchResult result;
  for (auto const& muonObject : muons) {
    reco::TrackRef const& innerTrack = muonObject.innerTrack();
    if (innerTrack.isNull() || !innerTrack.isAvailable() || innerTrack != trackRef) {
      continue;
    }

    ++result.multiplicity;
    if (leadingVertex != nullptr) {
      result.passesSoft = result.passesSoft || muon::isSoftMuon(muonObject, *leadingVertex);
      result.passesTight = result.passesTight || muon::isTightMuon(muonObject, *leadingVertex);
    }
    result.passesLoose = result.passesLoose || muon::isLooseMuon(muonObject);
    result.passesMedium = result.passesMedium || muon::isMediumMuon(muonObject);
  }
  return result;
}

void SPRLowPUDataAnalyzer::fillPFLinkingDiagnostics(reco::TrackRef const& trackRef,
                                                    reco::Track const& track,
                                                    reco::PFBlockCollection const& pfBlocks,
                                                    unsigned int numberOfGoodVertices,
                                                    MuonMatchResult const& muonMatch,
                                                    reco::PFCandidate const* matchedPFCandidate) {
  struct PFBlockMatch {
    reco::PFBlock const* block;
    unsigned int trackElementIndex;
  };


  //
  // Check how many PFBlocks does this track matched to
  //
  std::vector<PFBlockMatch> matches;
  for (auto const& block : pfBlocks) {
    auto const& elements = block.elements();
    for (unsigned int elementIndex = 0; elementIndex < elements.size(); ++elementIndex) {
      reco::PFBlockElement const& element = elements[elementIndex];
      if (element.type() == reco::PFBlockElement::TRACK && element.trackRef() == trackRef) {
        matches.push_back({&block, elementIndex});
        break;
      }
    }
  }

  pfLinkingCutflow_->Fill(1.0);
  pfBlockMatchMultiplicity_->Fill(matches.size());

  bool const hasPFBlockMatch = !matches.empty();
  bool const hasUniquePFBlockMatch = matches.size() == 1;
  if (hasPFBlockMatch) {
    pfLinkingCutflow_->Fill(2.0);
  } else {
    noPFBlockMatchTrackP_->Fill(track.p());
    noPFBlockMatchTrackPVarBins_->Fill(track.p());
  }


  if (hasUniquePFBlockMatch) {
    pfLinkingCutflow_->Fill(3.0);
  } else if (matches.size() > 1) {
    multiplePFBlockMatchesTrackP_->Fill(track.p());
    multiplePFBlockMatchesTrackPVarBins_->Fill(track.p());
  }

  bool hasLinkedECAL = false;
  bool hasLinkedHCAL = false;
  if (hasUniquePFBlockMatch) {
    reco::PFBlock const& block = *matches.front().block;
    unsigned int const trackElementIndex = matches.front().trackElementIndex;
    auto const& elements = block.elements();

    unsigned int numberOfTrackElements = 0;
    unsigned int numberOfECALElements = 0;
    unsigned int numberOfHCALElements = 0;
    unsigned int numberOfHOElements = 0;
    unsigned int numberOfOtherElements = 0;
    for (auto const& element : elements) {
      switch (element.type()) {
        case reco::PFBlockElement::TRACK:
          ++numberOfTrackElements;
          break;
        case reco::PFBlockElement::ECAL:
          ++numberOfECALElements;
          break;
        case reco::PFBlockElement::HCAL:
          ++numberOfHCALElements;
          break;
        case reco::PFBlockElement::HO:
          ++numberOfHOElements;
          break;
        default:
          ++numberOfOtherElements;
          break;
      }
    }

    matchedPFBlockNumberOfElements_->Fill(elements.size());
    matchedPFBlockNumberOfTrackElements_->Fill(numberOfTrackElements);
    matchedPFBlockNumberOfECALElements_->Fill(numberOfECALElements);
    matchedPFBlockNumberOfHCALElements_->Fill(numberOfHCALElements);
    matchedPFBlockNumberOfHOElements_->Fill(numberOfHOElements);
    matchedPFBlockNumberOfOtherElements_->Fill(numberOfOtherElements);

    struct ClosestLinkedCluster {
      reco::PFClusterRef cluster;
      double distance = -1.0;
      unsigned int validLinkMultiplicity = 0;
    };

    // PFBlock::associatedElements() sorts the linked elements by increasing
    // stored link distance. Select the first valid PFClusterRef of each type,
    // while retaining the full valid-link multiplicity as a diagnostic.
    auto const closestLinkedCluster = [&](reco::PFBlockElement::Type clusterType) {
      ClosestLinkedCluster result;
      std::multimap<double, unsigned int> linkedClusters;
      block.associatedElements(
          trackElementIndex, block.linkData(), linkedClusters, clusterType, reco::PFBlock::LINKTEST_ALL);

      for (auto const& [distance, elementIndex] : linkedClusters) {
        reco::PFClusterRef const& clusterRef = elements[elementIndex].clusterRef();
        if (clusterRef.isNull() || !clusterRef.isAvailable()) {
          continue;
        }

        ++result.validLinkMultiplicity;
        if (result.cluster.isNull()) {
          result.cluster = clusterRef;
          result.distance = distance;
        }
      }

      return result;
    };

    ClosestLinkedCluster const closestLinkedECAL = closestLinkedCluster(reco::PFBlockElement::ECAL);
    ClosestLinkedCluster const closestLinkedHCAL = closestLinkedCluster(reco::PFBlockElement::HCAL);
    ClosestLinkedCluster const closestLinkedHO = closestLinkedCluster(reco::PFBlockElement::HO);

    bool sharedClosestHCAL = false;

    if (closestLinkedHCAL.cluster.isNonnull() &&
        numberOfTrackElements >= 2) {
      for (unsigned int otherIndex = 0;
           otherIndex < elements.size(); ++otherIndex) {
        if (otherIndex == trackElementIndex ||
            elements[otherIndex].type() !=
                reco::PFBlockElement::TRACK) {
          continue;
        }

        std::multimap<double, unsigned int> otherHCALLinks;
        block.associatedElements(
            otherIndex, block.linkData(), otherHCALLinks,
            reco::PFBlockElement::HCAL,
            reco::PFBlock::LINKTEST_ALL);

        for (auto const& link : otherHCALLinks) {
          auto const& otherCluster =
              elements[link.second].clusterRef();

          if (otherCluster.isNonnull() &&
              otherCluster.isAvailable() &&
              otherCluster == closestLinkedHCAL.cluster) {
            sharedClosestHCAL = true;
            break;
          }
        }

        if (sharedClosestHCAL) break;
      }
    }

    linkedECALClusterMultiplicity_->Fill(closestLinkedECAL.validLinkMultiplicity);
    linkedHCALClusterMultiplicity_->Fill(closestLinkedHCAL.validLinkMultiplicity);
    linkedHOClusterMultiplicity_->Fill(closestLinkedHO.validLinkMultiplicity);

    hasLinkedECAL = closestLinkedECAL.cluster.isNonnull();
    hasLinkedHCAL = closestLinkedHCAL.cluster.isNonnull();
    bool const hasLinkedHO = closestLinkedHO.cluster.isNonnull();

    if (hasLinkedECAL) {
      trackECALLinkDistance_->Fill(closestLinkedECAL.distance);
    }
    if (hasLinkedHCAL) {
      trackHCALLinkDistance_->Fill(closestLinkedHCAL.distance);
    }
    if (hasLinkedHO) {
      trackHOLinkDistance_->Fill(closestLinkedHO.distance);
    }

    double const linkedECALEnergy = hasLinkedECAL ? closestLinkedECAL.cluster->energy() : 0.0;
    double const linkedHCALEnergy = hasLinkedHCAL ? closestLinkedHCAL.cluster->energy() : 0.0;
    double const linkedHOEnergy = hasLinkedHO ? closestLinkedHO.cluster->energy() : 0.0;

    double const linkedECALPlusHCALEnergy = linkedECALEnergy + linkedHCALEnergy;

    if (matchedPFCandidate != nullptr && linkedECALPlusHCALEnergy > 0.0) {
      double const pfCandidateECALEnergy = matchedPFCandidate->ecalEnergy();
      double const pfCandidateHCALEnergy = matchedPFCandidate->hcalEnergy();
      double const pfCandidateECALPlusHCALEnergy =
          pfCandidateECALEnergy + pfCandidateHCALEnergy;

      pfCandidateECALVsLinkedECAL_->Fill(
          linkedECALEnergy, pfCandidateECALEnergy);
      pfCandidateHCALVsLinkedHCAL_->Fill(
          linkedHCALEnergy, pfCandidateHCALEnergy);

      rawPFCandidateECALVsLinkedECAL_->Fill(
          linkedECALEnergy, matchedPFCandidate->rawEcalEnergy());
      rawPFCandidateHCALVsLinkedHCAL_->Fill(
          linkedHCALEnergy, matchedPFCandidate->rawHcalEnergy());

      double const pfCandidateToLinkedEnergyRatio =
          pfCandidateECALPlusHCALEnergy / linkedECALPlusHCALEnergy;

      pfCandidateToLinkedEnergyRatio_->Fill(pfCandidateToLinkedEnergyRatio);
      meanPFCandidateToLinkedEnergyRatioVsPVarBins_->Fill(
          track.p(), pfCandidateToLinkedEnergyRatio);

      double const sameTrackPFCandidateResponse =
          pfCandidateECALPlusHCALEnergy / track.p();
      double const sameTrackLinkedResponse =
          linkedECALPlusHCALEnergy / track.p();

      meanSameTrackPFCandidateResponseVsPVarBins_->Fill(
          track.p(), sameTrackPFCandidateResponse);
      meanSameTrackLinkedResponseVsPVarBins_->Fill(
          track.p(), sameTrackLinkedResponse);
    }

    double const linkedECALResponse = linkedECALEnergy / track.p();
    double const linkedHCALResponse = linkedHCALEnergy / track.p();
    double const linkedECALPlusHCALResponse = linkedECALPlusHCALEnergy / track.p();
    double const linkedECALPlusHCALPlusHOResponse = (linkedECALPlusHCALEnergy + linkedHOEnergy) / track.p();

    linkedECALEnergy_->Fill(linkedECALEnergy);
    linkedHCALEnergy_->Fill(linkedHCALEnergy);
    linkedHOEnergy_->Fill(linkedHOEnergy);
    linkedECALPlusHCALEnergy_->Fill(linkedECALPlusHCALEnergy);
    linkedECALResponse_->Fill(linkedECALResponse);
    linkedHCALResponse_->Fill(linkedHCALResponse);
    linkedECALPlusHCALResponse_->Fill(linkedECALPlusHCALResponse);
    linkedECALPlusHCALPlusHOResponse_->Fill(linkedECALPlusHCALPlusHOResponse);
    if (linkedECALPlusHCALEnergy > 0.0) {
      linkedECALEnergyFraction_->Fill(linkedECALEnergy / linkedECALPlusHCALEnergy);
    }

    linkedECALEnergyVsLinkedHCALEnergy_->Fill(linkedECALEnergy, linkedHCALEnergy);
    linkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
    linkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
    linkedECALPlusHCALResponseVsEta_->Fill(track.eta(), linkedECALPlusHCALResponse);
    linkedECALPlusHCALResponseVsPFBlockTracks_->Fill(numberOfTrackElements, linkedECALPlusHCALResponse);
    meanLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
    meanLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
    meanLinkedECALPlusHCALResponseVsEta_->Fill(track.eta(), linkedECALPlusHCALResponse);
    meanLinkedECALPlusHCALResponseVsPFBlockTracks_->Fill(numberOfTrackElements, linkedECALPlusHCALResponse);
    if (requirePrimaryVertex_) {
      linkedECALPlusHCALResponseVsGoodVertices_->Fill(numberOfGoodVertices, linkedECALPlusHCALResponse);
      meanLinkedECALPlusHCALResponseVsGoodVertices_->Fill(numberOfGoodVertices, linkedECALPlusHCALResponse);
    }

    // Apply the PF hadron calibration only when a closest linked HCAL cluster
    // is available, using the HCAL cluster position as in PFAlgo.
    if (hasLinkedHCAL) {
      bool const hasMatchedPFCandidate = (matchedPFCandidate != nullptr);
      bool const hasMatchedChargedHadron =
          hasMatchedPFCandidate &&
          matchedPFCandidate->particleId() == reco::PFCandidate::h;

      hcalLinkedPFCandidateMatchEfficiencyVsPVarBins_->Fill(
          track.p(), hasMatchedPFCandidate);
      hcalLinkedChargedHadronEfficiencyVsPVarBins_->Fill(
          track.p(), hasMatchedChargedHadron);

      if (matchedPFCandidate != nullptr) {
        double const hcalLinkedPFCandidateResponse =
            (matchedPFCandidate->ecalEnergy() + matchedPFCandidate->hcalEnergy()) /
            track.p();

        double const hcalLinkedRawPFCandidateResponse =
            (matchedPFCandidate->rawEcalEnergy() + matchedPFCandidate->rawHcalEnergy()) /
            track.p();

        meanHCALLinkedPFCandidateResponseVsPVarBins_->Fill(
            track.p(), hcalLinkedPFCandidateResponse);

        hcalLinkedPFCandidateResponseVsPVarBins_->Fill(
            track.p(), hcalLinkedPFCandidateResponse);

        meanHCALLinkedRawPFCandidateResponseVsPVarBins_->Fill(
            track.p(), hcalLinkedRawPFCandidateResponse);

        hcalLinkedRawPFCandidateResponseVsPVarBins_->Fill(
            track.p(), hcalLinkedRawPFCandidateResponse);
      }

      prePFHadronCalibrationLinkedECALEnergy_->Fill(linkedECALEnergy);
      prePFHadronCalibrationLinkedHCALEnergy_->Fill(linkedHCALEnergy);
      prePFHadronCalibrationLinkedECALPlusHCALEnergy_->Fill(linkedECALPlusHCALEnergy);
      prePFHadronCalibrationLinkedECALResponse_->Fill(linkedECALResponse);
      prePFHadronCalibrationLinkedHCALResponse_->Fill(linkedHCALResponse);
      prePFHadronCalibrationLinkedECALPlusHCALResponse_->Fill(linkedECALPlusHCALResponse);
      prePFHadronCalibrationLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
      prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
      prePFHadronCalibrationLinkedECALPlusHCALResponseVsEta_->Fill(track.eta(), linkedECALPlusHCALResponse);
      prePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks_->Fill(numberOfTrackElements,
                                                                             linkedECALPlusHCALResponse);
      meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
      meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
      meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsEta_->Fill(track.eta(), linkedECALPlusHCALResponse);
      meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPFBlockTracks_->Fill(numberOfTrackElements,
                                                                                 linkedECALPlusHCALResponse);
      if (requirePrimaryVertex_) {
        prePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices_->Fill(numberOfGoodVertices,
                                                                              linkedECALPlusHCALResponse);
        meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsGoodVertices_->Fill(numberOfGoodVertices,
                                                                                  linkedECALPlusHCALResponse);
      }

      double pfHadronCalibratedLinkedECALEnergy = linkedECALEnergy;
      double pfHadronCalibratedLinkedHCALEnergy = linkedHCALEnergy;
      auto const& hcalPosition = closestLinkedHCAL.cluster->positionREP();
      pfEnergyCalibration_.energyEmHad(track.p(),
                                       pfHadronCalibratedLinkedECALEnergy,
                                       pfHadronCalibratedLinkedHCALEnergy,
                                       hcalPosition.Eta(),
                                       hcalPosition.Phi());

      double const pfHadronCalibratedLinkedECALPlusHCALEnergy =
          pfHadronCalibratedLinkedECALEnergy + pfHadronCalibratedLinkedHCALEnergy;
      double const pfHadronCalibratedLinkedECALResponse = pfHadronCalibratedLinkedECALEnergy / track.p();
      double const pfHadronCalibratedLinkedHCALResponse = pfHadronCalibratedLinkedHCALEnergy / track.p();
      double const pfHadronCalibratedLinkedECALPlusHCALResponse =
          pfHadronCalibratedLinkedECALPlusHCALEnergy / track.p();

      pfHadronCalibratedLinkedECALEnergy_->Fill(pfHadronCalibratedLinkedECALEnergy);
      pfHadronCalibratedLinkedHCALEnergy_->Fill(pfHadronCalibratedLinkedHCALEnergy);
      pfHadronCalibratedLinkedECALPlusHCALEnergy_->Fill(pfHadronCalibratedLinkedECALPlusHCALEnergy);
      pfHadronCalibratedLinkedECALResponse_->Fill(pfHadronCalibratedLinkedECALResponse);
      pfHadronCalibratedLinkedHCALResponse_->Fill(pfHadronCalibratedLinkedHCALResponse);
      pfHadronCalibratedLinkedECALPlusHCALResponse_->Fill(pfHadronCalibratedLinkedECALPlusHCALResponse);
      pfHadronCalibrationResponseChange_->Fill(pfHadronCalibratedLinkedECALPlusHCALResponse -
                                                linkedECALPlusHCALResponse);
      if (linkedECALPlusHCALEnergy > 0.0) {
        pfHadronCalibrationFactor_->Fill(pfHadronCalibratedLinkedECALPlusHCALEnergy /
                                         linkedECALPlusHCALEnergy);
      }

      pfHadronCalibratedLinkedECALPlusHCALResponseVsP_->Fill(track.p(),
                                                              pfHadronCalibratedLinkedECALPlusHCALResponse);
      pfHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins_->Fill(
          track.p(), pfHadronCalibratedLinkedECALPlusHCALResponse);
      pfHadronCalibratedLinkedECALPlusHCALResponseVsEta_->Fill(
          track.eta(), pfHadronCalibratedLinkedECALPlusHCALResponse);
      pfHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks_->Fill(
          numberOfTrackElements, pfHadronCalibratedLinkedECALPlusHCALResponse);
      rawVsPFHadronCalibratedLinkedECALPlusHCALResponse_->Fill(
          linkedECALPlusHCALResponse, pfHadronCalibratedLinkedECALPlusHCALResponse);
      meanPFHadronCalibratedLinkedECALPlusHCALResponseVsP_->Fill(
          track.p(), pfHadronCalibratedLinkedECALPlusHCALResponse);
      meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins_->Fill(
          track.p(), pfHadronCalibratedLinkedECALPlusHCALResponse);
      meanPFHadronCalibratedLinkedECALPlusHCALResponseVsEta_->Fill(
          track.eta(), pfHadronCalibratedLinkedECALPlusHCALResponse);
      meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPFBlockTracks_->Fill(
          numberOfTrackElements, pfHadronCalibratedLinkedECALPlusHCALResponse);
      // Compare PF-candidate and separately calibrated linked-cluster
      // responses on the same HCAL-linked, PF-matched tracks.
      if (matchedPFCandidate != nullptr) {
        bool const simplePFBlock =
            numberOfTrackElements == 1 &&
            numberOfHCALElements == 1 &&
            numberOfECALElements <= 1;

        double const pfResponse =
            (matchedPFCandidate->ecalEnergy() +
             matchedPFCandidate->hcalEnergy()) / track.p();

        double const deltaResponse =
            pfResponse - pfHadronCalibratedLinkedECALPlusHCALResponse;

        if (numberOfTrackElements >= 2) {
          if (sharedClosestHCAL) {
            meanDeltaResponseSharedHCALVsP_->Fill(
                track.p(), deltaResponse);
          } else {
            meanDeltaResponseNonSharedHCALVsP_->Fill(
                track.p(), deltaResponse);
          }

          meanDeltaResponseMultiTrackBlockVsP_->Fill(
              track.p(), deltaResponse);
        } else if (numberOfHCALElements >= 2) {
          meanDeltaResponseMultiHCALBlockVsP_->Fill(
              track.p(), deltaResponse);
        } else if (numberOfECALElements >= 2) {
          meanDeltaResponseMultiECALBlockVsP_->Fill(
              track.p(), deltaResponse);
        }

        if (simplePFBlock) {
          meanPFResponseSimpleBlockVsP_->Fill(track.p(), pfResponse);
          meanDeltaResponseSimpleBlockVsP_->Fill(track.p(), deltaResponse);
          meanLinkedResponseSimpleBlockVsP_->Fill(
              track.p(), pfHadronCalibratedLinkedECALPlusHCALResponse);
        } else {
          meanPFResponseComplexBlockVsP_->Fill(track.p(), pfResponse);
          meanDeltaResponseComplexBlockVsP_->Fill(track.p(), deltaResponse);
          meanLinkedResponseComplexBlockVsP_->Fill(
              track.p(), pfHadronCalibratedLinkedECALPlusHCALResponse);
        }
      }

      if (requirePrimaryVertex_) {
        pfHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices_->Fill(
            numberOfGoodVertices, pfHadronCalibratedLinkedECALPlusHCALResponse);
        meanPFHadronCalibratedLinkedECALPlusHCALResponseVsGoodVertices_->Fill(
            numberOfGoodVertices, pfHadronCalibratedLinkedECALPlusHCALResponse);
      }
    }

    if (muonMatch.isMuonTagged()) {
      muonTaggedLinkedECALPlusHCALResponse_->Fill(linkedECALPlusHCALResponse);
      muonTaggedLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
      muonTaggedLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
      meanMuonTaggedLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
      meanMuonTaggedLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
    } else {
      nonMuonLinkedECALPlusHCALResponse_->Fill(linkedECALPlusHCALResponse);
      nonMuonLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
      nonMuonLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
      meanNonMuonLinkedECALPlusHCALResponseVsP_->Fill(track.p(), linkedECALPlusHCALResponse);
      meanNonMuonLinkedECALPlusHCALResponseVsPVarBins_->Fill(track.p(), linkedECALPlusHCALResponse);
    }

    if (hasLinkedECAL) {
      pfLinkingCutflow_->Fill(4.0);
    } else {
      noLinkedECALTrackP_->Fill(track.p());
      noLinkedECALTrackPVarBins_->Fill(track.p());
    }
    if (hasLinkedHCAL) {
      pfLinkingCutflow_->Fill(5.0);
    } else {
      noLinkedHCALTrackP_->Fill(track.p());
      noLinkedHCALTrackPVarBins_->Fill(track.p());
    }
    if (hasLinkedECAL || hasLinkedHCAL) {
      pfLinkingCutflow_->Fill(6.0);
    }
    if (hasLinkedECAL && hasLinkedHCAL) {
      pfLinkingCutflow_->Fill(7.0);
    }
  }

  pfBlockMatchEfficiencyVsP_->Fill(track.p(), hasPFBlockMatch);
  pfBlockMatchEfficiencyVsPVarBins_->Fill(track.p(), hasPFBlockMatch);
  pfBlockMatchEfficiencyVsEta_->Fill(track.eta(), hasPFBlockMatch);
  uniquePFBlockMatchEfficiencyVsP_->Fill(track.p(), hasUniquePFBlockMatch);
  uniquePFBlockMatchEfficiencyVsPVarBins_->Fill(track.p(), hasUniquePFBlockMatch);
  uniquePFBlockMatchEfficiencyVsEta_->Fill(track.eta(), hasUniquePFBlockMatch);
  linkedECALEfficiencyVsP_->Fill(track.p(), hasLinkedECAL);
  linkedECALEfficiencyVsPVarBins_->Fill(track.p(), hasLinkedECAL);
  linkedECALEfficiencyVsEta_->Fill(track.eta(), hasLinkedECAL);
  linkedHCALEfficiencyVsP_->Fill(track.p(), hasLinkedHCAL);
  linkedHCALEfficiencyVsPVarBins_->Fill(track.p(), hasLinkedHCAL);
  linkedHCALEfficiencyVsEta_->Fill(track.eta(), hasLinkedHCAL);
  if (requirePrimaryVertex_) {
    pfBlockMatchEfficiencyVsGoodVertices_->Fill(numberOfGoodVertices, hasPFBlockMatch);
    uniquePFBlockMatchEfficiencyVsGoodVertices_->Fill(numberOfGoodVertices, hasUniquePFBlockMatch);
    linkedECALEfficiencyVsGoodVertices_->Fill(numberOfGoodVertices, hasLinkedECAL);
    linkedHCALEfficiencyVsGoodVertices_->Fill(numberOfGoodVertices, hasLinkedHCAL);
  }
}

void SPRLowPUDataAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription description;
  description.add<bool>("doV0Tagging", false);
  description.add<edm::InputTag>(
      "lambdas", edm::InputTag("generalV0Candidates", "Lambda"));
  description.add<edm::InputTag>(
      "kshorts", edm::InputTag("generalV0Candidates", "Kshort"));
  description.add<edm::InputTag>("tracks", edm::InputTag("generalTracks"));
  description.add<edm::InputTag>("vertices", edm::InputTag("offlinePrimaryVertices"));
  description.add<edm::InputTag>("pfBlocks", edm::InputTag("particleFlowBlock"));
  description.add<edm::InputTag>("pfCandidates", edm::InputTag("particleFlow"));
  description.add<edm::InputTag>("muons", edm::InputTag("muons"));
  description.add<bool>("requirePrimaryVertex", true);
  description.add<double>("minVertexNdof", 4.0);
  description.add<double>("maxVertexAbsZ", 24.0);
  description.add<double>("maxVertexRho", 2.0);
  description.add<std::string>("trackQuality", "highPurity");
  description.add<double>("minTrackPt", 1.0);
  description.add<double>("minTrackP", 3.0);
  description.add<double>("minV0TrackPt", 1.0);
  description.add<double>("minV0TrackP", 3.0);
  description.add<double>("maxTrackAbsEta", 1.0);
  description.add<double>("maxTrackNormalizedChi2", 5.0);
  description.add<double>("maxTrackRelativePError", 0.1);
  description.add<unsigned int>("minTrackerLayers", 8);
  description.add<unsigned int>("minPixelLayers", 2);
  description.add<double>("maxTrackAbsDxy", 0.02);
  description.add<double>("maxTrackAbsDz", 0.05);
  description.add<double>("isolationCone", 0.3);
  description.add<double>("minIsolationTrackPt", 0.5);
  description.add<double>("maxIsolationTrackAbsEta", 2.5);
  description.add<double>("maxRelativeIsolation", 0.1);
  descriptions.add("sprLowPUDataAnalyzer", description);
}

DEFINE_FWK_MODULE(SPRLowPUDataAnalyzer);
