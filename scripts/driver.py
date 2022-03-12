#!/usr/bin/env python3

import sys
import subprocess
import os
import numpy as np
import matplotlib.pyplot as plt

runs = 50

def outlier_filter(datas, threshold = 2):
    datas = np.array(datas)
    z = np.abs((datas - datas.mean()) / datas.std())
    return datas[z < threshold]

def data_processing(data_set, n):
    samples = data_set[0].shape[0]
    final = np.zeros(samples)

    for s in range(samples):
        final[s] =                                                    \
        outlier_filter([data_set[i][s] for i in range(n)]).mean()
    return final

def measure(program, offset, method):
    Ys = []
    fileout = 'scripts/tmp.txt'
    if os.path.exists(fileout):
        os.remove(fileout)
    for i in range(runs):
        for j in range(1, offset + 1):
            try:
                comp_proc = subprocess.run('sudo taskset -c 7 ./%s %d %d >> %s' %(program, j, method, fileout), shell = True, check = True)
            except Exception as e:
                print(e)
                exit(1)
        
        output = np.loadtxt(fileout, dtype = 'float').T
        Ys.append(np.array(output))
        os.remove(fileout)
    X = [p for p in range(1, offset + 1)]
    Y = data_processing(Ys, runs)
    return [X, Y]

if __name__ == "__main__":
    offset = 0
    if len(sys.argv) == 2:
        offset = int(sys.argv[1])
    u_data = measure('static_btree', offset, 0)
    k_data = measure('static_btree', offset, 1)
    X = u_data[0]
    Y = [u_data[1], k_data[1]]
    fig, ax = plt.subplots(1, 1, sharey = True)
    ax.set_title('search performance', fontsize = 16)
    ax.set_xlabel(r'$2^N$ array size', fontsize = 16)
    ax.set_ylabel('time (ns)', fontsize = 16)
    
    ax.plot(X, Y[0], marker = '*', markersize = 3, label = 'S-tree')
    ax.plot(X, Y[1], marker = '+', markersize = 7, label = 'S-tree + hugepage')
    ax.legend(loc = 'upper left')
    
    plt.show()
