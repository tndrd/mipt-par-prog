import numpy as np
import glob
from tqdm import tqdm
import matplotlib.pyplot as plt

def load(prefix):
  paths = glob.glob(prefix+"*")

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

  for i, layer in enumerate(layers[::-1]):
    if layer is None: layers.pop()

  return np.array(layers)

layers1 = load("data1")
layers8 = load("data8")

diff = np.abs(layers1 - layers8)

print("Max absolute difference: " + str(np.max(diff)))


plt.pcolormesh(diff, cmap="inferno")
plt.colorbar()
plt.show()


