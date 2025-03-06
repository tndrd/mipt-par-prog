import matplotlib.pyplot as plt
import sys
import time
import numpy as np
import os

WORKERS_MAX = 3  # 2**5 = 32

if (len(sys.argv) < 2):
    print("Expected name of executable")
    quit()

executable = sys.argv[1]
args = []
if len(sys.argv) > 2:
  args = sys.argv[2:]

workers = np.array([2**i for i in range(0, WORKERS_MAX + 1)])
times = np.ndarray((len(workers)), dtype=np.float64)

for i in range(len(workers)):
    cmd = f"mpirun -np {workers[i]} ./{executable} " + " ".join(args)
    print(f'Executing "{cmd}"...')
    start = time.perf_counter()
    os.system(cmd)
    times[i] = time.perf_counter() - start
    print(f"Done in {round(times[i], 1)}s\n")

name = executable.split("/")[-1]

plt.title(f"{name} benchmark")
plt.xscale('log', base=2)
plt.xlabel("Number of workers")
plt.ylabel("Execution time, s")
plt.plot(workers, times)
plt.grid()

plt.show()