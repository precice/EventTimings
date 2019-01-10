#include "EventUtils.hpp"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <ctime>
#include <utility>
#include "prettyprint.hpp"
#include "TableWriter.hpp"

using std::cout;
using std::endl;

template<class... Args>
void dbgprint(const std::string& format, Args&&... args)
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  std::string with_rank = "[" + std::to_string(rank) + "] " + format + "\n";
  printf(with_rank.c_str(), std::forward<Args>(args)...);
}

struct MPI_EventData
{
  char name[255] = {'\0'};
  int rank, count = 0;
  long total = 0, max = 0, min = 0;
  int dataSize = 0, stateChangesSize = 0;
};


// -----------------------------------------------------------------------

EventData::EventData(std::string _name) :
  name(_name)
{
  MPI_Comm_rank(EventRegistry::instance().getMPIComm(), &rank);
}

EventData::EventData(std::string _name, int _rank, long _count, long _total,
                     long _max, long _min, std::vector<int> _data, Event::StateChanges _stateChanges)
  :  max(std::chrono::milliseconds(_max)),
     min(std::chrono::milliseconds(_min)),
     total(std::chrono::milliseconds(_total)),
     rank(_rank),
     stateChanges(_stateChanges),
     name(_name),
     count(_count),
     data(_data)
{}

void EventData::put(Event* event)
{
  count++;
  Event::Clock::duration duration = event->getDuration();
  total += duration;
  min = std::min(duration, min);
  max = std::max(duration, max);
  data.insert(std::end(data), std::begin(event->data), std::end(event->data));
  stateChanges.insert(std::end(stateChanges), std::begin(event->stateChanges), std::end(event->stateChanges));
}

std::string EventData::getName() const
{
  return name;
}

long EventData::getAvg() const
{
  return (std::chrono::duration_cast<std::chrono::milliseconds>(total) / count).count();
}

long EventData::getMax() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(max).count();
}

long EventData::getMin() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(min).count();
}

long EventData::getTotal() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(total).count();
}

long EventData::getCount() const
{
  return count;
}

int EventData::getTimePercentage() const
{
  return (static_cast<double>(total.count()) / EventRegistry::instance().getDuration().count()) * 100;
}

const std::vector<int> & EventData::getData() const
{
  return data;
}

void EventData::writeCSV(std::ostream &out)
{
  using namespace std::chrono;
  auto finalize_time = EventRegistry::instance().getTimestamp();
  std::time_t ts = system_clock::to_time_t(finalize_time);
  auto ms = duration_cast<milliseconds>(finalize_time.time_since_epoch()) % 1000;

  out << std::put_time(std::localtime(&ts), "%FT%T") << "." << std::setw(3) << ms.count() << ","
      << EventRegistry::instance().runName << ","
      << rank << ","
      << getName() << ","
      << getCount() << ","
      << getTotal() << ","
      << getMin() << "," << getMax() << "," << getAvg() << ","
      << getTimePercentage();

  /// Write attached data
  bool first = true;
  out << ",\"[";
  for (auto & d : data) {
    if (not first)
      out << ",";
    out << d;
    first = false;
  }
  out << "]\"" << std::endl;
}

void EventData::writeEventLog(std::ostream &out)
{
  using namespace std::chrono;
  auto finalize_time = EventRegistry::instance().getTimestamp();
  std::time_t ts = system_clock::to_time_t(finalize_time);
  auto ms = duration_cast<milliseconds>(finalize_time.time_since_epoch()) % 1000;

  for (auto & sc : stateChanges) {
    out << std::put_time(std::localtime(&ts), "%FT%T") << "." << std::setw(3) << ms.count() << ","
        << EventRegistry::instance().runName << ","
        << name << ","
        << rank << ","
        << duration_cast<milliseconds>(std::get<1>(sc).time_since_epoch()).count() << ","
        << static_cast<int>(std::get<0>(sc)) << std::endl;
  }
}


// -----------------------------------------------------------------------

RankData::RankData()
{
  // MPI_Comm_rank(EventRegistry::instance().getMPIComm(), &rank);
}

