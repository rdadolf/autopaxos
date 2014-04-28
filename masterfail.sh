# fails a master then brings it back up after a time > master_timeout_

make;
echo "running masterfail";
./nnodes -n 3&
sleep 4; # wait for everything to come up
./commands/stop_server;
sleep 6;
./commands/start_server;
sleep 10;
killall nnodes