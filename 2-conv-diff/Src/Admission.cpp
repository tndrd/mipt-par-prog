#include "Common.hpp"
#include <mpi.h>
#include <time.h>

timespec GetTimespec(clockid_t cid = CLOCK_REALTIME) {
  timespec ts = {};
  if (clock_gettime(cid, &ts) < 0)
    throw std::runtime_error(std::string("clock_gettime: ") + strerror(errno));
  return ts;
}

// ts2 >= ts1
timespec GetTimeDiff(timespec ts1, timespec ts2) {
  timespec tsd = {};
  if (ts1.tv_sec > ts2.tv_sec ||
      (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec > ts2.tv_nsec))
    throw std::runtime_error("GetTimeDiff: wrong order");

  tsd.tv_sec = ts2.tv_sec - ts1.tv_sec;
  tsd.tv_nsec = ts2.tv_nsec - ts1.tv_nsec;

  if (tsd.tv_nsec < 0) {
    tsd.tv_sec--;
    tsd.tv_nsec += 1000000000;
  }

  return tsd;
}

double Timespec2Ms(timespec ts) {
  double ms = ts.tv_sec * 1000 + float(ts.tv_nsec) / (1000 * 1000);
  return ms;
}

void TxRoutine(double &timeval, MPILogger &log, size_t nsends) {
  for (size_t i = 0; i < nsends; ++i) {
    auto start = GetTimespec();
    int dummy = 0;
    MPI::COMM_WORLD.Send(&dummy, 1, MPI::INT, 1, 42);
    auto end = GetTimespec();

    auto timems = Timespec2Ms(GetTimeDiff(start, end));
    //log << timems << MPILogger::endl;

    timeval += timems;
  }
}

void RxRoutine(size_t nsends) {
  for (size_t i = 0; i < nsends; ++i) {
    int dummy = 0;
    MPI::COMM_WORLD.Recv(&dummy, 1, MPI::INT, 0, MPI::ANY_TAG);
  }
}

int main(int argc, char *argv[]) try {
  MPI::Init(argc, argv);
  Defer _([] { MPI::Finalize(); });

  MPILogger log("log.txt", "Master");

  size_t nreps = std::stoul(argv[argc - 2]);
  size_t nsends = std::stoul(argv[argc - 1]);

  const int csize = MPI::COMM_WORLD.Get_size();
  const int crank = MPI::COMM_WORLD.Get_rank();

  if (csize != 2)
    throw std::runtime_error("NP should be equal to 2");

  double time = 0;

  for (size_t i = 0; i < nreps; ++i) {
    if (crank == 0)
      TxRoutine(time, log, nsends);
    if (crank == 1)
      RxRoutine(nsends);
  }

  if (crank == 1)
    return 0;

  std::cout << "Avg wall time for Send: " << 1000 * time / (nreps * nsends) << "us" << std::endl;

  return 0;
} catch (std::exception &e) {
  std::cerr << "std::exception: " << e.what() << std::endl;
} catch (MPI::Exception &e) {
  std::cerr << "MPI::Exception: " << e.Get_error_string() << std::endl;
}