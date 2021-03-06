/****************************************************************************
*
* This is a part of TOTEM offline software.
* Authors: 
*  Jan Kaspar (jan.kaspar@gmail.com) 
*
****************************************************************************/

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/SourceFactory.h"
#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"

#include "DataFormats/CTPPSAlignment/interface/RPAlignmentCorrectionsDataSequence.h"

#include "Geometry/VeryForwardGeometryBuilder/interface/RPAlignmentCorrectionsMethods.h"

#include "CondFormats/AlignmentRecord/interface/RPMeasuredAlignmentRecord.h"
#include "CondFormats/AlignmentRecord/interface/RPRealAlignmentRecord.h"
#include "CondFormats/AlignmentRecord/interface/RPMisalignedAlignmentRecord.h"

#include <vector>
#include <string>
#include <map>
#include <set>


/**
 * Loads alignment corrections to EventSetup.
 **/
class  CTPPSIncludeAlignmentsFromXML : public edm::ESProducer, public edm::EventSetupRecordIntervalFinder
{
  public:
    CTPPSIncludeAlignmentsFromXML(const edm::ParameterSet &p);
    ~CTPPSIncludeAlignmentsFromXML() override;

    std::unique_ptr<RPAlignmentCorrectionsData> produceMeasured(const RPMeasuredAlignmentRecord &);
    std::unique_ptr<RPAlignmentCorrectionsData> produceReal(const RPRealAlignmentRecord &);
    std::unique_ptr<RPAlignmentCorrectionsData> produceMisaligned(const RPMisalignedAlignmentRecord &);

  protected:
    unsigned int verbosity;
    RPAlignmentCorrectionsDataSequence acsMeasured, acsReal, acsMisaligned;
    RPAlignmentCorrectionsData acMeasured, acReal, acMisaligned;

    void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&, const edm::IOVSyncValue&, edm::ValidityInterval&) override;

    static edm::EventID previousLS(const edm::EventID &src)
    {
      if (src.run() == edm::EventID::maxRunNumber() && src.luminosityBlock() == edm::EventID::maxLuminosityBlockNumber())
          return src;

      if (src.luminosityBlock() == 0)
        return edm::EventID(src.run() - 1, edm::EventID::maxLuminosityBlockNumber(), src.event());

      return edm::EventID(src.run(), src.luminosityBlock() - 1, src.event());
    }

    static edm::EventID nextLS(const edm::EventID &src)
    {
      if (src.luminosityBlock() == edm::EventID::maxLuminosityBlockNumber())
      {
        if (src.run() == edm::EventID::maxRunNumber())
          return src;

        return edm::EventID(src.run() + 1, 0, src.event());
      }

      return edm::EventID(src.run(), src.luminosityBlock() + 1, src.event());
    }

    /// merges an array of sequences to one
    RPAlignmentCorrectionsDataSequence Merge(const std::vector<RPAlignmentCorrectionsDataSequence>&) const;

    /// builds a sequence of corrections from provided sources and runs a few checks
    void PrepareSequence(const std::string &label, RPAlignmentCorrectionsDataSequence &seq, const std::vector<std::string> &files) const;
};

//----------------------------------------------------------------------------------------------------

using namespace std;
using namespace edm;

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

CTPPSIncludeAlignmentsFromXML::CTPPSIncludeAlignmentsFromXML(const edm::ParameterSet &pSet) :
  verbosity(pSet.getUntrackedParameter<unsigned int>("verbosity", 0))
{
  std::vector<std::string> measuredFiles;
  for (const auto &f: pSet.getParameter< vector<string> >("MeasuredFiles"))
    measuredFiles.push_back(edm::FileInPath(f).fullPath());
  PrepareSequence("Measured", acsMeasured, measuredFiles);

  std::vector<std::string> realFiles;
  for (const auto &f: pSet.getParameter< vector<string> >("RealFiles"))
    realFiles.push_back(edm::FileInPath(f).fullPath());
  PrepareSequence("Real", acsReal, realFiles);

  std::vector<std::string> misalignedFiles;
  for (const auto &f: pSet.getParameter< vector<string> >("MisalignedFiles"))
    misalignedFiles.push_back(edm::FileInPath(f).fullPath());
  PrepareSequence("Misaligned", acsMisaligned, misalignedFiles);

  setWhatProduced(this, &CTPPSIncludeAlignmentsFromXML::produceMeasured);
  setWhatProduced(this, &CTPPSIncludeAlignmentsFromXML::produceReal);
  setWhatProduced(this, &CTPPSIncludeAlignmentsFromXML::produceMisaligned);

  findingRecord<RPMeasuredAlignmentRecord>();
  findingRecord<RPRealAlignmentRecord>();
  findingRecord<RPMisalignedAlignmentRecord>();
}

