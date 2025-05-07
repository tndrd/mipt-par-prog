import sys
import matplotlib.pyplot as plt
from tqdm import tqdm
import glob

prefix = sys.argv[-1]
paths = glob.glob(prefix+"*")

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
      if (k>=nlayers): print(k)
      layers[k] = data

plt.pcolormesh(layers, cmap="inferno")

xtics = range(0, len(layers[0]), len(layers[0])//10)
xlabels = [str(x * h) for x in xtics]

ytics = range(0, len(layers), len(layers)//10)
ylabels = [str(t * y) for y in ytics]


plt.xlabel("x")
plt.xticks(xtics, xlabels)

plt.ylabel("t")
plt.yticks(ytics, ylabels)

plt.colorbar()
plt.show()