import FWCore.ParameterSet.Config as cms

from RecoJets.JetProducers.GenJetParameters_cfi import *
from RecoJets.JetProducers.AnomalousCellParameters_cfi import *


iterativeCone5HiGenJets = cms.EDProducer("SubEventGenJetProducer",
                                         GenJetParameters,
                                         AnomalousCellParameters,
                                         jetAlgorithm = cms.string("IterativeCone"),
                                         rParam = cms.double(0.5),
                                         signalOnly = cms.bool(False)
                                         )

iterativeCone5HiGenJets.doAreaFastjet = cms.bool(True)
iterativeCone5HiGenJets.doRhoFastjet  = cms.bool(False)

iterativeCone7HiGenJets = iterativeCone5HiGenJets.clone(rParam=0.7)

ak5HiGenJets = cms.EDProducer("SubEventGenJetProducer",
                              GenJetParameters,
                              AnomalousCellParameters,
                              jetAlgorithm = cms.string("AntiKt"),
                              rParam = cms.double(0.5),
                              signalOnly = cms.bool(False)
                              )

ak5HiGenJets.doAreaFastjet = cms.bool(True)
ak5HiGenJets.doRhoFastjet  = cms.bool(False)

ak1HiGenJets = ak5HiGenJets.clone(rParam = 0.1)
ak2HiGenJets = ak5HiGenJets.clone(rParam = 0.2)
ak3HiGenJets = ak5HiGenJets.clone(rParam = 0.3)
ak4HiGenJets = ak5HiGenJets.clone(rParam = 0.4)
ak6HiGenJets = ak5HiGenJets.clone(rParam = 0.6)
ak7HiGenJets = ak5HiGenJets.clone(rParam = 0.7)

kt4HiGenJets = cms.EDProducer("SubEventGenJetProducer",
                              GenJetParameters,
                              AnomalousCellParameters,
                              jetAlgorithm = cms.string("Kt"),
                              rParam = cms.double(0.4)
                              )

kt4HiGenJets.doAreaFastjet = cms.bool(True)
kt4HiGenJets.doRhoFastjet  = cms.bool(False)

kt6HiGenJets = kt4HiGenJets.clone(rParam=0.6)



hiRecoGenJets = cms.Sequence(
    iterativeCone5HiGenJets +
    kt4HiGenJets +
    kt6HiGenJets +
    ak1HiGenJets +
    ak2HiGenJets +
    ak3HiGenJets +
    ak4HiGenJets +
    ak5HiGenJets +
    ak6HiGenJets +
    ak7HiGenJets
    )



