#pragma once
#include <ConvDiffSolver.hpp>

template <typename F> class IPDEMethod : public IMethod {
protected:
  DataT Tau;
  DataT H;
  size_t K;
  F Func;

public:
  IPDEMethod(DataT t, DataT h, size_t k, F func)
      : IMethod{}, Tau{t}, H{h}, Func{func}, K{k} {}
  virtual ~IPDEMethod() = default;
};

// Explicit left corner:
//       ^
//       |
//   <--->
// Method formula:
//   u^{k+1}_m = (1-t/h)u^k_m + (t/h)u^k_{m-1} + tf^k_m
template <typename F> class LCornerMethod final : public IPDEMethod<F> {
private:
  using IPDEMethod<F>::Tau;
  using IPDEMethod<F>::H;
  using IPDEMethod<F>::K;
  using IPDEMethod<F>::Func;

public:
  LCornerMethod(DataT t, DataT h, size_t k, F func)
      : IPDEMethod<F>(t, h, k, func) {}

  DataT EvalNext(const CDataBufIt &cit, const CDataCacheIt &pit,
                 size_t m) override {
    DataT ukm = *pit;
    DataT ukm1 = *(pit - 1);

    return (1 - Tau / H) * ukm + (Tau / H) * ukm1 + Tau * Func(H * K, Tau * m);
  }

  size_t GetLStride() const override { return 1; };
  size_t GetRStride() const override { return 0; };
};
