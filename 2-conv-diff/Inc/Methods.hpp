#pragma once
#include <LayerSolver.hpp>

template <typename F> class IPDEMethod : public IMethod {
public:
  using Ptr = std::unique_ptr<IPDEMethod<F>>;

protected:
  DataT Tau;
  DataT H;
  DataT A;
  size_t K;
  F Func;

public:
  IPDEMethod(DataT a, DataT t, DataT h, size_t k, F func)
      : IMethod{}, A{a}, Tau{t}, H{h}, Func{func}, K{k} {}
  virtual ~IPDEMethod() = default;
};

// Explicit left corner:
//       ^
//       |
//   <--->
// Method formula:
//   u^{k+1}_m = (1-at/h)u^k_m + (at/h)u^k_{m-1} + tf^k_m
template <typename F> class LCornerMethod final : public IPDEMethod<F> {
public:
  using Ptr = std::unique_ptr<LCornerMethod<F>>;

private:
  using IPDEMethod<F>::Tau;
  using IPDEMethod<F>::H;
  using IPDEMethod<F>::K;
  using IPDEMethod<F>::Func;
  using IPDEMethod<F>::A;

public:
  LCornerMethod(DataT a, DataT t, DataT h, size_t k, F func)
      : IPDEMethod<F>(a, t, h, k, func) {}

  DataT EvalNext(const CDataBufIt &cit, const CDataCacheIt &pit,
                 size_t m) override {
    DataT ukm = *pit;
    DataT ukm1 = *(pit - 1);
    DataT h = H / A;

    return (1 - Tau / h) * ukm + (Tau / h) * ukm1 + Tau * Func(H * K, Tau * m);
  }

  size_t GetLStride() const override { return 1; };
  size_t GetRStride() const override { return 0; };
};