#include <cassert>
#include <iostream>
#include <mpi.h>
#include <unistd.h>

#include "Common.hpp"

int main(int argc, char **argv) {
  MPI::Init(argc, argv);
  Defer _{[] { MPI::Finalize(); }};

  size_t size = MPI::COMM_WORLD.Get_size();
  size_t rank = MPI::COMM_WORLD.Get_rank();

  MPILogger log{"log.txt", "Process #" + std::to_string(rank)};

  int i = 1;

  if (rank == 0) { // Master routine

    int next = size > 1 ? 1 : 0;
    int last = size > 1 ? size - 1 : 0;

    log << "Sending \"" << i << "\" to #" << next << MPILogger::endl;
  
    // Need to use Send, not Ssend
    MPI::COMM_WORLD.Send(&i, 1, MPI::INT, next, 42);
    MPI::COMM_WORLD.Recv(&i, 1, MPI::INT, last, MPI::ANY_TAG);
    
    log << "Received \"" << i << "\" from #" << last << MPILogger::endl;
    assert(i == size);
  } else { // Slave routine
    assert(size > 1);
    int next = (rank + 1) % size;
    int prev = rank - 1;

    MPI::COMM_WORLD.Recv(&i, 1, MPI::INT, prev, MPI::ANY_TAG);

    log << "Received \"" << i << "\" from #" << prev << MPILogger::endl;
    i++;
    log << "Sending \"" << i << "\" to #" << next << MPILogger::endl;

    MPI::COMM_WORLD.Send(&i, 1, MPI::INT, next, 42);
  }
}