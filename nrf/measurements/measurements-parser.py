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


if len(sys.argv) < 2:
    print("pass filename as first parameter")
    sys.exit(1)
filename = sys.argv[1]


# Load file
if filename.endswith('.csv'):
    df = pd.read_csv(filename)
    msrs = df.to_numpy()
else:
    msrs = []
    with open(filename, "r") as file:
        for line in file.readlines():
            if "<info> app: RSSI" in line:
                splitted = line.split(" ")
                rssi = int(splitted[3][:-1])
                pr = int(splitted[5][:-1])
                lp = int(splitted[7][:-1])
                per = float(splitted[10])
                lost = lp - pr
                msrs.append([lp, pr, lost, rssi, per])
    df = pd.DataFrame(msrs, columns=["lp", "pr", "lost", "rssi", "per"])

print(df)

number_of_batches = int(math.ceil(msrs[-1][0] / BATCH_SIZE))
batches = [0] * number_of_batches
print(f"NUMBER OF BATCHES: {number_of_batches}")

for i, packet in enumerate(msrs):
    batch_no = int((packet[0] - 1) / BATCH_SIZE)
    batches[batch_no] += 1

# convert value of batches to %
for i in range(number_of_batches):
    batches[i] = batches[i]/BATCH_SIZE*100

#the last batch has size of (last packet receives) mod BATCH_SIZE
batches[-1] = batches[-1]*BATCH_SIZE/100 / msrs[-1][0]%BATCH_SIZE

batch_df = pd.DataFrame(batches, columns=["pr"])

# scale lp to fit batch range
df.lp = df.lp.div(BATCH_SIZE)

fig, axs = plt.subplots(
    2, 1, figsize=(16, 9), sharex=True, gridspec_kw=dict(height_ratios=[0.5, 3])
)
per_plot = sns.lineplot(ax=axs[0], data=df, x="lp", y="per", color="darkred")
per_plot.set(ylabel="Packet Error Rate [%]", title="Cumulative Packet Error Rate")
ax1 = axs[1]
ax2 = ax1.twinx()
# fig.ylim(0,110)
batched_plot = sns.barplot(data=batch_df, x=batch_df.index, y="pr", palette=colors_from_values(np.array(batches), "RdYlGn"), ax=ax1, alpha=.5, label='Batch Packets Received [%]')
batched_plot.set(xlabel='Batch number', ylabel='Batch Packets Received [%]', title ='Batched Signal Quality', ylim = [0,110])
ax1.legend(loc='upper left', frameon = False)
lost_plot = sns.lineplot( data=df, x="lp", y="lost", ax=ax2, color="darkred", label='Cumulative loss')
lost_plot.set(ylabel='Cumulative Packet Lost', ylim = [0, df.lost.iat[-1]*1.1])
new_ticks = [i.get_text() for i in batched_plot.get_xticklabels()]
plt.xticks(range(0, len(new_ticks), 10), new_ticks[::10])
ax2.legend(loc='upper right', frameon = False)
fig.savefig(f"{Path(filename).stem}.png", format = "png")
plt.show()
