import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing


options = VarParsing("analysis")
options.inputFiles = ["file:./RECO_1.root"]
options.outputFile = "sprLowPUData.root"
options.maxEvents = -1
options.parseArguments()

process = cms.Process("SPRLOWPU")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.load("SPRLowPU.RECOAnalysis.sprLowPUDataAnalyzer_cfi")

from Configuration.AlCa.GlobalTag import GlobalTag

process.GlobalTag = GlobalTag(process.GlobalTag, "160X_dataRun3_Prompt_v1", "")

process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source("PoolSource", fileNames=cms.untracked.vstring(options.inputFiles))

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string(options.outputFile),
)

process.analysisPath = cms.Path(process.sprLowPUDataAnalyzer)

# V0 integration test
process.sprLowPUDataAnalyzer.doV0Tagging = cms.bool(True)
process.maxEvents.input = cms.untracked.int32(100000)
process.TFileService.fileName = cms.string("/tmp/mmarjama/spr_v0_test/sprLowPU_v0_loosecuts.root")

# Diagnostic V0 daughter selection
process.sprLowPUDataAnalyzer.minV0TrackPt = cms.double(0.5)
process.sprLowPUDataAnalyzer.minV0TrackP = cms.double(1.0)