void RankData::initialize()
{
  initializedAt = std::chrono::system_clock::now();
  initializedAtTicks = std::chrono::steady_clock::now();
}

void RankData::finalize()
{
  finalizedAt = std::chrono::system_clock::now();
  finalizedAtTicks = std::chrono::steady_clock::now();
}

void RankData::put(Event* event)
{
  /// Construct or return EventData object with name as key and name as arg to ctor.
  auto data = std::get<0>(evData.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(event->name),
                                         std::forward_as_tuple(event->name)));
  data->second.put(event);
}

void RankData::addEventData(EventData ed)
{
  // evData[ed.getName()] = ed;
  evData.emplace(ed.getName(), std::move(ed));
}

std::chrono::steady_clock::duration RankData::getDuration()
{
  if (isFinalized)
    return finalizedAtTicks - initializedAtTicks;
  else
    return std::chrono::steady_clock::now() - initializedAtTicks;
}



// -----------------------------------------------------------------------


EventRegistry & EventRegistry::instance()
{
  static EventRegistry instance;
  return instance;
}

void EventRegistry::initialize(std::string applicationName, std::string runName, MPI_Comm comm)
{
  this->applicationName = applicationName;
  this->runName = runName;
  this->comm = comm;

  localRankData.initialize();

  globalEvent.start(true);
  initialized = true;
}

void EventRegistry::finalize()
{
  globalEvent.stop(true); // acts as a barrier
  localRankData.finalize();

  timestamp = std::chrono::system_clock::now();
  initialized = false;
  for (auto & e : storedEvents)
    e.second.stop();
  collect();
}

void EventRegistry::clear()
{
  localRankData.evData.clear();
}

void EventRegistry::signal_handler(int signal)
{
  if (initialized) {
    finalize();
    printAll();
  }
}

void EventRegistry::put(Event* event)
{
  localRankData.put(event);
}

Event & EventRegistry::getStoredEvent(std::string const & name)
{
  // Reset the prefix for creation of a stored event. Using prefixes with stored events is possible
  // but leads to unexpected results, such as not getting the event you want, because someone else up the
  // stack set a prefix.
  auto previousPrefix = prefix;
  prefix = "";
  auto insertion = storedEvents.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(name),
                                        std::forward_as_tuple(name, false, false));

  prefix = previousPrefix;
  return std::get<0>(insertion)->second;
}


std::chrono::system_clock::time_point EventRegistry::getTimestamp()
{
  return timestamp;
}

Event::Clock::duration EventRegistry::getDuration()
{
  // return localRankData.evData.at("_GLOBAL").total;
  return localRankData.getDuration();
}

void EventRegistry::printAll()
{
  print();

  std::string csvFile, logFile;
  if (applicationName.empty()) {
    csvFile = "EventTimings.log";
    logFile = "Events.log";
  }
  else {
    csvFile = applicationName + "-eventTimings.log";
    logFile = applicationName + "-events.log";
  }
  writeCSV(csvFile);
  writeEventLogs(logFile);
}


void EventRegistry::print(std::ostream &out)
{
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  if (rank == 0) {
    using std::endl;
    using std::setw; using std::setprecision;
    using std::left; using std::right;

    std::time_t ts = std::chrono::system_clock::to_time_t(timestamp);
    auto totalDuration = localRankData.evData.at("_GLOBAL").getTotal();

    out << "Run finished at " << std::asctime(std::localtime(&ts));

    out << "Global runtime       = "
        << totalDuration << "ms / "
        << totalDuration / 1000 << "s" << std::endl
        << "Number of processors = " << size << std::endl
        << "# Rank: " << rank << std::endl << std::endl;

    Table table( {
        {getMaxNameWidth(), "Event"},
        {10, "Count"},
        {10, "Total[ms]"},
        {10, "Max[ms]"},
        {10, "Min[ms]"},
        {10, "Avg[ms]"},
        {10, "T[%]"}
      });
    table.printHeader();

    for (auto & e : localRankData.evData) {
      auto & ev = e.second;
      table.printLine(ev.getName(), ev.getCount(), ev.getTotal(), ev.getMax(),
                      ev.getMin(), ev.getAvg(), ev.getTimePercentage());
    }

    out << endl;
    printGlobalStats();
    out << endl << std::flush;
  }
}

