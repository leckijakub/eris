#!/usr/bin/env python3

import sys
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import math
from pathlib import Path


BATCH_SIZE = 100


def colors_from_values(values, palette_name):
    # normalize the values to range [0, 1]
    normalized = (values - min(values)) / (max(values) - min(values))
    # convert to indices
    indices = np.round(normalized * (len(values) - 1)).astype(np.int32)
    # use the indices to get the colors
    palette = sns.color_palette(palette_name, len(values))
    return np.array(palette).take(indices, axis=0)


def get_color(name, number):
    pal = list(sns.color_palette(palette=name, n_colors=number).as_hex())
    return pal


def load_data(filename):
    msrs = []

    if filename.endswith('.csv'):
        return pd.read_csv(filename)
    
    with open(filename, "r") as file:
        for line in file.readlines():
            if "<info> app: [BATCH" in line:
                splitted = line.split(" ")
                characteristic = splitted[7][:-1]
                bpr  = int(splitted[9][:-1])
                bper  = float(splitted[11][:-1])
                lost = 100 - bpr
                msrs.append([characteristic, bpr, lost, bper])

    return pd.DataFrame(msrs, columns=["characteristic", "bpr", "lost", "bper"])


if len(sys.argv) < 2:
    print("pass filename as first parameter")
    sys.exit(1)
filename = sys.argv[1]


# Load file
df = load_data(filename)

fig = plt.figure(figsize=(12,12))
ax = plt.subplot(111, polar=True)
plt.axis()

indexes = list(range(1,13))
width = 2 * np.pi / len(indexes)
lowerLimit = 0
heights = [0] * 12

# calculate values
for index, min_row in df.iterrows():
    # bpcr - batch packet correctness rate = 100% - bper
    bpcr = 1 - min_row['bper']
    for i, director in enumerate(min_row['characteristic'][::-1]):
        if director == '1':
            heights[i] += bpcr

max_v = max(heights)
angles = [element * width for element in indexes]
pal_vi = colors_from_values(np.array(heights), 'RdYlGn')

bars = ax.bar(x=angles, height=heights, width=width, bottom=lowerLimit,
              linewidth=1, edgecolor="white", color=pal_vi, alpha=0.5)
labelPadding = 0

for angle, height, label in zip(angles, heights, indexes):
    rotation = np.rad2deg(angle)
    alignment = ""
    #deal with alignment
    if angle >= np.pi/2 and angle < 3*np.pi/2:
        alignment = "right"
        rotation = rotation + 180
    else:
        alignment = "left"
    ax.text(x=angle, y=lowerLimit + max_v + labelPadding,
            s=label, ha=alignment, va='center', rotation=rotation,
            rotation_mode="anchor")
    ax.set_thetagrids([], labels=[])

fig.savefig(f"{Path(filename).stem}.png", format = "png")
plt.show()