//----------------------------------------------------------------------------------------------------

CTPPSIncludeAlignmentsFromXML::~CTPPSIncludeAlignmentsFromXML()
{
}

//----------------------------------------------------------------------------------------------------

RPAlignmentCorrectionsDataSequence CTPPSIncludeAlignmentsFromXML::Merge(const vector<RPAlignmentCorrectionsDataSequence>& seqs) const
{
  // find interval boundaries
  map< edm::EventID, vector< pair<bool, const RPAlignmentCorrectionsData*> > > bounds;

  for (const auto & seq : seqs)
  {
    for (const auto &p : seq)
    {
      const ValidityInterval &iov = p.first;
      const RPAlignmentCorrectionsData *corr = & p.second;

      const EventID &event_first = iov.first().eventID();
      bounds[event_first].emplace_back( pair<bool, const RPAlignmentCorrectionsData*>(true, corr) );

      const EventID &event_after = nextLS(iov.last().eventID());
      bounds[event_after].emplace_back( pair<bool, const RPAlignmentCorrectionsData*>(false, corr) );
    }
  }

  // build correction sums per interval
  set<const RPAlignmentCorrectionsData*> accumulator;
  RPAlignmentCorrectionsDataSequence result;
  for (map< EventID, vector< pair<bool, const RPAlignmentCorrectionsData*> > >::const_iterator tit = bounds.begin(); tit != bounds.end(); ++tit)
  {
    for (const auto & cit : tit->second)
    {
      bool add = cit.first;
      const RPAlignmentCorrectionsData *corr = cit.second;

      if (add)
        accumulator.insert(corr);
      else 
        accumulator.erase(corr);
    }

    auto tit_next = tit;
    tit_next++;
    if (tit_next == bounds.end())
      break;

    const EventID &event_first = tit->first;
    const EventID &event_last = previousLS(tit_next->first);

    if (verbosity)
    {
      LogVerbatim("CTPPSIncludeAlignmentsFromXML")
        << "    first=" << RPAlignmentCorrectionsMethods::iovValueToString(edm::IOVSyncValue(event_first))
        << ", last=" << RPAlignmentCorrectionsMethods::iovValueToString(edm::IOVSyncValue(event_last))
        << ": alignment blocks " << accumulator.size();
    }

    RPAlignmentCorrectionsData corr_sum;
    for (auto sit : accumulator)
      corr_sum.addCorrections(*sit);

    result.insert(edm::ValidityInterval(edm::IOVSyncValue(event_first), edm::IOVSyncValue(event_last)), corr_sum);
  }

  return result;
}

//----------------------------------------------------------------------------------------------------

void CTPPSIncludeAlignmentsFromXML::PrepareSequence(const string &label, RPAlignmentCorrectionsDataSequence &seq, const vector<string> &files) const
{
  if (verbosity)
    LogVerbatim(">> CTPPSIncludeAlignmentsFromXML") << "CTPPSIncludeAlignmentsFromXML::PrepareSequence(" << label << ")";

  vector<RPAlignmentCorrectionsDataSequence> sequences;
  for (const auto & file : files)
    sequences.emplace_back(RPAlignmentCorrectionsMethods::loadFromXML(file));

  seq = Merge(sequences);
}

//----------------------------------------------------------------------------------------------------

std::unique_ptr<RPAlignmentCorrectionsData> CTPPSIncludeAlignmentsFromXML::produceMeasured(const RPMeasuredAlignmentRecord &iRecord)
{
  return std::make_unique<RPAlignmentCorrectionsData>(acMeasured);
}

//----------------------------------------------------------------------------------------------------

std::unique_ptr<RPAlignmentCorrectionsData> CTPPSIncludeAlignmentsFromXML::produceReal(const RPRealAlignmentRecord &iRecord)
{
  return std::make_unique<RPAlignmentCorrectionsData>(acReal);
}

//----------------------------------------------------------------------------------------------------