void EventRegistry::print()
{
  EventRegistry::print(std::cout);
}

void EventRegistry::writeCSV(std::string filename)
{
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  if (rank != 0)
    return;

  bool fileExists = std::ifstream(filename).is_open();

  std::ofstream outfile;
  outfile.open(filename, std::ios::out | std::ios::app);
  if (not fileExists)
    outfile << "Timestamp,RunName,Rank,Name,Count,Total,Min,Max,Avg,T%,Data" << std::endl;

  std::time_t ts = std::chrono::system_clock::to_time_t(timestamp);
  std::tm tm = *std::localtime(&ts);

  outfile << "# Run finished at: " << std::put_time(&tm, "%F %T") << std::endl
          << "# Number of processors: " << size << std::endl
          << "# Timestamp,RunName,Rank,Name,Count,Total,Min,Max,Avg,T%,Data" << std::endl;


  for (auto e : globalEvents) {
    auto & ev = e.second;
    ev.writeCSV(outfile);
  }

  outfile.close();
}

void EventRegistry::writeEventLogs(std::string filename)
{
  int rank;
  MPI_Comm_rank(comm, &rank);

  if (rank != 0)
    return;

  bool fileExists = std::ifstream(filename).is_open();

  std::ofstream outfile;
  outfile.open(filename, std::ios::out | std::ios::app);
  if (not fileExists)
    outfile << "RunTimestamp,RunName,Name,Rank,Timestamp,State" << "\n";

  for (auto e : globalEvents) {
    auto & ev = e.second;
    ev.writeEventLog(outfile);
  }

  outfile.close();
}


std::map<std::string, GlobalEventStats> getGlobalStats(GlobalEvents events)
{
  std::map<std::string, GlobalEventStats> globalStats;
  for (auto & e : events) {
    auto ev = e.second;
    GlobalEventStats & stats = globalStats[e.first];
    if (ev.max > stats.max) {
      stats.max = ev.max;
      stats.maxRank = ev.rank;
    }
    if (ev.min < stats.min) {
      stats.min = ev.min;
      stats.minRank = ev.rank;
    }
  }
  return globalStats;
}

void EventRegistry::printGlobalStats()
{
  Table t({ {getMaxNameWidth(), "Name"},
      {10, "Max"}, {10, "MaxOnRank"}, {10, "Min"}, {10, "MinOnRank"}, {10, "Min/Max"} });
  t.printHeader();

  auto stats = getGlobalStats(globalEvents);
  for (auto & e : stats) {
    auto & ev = e.second;
    double rel = 0;
    if (ev.max != Event::Clock::duration::zero()) // Guard against division by zero
      rel = static_cast<double>(ev.min.count()) / ev.max.count();
    t.printLine(e.first, ev.max, ev.maxRank, ev.min, ev.minRank, rel);
  }
}

MPI_Comm & EventRegistry::getMPIComm()
{
  return comm;
}


