import FWCore.ParameterSet.Config as cms

process = cms.Process("PFANA")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.maxEvents = cms.untracked.PSet(
   input = cms.untracked.int32(-1)
)

#
#
#
# process.source = cms.Source("PoolSource",
#    fileNames = cms.untracked.vstring(
#       'file:root://hip-cms-se.csc.fi//store/user/nbinnorj/RAWToRECO_2025_v0p3/CRABOUTPUT/SpecialZeroBias1/Run2025G_PromptRECO_v1_LowPUJSON_RAWToRECO_2025_v0p3/260320_213619/0000/RECO_1.root'
#    )
# )

process.source = cms.Source("PoolSource",
   fileNames = cms.untracked.vstring(
      'root://hip-cms-se.csc.fi//store/user/nbinnorj/RAWToRECO_2025_v0p3/CRABOUTPUT/SpecialZeroBias1/Run2025G_PromptRECO_v1_LowPUJSON_RAWToRECO_2025_v0p3/260320_213619/0000/RECO_1.root'
   )
)


#
#
#
process.TFileService = cms.Service("TFileService",
   fileName = cms.string(f'histo_SPRLowPU.root')
)


process.recoAnalyzer = cms.EDAnalyzer(
   'RECOAnalyzer',
   # 'RECODumper',
   pfBlock   = cms.InputTag('particleFlowBlock'),
   primaryVertices = cms.InputTag('offlinePrimaryVertices'),
)

process.p = cms.Path(process.recoAnalyzer)