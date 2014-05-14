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

import goodplot

if __name__=='__main__':

  #T_hb = 280.
  T_bf = 1000. # MTBF (ms)
  #T_to = 1000. # master timeout, in ms
  #T_l = 150. # latency, in ms
  C_r = 20. # recovery cost, in packets
  C_hb = 2. # heartbeat cost, in packets

  T_l = (np.linspace(50,500,200))  

  (fig,ax) = plt.subplots()
  for T_hb in np.linspace(50,800,8):
    for T_to in np.linspace(800,1600,8):
      p = goodplot.pmetric(T_hb, T_bf, T_to, T_l, C_r, C_hb)
      ax.plot(T_l, p, label=r'$T_{to}$='+str(T_to))
  #ax.axvline(T_bf,color='k',label=r'$T_{bf}$')
  ax.set_xlabel(r'Latency ($T_{l}$)')
  ax.set_ylabel('PMetric')
  #ax.legend()
  fig.savefig('lat.png')
  
