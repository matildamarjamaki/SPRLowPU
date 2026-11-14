import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing


options = VarParsing("analysis")
options.inputFiles = ["root://hip-cms-se.csc.fi//store/user/nbinnorj/RAWToRECO_2026_v0p1/CRABOUTPUT/EphemeralZeroBias0/Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1/260728_163447/0000/RECO_6.root?scitag.flow=196664"]
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


# V0 response production
process.sprLowPUDataAnalyzer.doV0Tagging = cms.bool(True)
process.sprLowPUDataAnalyzer.minV0TrackPt = cms.double(0.5)
process.sprLowPUDataAnalyzer.minV0TrackP = cms.double(3.0)
