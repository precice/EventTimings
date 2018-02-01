#pragma once

#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <mpi.h>


/// Represents an event that can be started and stopped.
/** Additionally to the duration there is a special property that can be set for a event.
A property is a a key-value pair with a numerical value that can be used to trace certain events,
like MPI calls in an event. It is intended to be set by the user. */
class Event
{
public:
  /// Default clock type. All other chrono types are derived from it.
  using Clock = std::chrono::steady_clock;

  /// Name used to identify the timer. Events of the same name are accumulated to
  std::string name;

  /// Allows to put a non-measured (i.e. with a given duration) Event to the measurements.
  Event(std::string eventName, Clock::duration initialDuration);

  /// Creates a new event and starts it, unless autostart = false, synchronize processes, when barrier == true
  /** Use barrier == true with caution, as it can lead to deadlocks. */
  Event(std::string eventName, bool barrier = false, bool autostart = true);

  /// Stops the event if it's running and report its times to the EventRegistry
  ~Event();

  /// Starts an event. If it's already started it has no effect.
  void start(bool barrier = false);

  /// Stops an event and commit it. If it's already stopped it has no effect.
  void stop(bool barrier = false);

  /// Pauses an event, does not commit. If it's already paused it has no effect.
  void pause(bool barrier = false);

  /// Gets the duration of the event.
  Clock::duration getDuration();

  std::vector<int> data;

private:
  
  enum class State {
    STOPPED,
    RUNNING,
    PAUSED      
  };
  
  Clock::time_point starttime;
  // Clock::time_point stoptime;
  Clock::duration duration = Clock::duration::zero();
  State state = State::STOPPED;
  bool _barrier = false;

};



/// Class that aggregates durations for a specific event.
class EventData
{
public:
  // Do not add explicit here, it fails on some (older?) compilers
  EventData(std::string _name);
  
  EventData(std::string _name, int _rank, long _count, long _total,
            long _max, long _min, std::vector<int> _data);
  
  /// Adds an Events data.
  void put(Event* event);

  std::string getName() const;

  /// Get the average duration of all events so far.
  long getAvg() const;

  /// Get the maximum duration of all events so far
  long getMax() const;

  /// Get the minimum duration of all events so far
  long getMin() const;

  /// Get the total duration of all events so far
  long getTotal() const;

  /// Get the number of all events so far
  long getCount() const;

  /// Get the time percentage that the total time of this event took in relation to the global duration.
  int getTimePercentage() const;

  const std::vector<int> & getData() const;

  void print(std::ostream &out);

  void writeCSV(std::ostream &out);

  Event::Clock::duration max = Event::Clock::duration::min();
  Event::Clock::duration min = Event::Clock::duration::max();
  int rank;
  
private:
  std::string name;
  long count = 0;
  Event::Clock::duration total = Event::Clock::duration::zero();
  std::vector<int> data;
};

/// Holds data aggregated from all MPI ranks for one event
struct GlobalEventStats
{
  // std::string name;
  int maxRank, minRank;
  Event::Clock::duration max   = Event::Clock::duration::min();
  Event::Clock::duration min   = Event::Clock::duration::max();  
};


using GlobalEvents = std::multimap<std::string, EventData>;

/// High level object that stores data of all events.
/** Call EventRegistry::intialize at the beginning of your application and
EventRegistry::finalize at the end. Event timings will be usuable without calling this
function at all, but global timings as well as percentages do not work this way.  */
class EventRegistry
{
public:
    
  /// Sets the global start time
  /**
   * @param[in] applicationName A name that is added to the logfile to distinguish different participants
   */
  static void initialize(std::string appName = "");

  /// Sets the global end time
  static void finalize();

  /// Clears the registry. needed for tests
  static void clear();

  /// Finalizes the timings and calls print. Can be used as a crash handler to still get some timing results.
  static void signal_handler(int signal);

  /// Records the event.
  static void put(Event* event);

  /// Returns the timestamp of the run, i.e. when the run finished
  static std::chrono::system_clock::time_point getTimestamp();
  
  /// Returns the duration of the run in ms
  /***
   * @pre Requires finalize to be called before
   */
  static Event::Clock::duration getDuration();

  static void collect();

  /// Prints a verbose report to stdout and a terse one to EventTimings-AppName.log
  static void printAll();

  /// Prints the result table to an arbitrary stream.
  /** terse enables a more machine readable format with one event per line, seperated by whitespace. */
  static void print(std::ostream &out);

  /// Convenience function: Prints to std::cout
  static void print();

  static void writeCSV(std::string filename);

  static void printGlobalStats();

private:
  static bool initialized;
  static Event::Clock::time_point starttime;
  static Event::Clock::duration duration;
  static std::chrono::system_clock::time_point timestamp; // Timestamp when the run finished

  /// Map of name -> events for this rank only
  static std::map<std::string, EventData> events;

  /// Multimap of name -> EventData of events for all ranks
  static GlobalEvents globalEvents;

  /// A name that is added to the logfile to distinguish different participants
  static std::string applicationName;

  static MPI_Datatype MPI_EVENTDATA;
};


double toMs(Event::Clock::duration duration);
