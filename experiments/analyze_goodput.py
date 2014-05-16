#!/usr/bin/env python

import sys
import re
import numpy as np

import goodplot

################################################################################
# vvvvv MPL Boilerplate vvvvv
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import rcParams
import matplotlib.cm as cm
import matplotlib as mpl
def rgb(r,g,b):
        return (float(r)/256.,float(g)/256.,float(b)/256.)
# Modified colorbrewer2.org qualitative color table (8-paired)
cb2 = [ rgb(31,120,180), rgb(255,127,0), rgb(51,160,44), rgb(227,26,28), rgb(166,206,227), rgb(253,191,111), rgb(178,223,138), rgb(251,154,153) ]
rcParams['figure.figsize'] = (8,6)
rcParams['figure.dpi'] = 150
rcParams['axes.color_cycle'] = cb2
rcParams['lines.linewidth'] = 1
rcParams['axes.facecolor'] = 'white'
rcParams['font.size'] = 12
rcParams['patch.edgecolor'] = 'white'
rcParams['patch.facecolor'] = cb2[0]
rcParams['font.family'] = 'Helvetica'
rcParams['font.weight'] = 100
# ^^^^^ MPL Boilerplate ^^^^^
################################################################################

# Relevant plotting events:
#     Actual MTBF set points => mtbf
#     Estimated MTBF => e_mtbf
#     Actual latency changes => latency
#     Estimated latency => e_latency
#     Heartbeat events => hb
#     PMetric estimates => e_pmet
#     Measured traffic => traf
#     Master starts => starts
#     Master stops => stops
#     Node drop events => drop
#     Estimated drop events => e_drop
# 
# Data format:
#     np.array( [ [time, event], ... ] )

CHANGE_DELAY = 1 # (in sec)

def parse_events(file):
    latency = []
    e_pmet = []
    goodput = []

    master = 0 # FIXME: Add a regex for master changes if/when that works

    for line in [raw.strip() for raw in file.readlines()]:

        m = re.match('(\d+\.\d+).*DATA: Latency set to: (\d+)', line)
        if m is not None:
            latency.append( (float(m.group(1)), float(m.group(2))) )

        m = re.match('(\d+\.\d+).*DATA: PMetric estimate before tuning: ([-\d\.]+)', line)
        if m is not None:
            e_pmet.append( (float(m.group(1)), float(m.group(2))) )

        m = re.match('(\d+\.\d+).*DATA: \[goodput\]: (\d+) sent in (\d+)', line)
        if m is not None:
            goodput.append((float(m.group(1)), float(m.group(2)) / (float(m.group(3)) / 1000000.)))


    print 'latency',len(latency)
    print 'e_pmet',len(e_pmet)
    print 'goodput',goodput

    #t0 = min(mtbf[0][0], latency[0][0], e_pmet[0][0], hb[0][0], eff[0][0])
    t0 = min(latency[0][0], e_pmet[0][0], goodput[0][0])
    tN = max(latency[-1][0], e_pmet[-1][0], goodput[-1][0])
    return ( np.array([(t-t0,x) for (t,x) in latency]),
             np.array([(t-t0,x) for (t,x) in e_pmet]),
             np.array([(t-t0,x) for (t,x) in goodput]),
             t0, tN)

def plot_events(goodput_on, latency, e_pmet, goodput_off, t0, tN):
    (fig,ax) = plt.subplots()

    # Turn mtbf into a step function
    #mtbf_step_t = np.zeros((mtbf.shape[0]*2))
    #mtbf_step_v = np.zeros((mtbf.shape[0]*2))
    #mtbf_step_t[::2] = mtbf[:,0]
    #mtbf_step_t[1:-1:2]= mtbf[1:,0]
    #mtbf_step_t[-1] = mtbf[-1,0] + CHANGE_DELAY
    #mtbf_step_v[::2] = mtbf[:,1]
    #mtbf_step_v[1::2]= mtbf[:,1]
    # Turn latency into a step function
    latency_step_t = np.zeros((latency.shape[0]*2))
    latency_step_v = np.zeros((latency.shape[0]*2))
    latency_step_t[::2] = latency[:,0]
    latency_step_t[1:-1:2]= latency[1:,0]
    latency_step_t[-1] = latency[-1,0] + CHANGE_DELAY
    latency_step_v[::2] = latency[:,1]
    latency_step_v[1::2]= latency[:,1]
    lins = []
    # Plot actual MTBF and latency
    #ax.axhline(mtbf[0,1], color=cb2[0], label='Actual MTBF' )
    #ax.plot( mtbf_step_t, mtbf_step_v, color=cb2[0], label='Actual MTBF' )
    lins.append(ax.plot( latency_step_t, latency_step_v, color=cb2[2], label='Latency' ))
    # ax.axvline(e_drop[0,0],ls=':',color='k',alpha=0.2, label='Node drops') # FIXME: should we highlight master drops
    # for (t,_) in e_drop[1:]:
    #     ax.axvline(t,ls=':',color='k',alpha=0.2)

    # Plot mtbf_ and rtt_estimate_
    # ax.plot( e_mtbf[:,0], e_mtbf[:,1]/1000., color=cb2[4], label='Estimated MTBF' )
    # ax.plot( e_latency[:,0], e_latency[:,1]/2000., color=cb2[5], label='Estimated Latency' )

    # measured goodput
    axG = ax.twinx();
    lins.append(axG.plot( goodput_off[:,0], goodput_off[:,1], color=cb2[1], label='Goodput without autotuning'))
    lins.append(axG.plot( goodput_on[:,0], goodput_on[:,1], color=cb2[3], label='Goodput with autotuning'))
    axG.set_ylabel(r'Goodput (requests / s)')

    # Polish off plot
    #t_max = max(np.max(mtbf[:,0]),np.max(latency[:,0]),np.max(hb[:,0]),np.max(e_pmet[:,0]),np.max(eff[:,0]))
    # t_max = max(np.max(mtbf[:,0]),np.max(latency[:,0]),np.max(hb[:,0]),np.max(e_pmet[:,0]))
    # ax.set_xlim((0,t_max))
    ax.set_xlabel(r'Time')
    ax.set_ylabel(r'ms')
    ax.set_title('Goodput Tracking')
    lins = reduce(lambda x,y: x+y,lins)
    ax.legend(lins,[l.get_label() for l in lins],loc=2)
    fig.tight_layout()
    return (fig,ax)


if __name__=='__main__':
    # off first then on
    if len(sys.argv)>1:
        file=open(sys.argv[1])
    else:
        file=sys.stdin
    args = parse_events(file)
    file.close()
    if len(sys.argv) > 2:
        file = open(sys.argv[2])
    else: 
        file = sys.stdin
    _,_,goodput_on,_,_= parse_events(file)
    (fig,ax) = plot_events(goodput_on,*args)
    fig.savefig('goodput.png')

