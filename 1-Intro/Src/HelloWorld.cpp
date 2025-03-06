#include <iostream>
#include <mpi.h>

#include "Common.hpp"

int main(int argc, char **argv) {
  MPI::Init(argc, argv);
  Defer _{[] { MPI::Finalize(); }};

  int size = MPI::COMM_WORLD.Get_size();
  int rank = MPI::COMM_WORLD.Get_rank();

  std::cout << "Size: " << size << ", rank: " << rank << std::endl;

  return 0;
}