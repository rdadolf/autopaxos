make; 
./experiments/monotonic_shift ;
mv log.txt log_off.txt ; 
./experiments/monotonic_shift -a;
mv log.txt log_on.txt ; 
./experiments/analyze_performance_both.py log_off.txt log_on.txt