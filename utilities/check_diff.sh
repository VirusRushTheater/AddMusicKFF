#!/bin/bash

# ---------------------------------------------------------------------------
# Compares two binary files, shows the hexdump of both of them, and their
# diff. Requires tmux to be installed.
# ---------------------------------------------------------------------------

# TMux session name
session=$(basename ${0/\.sh/})
tmpdir="$(mktemp)"

# Require two arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 <file_1> <file_2>"
    exit 1
fi

# Create a temporary directory and dump hexfiles there
rm ${tmpdir}
tmpdir="${tmpdir}_hexcmp"

mkdir ${tmpdir}
hexdump -C "$1" > ${tmpdir}/h1.hex
hexdump -C "$2" > ${tmpdir}/h2.hex

tmux new-session -d -s $session

tmux split-window -h
tmux split-window -v -f -p 20

tmux select-pane -t 0
tmux send-keys "less ${tmpdir}/h1.hex" C-m

tmux select-pane -t 1
tmux send-keys "less ${tmpdir}/h2.hex" C-m

tmux select-pane -t 2
tmux send-keys "diff --side-by-side --suppress-common-lines \"${tmpdir}/h1.hex\" \"${tmpdir}/h2.hex\" | less" C-m

tmux select-pane -t 0
tmux select-pane -T "$(basename "$1")"
tmux select-pane -t 1
tmux select-pane -T "$(basename "$2")"
tmux select-pane -t 2
tmux select-pane -T "Diff between \"$(basename "$1")\" and \"$(basename "$2")\""

tmux set -g pane-border-status bottom

tmux attach-session -t $session

rm -r ${tmpdir}