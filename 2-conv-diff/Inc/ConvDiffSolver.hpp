#pragma once

#include <memory>
#include <queue>
#include <vector>

using DataT = double;

using DataBufT = std::vector<DataT>;
using CDataBufIt = DataBufT::const_iterator;
using DataBufIt = DataBufT::iterator;

using DataCacheT = std::deque<DataT>;
using CDataCacheIt = DataCacheT::const_iterator;
using DataCacheIt = DataCacheT::iterator;

struct IMethod {
  using Ptr = std::unique_ptr<IMethod>;

  // @brief Function that uses difference schema to evaluate PDE step 
  // @param cit current (k + 1) layer iterator
  // @param pit value cache iterator of previous (k) layer
  // @param m offset from layer start (meant to be t = tau*m)
  // @details
  //           [cit-2]         [cit-1]       [cit]   [cit+1]
  //       -----------------------------------------------------
  //   ... | u^{k+1}_{m-2} | u^{k+1}_{m-1} |   x   |     x     | ...
  //       -----------------------------------------------------
  //   ... |   u^k_{m-2}   |   u^k_{m-1}   | u^k_m | u^k_{m+1} | ...
  //       -----------------------------------------------------
  //           [pit-2]         [pit-1]       [pit]   [pit+1]
  //
  // k (x = h*k) offset is meant to be incapsulated in derived class state
  // 
  // The validity of iterators should be provided from caller knowing
  // L and R strides
  virtual DataT EvalNext(const CDataBufIt &cit, const CDataCacheIt &pit, size_t m) = 0;

  // @brief Provides the left stride for calculation
  virtual size_t GetLStride() const = 0;

  // @brief Provides the right stride for calculation
  virtual size_t GetRStride() const = 0;

  virtual ~IMethod() = default;
};

struct GetStrategy {
  using Ptr = std::unique_ptr<GetStrategy>;

  virtual DataT Get() = 0;
  virtual ~GetStrategy() = default;
};

struct PutStrategy {
  using Ptr = std::unique_ptr<PutStrategy>;

  virtual void Put(DataT) = 0;
  virtual ~PutStrategy() = default;
};

class InputGet final: public GetStrategy {
  CDataBufIt Curr;
  CDataBufIt End;

public:
  InputGet(const DataBufT &data)
      : GetStrategy(), Curr{data.cbegin()}, End{data.cend()} {}

  DataT Get() override {
    if (Curr == End)
      throw std::runtime_error("End of stream");
    return *(Curr++);
  }
};

struct DummyPut final: public PutStrategy {
  DummyPut(): PutStrategy{} {}
  void Put(DataT value) override {}
};

struct Layer final {
private:
  GetStrategy::Ptr Getter;
  PutStrategy::Ptr Putter;
  IMethod::Ptr Method;

public:
  Layer(IMethod::Ptr &&method, GetStrategy::Ptr &&getter,
        PutStrategy::Ptr &&putter)
      : Getter(std::move(getter)), Putter(std::move(putter)),
        Method(std::move(method)) {}

  // Assuming that all the value[i < start] were calculated
  // end - exclusive
  void Process(DataBufT &buf, size_t start, size_t end) {
    DataCacheT cache;

    if (start < Method->GetLStride())
      throw std::runtime_error(
          "Method can't start as some values on left are unknown");

    if (end > buf.size() - Method->GetRStride())
      throw std::runtime_error(
          "Metod can't start as some of values on right are unknown");

    for (int i = 0; i < Method->GetLStride() + Method->GetRStride() + 1; ++i)
      cache.push_back(Getter->Get());

    for (size_t i = start; i < end; ++i) {
      auto pit = cache.cbegin() + Method->GetLStride();
      auto cit = buf.cbegin() + i;

      DataT val = Method->EvalNext(cit, pit, i);
      buf[i] = val;
      Putter->Put(val);

      if (i + 1 < end) {
        cache.push_back(Getter->Get());
        cache.pop_front();
      }
    }
  }
};