import FWCore.ParameterSet.Config as cms


sprLowPUDataAnalyzer = cms.EDAnalyzer(
    "SPRLowPUDataAnalyzer",
    tracks=cms.InputTag("generalTracks"),
    vertices=cms.InputTag("offlinePrimaryVertices"),
    pfBlocks=cms.InputTag("particleFlowBlock"),
    muons=cms.InputTag("muons"),
    requirePrimaryVertex=cms.bool(True),
    minVertexNdof=cms.double(4.0),
    maxVertexAbsZ=cms.double(24.0),
    maxVertexRho=cms.double(2.0),
    trackQuality=cms.string("highPurity"),
    minTrackPt=cms.double(1.0),
    minTrackP=cms.double(3.0),
    maxTrackAbsEta=cms.double(1.0),
    maxTrackNormalizedChi2=cms.double(5.0),
    maxTrackRelativePError=cms.double(0.1),
    minTrackerLayers=cms.uint32(8),
    minPixelLayers=cms.uint32(2),
    maxTrackAbsDxy=cms.double(0.02),
    maxTrackAbsDz=cms.double(0.05),
    isolationCone=cms.double(0.3),
    minIsolationTrackPt=cms.double(0.5),
    maxIsolationTrackAbsEta=cms.double(2.5),
    maxRelativeIsolation=cms.double(0.1),
)
