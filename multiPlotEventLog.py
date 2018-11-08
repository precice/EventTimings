#!/usr/bin/env python3
import pandas as pd, numpy as np
import matplotlib
import matplotlib.pyplot as plt
import argparse, sys

STOPPED = 0
STARTED = 1
PAUSED  = 2

def create_color_map(keys):
    """ Create a mapping: inner most key (e.g. Name) -> plot color """
    mpl_cmap = matplotlib.cm.get_cmap()
    color_map = {}
    for i, name in enumerate(keys):
        color_map[name] = matplotlib.cm.get_cmap()(i / len(keys))

    return color_map


def normalize_times(df):
    """ Sets zero for each file individually """
    for f in df.File.unique():
        df.loc[df.File == f, "Timestamp"] -= min(df.Timestamp[df.File == f])


def groupby(grouping, names, y_pos):
    """ Takes two equally sized list of names and y_positions.
    Events that match according to name or prefix are given the same y_pos. """
    if grouping == "none":
        return names, y_pos
    elif grouping == "name":
        new = {}
        ys = [-1] # so that ys[-1] always works
        for name, y in zip(names, y_pos):
            if name not in new:
                new[name] = ys[-1] + 1
            ys.append(new[name])
                
        return new.keys(), ys[1:]

    elif grouping == "prefix":
        raise NotImplementedError
                



parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                 description = "Visualize multiple event logs")
parser.add_argument('--file', help = "File names of log file",
                    nargs = "+")
parser.add_argument('--names', help = "Label for each file, must be same length as --file or None",
                    nargs = "+")
parser.add_argument('--sort', help = "Whitespace separated list of fields, determining sort order",
                    nargs = "*", default = ["File", "Rank", "Name"])
parser.add_argument('--filter', help = "Filter expression used on pandas.query",
                    type = str)
parser.add_argument('--runindex', help = "Index of run, -1 is latest",
                    type = int, default = -1)
parser.add_argument('--group', help = "Group Events together on a line based on a criteria",
                    choices = ["none", "name", "prefix"], default = "name")



if len(sys.argv) < 2:
    parser.print_help()
    sys.exit(1)

args = parser.parse_args()

if args.names:
    assert(len(args.names) == len(args.file))
    names = args.names
else:
    names = args.file

# Read in data into multiple data frames
dfs = [pd.read_csv(f, index_col = 0, parse_dates = [0]) for f in args.file]
# Select run index
dfs = [df.loc[df.index.unique()[args.runindex]] for df in dfs]
# Concat to one data frame
df = pd.concat(dfs, keys = names, names = ["File"])
# We do not care about paused state and treat them as stopped
df.loc[df.State == PAUSED, "State"] = STOPPED
# Apply filter
if args.filter:
    try:
        df.query(args.filter, inplace = True)
    except:
        print("There is something wrong with the filter string you supplied.")
        sys.exit(-1)

# Remove all indexes
df.reset_index(inplace = True)

# Normalize times for file individually
normalize_times(df)

# Color map: inner most key -> color
color_map = create_color_map(df[args.sort[-1]].unique())

# Remove the _GLOBAL event, this must be done after normalize_events
df = df[df.Name != "_GLOBAL"]

df.sort_values(args.sort, inplace = True)


height = 1    # Height of one event

y_pos = []
lefts = []
widths = []
heights = []
names = []
colors = []

count = 0
for row in df.itertuples():
    if row.State == STARTED:
        lefts.append(row.Timestamp)
        # Find the corresponding stop event: same name, rank and file, later timestamp and stopped state
        eventStop = df[(df.Name == row.Name) & (df.Rank == row.Rank) &
                       (df.File == row.File) & (df.Timestamp >= row.Timestamp) &
                       (df.State == STOPPED)]
        y_pos.append(height * count)
        heights.append(height)

        if len(eventStop):
            widths.append(eventStop.iloc[0].Timestamp - row.Timestamp)
        else: # zero length event
            widths.append(0)
        names.append("{file} @ Rank {rank}: {name}".format(file = row.File, rank = row.Rank, name = row.Name))
        colors.append(color_map[getattr(row, args.sort[-1])])
        count += 1


names, y_pos = groupby(args.group, names, y_pos)
        
ax = plt.gca()
ax.barh(y_pos, widths, heights, lefts, color = colors, align = 'center')
ax.set_yticks(sorted(list(set(y_pos)))) # Make y_pos unique
ax.set_yticklabels(names)
ax.invert_yaxis()
ax.set_xlabel("Time [ms]")
ax.grid(True)
ax.set_title("Run at " + str(df.RunTimestamp.iloc[0]))

plt.show()
