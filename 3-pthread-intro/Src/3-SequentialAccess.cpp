#include <pthread.h>
#include <signal.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <string>


struct ThreadArg {
  pthread_mutex_t* Mutex;
  pthread_cond_t* WakeUpCond;
  pthread_cond_t* FinishedCond;
  bool* WakeUpFlag;
  bool* FinishedFlag;

  size_t* Value;
  size_t SelfId;
};

std::string ErrorString(const char* name, size_t line, int err) {
  return std::string(name) + " at line " + std::to_string(line) + ": " +
         strerror(err);
}

template <typename F, typename... Args>
void PthreadCall(F func, const char* name, size_t line, Args... args) {
  int ret = func(args...);
  if (ret != 0) throw std::runtime_error(ErrorString(name, line, ret));
}

template <typename F, typename... Args>
int PthreadCallNoexcept(F func, const char* name, size_t line, Args... args) {
  int ret = func(args...);
  if (ret != 0) std::cerr << ErrorString(name, line, ret) << std::endl;
  return ret;
}

#define PCALL(func, ...) PthreadCall(func, #func, __LINE__, __VA_ARGS__)
#define PCALL_NOEXCEPT(func, ...) \
  PthreadCallNoexcept(func, #func, __LINE__, __VA_ARGS__)

void* ThreadRoutine(void* argptr) {
  assert(argptr);
  ThreadArg arg = *reinterpret_cast<ThreadArg*>(argptr);
  assert(arg.WakeUpCond);
  assert(arg.FinishedCond);
  assert(arg.WakeUpFlag);
  assert(arg.FinishedFlag);
  assert(arg.Mutex);
  assert(arg.Value);

  PCALL(pthread_mutex_lock, arg.Mutex);
  while (!(*arg.WakeUpFlag)) PCALL(pthread_cond_wait, arg.WakeUpCond, arg.Mutex);

  std::cout << "Thread " << arg.SelfId << ": " << ((*arg.Value) *= 2) << std::endl;
  *arg.FinishedFlag = true;
  PCALL(pthread_cond_signal, arg.FinishedCond);
  PCALL(pthread_mutex_unlock, arg.Mutex);

  return nullptr;
}

struct MutexDeleter {
  void operator()(pthread_mutex_t* ptr) noexcept {
    if (!ptr) return;
    PCALL_NOEXCEPT(pthread_mutex_destroy, ptr);
    delete ptr;
  }
};

struct CondDeleter {
  void operator()(pthread_cond_t* ptr) noexcept {
    if (!ptr) return;
    PCALL_NOEXCEPT(pthread_cond_destroy, ptr);
    delete ptr;
  }
};

struct ThreadDeleter {
  void operator()(pthread_t* ptr) noexcept {
    if (!ptr) return;
    DeleteStrategy(*ptr);
    delete ptr;
  }

  static void DeleteStrategy(const pthread_t & tid) {
    int ret = PCALL_NOEXCEPT(pthread_tryjoin_np, tid, nullptr);
    if (ret != EBUSY) return;

    PCALL_NOEXCEPT(pthread_kill, tid, SIGKILL);
    PCALL_NOEXCEPT(pthread_join, tid, nullptr);    
  }
};

using MutexPtr = std::unique_ptr<pthread_mutex_t, MutexDeleter>;
using CondPtr = std::unique_ptr<pthread_cond_t, CondDeleter>;
using ThreadPtr = std::unique_ptr<pthread_t, ThreadDeleter>;

std::vector<CondPtr> MakeCondVec(size_t count) {
  std::vector<CondPtr> cVec(count);

  for (CondPtr& cond : cVec) {
    CondPtr newCond{new pthread_cond_t(PTHREAD_COND_INITIALIZER)};
    cond = std::move(newCond);
  }

  return cVec;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: ./3-SequentialAccess <NTHREADS>" << std::endl;
    return 1;
  }

  size_t nThreads = std::stoul(argv[1]);
  size_t value = 1;

  MutexPtr mutex{new pthread_mutex_t(PTHREAD_MUTEX_INITIALIZER)};

  std::vector<CondPtr> wakeUpConds = MakeCondVec(nThreads + 1);
  std::basic_string<bool> wakeUpFlags (nThreads + 1, false);

  std::vector<ThreadPtr> threads (nThreads);
  std::vector<ThreadArg> args (nThreads);

  for (int i = 0; i < nThreads; ++i) {
    args[i].Mutex = mutex.get();
    args[i].SelfId = i;
    args[i].Value = &value;
    
    args[i].WakeUpCond = wakeUpConds[i].get();
    args[i].WakeUpFlag = &wakeUpFlags[i];

    args[i].FinishedCond = wakeUpConds[i + 1].get();
    args[i].FinishedFlag = &wakeUpFlags[i + 1];

    ThreadPtr newThread {new pthread_t};
    PCALL(pthread_create, newThread.get(), nullptr, ThreadRoutine, &args[i]);
    threads[i] = std::move(newThread);
  }

  PCALL(pthread_mutex_lock, mutex.get());
  wakeUpFlags[0] = true;
  PCALL(pthread_cond_signal, wakeUpConds[0].get());

  while(!(wakeUpFlags[nThreads]))
    PCALL(pthread_cond_wait, wakeUpConds[nThreads].get(), mutex.get());
  
  std::cout << "Last thread finished, value = " << value << std::endl;

  PCALL(pthread_mutex_unlock, mutex.get());
}