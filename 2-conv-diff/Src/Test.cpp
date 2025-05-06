#include <Methods.hpp>
#include <iostream>
#include <cmath>

int main(int argc, char **argv) {
  DataT X = 5;
  DataT T = 1;

  DataT tau = 1e-3;
  DataT h = 2e-3;

  size_t layerSize = X / h;
  size_t nsteps = T / tau;

  std::vector<DataBufT> layers (nsteps, DataBufT(layerSize, 0));

  layers[0] = DataBufT(layerSize, 0);

  int start = int(2.5f / h);
  int hwidth = int(0.1f/h);
  DataT val = 1;

  for (int i = start - hwidth; i < start + hwidth; ++i)
    layers[0][i] = val;

  auto dummyFunc = [](DataT x, DataT t) { return 0; };

  for (size_t k = 1; k < nsteps; ++k) {
    std::unique_ptr<InputGet> getter = std::make_unique<InputGet>(layers[k-1]);
    auto putter = std::make_unique<DummyPut>();
    auto method = std::make_unique<LCornerMethod<decltype(dummyFunc)>>(
        tau, h, k, dummyFunc);
    
    Layer layer {std::move(method), std::move(getter), std::move(putter)};
    layer.Process(layers[k], 1, layerSize);
  }

  std::cout << h << " " << tau << std::endl;

  for (const auto& layer: layers) {
    for (const auto u: layer)
      std::cout << u << " ";
    std::cout << "\n";
  }

  return 0;
}