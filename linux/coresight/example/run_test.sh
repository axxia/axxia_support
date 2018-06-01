#!/bin/bash

# Make sure client and server are up to date.
make

# Start the server on core 2 and remember the PID.

taskset -c 2 ./server &
SERVER_PID=$!

# Start recording.
perf record -e cs_etm/@8008046000.etr/ --per-thread --pid $SERVER_PID &

# Wait a bit, and start the client on core 1.
sleep 1
taskset -c 1 ./client
