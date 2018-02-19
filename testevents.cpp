#include <thread>
#include <iostream>
#include <random>
#include <mpi.h>
#include "EventTimings.hpp"

using std::cout;
using std::endl;


double rand(double min, double max) {
  std::random_device rd;
  std::uniform_real_distribution<double> dist(min, max);
  return dist(rd);
  // return 1;
}

void sleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void solve() {
  Event e("solve");
  sleep(rand(0.6, 1.4) * 8000.0);
  Event e2("zero time");
}

void advance() {
  auto & pe = EventRegistry::instance().getStoredEvent("Solver");
  cout << "Paused pe" << endl;
  pe.pause();
  Event e("advance");
  sleep(rand(0.8, 1.2) * 100.0);
  cout << "Started pe" << endl;
  pe.start();
}


void testevents() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // if (rank == 0 or rank == 2) {
  //   Event e("On rank 0 and 2");
  //   e.data.push_back(3); e.data.push_back(2); e.data.push_back(1);
  //   sleep(50);
  // }

  Event e1("Testevent");
  e1.data.push_back(rank * 5);  e1.data.push_back(rank * 7); e1.data.push_back(rank * 8);
  
  sleep(100 * (rank+1));
  e1.stop();

  e1.start();
  sleep(150 * (rank+1));
  e1.stop();

  Event e2("Anothertestevent");
  sleep(500 * (rank+1));
  e2.stop();

  // Event e3("Paused Event");
  // sleep(10);
  // e3.pause();
  // sleep(100); // should not count
  // e3.start();
  // sleep(10);
  // e3.stop();

  // // Solver loop
  // for (int i = 0; i < 3; ++i) {
  //   solve();
  //   advance();
  // }
  
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  EventRegistry::instance().initialize();

  testevents();
  
  EventRegistry::instance().finalize();
  EventRegistry::instance().printAll();
  MPI_Finalize();
}
