#pragma once

#include <iostream>
#include <mpi.h>
#include <sstream>
#include <stdexcept>

template <typename F> class Defer final {
private:
  F Action;

public:
  explicit Defer(const F &action) noexcept : Action{action} {}
  explicit Defer(F &&action) noexcept : Action{std::move(action)} {}

  Defer(const Defer &) = delete;
  Defer &operator=(const Defer &) = delete;

  Defer(Defer &&) = delete;
  Defer &operator=(Defer &&) = delete;

  ~Defer() noexcept {
    try {
      Action();
    } catch (std::exception &e) {
      std::cerr << "Native exception on deferred call: " << e.what()
                << std::endl;
    } catch (MPI::Exception &e) { // IDK why MPI::Exception is not derived from
                                  // std::exception
      std::cerr << "MPI Exception on deferred call: " << e.Get_error_string()
                << std::endl;
    } catch (...) {
      std::cerr << "Unknown exception on deferred call" << std::endl;
    }
  }
};

class MPILogger final {
private:
  MPI::File File;
  std::string User;
  std::stringstream Stream;

  static MPI::File MakeFile(const std::string &fname,
                            const MPI::Intracomm &comm) {
    auto f = MPI::File::Open(comm, fname.c_str(),
                             MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI::INFO_NULL);
    f.Set_atomicity(true);
    return f;
  }

public:
  MPILogger(const std::string &fname, const std::string &user,
            const MPI::Intracomm &comm = MPI::COMM_WORLD)
      : User{user}, Stream{}, File{MakeFile(fname, comm)} {
    File.Set_size(0); // Truncate log file
  }

  template <typename T> MPILogger &operator<<(const T &rhs) {
    Stream << rhs;
    return *this;
  }

  // I didn't manage to make use of native endl :(
  static constexpr struct EndlT {
  } endl{};

  MPILogger &operator<<(const EndlT &) {
    std::string str = "[" + User + "]: ";
    str += Stream.str() + "\n";
    Stream.str("");
  
    File.Write_shared(str.c_str(), str.size(), MPI::CHARACTER);
    return *this;
  }
};