std::unique_ptr<RPAlignmentCorrectionsData> CTPPSIncludeAlignmentsFromXML::produceMisaligned(const RPMisalignedAlignmentRecord &iRecord)
{
  return std::make_unique<RPAlignmentCorrectionsData>(acMisaligned);
}

//----------------------------------------------------------------------------------------------------

void CTPPSIncludeAlignmentsFromXML::setIntervalFor(const edm::eventsetup::EventSetupRecordKey &key,
    const IOVSyncValue& iosv, ValidityInterval& valInt) 
{
  if (verbosity)
  {
    time_t unixTime = iosv.time().unixTime();
    char timeStr[50];
    strftime(timeStr, 50, "%F %T", localtime(&unixTime));

    LogVerbatim("CTPPSIncludeAlignmentsFromXML")
      << ">> CTPPSIncludeAlignmentsFromXML::setIntervalFor(" << key.name() << ")";

    LogVerbatim("CTPPSIncludeAlignmentsFromXML")
      << "    event=" << iosv.eventID() << ", UNIX timestamp=" << unixTime << " (" << timeStr << ")";
  }

  // determine what sequence and corrections should be used
  RPAlignmentCorrectionsDataSequence *p_seq = nullptr;
  RPAlignmentCorrectionsData *p_corr = nullptr;

  if (strcmp(key.name(), "RPMeasuredAlignmentRecord") == 0)
  {
    p_seq = &acsMeasured;
    p_corr = &acMeasured;
  }

  if (strcmp(key.name(), "RPRealAlignmentRecord") == 0)
  {
    p_seq = &acsReal;
    p_corr = &acReal;
  }

  if (strcmp(key.name(), "RPMisalignedAlignmentRecord") == 0)
  {
    p_seq = &acsMisaligned;
    p_corr = &acMisaligned;
  }

  if (p_seq == nullptr)
    throw cms::Exception("CTPPSIncludeAlignmentsFromXML::setIntervalFor") << "Unknown record " << key.name();

  // find the corresponding interval
  bool next_exists = false;
  const edm::EventID &event_curr = iosv.eventID();
  edm::EventID event_next_start(edm::EventID::maxRunNumber(), edm::EventID::maxLuminosityBlockNumber(), 1);

  for (const auto &it: *p_seq)
  {
    const auto &it_event_first = it.first.first().eventID();
    const auto &it_event_last = it.first.last().eventID();

    bool it_contained_lo = ( (it_event_first.run() < event_curr.run()) ||
        ((it_event_first.run() == event_curr.run()) && (it_event_first.luminosityBlock() <= event_curr.luminosityBlock())) );

    bool it_contained_up = ( (it_event_last.run() > event_curr.run()) ||
        ((it_event_last.run() == event_curr.run()) && (it_event_last.luminosityBlock() >= event_curr.luminosityBlock())) );

    if (it_contained_lo && it_contained_up)
    {
      valInt = it.first;
      *p_corr = it.second;

      if (verbosity)
      {
        LogVerbatim("CTPPSIncludeAlignmentsFromXML")
          << "    setting validity interval ["
          << RPAlignmentCorrectionsMethods::iovValueToString(valInt.first())
          << ", " << RPAlignmentCorrectionsMethods::iovValueToString(valInt.last()) << "]";
      }

      return;
    }

    bool it_in_future = ( (it_event_first.run() > event_curr.run()) ||
        ((it_event_first.run() == event_curr.run() && (it_event_first.luminosityBlock() > event_curr.luminosityBlock()))) );

    if (it_in_future)
    {
      next_exists = true;
      if (event_next_start > it_event_first)
        event_next_start = it_event_first;
    }
  }

  // no interval found, set empty corrections
  *p_corr = RPAlignmentCorrectionsData();

  if (!next_exists)
  {
    valInt = ValidityInterval(iosv, iosv.endOfTime());
  } else {
    const EventID &event_last = previousLS(event_next_start);
    valInt = ValidityInterval(iosv, IOVSyncValue(event_last));
  }

  if (verbosity)
  {
    LogVerbatim("CTPPSIncludeAlignmentsFromXML")
      << "    setting validity interval ["
      << RPAlignmentCorrectionsMethods::iovValueToString(valInt.first())
      << ", " << RPAlignmentCorrectionsMethods::iovValueToString(valInt.last()) << "] (empty alignment corrections)";
  }
}

//----------------------------------------------------------------------------------------------------

DEFINE_FWK_EVENTSETUP_SOURCE(CTPPSIncludeAlignmentsFromXML);
