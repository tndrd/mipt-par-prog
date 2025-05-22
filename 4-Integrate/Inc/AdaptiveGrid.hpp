#pragma once

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

struct AdaptiveGrid {
 public:
  using FuncT = double (*)(double);
  using StepEvalT = double (*)(FuncT, double, double, double);

 public:
  struct HomogenousPartition {
    double Start;
    double End;
    double Step;
    size_t NSteps;
  };

  struct HeterogenousPartition {
    struct Part {
      double Step;
      size_t NSteps;
    };

    double Start;
    std::vector<Part> Parts;
  };

 public:
  std::vector<HomogenousPartition> EvenPartitions;
  std::vector<HeterogenousPartition> Partitions;

 public:
  void Dump() const {
    std::cout << "Adaptive grid: " << std::endl;
    for (int i = 0; i < EvenPartitions.size(); ++i) {
      const auto& part = EvenPartitions[i];
      std::cout << "Even partition #" << i << std::endl;
      std::cout << "  Start: " << part.Start << std::endl;
      std::cout << "  End: " << part.End << std::endl;
      std::cout << "  Step: " << part.Step << std::endl;
      std::cout << "  NSteps: " << part.NSteps << std::endl;
    }
    for (int i = 0; i < Partitions.size(); ++i) {
      std::cout << "Partition #" << i << std::endl;
      double end = Partitions[i].Start;
      size_t nSteps = 0;
      for (const auto& part : Partitions[i].Parts) {
        nSteps += part.NSteps;
        end += part.NSteps * part.Step;
        std::cout << "  - " << part.NSteps << " x " << part.Step << std::endl;
      }

      std::cout << "  (" << Partitions[i].Start << ", " << end
                << "), nsteps=" << nSteps << "\n" << std::endl;
    }
  }

 public:
  // O(nWorkers*resolution)
  static AdaptiveGrid Create(StepEvalT rem2h, FuncT func, double start,
                             double end, double precision, size_t nWorkers,
                             size_t resolution = 1) {
    assert(resolution);

    size_t nPartitions = resolution;
    std::vector<HomogenousPartition> partitions(nPartitions);

    double partSize = (end - start) / nPartitions;
    size_t nSteps = 0;

    for (int i = 0; i < nPartitions; ++i) {
      auto& part = partitions[i];
      part.Start = start + partSize * i;
      part.End = part.Start + partSize;

      double h = rem2h(func, part.Start, partSize, precision);
      double steps = partSize / h;
      
      h *= steps / ceil(steps); // Evenly quantize the interval
      steps = ceil(steps);

      part.Step = h;
      part.NSteps = steps;

      nSteps += part.NSteps;
    }

    std::vector<HeterogenousPartition> task(nWorkers);
    size_t taskSize = ceil(double(nSteps) / nWorkers);

    task[0].Start = start;

    size_t taskIndex = 0;
    size_t taskFilled = 0;

    size_t partIndex = 0;
    size_t partUsed = 0;

    while (partIndex < nPartitions) {
      size_t remainsInPartition = partitions[partIndex].NSteps - partUsed;
      size_t remainsInTask = taskSize - taskFilled;

      HeterogenousPartition::Part taskPart;
      taskPart.Step = partitions[partIndex].Step;
      size_t indexToPush = taskIndex;

      if (remainsInPartition <= remainsInTask) {
        taskPart.NSteps = remainsInPartition;
        taskFilled += remainsInPartition;
        partIndex++;
        partUsed = 0;
      } else {  // remainsInPartition > remainsInTask
        taskPart.NSteps = remainsInTask;
        partUsed += remainsInTask;

        taskIndex++;
        task[taskIndex].Start = start + partSize * partIndex +
                                partUsed * partitions[partIndex].Step;

        taskFilled = 0;
      }

      task[indexToPush].Parts.push_back(taskPart);
    }

    assert(taskIndex >= nWorkers - 1);
    return {partitions, task};
  }
};