import FWCore.ParameterSet.Config as cms

hltobject = cms.EDAnalyzer("TriggerObjectAnalyzer",
   processName = cms.string("HLT"),
   treeName = cms.string("JetTriggers"),
   triggerNames = cms.vstring(),
   triggerResults = cms.InputTag("TriggerResults","","HLT"),
   triggerEvent = cms.InputTag("hltTriggerSummaryAOD","","HLT")
)
