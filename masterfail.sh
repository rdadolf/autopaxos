# fails a master then brings it back up after a time > master_timeout_

make;
./nnodes -n 3&
sleep 2;
./commands/stop_server;
sleep 6;
./commands/start_server;
sleep 10;
killall nnodes