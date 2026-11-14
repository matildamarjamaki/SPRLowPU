#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/Candidate/interface/VertexCompositeCandidate.h"
#include "DataFormats/RecoCandidate/interface/RecoChargedCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlock.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TProfile.h"

#include <cmath>
#include <map>
#include <set>
#include <utility>

class SPRLowPUV0Analyzer
    : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit SPRLowPUV0Analyzer(const edm::ParameterSet& config)
      : kshortToken_(consumes<reco::VertexCompositeCandidateCollection>(
            config.getParameter<edm::InputTag>("kshorts"))),
        lambdaToken_(consumes<reco::VertexCompositeCandidateCollection>(
            config.getParameter<edm::InputTag>("lambdas"))),
        pfCandidatesToken_(consumes<reco::PFCandidateCollection>(
            config.getParameter<edm::InputTag>("pfCandidates"))) {
    usesResource("TFileService");

    edm::Service<TFileService> fs;

    kshortMass_ = fs->make<TH1D>(
        "kshortMass", ";K_{S}^{0} invariant mass [GeV];Candidates",
        140, 0.42, 0.56);

    lambdaMass_ = fs->make<TH1D>(
        "lambdaMass", ";#Lambda / anti-#Lambda invariant mass [GeV];Candidates",
        120, 1.06, 1.18);

    lambdaParticleMass_ = fs->make<TH1D>(
        "lambdaParticleMass", ";#Lambda invariant mass [GeV];Candidates",
        120, 1.06, 1.18);

    lambdaAntiParticleMass_ = fs->make<TH1D>(
        "lambdaAntiParticleMass", ";anti-#Lambda invariant mass [GeV];Candidates",
        120, 1.06, 1.18);

    kshortEta_ = fs->make<TH1D>(
        "kshortEta", ";K_{S}^{0} #eta;Candidates",
        100, -3.0, 3.0);

    lambdaEta_ = fs->make<TH1D>(
        "lambdaEta", ";#Lambda / anti-#Lambda #eta;Candidates",
        100, -3.0, 3.0);

    collectionStatus_ = fs->make<TH1D>(
        "collectionStatus", ";Collection availability;Events",
        4, -0.5, 3.5);

    nKshort_ = fs->make<TH1D>(
        "nKshort", ";Number of K_{S}^{0} candidates per event;Events",
        21, -0.5, 20.5);

    nLambda_ = fs->make<TH1D>(
        "nLambda", ";Number of #Lambda candidates per event;Events",
        21, -0.5, 20.5);

    kshortPt_ = fs->make<TH1D>(
        "kshortPt", ";K_{S}^{0} p_{T} [GeV];Candidates",
        100, 0.0, 20.0);

    lambdaPt_ = fs->make<TH1D>(
        "lambdaPt", ";#Lambda p_{T} [GeV];Candidates",
        100, 0.0, 20.0);

    lambdaHigherMomentumTrackP_ = fs->make<TH1D>(
        "lambdaHigherMomentumTrackP",
        ";Higher-momentum #Lambda daughter p [GeV];Candidates",
        120, 0.0, 30.0);

    lambdaHigherMomentumTrackPt_ = fs->make<TH1D>(
        "lambdaHigherMomentumTrackPt",
        ";Higher-momentum #Lambda daughter p_{T} [GeV];Candidates",
        100, 0.0, 20.0);

    lambdaProtonCandidateResponseVsPt_ = fs->make<TH2D>(
        "lambdaProtonCandidateResponseVsPt",
        ";Higher-momentum #Lambda daughter p_{T} [GeV];PF candidate (ECAL+HCAL)/p",
        100, 0.0, 20.0, 120, 0.0, 3.0);

    meanLambdaProtonCandidateResponseVsPt_ = fs->make<TProfile>(
        "meanLambdaProtonCandidateResponseVsPt",
        ";Higher-momentum #Lambda daughter p_{T} [GeV];Mean PF candidate (ECAL+HCAL)/p",
        100, 0.0, 20.0);

    lambdaProtonPFBlockResponseVsPt_ = fs->make<TH2D>(
        "lambdaProtonPFBlockResponseVsPt",
        ";Higher-momentum #Lambda daughter p_{T} [GeV];PFBlock linked-cluster (ECAL+HCAL)/p",
        100, 0.0, 20.0, 120, 0.0, 3.0);

    meanLambdaProtonPFBlockResponseVsPt_ = fs->make<TProfile>(
        "meanLambdaProtonPFBlockResponseVsPt",
        ";Higher-momentum #Lambda daughter p_{T} [GeV];Mean PFBlock linked-cluster (ECAL+HCAL)/p",
        100, 0.0, 20.0);

    lambdaProtonPFBlockMatchStatus_ = fs->make<TH1D>(
        "lambdaProtonPFBlockMatchStatus",
        ";PFBlock matching status;Candidates", 5, -0.5, 4.5);

    lambdaProtonPFBlockMatchStatus_->GetXaxis()->SetBinLabel(1, "PF candidate");
    lambdaProtonPFBlockMatchStatus_->GetXaxis()->SetBinLabel(2, "Track element");
    lambdaProtonPFBlockMatchStatus_->GetXaxis()->SetBinLabel(3, "Linked cluster");
    lambdaProtonPFBlockMatchStatus_->GetXaxis()->SetBinLabel(4, "Shared cluster");
    lambdaProtonPFBlockMatchStatus_->GetXaxis()->SetBinLabel(5, "Filled E/p");

    lambdaProtonPFBlockNSharedClusters_ = fs->make<TH1D>(
        "lambdaProtonPFBlockNSharedClusters",
        ";Number of linked clusters shared with other tracks;Candidates",
        11, -0.5, 10.5);

    lambdaDaughterMatchStatus_ = fs->make<TH1D>(
        "lambdaDaughterMatchStatus",
        ";Matching status;Candidates", 5, -0.5, 4.5);
    lambdaDaughterMatchStatus_->GetXaxis()->SetBinLabel(1, "All Lambda");
    lambdaDaughterMatchStatus_->GetXaxis()->SetBinLabel(2, "Valid daughters");
    lambdaDaughterMatchStatus_->GetXaxis()->SetBinLabel(3, "One PF match");
    lambdaDaughterMatchStatus_->GetXaxis()->SetBinLabel(4, "Charged hadron");
    lambdaDaughterMatchStatus_->GetXaxis()->SetBinLabel(5, "Filled E/p");
  }

  void analyze(const edm::Event& event,
               const edm::EventSetup&) override {
    edm::Handle<reco::VertexCompositeCandidateCollection> kshorts;
    edm::Handle<reco::VertexCompositeCandidateCollection> lambdas;

    event.getByToken(kshortToken_, kshorts);
    event.getByToken(lambdaToken_, lambdas);
    auto const pfCandidates = event.getHandle(pfCandidatesToken_);

    collectionStatus_->Fill(
        (kshorts.isValid() ? 1 : 0) +
        (lambdas.isValid() ? 2 : 0));

    if (kshorts.isValid()) {
      nKshort_->Fill(kshorts->size());

      for (const auto& candidate : *kshorts) {
        kshortMass_->Fill(candidate.mass());
        kshortPt_->Fill(candidate.pt());
        kshortEta_->Fill(candidate.eta());
      }
    }

    if (lambdas.isValid()) {
      nLambda_->Fill(lambdas->size());

      for (const auto& candidate : *lambdas) {
        lambdaDaughterMatchStatus_->Fill(0.0);
        lambdaMass_->Fill(candidate.mass());
        lambdaPt_->Fill(candidate.pt());
        lambdaEta_->Fill(candidate.eta());

        if (candidate.pdgId() == 3122)
          lambdaParticleMass_->Fill(candidate.mass());
        else if (candidate.pdgId() == -3122)
          lambdaAntiParticleMass_->Fill(candidate.mass());

        if (candidate.numberOfDaughters() != 2)
          continue;

        auto const* daughter0 =
            dynamic_cast<reco::RecoChargedCandidate const*>(candidate.daughter(0));
        auto const* daughter1 =
            dynamic_cast<reco::RecoChargedCandidate const*>(candidate.daughter(1));

        if (!daughter0 || !daughter1)
          continue;

        reco::TrackRef const track0 = daughter0->track();
        reco::TrackRef const track1 = daughter1->track();

        if (track0.isNull() || track1.isNull())
          continue;

        reco::TrackRef const protonTrack =
            track0->p() >= track1->p() ? track0 : track1;

        lambdaDaughterMatchStatus_->Fill(1.0);
        lambdaHigherMomentumTrackP_->Fill(protonTrack->p());
        lambdaHigherMomentumTrackPt_->Fill(protonTrack->pt());

        if (!pfCandidates.isValid())
          continue;

        unsigned int matches = 0;
        reco::PFCandidate const* matchedPF = nullptr;

        for (auto const& pf : *pfCandidates) {
          if (pf.trackRef().isNonnull() && pf.trackRef() == protonTrack) {
            ++matches;
            matchedPF = &pf;
          }
        }

        if (matches != 1 || !matchedPF)
          continue;

        lambdaDaughterMatchStatus_->Fill(2.0);

        if (matchedPF->particleId() != reco::PFCandidate::h)
          continue;

        lambdaDaughterMatchStatus_->Fill(3.0);
        lambdaProtonPFBlockMatchStatus_->Fill(0.0);

        // Find the selected track element in the matched PFCandidate's blocks.
        double blockEcal = 0.0;
        double blockHcal = 0.0;
        bool foundTrackElement = false;
        bool foundLinkedCluster = false;
        unsigned int nSharedClusters = 0;

        // Avoid counting the same cluster more than once.
        std::set<std::pair<edm::ProductID, unsigned int>> usedClusters;

        for (auto const& elementInBlock : matchedPF->elementsInBlocks()) {
          auto const& blockRef = elementInBlock.first;
          unsigned int const trackIndex = elementInBlock.second;

          if (blockRef.isNull())
            continue;

          auto const& block = *blockRef;
          auto const& elements = block.elements();

          if (trackIndex >= elements.size())
            continue;

          auto const& trackElement = elements[trackIndex];

          if (trackElement.type() != reco::PFBlockElement::TRACK ||
              trackElement.trackRef().isNull() ||
              trackElement.trackRef() != protonTrack)
            continue;

          foundTrackElement = true;

          for (auto const type : {reco::PFBlockElement::ECAL,
                                  reco::PFBlockElement::HCAL}) {
            std::multimap<double, unsigned> associated;

            block.associatedElements(
                trackIndex, block.linkData(), associated,
                type, reco::PFBlock::LINKTEST_ALL);

            for (auto const& association : associated) {
              unsigned int const clusterIndex = association.second;

              if (clusterIndex >= elements.size())
                continue;

              auto const& clusterElement = elements[clusterIndex];
              auto const& clusterRef = clusterElement.clusterRef();

              if (clusterRef.isNull())
                continue;

              auto const clusterKey =
                  std::make_pair(clusterRef.id(), clusterRef.key());

              if (!usedClusters.insert(clusterKey).second)
                continue;

              foundLinkedCluster = true;

              if (type == reco::PFBlockElement::ECAL)
                blockEcal += clusterRef->energy();
              else
                blockHcal += clusterRef->energy();

              // Check whether another track is linked to this cluster.
              std::multimap<double, unsigned> linkedTracks;
              block.associatedElements(
                  clusterIndex, block.linkData(), linkedTracks,
                  reco::PFBlockElement::TRACK,
                  reco::PFBlock::LINKTEST_ALL);

              bool shared = false;
              for (auto const& linked : linkedTracks) {
                if (linked.second != trackIndex) {
                  shared = true;
                  break;
                }
              }

              if (shared)
                ++nSharedClusters;
            }
          }
        }

        if (foundTrackElement)
          lambdaProtonPFBlockMatchStatus_->Fill(1.0);

        if (foundLinkedCluster)
          lambdaProtonPFBlockMatchStatus_->Fill(2.0);

        if (nSharedClusters > 0)
          lambdaProtonPFBlockMatchStatus_->Fill(3.0);

        if (foundTrackElement) {
          lambdaProtonPFBlockNSharedClusters_->Fill(nSharedClusters);

          if (protonTrack->p() > 0.0) {
            double const blockResponse =
                (blockEcal + blockHcal) / protonTrack->p();

            if (std::isfinite(blockResponse) &&
                (blockEcal + blockHcal) > 0.0) {
              lambdaProtonPFBlockResponseVsPt_->Fill(
                  protonTrack->pt(), blockResponse);

              meanLambdaProtonPFBlockResponseVsPt_->Fill(
                  protonTrack->pt(), blockResponse);

              lambdaProtonPFBlockMatchStatus_->Fill(4.0);
            }
          }
        }

        if (protonTrack->p() <= 0.0)
          continue;

        double const response =
            (matchedPF->ecalEnergy() + matchedPF->hcalEnergy()) /
            protonTrack->p();

        if (!std::isfinite(response) ||
            (matchedPF->ecalEnergy() + matchedPF->hcalEnergy()) <= 0.0)
          continue;

        lambdaProtonCandidateResponseVsPt_->Fill(protonTrack->pt(), response);
        meanLambdaProtonCandidateResponseVsPt_->Fill(protonTrack->pt(), response);
        lambdaDaughterMatchStatus_->Fill(4.0);
      }
    }
  }

  static void fillDescriptions(
      edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;

    desc.add<edm::InputTag>(
        "kshorts", edm::InputTag("generalV0Candidates", "Kshort"));

    desc.add<edm::InputTag>(
        "lambdas", edm::InputTag("generalV0Candidates", "Lambda"));

    desc.add<edm::InputTag>(
        "pfCandidates", edm::InputTag("particleFlow"));

    descriptions.add("sprLowPUV0Analyzer", desc);
  }

