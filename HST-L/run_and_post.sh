#!/bin/bash


# Run host_code and save output to 4_small_size.txt
NR_DPUS=1 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 1_small_size.txt &

NR_DPUS=2 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 2_small_size.txt &

NR_DPUS=4 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 4_small_size.txt &

NR_DPUS=8 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 8_small_size.txt &

NR_DPUS=16 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 16_small_size.txt &

NR_DPUS=32 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 32_small_size.txt &

NR_DPUS=64 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 64_small_size.txt &

NR_DPUS=128 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 128_small_size.txt &

NR_DPUS=256 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 256_small_size.txt &

NR_DPUS=512 NR_TASKLETS=16 make all
./bin/host_code -x 1 > 512_small_size.txt &

# Get the process ID (PID) of the background process
HOST_PID=$!

# Wait for host_code to finish
wait $HOST_PID

# Send JSON data using wget
wget --method=POST --header="Content-Type: application/json" \
     --body-data='{"status": "completed", "file": "4_small_size.txt"}' \
     https://fa20-1-173-37-18.ngrok-free.app/webhook

echo "Script execution completed."
