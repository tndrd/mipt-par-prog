#include "Integration.hpp"

static constexpr double RealVal = 2.50344;

double Prec2h(AdaptiveGrid::FuncT, double start, double size, double prec) {
  double den = size * (2 / pow(start, 3) + 1 / pow(start, 4));
  double h2 = prec / den;

  return sqrt(h2);
}

double Func(double x) { return sin(1 / x); }

double FuncD(double x) { return -cos(1 / x) / (x * x); }

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: ./4-Integrate <NWORKERS>";
    return 1;
  }

  size_t nWorkers = std::stoul(argv[1]);

  IntegrateArgs args;

  args.Func = Func;
  args.FuncD = FuncD;
  args.StepEval = Prec2h;

  args.A = 0.01;
  args.B = 8;

  args.GridResolution = 10;

  double nDigits = 10;

  args.Prec = 1 / pow(10, nDigits);

  double val = Integrate(args, nWorkers, true);
  double formatNorm = (pow(10, nDigits + 1));
  double err = fabs(RealVal - val);

  std::cout << "Integral value: " << round(val * formatNorm) / formatNorm
            << "+=" << args.Prec << std::endl;

  if (nDigits > 5) {
    assert(err < 3e-6);
    return 0;
  }

  std::cout << "Error value:    " << round(err * formatNorm) / formatNorm
            << std::endl;

  // Precision of value integrated by Wolfram
  assert(err <= args.Prec);
  return 0;
}