private:
  edm::EDGetTokenT<reco::VertexCompositeCandidateCollection> kshortToken_;
  edm::EDGetTokenT<reco::VertexCompositeCandidateCollection> lambdaToken_;
  edm::EDGetTokenT<reco::PFCandidateCollection> pfCandidatesToken_;

  TH1D* lambdaHigherMomentumTrackP_;
  TH1D* lambdaHigherMomentumTrackPt_;
  TH2D* lambdaProtonCandidateResponseVsPt_;
  TProfile* meanLambdaProtonCandidateResponseVsPt_;
  TH2D* lambdaProtonPFBlockResponseVsPt_;
  TProfile* meanLambdaProtonPFBlockResponseVsPt_;
  TH1D* lambdaProtonPFBlockMatchStatus_;
  TH1D* lambdaProtonPFBlockNSharedClusters_;
  TH1D* lambdaDaughterMatchStatus_;

  TH1D* lambdaParticleMass_;
  TH1D* lambdaAntiParticleMass_;
  TH1D* kshortEta_;
  TH1D* lambdaEta_;
  TH1D* collectionStatus_;
  TH1D* kshortMass_;
  TH1D* lambdaMass_;
  TH1D* nKshort_;
  TH1D* nLambda_;
  TH1D* kshortPt_;
  TH1D* lambdaPt_;
};

DEFINE_FWK_MODULE(SPRLowPUV0Analyzer);
