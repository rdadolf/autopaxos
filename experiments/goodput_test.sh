make; 
./experiments/goodput ;
mv log.txt log_off.txt ; 
./experiments/goodput -a ;
mv log.txt log_on.txt ; 
./experiments/analyze_goodput.py log_off.txt log_on.txt