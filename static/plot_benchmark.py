import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def get_avg(filepath):
    res=0.0
    i=0
    with open(filepath,encoding='utf-8') as file:
        file.readline()
        for line in file:
            line=line.strip().split(',')
            temp=float(line[1])
            res+=temp
            i+=1
    return round(res/i,2)
def get_n_list():
    res=[]
    for i in range(0,9):
        n=16*2**i
        res.append(n)
    return res
def get_data(type,res,rel_path):
    dat=[]
    for n in res:
        filepath=f"{rel_path}{type}_{n}.csv"
        avg=get_avg(filepath)
        dat.append(avg)
    return dat

def plot(bvh_dat,naive_dat,n_list):
    colors=["#fc2339","#1eb80d"]
    _,ax=plt.subplots(figsize=(8,8))
    ax.plot(n_list, bvh_dat, color=colors[0], marker='o', linewidth=2, markersize=6, label='BVH')
    ax.plot(n_list, naive_dat, color=colors[1], marker='s', linewidth=2, markersize=6, label='Наївна реалізація')
    ax.set_xscale('log', base=2)
    ax.set_xticks(n_list)
    ax.set_xticklabels([f'$2^{{{int(np.log2(n))}}}$' for n in n_list])
    ax.set_xlabel('Кількість примітивів', fontsize=12)
    ax.set_ylabel('Середній FPS', fontsize=12)
    ax.set_title('ефективність BVH',fontsize=14)
    ax.legend(fontsize=11)
    ax.grid(True, which='both', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    plt.show()

if __name__=='__main__':
    n_list=get_n_list()
    rel_path="benchmark_results/"
    bvh_dat=get_data("bvh",n_list,rel_path)
    naive_dat=get_data("naive",n_list,rel_path)
    plot(bvh_dat,naive_dat,n_list)
