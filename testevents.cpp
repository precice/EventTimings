#include <thread>
#include <iostream>
#include <mpi.h>
#include "EventTimings.hpp"

using std::cout;
using std::endl;


int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  EventRegistry::initialize();

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  Event e1("Testevent");
  e1.data.push_back(5);  e1.data.push_back(7); e1.data.push_back(8);
  
  std::this_thread::sleep_for(std::chrono::milliseconds(100 * (rank+1)));
  e1.stop();

  e1.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(150 * (rank+1)));
  e1.stop();

  Event e2("Anothertestevent");
  std::this_thread::sleep_for(std::chrono::milliseconds(50 * (rank+1)));
  e2.stop();

  Event e3("Paused Event");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  e3.pause();
  std::this_thread::sleep_for(std::chrono::milliseconds(500)); // should not count
  e3.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  e3.stop();

  EventRegistry::finalize();
  EventRegistry::printAll();
  MPI_Finalize();
}
