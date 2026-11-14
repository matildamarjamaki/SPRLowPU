import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing
import os
from pathlib import Path


options = VarParsing("analysis")
options.register(
    "sampleName",
    "",
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Name of a dataset file list in the samples directory, with or without the .txt suffix",
)
options.outputFile = "sprLowPUData.root"
options.maxEvents = -1
options.parseArguments()

if not options.sampleName:
    raise ValueError("The sampleName option is required")

sample_file_name = options.sampleName
if not sample_file_name.endswith(".txt"):
    sample_file_name += ".txt"

cmssw_base = os.environ.get("CMSSW_BASE")
if not cmssw_base:
    raise EnvironmentError("CMSSW_BASE is not set; initialize the CMSSW runtime before running this configuration")

sample_file = Path(cmssw_base) / "src" / "SPRLowPU" / "RECOAnalysis" / "test" / "samples" / sample_file_name
if not sample_file.is_file():
    raise FileNotFoundError(f"Dataset file list not found: {sample_file}")

with sample_file.open(encoding="utf-8") as input_file:
    file_paths = [
        line.strip()
        for line in input_file
        if line.strip() and not line.lstrip().startswith("#")
    ]

if not file_paths:
    raise ValueError(f"Dataset file list is empty: {sample_file}")

invalid_file_paths = [path for path in file_paths if not path.startswith("/store/")]
if invalid_file_paths:
    raise ValueError(
        f"Expected /store/ paths in {sample_file}; first invalid entry: {invalid_file_paths[0]}"
    )

xrootd_redirector = "root://hip-cms-se.csc.fi/"
input_files = [xrootd_redirector + path for path in file_paths]

print(f"Reading {len(input_files)} files from {sample_file}")

process = cms.Process("SPRLOWPU")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.load("SPRLowPU.RECOAnalysis.sprLowPUDataAnalyzer_cfi")

from Configuration.AlCa.GlobalTag import GlobalTag

process.GlobalTag = GlobalTag(process.GlobalTag, "160X_dataRun3_Prompt_v1", "")

process.MessageLogger.cerr.FwkReport.reportEvery = 1000

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source("PoolSource", fileNames=cms.untracked.vstring(input_files))

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string(options.outputFile),
)

process.analysisPath = cms.Path(process.sprLowPUDataAnalyzer)
