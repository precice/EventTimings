#include<EventTimings/EventUtils.hpp>
#include<EventTimings/Event.hpp>
#include<mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    auto& registry = EventTimings::EventRegistry::instance();
    registry.initialize();
    registry.finalize();
    registry.printAll();
    MPI_Finalize();
}
