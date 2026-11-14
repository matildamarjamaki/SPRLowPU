import FWCore.ParameterSet.Config as cms

process = cms.Process("V0ANALYSIS")

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(
        "root://hip-cms-se.csc.fi//store/user/nbinnorj/RAWToRECO_2026_v0p1/CRABOUTPUT/EphemeralZeroBias0/Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1/260728_163447/0000/RECO_6.root"
    )
)

process.maxEvents = cms.untracked.PSet(
    input=cms.untracked.int32(1000)
)

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string("/tmp/spr_lambda_ep_test1000_v2.root")
)

process.v0Analyzer = cms.EDAnalyzer(
    "SPRLowPUV0Analyzer",
    kshorts=cms.InputTag("generalV0Candidates", "Kshort"),
    lambdas=cms.InputTag("generalV0Candidates", "Lambda"),
    pfCandidates=cms.InputTag("particleFlow")
)

process.p = cms.Path(process.v0Analyzer)
