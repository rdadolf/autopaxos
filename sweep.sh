#!/bin/bash

rm -f timing.txt
touch timing.txt

for beat in 150 200 250 300 350 400 450 500 550 600 650 700
do
  make clean
  make DEFINES="-DMODCOMM_DELAY=0 -DHEARTBEAT_FREQ=$beat -DMASTER_TIMEOUT=400"
  ./main
  grep 'Session stats' log.txt >> timing.txt
done
