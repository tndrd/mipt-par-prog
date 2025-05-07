#include <Solver.hpp>
#include <cmath>
#include <iostream>

int main(int argc, char **argv) {
  auto ut0 = [](DataT x) {
    if (x > 2.25 && x < 2.75)
      return 1;
    else
      return 0;
  };

  auto ux0 = [](DataT) { return 0; };
  auto func = [](DataT, DataT) {return 0;};

  auto problem = CreateProblem(func, ux0, ut0);
  
  problem.Borders.T = 10;
  problem.Borders.X = 5;

  problem.Problem.A = 1;

  problem.Steps.H = 1e-3;
  problem.Steps.T = 1e-3;

  MPISolver::SolverConfig conf;
  conf.Name = "out";
  conf.Write = false;

  MPISolver::Participate<LCornerMethod>(problem, conf);
}