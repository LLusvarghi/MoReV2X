#!/bin/bash
echo Testing python scripts
cd CAM-tools/CAM-model/
python3 NS3_traces_generation.py -p CAMtraces --model Complete --scenario Highway --profile Volkswagen -m 5 -n 345 -t 30 