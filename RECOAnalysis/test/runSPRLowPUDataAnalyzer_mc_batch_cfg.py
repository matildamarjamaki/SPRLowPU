import os
from pathlib import Path

import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing


options = VarParsing("analysis")
options.register(
    "sampleName",
    "",
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Name of a simulation file list in the samples directory, with or without the .txt suffix",
)
options.outputFile = "sprLowPUSingleParticleGun.root"
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
    raise FileNotFoundError(f"Simulation file list not found: {sample_file}")

with sample_file.open(encoding="utf-8") as input_file:
    file_paths = [
        line.strip()
        for line in input_file
        if line.strip() and not line.lstrip().startswith("#")
    ]

if not file_paths:
    raise ValueError(f"Simulation file list is empty: {sample_file}")

global_redirector = "root://cms-xrd-global.cern.ch/"
input_files = []
for path in file_paths:
    if path.startswith("root://"):
        input_files.append(path)
    elif path.startswith("/store/"):
        input_files.append(global_redirector + path)
    else:
        raise ValueError(
            f"Expected a root:// URL or /store/ path in {sample_file}; invalid entry: {path}"
        )

print(f"Reading {len(input_files)} simulation files from {sample_file}")

process = cms.Process("SPRLOWPUMC")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.load("SPRLowPU.RECOAnalysis.sprLowPUDataAnalyzer_cfi")

from Configuration.AlCa.GlobalTag import GlobalTag

process.GlobalTag = GlobalTag(process.GlobalTag, "150X_mcRun3_2026_realistic_v5", "")

process.sprLowPUDataAnalyzer.requirePrimaryVertex = cms.bool(False)

process.MessageLogger.cerr.FwkReport.reportEvery = 1000

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.source = cms.Source("PoolSource", fileNames=cms.untracked.vstring(input_files))

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string(options.outputFile),
)

process.analysisPath = cms.Path(process.sprLowPUDataAnalyzer)
