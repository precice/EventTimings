#!env python3

import pandas as pd, numpy as np
import matplotlib
import matplotlib.pyplot as plt
import argparse, sys

STOPPED = 0
STARTED = 1
PAUSED  = 2

def normalize_times2(df):
    df.reset_index(inplace = True)
    first_start = df.RunTimestamp.min()

    for fileno in df.File.unique():
        for rank in df.Rank.unique():
            events = df[(df.File == fileno) & (df.Rank == rank)]
            # The time the _GLOBAL event was started on this rank and file
            start = events[(df.Name == "_GLOBAL") & (df.State ==1)]
            # start = df[(df.Rank == rank) & (df.File = fileno) &
                       # (df.Name == "_GLOBAL") & (df.State == 1)]
            events = (events.Timestamp - int(start.Timestamp)) + (start.RunTimestamp - first_start)
            import pdb; pdb.set_trace()
    print(df)


def normalize_times(df):
    """ Sets zero for each file individually """
    for f in df.File.unique():
        df.loc[df.File == f, "Timestamp"] -= min(df.Timestamp[df.File == f])




parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                 description = "Visualize multiple event logs")
parser.add_argument('--file', help = "File names of log file",
                    nargs = "+")
parser.add_argument('--names', help = "Label for each file, must be same length as --file or None",
                    nargs = "+")
parser.add_argument('--sort', help = "Whitespace separated list of fields, determining sort order",
                    nargs = 3, default = ["File", "Rank", "Name"])
parser.add_argument('--filter', help = "Filter expression used on pandas.query",
                    type = str)
parser.add_argument('--runindex', help = "Index of run, -1 is latest",
                    type = int, default = -1)

if len(sys.argv) < 2:
    parser.print_help()
    sys.exit(1)

args = parser.parse_args()

if args.names:
    assert(len(args.names) == len(args.file))
    names = args.names
else:
    names = args.file

# Read in data
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

sort_values = args.sort

height = 1
padding = 0.2

y_pos = []
lefts = []
widths = []
heights = []
names = []
colors = []

# Create a mapping: inner most key (e.g. Name) -> plot color
mpl_cmap = matplotlib.cm.get_cmap()
color_map = {}
for i, name in enumerate(df[sort_values[-1]].unique()):
    count = df[sort_values[-1]].unique()
    color_map[name] = matplotlib.cm.get_cmap()(i / len(df[sort_values[-1]].unique()))

# Remove all indexes
df.reset_index(inplace = True)

normalize_times(df)

# Remove the _GLOBAL event, this must be done after normalize_events
df = df[df.Name != "_GLOBAL"]

df.sort_values(sort_values, inplace = True)

count = 0
for row in df.itertuples():
    if row.State == STARTED:
        lefts.append(row.Timestamp)
        # Find the corresponding stop event
        eventStop = df[(df.Name == row.Name) & (df.Rank == row.Rank) &
                       (df.File == row.File) & (df.Timestamp >= row.Timestamp) &
                       (df.State == STOPPED)]
        y_pos.append(1 * count)
        heights.append(1)

        if len(eventStop):
            widths.append(eventStop.iloc[0].Timestamp - row.Timestamp)
        else: # zero length event
            widths.append(0)
        names.append("{file} @ Rank {rank}: {name}".format(file = row.File, rank = row.Rank, name = row.Name))
        colors.append(color_map[getattr(row, sort_values[-1])])
        count += 1



ax = plt.gca()
ax.barh(y_pos, widths, heights, lefts, color = colors, align = 'center')
ax.set_yticks(sorted(list(set(y_pos)))) # Make y_pos unique
ax.set_yticklabels(names)
ax.invert_yaxis()
ax.set_xlabel("Time [ms]")
ax.grid(True)
ax.set_title("Run at " + str(df.RunTimestamp.iloc[0]))

plt.show()
