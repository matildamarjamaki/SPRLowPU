import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing


options = VarParsing("analysis")
options.inputFiles = ["file:./step3.root"]
options.outputFile = "sprLowPUSingleParticleGun.root"
options.maxEvents = -1
options.parseArguments()

process = cms.Process("SPRLOWPUMC")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.load("SPRLowPU.RECOAnalysis.sprLowPUDataAnalyzer_cfi")

from Configuration.AlCa.GlobalTag import GlobalTag

process.GlobalTag = GlobalTag(process.GlobalTag, "150X_mcRun3_2026_realistic_v5", "")

process.sprLowPUDataAnalyzer.requirePrimaryVertex = cms.bool(False)

process.MessageLogger.cerr.FwkReport.reportEvery = 1000

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source("PoolSource", fileNames=cms.untracked.vstring(options.inputFiles))

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string(options.outputFile),
)

process.analysisPath = cms.Path(process.sprLowPUDataAnalyzer)
