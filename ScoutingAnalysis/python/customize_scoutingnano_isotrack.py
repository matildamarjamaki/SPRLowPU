
import FWCore.ParameterSet.Config as cms

def add_isotrack_objects(process):

  process.isoTrackScoutingTable = cms.EDProducer("IsoTrackScoutingTableProducer",
    tracks = cms.InputTag("hltScoutingTrackPacker"),
    rechitsHBHE = cms.InputTag("hltScoutingRecHitPacker","HBHE"),
    rechitsEB = cms.InputTag("hltScoutingRecHitPacker","EB"),
    rechitsEE = cms.InputTag("hltScoutingRecHitPacker","EE"),
    saveAllRecHitsEB = cms.bool(False),
    saveAllRecHitsEE = cms.bool(False),
    saveAllRecHitsHBHE = cms.bool(False),
  )
  process.scoutingIsoTrackTask = cms.Task(process.isoTrackScoutingTable)

  process.scoutingNanoTaskCommon = cms.Task()
  process.scoutingNanoTaskCommon.add(process.scoutingIsoTrackTask)

  return process