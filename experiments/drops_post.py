#!/usr/local/bin/python
import json,re,sys
import matplotlib.pyplot as plt 

def plot_master_drops(file,master):
    f = open(file,"r+")
    buf = f.read()
    f.close
    true = []
    perceived = []
    m = re.search('.+DATA: \[True Drops\] : (.+)\n',buf)
    if m is not None and m.group(1) != 'null':
        true = json.loads(m.group(1))
    m = re.search('.+DATA: \[Perceived Drops\] : (.+)\n',buf)
    if m is not None and m.group(1) != 'null':
        perceived = json.loads(m.group(1))
    
    for e in true:
        if e[1] == master:
            plt.vlines(e[0]/1000,0,1,colors='b')
    for e in perceived:
        if e[1] == master:
            plt.vlines(e[0]/1000,0,1,colors='r',linestyles='dashed')
    plt.show()

def main(file,master):
    # second parameter is whether we are looking for a master stop
    plot_master_drops(file,master)

if __name__ == '__main__':
    fname = "log.txt"
    master = True
    while len(sys.argv) > 2: 
        if sys.argv[1] == "-n":
            fname = sys.argv[2]
        elif sys.argv[1] == '-m':
            master = sys.argv[2]
        sys.argv = sys.argv[2:]
    main(fname,master) 