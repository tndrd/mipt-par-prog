import sys
import matplotlib.pyplot as plt
from tqdm import tqdm

path = sys.argv[-1]

with open(path, "r") as f:
  lines = f.readlines()
  for i in tqdm(range(len(lines))):
    lines[i] = list(map(float, lines[i].split()))

h, t = lines[0]

plt.pcolormesh(lines[1:], cmap="inferno")

xtics = range(0, len(lines[1]), len(lines[1])//10)
xlabels = [str(x * h) for x in xtics]

ytics = range(0, len(lines), len(lines)//10)
ylabels = [str(t * y) for y in ytics]


plt.xlabel("x")
plt.xticks(xtics, xlabels)

plt.ylabel("t")
plt.yticks(ytics, ylabels)

plt.colorbar()
plt.show()