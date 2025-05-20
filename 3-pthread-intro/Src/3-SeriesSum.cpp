#include <pthread.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

struct ThreadArg {
  size_t Start;
  size_t Count;
  double (*Function)(size_t);
  double* Dst;
};

struct ThreadDeleter {
  void operator()(pthread_t* ptr) noexcept {
    if (!ptr) return;

    int ret = pthread_join(*ptr, NULL);
    delete ptr;

    if (ret != 0) std::cerr << "Failed to join: " << strerror(ret) << std::endl;
  }
};

using ThreadPtr = std::unique_ptr<pthread_t, ThreadDeleter>;

void* ThreadRoutine(void* argptr) {
  assert(argptr);
  ThreadArg arg = *reinterpret_cast<ThreadArg*>(argptr);
  assert(arg.Function);
  assert(arg.Dst);

  double sum = 0;

  for (size_t i = 0; i < arg.Count; ++i) sum += arg.Function(arg.Start + i);

  *arg.Dst = sum;

  return nullptr;
}

double function(size_t N) { return 1. / (N + 1); }

void PrintResult(double sum, size_t nTerms) {
  std::cout << "Sum [1, " << nTerms << "] = " << sum << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: ./3-SeriesSum <NTERMS> <NWORKERS>" << std::endl;
    return 1;
  }

  size_t nTerms = std::stoul(argv[1]);
  size_t nWorkers = std::stoul(argv[2]);

  if (nWorkers == 0) throw std::runtime_error("nWorkers should be non zero");

  if (nWorkers > nTerms) {
    std::cout << "Warning: too many workers, truncating" << std::endl;
    nWorkers = nTerms;
  }

  if (nWorkers == 1) {
    double sum = 0;
    for (size_t i = 0; i < nTerms; ++i)
      sum += function(i);
    PrintResult(sum, nTerms);
    return 0;
  }

  std::vector<ThreadPtr> threads(nWorkers);
  std::vector<ThreadArg> args(nWorkers);
  std::vector<double> results(nWorkers, 1e6);

  size_t blockSize = nTerms / nWorkers;
  assert(blockSize != 0);

  for (int i = 0; i < nWorkers; ++i) {
    args[i].Function = function;
    args[i].Start = i * blockSize;
    args[i].Count = blockSize;
    args[i].Dst = &results[i];

    pthread_t newThread;
    int ret = pthread_create(&newThread, NULL, ThreadRoutine, &args[i]);
    if (ret != 0)
      throw std::runtime_error(std::string("Failed to create thread: ") +
                               strerror(ret));

    threads[i] = ThreadPtr{new pthread_t{newThread}};
  }

  // joining
  threads.clear();

  double sum = 0;
  for (size_t i = 0; i < nTerms % blockSize; ++i)
    sum += function(nTerms - i - 1);

  sum = std::accumulate(results.begin(), results.end(), sum);
  PrintResult(sum, nTerms);
}