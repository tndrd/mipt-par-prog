#pragma once

#include <AdaptiveGrid.hpp>
#include <cstring>
#include <mutex>
#include <numeric>
#include <thread>

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

double IntegratePart(AdaptiveGrid::FuncT func, AdaptiveGrid::FuncT funcd,
                     const AdaptiveGrid::HeterogenousPartition& part) {
  double sum = 0;
  double pos = part.Start;

  for (const auto& interval : part.Parts) {
    double h = interval.Step;
    double fsum = 0;
    double fdsum = 0;

    for (int i = 0; i < interval.NSteps; ++i) {
      double x = pos + i * h;
      fsum += h * func(x);
      fdsum += (h * h / 2) * funcd(x);
    }

    pos += interval.NSteps * h;
    sum += fsum + fdsum;
  }

  return sum;
}

void IntegratePartWrapper(AdaptiveGrid::FuncT func, AdaptiveGrid::FuncT funcd,
                          const AdaptiveGrid::HeterogenousPartition& part,
                          double& result, std::mutex& mutex) {
  auto start = GetTimespec();
  result = IntegratePart(func, funcd, part);
  auto end = GetTimespec();

  auto diff = GetTimeDiff(start, end);
  double ms = Timespec2Ms(diff);

  std::lock_guard<std::mutex> _(mutex);

  std::cout << "Thread @" << part.Start << " finished in "
            << round(ms / 100) / 10 << "s" << std::endl;
}

double IntegrateParallel(AdaptiveGrid::FuncT func, AdaptiveGrid::FuncT funcd,
                         const AdaptiveGrid& grid, size_t nWorkers) {
  std::vector<double> results(nWorkers, 42);
  std::vector<std::thread> workers;
  std::mutex mutex;

  for (int i = 0; i < nWorkers; ++i)
    workers.emplace_back(IntegratePartWrapper, func, funcd,
                         std::cref(grid.Partitions[i]), std::ref(results[i]), std::ref(mutex));

  for (auto& worker : workers) worker.join();

  double sum = 0;
  for (auto res : results) sum += res;

  return sum;
}

struct IntegrateArgs {
  AdaptiveGrid::FuncT Func;
  AdaptiveGrid::FuncT FuncD;
  AdaptiveGrid::StepEvalT StepEval;

  double A;
  double B;
  double Prec;

  double GridResolution = 1;
};

double Integrate(const IntegrateArgs& args, size_t nWorkers,
                 bool dumpGrid = false) {
  assert(nWorkers != 0);

  auto grid = AdaptiveGrid::Create(args.StepEval, args.Func, args.A, args.B,
                                   args.Prec, nWorkers, args.GridResolution);

  if (dumpGrid) grid.Dump();

  assert(grid.Partitions.size() == nWorkers);

  if (nWorkers == 1)
    return IntegratePart(args.Func, args.FuncD, grid.Partitions[0]);

  return IntegrateParallel(args.Func, args.FuncD, grid, nWorkers);
}
