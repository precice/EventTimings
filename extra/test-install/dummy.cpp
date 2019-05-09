#include<EventTimings/EventUtils.hpp>
#include<EventTimings/Event.hpp>
#include<mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    EventRegistry::instance().initialize();
    EventRegistry::instance().finalize();
    EventRegistry::instance().printAll();
    MPI_Finalize();
}
