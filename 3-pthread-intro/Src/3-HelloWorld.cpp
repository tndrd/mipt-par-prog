#include <pthread.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

struct ThreadArg {
  size_t Id;
  size_t NWorkers;
};

void* ThreadRoutine(void* argptr) {
  assert(argptr);

  ThreadArg arg = *reinterpret_cast<ThreadArg*>(argptr);

  std::cout << "tid " << pthread_self() << ": id " << arg.Id << "/"
            << arg.NWorkers << std::endl;

  return nullptr;
}

struct ThreadDeleter {
  void operator()(pthread_t* ptr) noexcept {
    assert(ptr);
    int ret = pthread_join(*ptr, NULL);
    delete ptr;

    if (ret != 0) std::cerr << "Failed to join: " << strerror(ret) << std::endl;
  }
};

using ThreadPtr = std::unique_ptr<pthread_t, ThreadDeleter>;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: ./3-HelloWorld <NWORKERS>" << std::endl;
    return 1;
  }

  size_t nWorkers = std::stoul(argv[1]);

  std::vector<ThreadPtr> threads(nWorkers);
  std::vector<ThreadArg> args (nWorkers);

  for (size_t i = 0; i < nWorkers; ++i) {
    args[i].Id = i;
    args[i].NWorkers = nWorkers;

    pthread_t newThread;
    int ret = pthread_create(&newThread, NULL, ThreadRoutine, &args[i]);
    if (ret != 0)
      throw std::runtime_error(std::string("Failed to create thread: ") +
                               strerror(ret));

    threads[i] = ThreadPtr{new pthread_t{newThread}};
  }
}