
import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing
import os

process = cms.Process("PFANA")

process.load("FWCore.MessageService.MessageLogger_cfi")

options = VarParsing("analysis")
options.parseArguments()

if len(options.inputFiles) == 0:
    raise RuntimeError(
        "Anna input-tiedosto: cmsRun ConfFile_cfg.py inputFiles=file:pion_chunk_aa"
    )

filelist_name = options.inputFiles[0]

# Remove possible file: prefix
if filelist_name.startswith("file:"):
    filelist_name = filelist_name[5:]

with open(filelist_name) as f:
    file_list = [
        "root://cms-xrd-global.cern.ch//" + line.strip().lstrip("/")
        for line in f if line.strip()
    ]

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)

process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(*file_list)
)

outname = "histo_" + os.path.basename(filelist_name) + ".root"

process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string(outname)
)

process.recoAnalyzer = cms.EDAnalyzer(
    "RECOAnalyzer",
    pfBlock = cms.InputTag("particleFlowBlock"),
    primaryVertices = cms.InputTag("offlinePrimaryVertices"),
    genParticles = cms.InputTag("genParticles"),
    simTracks = cms.InputTag("g4SimHits"),
    simVertices = cms.InputTag("g4SimHits"),
)

process.p = cms.Path(process.recoAnalyzer)