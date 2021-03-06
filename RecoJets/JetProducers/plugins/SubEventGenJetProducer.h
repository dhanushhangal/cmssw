#ifndef RecoJets_JetProducers_SubEventGenJetProducer_h
#define RecoJets_JetProducers_SubEventGenJetProducer_h

/* *********************************************************
  \class SubEventGenJetProducer

  \brief Jet producer to produce jets from 
  \causally independent sub-events inside one event 
  \(for heavy ions or pile up)

 ************************************************************/

#include <vector>
#include "RecoJets/JetProducers/plugins/VirtualJetProducer.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"

namespace cms
{
  class SubEventGenJetProducer : public VirtualJetProducer
  {
  public:

    SubEventGenJetProducer(const edm::ParameterSet& ps);
    ~SubEventGenJetProducer() override {}
    void produce(edm::Event&, const edm::EventSetup&) override;
    void runAlgorithm(edm::Event&, const edm::EventSetup&) override;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
    static void fillDescriptionsFromSubEventGenJetProducer(edm::ParameterSetDescription& desc);
    
  protected:
   std::vector<std::vector<fastjet::PseudoJet> > subInputs_;
   std::vector<reco::GenJet>* subJets_;
   std::vector<int> hydroTag_;
   std::vector<int> nSubParticles_;
   bool signalOnly_;
   bool ignoreHydro_;

  protected:

    // overridden inputTowers method. Resets fjCompoundJets_ and 
    // calls VirtualJetProducer::inputTowers
    void inputTowers() override;

  private:
    edm::EDGetTokenT<reco::CandidateView> input_cand_token_;
  
  };
  
}


#endif
