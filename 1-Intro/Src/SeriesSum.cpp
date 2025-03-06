#include <iostream>
#include <mpi.h>
#include <numeric>
#include <unistd.h>
#include <vector>

#include "Common.hpp"

using IndT = unsigned long long;

static constexpr IndT BigNumber = std::numeric_limits<IndT>::max() >> 32;

double seriesTerm(IndT n) { return 1.0 / (n + 1); }

double calculateSeriesInterval(IndT offset, IndT count) {
  double sum = 0;
  for (IndT i = 0; i < count; ++i) {
    sum += seriesTerm(offset + i);
  }
  return sum;
}

int main(int argc, char **argv) {
  MPI::Init(argc, argv);
  Defer _{[] { MPI::Finalize(); }};

  IndT N = std::stoul(argv[argc - 1]);

  if (N == 0) N = BigNumber;
  else N--;

  IndT rank = MPI::COMM_WORLD.Get_rank();
  IndT size = MPI::COMM_WORLD.Get_size();

  MPILogger log{"log.txt", "Process #" + std::to_string(rank)};

  IndT len = N / size; // Couldn't come up with better name
  IndT start = len * rank;

  double sum = calculateSeriesInterval(start, len);
  IndT remStart = len * size;

  log << "[" << start << "-" << start + len << ")";

  if (remStart + rank <= N) {
    sum += seriesTerm(remStart + rank);
    log << ", {" << remStart + rank << "}";
  }

  log << MPILogger::endl;

  if (rank == 0) { // Master routine
    std::vector<MPI::Request> requests(size - 1);
    std::vector<double> data(size - 1, 0);

    for (IndT i = 0; i < size - 1; ++i)
      requests[i] =
          MPI::COMM_WORLD.Irecv(&data[i], 1, MPI::DOUBLE, i + 1, MPI::ANY_TAG);

    MPI::Request::Waitall(requests.size(), requests.data());

    sum = std::accumulate(data.begin(), data.end(), sum);
    std::cout << "Sum [1, " << N + 1 << "] = " << sum << std::endl;

  } else { // Slave routine
    MPI::COMM_WORLD.Send(&sum, 1, MPI::DOUBLE, 0, 42);
  }

  return 0;
}