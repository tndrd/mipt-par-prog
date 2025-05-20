#include <Solver.hpp>
#include <cmath>
#include <iostream>

int main(int argc, char **argv) {
  const char *outName = nullptr;
  if (argc == 2) {
    outName = argv[argc - 1];
  }

  DataT A = 1;

  DataT X = 5;
  DataT T = (X / 2) / A;

  DataT x0 = X / 2;
  DataT dx = X / 6;
  DataT amp = 1;

  auto ut0 = [x0, dx, amp](DataT x) -> DataT {
    if (x < (x0 - dx / 2) || x > (x0 + dx / 2))
      return 0;

    DataT val = amp * cos(3.1415 * (x - x0) / dx);
    return val * val;
  };

  auto ux0 = [](DataT) { return 0; };
  auto func = [](DataT, DataT) { return 0; };

  auto problem = ProblemConfig(func, ux0, ut0);

  problem.Borders.T = T;
  problem.Borders.X = X;
  problem.Problem.A = A;
  problem.Steps.H = 5e-4;
  problem.Steps.T = 5e-4;

  MPISolver::SolverConfig conf;
  conf.Name = outName ? outName : "";
  conf.Write = outName;

  MPISolver::Participate<RectMethod>(problem, conf);
}