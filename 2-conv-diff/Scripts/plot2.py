import numpy as np
import matplotlib.pyplot as plt
import time
import os

p = range(1, 9)
t = []

os.system(f"mpirun -np 1 ./2-Task")

for i in p:
  start = time.perf_counter()
  os.system(f"mpirun -np {i} ./2-Task")
  t.append(time.perf_counter() - start)

print(p, t)

p = np.array(p)
t = np.array(t)

s = (1/t) * t[0]
e = s / p

fig, (sfig, efig) = plt.subplots(1, 2)

sfig.plot(p, s)
efig.plot(p, e)
efig.set_ylim(0, 1.5)
efig.set_xlim(1, 8)
sfig.set_xlim(1, 8)
efig.set_xticks(p, list(map(str, p)))
sfig.set_xticks(p, list(map(str, p)))

sfig.set_title("Ускорение")
efig.set_title("Эффективность")
sfig.grid()
efig.grid()

sfig.set_xlabel("p")
efig.set_xlabel("p")

plt.show()