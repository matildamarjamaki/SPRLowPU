import FWCore.ParameterSet.Config as cms

process = cms.Process("V0ANALYSIS")

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(
        "file:RECO_1.root"
    )
)

process.maxEvents = cms.untracked.PSet(
    input=cms.untracked.int32(1000)
)

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string("sprLowPUV0.root")
)

process.v0Analyzer = cms.EDAnalyzer(
    "SPRLowPUV0Analyzer",
    kshorts=cms.InputTag("generalV0Candidates", "Kshort"),
    lambdas=cms.InputTag("generalV0Candidates", "Lambda")
)

process.p = cms.Path(process.v0Analyzer)
