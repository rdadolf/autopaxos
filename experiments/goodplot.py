#!/usr/bin/env python

import sys
import re
import numpy as np
import scipy.stats

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

def pmetric(T_hb, T_bf, T_to, T_l, C_r, C_hb):
  # UPTIME
  # True failure detection & recovery
  #   (MTBF - blindWin+electionWin)
  # False failure recovery
  #   (1-P_ff) * electionWin
  # P_ff Fixed latency model
  #P_ff = (T_hb+T_l) > T_to
  # P_ff Gaussian latency model (std=100ms)
  P_ff = scipy.stats.norm.cdf( T_hb+T_l-T_to, loc=0, scale=100 )
  #P_tf = 1-scipy.stats.expon.cdf( 1, loc=0, scale=T_bf ) # scale = 1/lambda = 1/1/MTBF
  P_tf = np.exp(-1000./(T_bf-T_l))

  # TRAFFIC
  # Heartbeat overhead
  #   C_hb/T_hb
  # True failure recovery
  #   C_r/T_bf
  # False failure recovery
  #   P_ff * C_r
  
  uptime = 1 - P_tf - P_ff + P_tf*P_ff
  traffic= C_hb/T_hb + C_r/T_bf + P_ff*C_r
  return uptime/traffic
  

if __name__=='__main__':
  T_hb = (np.linspace(10,2000,200))
  
  T_bf = 1200. # MTBF (ms)
  #T_to = 1000. # master timeout, in ms
  T_l = 280. # latency, in ms
  C_r = 20. # recovery cost, in packets
  C_hb = 2. # heartbeat cost, in packets
  (fig,ax) = plt.subplots()
  for T_to in np.linspace(600,2000,8):
    p = pmetric(T_hb, T_bf, T_to, T_l, C_r, C_hb)
    ax.plot(T_hb, p, label=r'$T_{to}$='+str(T_to))
    print T_hb[np.argmax(p)], np.max(p)
  ax.axvline(T_bf,color='k',label=r'MTBF ($T_{bf}$)')
  ax.set_xlabel(r'Heartbeat interval ($T_{hb}$)')
  ax.set_ylabel('PMetric')
  ax.legend()
  fig.savefig('good.png')
  
