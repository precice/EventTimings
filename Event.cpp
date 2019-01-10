#include "Event.hpp"
#include "EventUtils.hpp"

Event::Event(std::string eventName, Clock::duration initialDuration)
  : name(EventRegistry::instance().prefix + eventName),
    duration(initialDuration)
{
  EventRegistry::instance().put(this);
}

Event::Event(std::string eventName, bool barrier, bool autostart)
  : name(eventName),
    _barrier(barrier)
{
  // Set prefix here: workaround to omit data lock between instance() and Event ctor
  if (eventName != "_GLOBAL")
    name = EventRegistry::instance().prefix + eventName;
  if (autostart) {
    start(_barrier);
  }
}

Event::~Event()
{
  stop(_barrier);
}

void Event::start(bool barrier)
{
  if (barrier)
    MPI_Barrier(EventRegistry::instance().getMPIComm());

  state = State::STARTED;
  stateChanges.push_back(std::make_tuple(State::STARTED, Clock::now()));
  starttime = Clock::now();
}

void Event::stop(bool barrier)
{
  if (state == State::STARTED or state == State::PAUSED) {
    if (barrier)
      MPI_Barrier(EventRegistry::instance().getMPIComm());

    if (state == State::STARTED) {
      auto stoptime = Clock::now();
      duration += Clock::duration(stoptime - starttime);
    }
    stateChanges.push_back(std::make_tuple(State::STOPPED, Clock::now()));
    state = State::STOPPED;
    EventRegistry::instance().put(this);
    data.clear();
    stateChanges.clear();
    duration = Clock::duration::zero();
  }
}

void Event::pause(bool barrier)
{
  if (state == State::STARTED) {
    if (barrier)
      MPI_Barrier(EventRegistry::instance().getMPIComm());

    auto stoptime = Clock::now();
    stateChanges.push_back(std::make_tuple(State::PAUSED, Clock::now()));
    state = State::PAUSED;
    duration += Clock::duration(stoptime - starttime);
  }
}

Event::Clock::duration Event::getDuration()
{
  return duration;
}