void EventRegistry::collect()
{
  // Register MPI datatype
  MPI_Datatype MPI_EVENTDATA;
  int blocklengths[] = {255, 2, 3, 2};
  MPI_Aint displacements[] = {offsetof(MPI_EventData, name), offsetof(MPI_EventData, rank),
                              offsetof(MPI_EventData, total), offsetof(MPI_EventData, dataSize)};
  MPI_Datatype types[] = {MPI_CHAR, MPI_INT, MPI_LONG, MPI_INT};
  MPI_Type_create_struct(4, blocklengths, displacements, types, &MPI_EVENTDATA);
  MPI_Type_commit(&MPI_EVENTDATA);

  int rank, MPIsize;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &MPIsize);

  std::vector<MPI_Request> requests;
  std::vector<int> eventsPerRank(MPIsize);
  size_t eventsSize = localRankData.evData.size();
  MPI_Gather(&eventsSize, 1, MPI_INT, eventsPerRank.data(), 1, MPI_INT, 0, comm);

  std::vector<MPI_EventData> eventSendBuf(eventsSize);
  std::vector<std::unique_ptr<char[]>> packSendBuf(eventsSize);
  int i = 0;

  // Send all events from all ranks, including rank 0
  for (const auto & ev : localRankData.evData) {
    MPI_EventData eventdata;
    MPI_Request req;

    // Send aggregated EventData
    assert(ev.first.size() <= 255);
    ev.first.copy(eventSendBuf[i].name, 255);
    eventSendBuf[i].rank = rank;
    eventSendBuf[i].count = ev.second.getCount();
    eventSendBuf[i].total = ev.second.getTotal();
    eventSendBuf[i].max = ev.second.getMax();
    eventSendBuf[i].min = ev.second.getMin();
    eventSendBuf[i].dataSize = ev.second.getData().size();
    eventSendBuf[i].stateChangesSize = ev.second.stateChanges.size();

    int packSize = 0, pSize = 0;
    // int packSize = sizeof(int) * ev.second.getData().size() +
      // sizeof(Event::StateChanges::value_type) * ev.second.stateChanges.size();
    MPI_Pack_size(ev.second.getData().size(), MPI_INT, comm, &pSize);
    packSize += pSize;
    MPI_Pack_size(ev.second.stateChanges.size() * sizeof(Event::StateChanges::value_type),
                  MPI_BYTE, comm, &pSize);
    packSize += pSize;

    packSendBuf[i] = std::unique_ptr<char[]>(new char[packSize]);
    int position = 0;
    // Pack data attached to an Event
    MPI_Pack(ev.second.getData().data(), ev.second.getData().size(),
             MPI_INT, packSendBuf[i].get(), packSize, &position, comm);
    // Pack state changes with associated time_points
    MPI_Pack(ev.second.stateChanges.data(),
             ev.second.stateChanges.size() * sizeof(Event::StateChanges::value_type),
             MPI_BYTE, packSendBuf[i].get(), packSize, &position, comm);

    MPI_Isend(&eventSendBuf[i], 1, MPI_EVENTDATA, 0, 0, comm, &req);
    requests.push_back(req);
    MPI_Isend(packSendBuf[i].get(), position, MPI_PACKED, 0, 0, comm, &req);
    requests.push_back(req);
    ++i;
  }

  // Receive
  if (rank == 0) {
    for (int i = 0; i < MPIsize; ++i) {
      for (int j = 0; j < eventsPerRank[i]; ++j) {
        MPI_EventData ev;
        MPI_Recv(&ev, 1, MPI_EVENTDATA, i, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);
        std::vector<int> recvData(ev.dataSize);
        Event::StateChanges recvStateChanges(ev.stateChangesSize);
        MPI_Status status;
        int packSize = 0, position = 0;
        MPI_Probe(i, MPI_ANY_TAG, comm, &status);
        MPI_Get_count(&status, MPI_PACKED, &packSize);
        char packBuffer[packSize];
        MPI_Recv(packBuffer, packSize, MPI_PACKED, i, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);
        MPI_Unpack(packBuffer, packSize, &position, recvData.data(), ev.dataSize, MPI_INT, comm);
        MPI_Unpack(packBuffer, packSize, &position, recvStateChanges.data(),
                   ev.stateChangesSize * sizeof(Event::StateChanges::value_type), MPI_BYTE, comm);

        globalEvents.emplace(std::piecewise_construct, std::forward_as_tuple(ev.name),
                             std::forward_as_tuple(ev.name, ev.rank, ev.count, ev.total, ev.max, ev.min,
                                                   recvData, recvStateChanges));

        EventData ed(ev.name, ev.rank, ev.count, ev.total, ev.max, ev.min, recvData, recvStateChanges);
        globalRankData[ev.rank].addEventData(std::move(ed));

      }
    }
  }
  MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);
  MPI_Type_free(&MPI_EVENTDATA);
}


size_t EventRegistry::getMaxNameWidth()
{
  size_t maxEventWidth = 0;
  for (auto & ev : localRankData.evData)
    if (ev.second.getName().size() > maxEventWidth)
      maxEventWidth = ev.second.getName().size();

  return maxEventWidth;
}
