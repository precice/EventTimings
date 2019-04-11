# EventTimings -- A Framework to measure events in an MPI environment

## Usage

### Initialization
Initalize the timing framework at program start:
```
EventRegistry.instance().initialize("applicationName");
```
finalize the same way
```
EventRegistry.instance().finalize();
```
`"applicationName"` is optional and is used for naming the output files.

### Timings
To start timing, simply instantiate an `Event` object.
```
Event e1("My first event");
```
it is autostarted and stopped on destruction. It can be explicitely stopped or paused
```
e1.pause();
// do some non-timed stuff
e1.start();
// time stuff
e1.stop(); // Finish the event
```
Stopping and restarting the `Event` counts as two events, whereas pausing simply stops and timing.
An `Event` can be instantiated in a stopped state:
```
Event e2("Another event", false, false);
```
which calls
```
Event::Event(std::string eventName, bool barrier, bool autostart);
```
`barrier` sets calling of `MPI_Barrier` when implicitly starting or stopping an event, that is if the `Event` is started / stopped from the constructor / destructor.
When you explictly start an `Event` and want to invoke a barrier use:
```
e2.start(true);
e2.stop(true);
```
The barrier can be used to synchronize measurements across the MPI communicator.

If you don't want an `Event` to be stopped when it goes out of scope, you can retrieve a so called stored `Event`
```
EventRegistry.instance().getStoredEvent("A stored Event");
```
it needs to be started and stopped explicitly.

### Ataching data to Events
You can attach named data to an Event:
```
Event e1("Testevent");
e1.addData("IterationCount", iterations);
```
Currently, only integer data is supported. It can be used to store iterations or residuals. The data is collected for each Event and printed with the timings.

### Reporting
After calling `finalize`, a report can be printed to `stdout`
```
EventRegistry::instance().printAll();
```
it also creates or appends to two files `applicationName-eventTimings.log` which contains aggregated timing information and `applicationName-events.log`, which logs all state changes of Events and is used by auxiliary scripts for plotting or further statistical insights. 

## Reporting Scripts
### Transform Events to the trace format
`events2trace.py` can combine arbitrary `applicationName-events.json` files and output a JSON file in the trace format.
The chromium trace tool `chrome://tracing` can read and display this format. [Read more](events2trace.md)

