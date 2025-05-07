#pragma once
#include <Common.hpp>
#include <LayerSolver.hpp>
#include <Methods.hpp>
#include <cassert>
#include <fstream>

template <typename F, typename Fx0T, typename Ft0T> struct ProblemConfig {
  struct {
    DataT X;
    DataT T;
  } Borders;

  struct {
    DataT H;
    DataT T;
  } Steps;

  struct {
    Fx0T Fx0;
    Ft0T Ft0;
  } BoundCond;

  struct {
    DataT A;
    F Func;
  } Problem;
};

template <typename F, typename Fx0T, typename Ft0T>
auto CreateProblem(F func, Fx0T fx0, Ft0T ft0) {
  auto problem = ProblemConfig<F, Fx0T, Ft0T>{};
  problem.BoundCond.Ft0 = ft0;
  problem.BoundCond.Fx0 = fx0;
  problem.Problem.Func = func;

  return problem;
}

class MPISolver {
private:
  class MPIGetter final : public GetStrategy {
  private:
    int Src;
    std::vector<DataT> Buf;
    int Left;
    int Filled;

  public:
    MPIGetter(int src, size_t bufSz = 1)
        : Src{src}, Buf(bufSz, 0), Left{0}, Filled{0} {
      assert(bufSz != 0);
    }

    DataT Get() override {
      if (Left == 0) {
        MPI::Status status;
        MPI::COMM_WORLD.Recv(Buf.data(), Buf.size(), MPI::DOUBLE, Src,
                             MPI::ANY_TAG, status);
        // std::cout << "Received" << status.Get_count(MPI::DOUBLE) <<
        // std::endl;
        Filled = status.Get_count(MPI::DOUBLE);
        Left = Filled;
      }

      return Buf[Filled - (Left--)];
    }
  };

  class MPIPutter final : public PutStrategy {
  private:
    int Dst;
    std::vector<DataT> Buf;
    int Filled;

  public:
    MPIPutter(int dst, size_t bufSz = 1) : Dst{dst}, Buf(bufSz, 0), Filled{0} {
      assert(bufSz != 0);
    }

    void Put(DataT value) override {
      Buf[Filled++] = value;

      if (Filled == Buf.size()) {
        MPI::COMM_WORLD.Send(Buf.data(), Buf.size(), MPI::DOUBLE, Dst, 42);
        Filled = 0;
      }
    }
  };

  class Output {
  private:
    std::ofstream out;
    bool Active;

  private:
    static std::string GenFName(const std::string &prefix, int rank) {
      return prefix + "-" + std::to_string(rank) + ".txt";
    }

  public:
    Output(const std::string &name, int rank, bool active = true)
        : Active{active} {
      if (!active)
        return;
      out.open(GenFName(name, rank));
    }

    void PutHeader(DataT h, DataT t, size_t nLayers, size_t layerSize) {
      if (!Active)
        return;

      out << h << " " << t << std::endl;
      out << nLayers << " " << layerSize << std::endl;
    }

    void PutLine(int k, const DataBufT &layer) {
      if (!Active)
        return;

      out << k << " ";
      for (auto x : layer)
        out << x << " ";
      out << std::endl;
    }
  };

  static int mod(int a, int b) {
    assert(b >= 0);
    if (a > 0)
      return a % b;
    else
      return (b - ((-a) % b)) % b;
  }

public:
  static constexpr size_t DefaultBufferSize = 8;

public:
  struct SolverConfig {
    bool Write = false;
    std::string Name = "out";
    size_t BufferSize = DefaultBufferSize;
  };

  template <template <typename> typename Method, typename F, typename Fx0T,
            typename Ft0T>
  static void Participate(ProblemConfig<F, Fx0T, Ft0T> problem,
                          const SolverConfig &config = {}) {
    MPI::Init();
    Defer _{[] { MPI::Finalize(); }};

    int commSize = MPI::COMM_WORLD.Get_size();
    int selfRank = MPI::COMM_WORLD.Get_rank();

    size_t nLayers = problem.Borders.T / problem.Steps.T;
    size_t layerSize = problem.Borders.X / problem.Steps.H;

    Output out(config.Name, selfRank, config.Write);
    out.PutHeader(problem.Steps.H, problem.Steps.T, nLayers + 1, layerSize);

    DataBufT layerBuf(layerSize, 0);
    DataBufT auxBuf = layerBuf;

    auto create_method = [&](size_t k) {
      return std::make_unique<Method<F>>(problem.Problem.A, problem.Steps.T,
                                         problem.Steps.H, k,
                                         problem.Problem.Func);
    };

    if (selfRank == 0) { // Master
      for (int i = 0; i < layerSize; ++i)
        auxBuf[i] = problem.BoundCond.Ft0(i * problem.Steps.H);

      out.PutLine(0, auxBuf);
    }

    std::unique_ptr<GetStrategy> getter;
    std::unique_ptr<PutStrategy> putter;

    size_t nsteps = nLayers / commSize;

    for (int i = 0; i < nsteps; ++i) {
      if (selfRank == 0)
        std::cout << i << " / " << nsteps << "\r";

      size_t k = i * commSize + selfRank + 1;
      if (k > nLayers)
        break;

      int left = mod(-i, commSize);
      int right = (i != nsteps) ? mod(left - 1, commSize)
                                : mod(left - 1 + nLayers % commSize, commSize);

      int prev = mod(selfRank - 1, commSize);
      int next = mod(selfRank + 1, commSize);

      if (selfRank == left)
        getter = std::move(std::make_unique<InputGet>(auxBuf));
      else
        getter = std::move(std::make_unique<MPIGetter>(prev, config.BufferSize));

      if (selfRank == right)
        putter = std::move(std::make_unique<DummyPut>());
      else
        putter = std::move(std::make_unique<MPIPutter>(next, config.BufferSize));

      auto method = create_method(k);
      size_t lstride = method->GetLStride();
      size_t rstride = method->GetRStride();

      layerBuf[0] = problem.BoundCond.Fx0(k * problem.Steps.T);
      LayerSolver solver(std::move(method), std::move(getter),
                         std::move(putter));

      solver.Process(layerBuf, lstride, layerSize - rstride);
      out.PutLine(k, layerBuf);

      std::swap(auxBuf, layerBuf);
    }

    if (selfRank == 0) std::cout << std::endl;
  }
};