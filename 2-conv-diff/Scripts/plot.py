import sys
import matplotlib.pyplot as plt
from tqdm import tqdm
import glob
import numpy as np

T_STEP = 3 

prefix = sys.argv[-1]
paths = glob.glob(prefix+"*")

nlayers, lsize = None, None

print(paths)
with open(paths[0], "r") as f:
  line = f.readline()
  h, t = map(float, line.split())

  line = f.readline()
  nlayers, lsize = map(int, line.split())

layers = [None]*(nlayers)
print(nlayers)

for path in paths:
  with open(path, "r") as f:
    lines = f.readlines()
    for i in tqdm(range(2, len(lines))):
      tokens = lines[i].split()
      k = int(tokens[0])
      data = list(map(float, tokens[1:]))
      layers[k] = data

fig, (lfig, rfig) = plt.subplots(1, 2)

mesh = lfig.pcolormesh(layers, cmap="inferno")

xtics = range(0, len(layers[0]), len(layers[0])//10)
xlabels = [str(x * h) for x in xtics]

ytics = range(0, len(layers), len(layers)//10)
ylabels = [str(round(t * y, 1)) for y in ytics]

lfig.set_xlabel("x")
lfig.set_xticks(xtics, xlabels)

lfig.set_ylabel("t")
lfig.set_yticks(ytics, ylabels)

X = np.arange(0, lsize) * h

for t, lt in zip(ytics[::T_STEP], ylabels[::T_STEP]):
  rfig.plot(X, layers[t], label=f"t = {lt}")

rfig.set_xlabel("x")
rfig.set_ylabel("u")
rfig.grid()
plt.legend()
plt.colorbar(mesh)
plt.suptitle("Уравнение переноса")
plt.show()