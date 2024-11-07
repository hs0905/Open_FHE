import csv
from sortedcontainers import SortedDict
import math

file_path = 'build/bin/examples/pke/commandrecord.csv'

ntt_time = 3454/0.45 # ns
intt_time = 3343/0.45
auto_time = 2578/0.45
add_time = 149/0.45
mult_time = 233/0.45
sub_time = 146/0.45
bconv_up_time = 275/0.45
bconv_down_time = 359/0.45
# pcie_time = 31737.97 # 15754 MB/s = 31,508 poly/s = 0.00003173797 s/poly = 31737.97 ns/poly - pcie 3.0 x16
pcie_time = 7934.49 # 15754 MB/s = 31,508 poly/s = 0.00003173797 s/poly = 31737.97 ns/poly - pcie 5.0 x16
hbm_time = 1086.95 # 460000 MB/s = 920000 poly/s = 0.00000108695 s/poly = 1086.95652174ns/poly
sram_time = 271.22 # 1843488 MB/s = 3686976 poly/s = 2.7122498220764116717873943307469e-7 s/poly =  ns/poly
poly_size = 0.5 

memory_dict = {} # key : memory address, value : end_time

compute_hop_dict = SortedDict()

PCIE_hop_dict = SortedDict()
# PCIE_hop_dict[0] = [float('inf'), float('inf')] # key : start_time, value : [end_time, length]
PCIE_hop_dict[float('inf')] = [0, float('inf')] # key : end_time, value : [start_time, length]

HBM_hop_dict = SortedDict()
HBM_hop_dict[float('inf')] = [0, float('inf')] # key : end_time, value : [start_time, length]
# HBM_hop_dict[0] = [float('inf'), float('inf')] # key : start_time, value : [end_time, length]

compute_hop_dict = SortedDict()
compute_hop_dict[float('inf')] = [0, float('inf')] # key : end_time, value : [start_time, length]

compute_time_dict = {
    'NTT': ntt_time,
    'INTT': intt_time,
    'Auto': auto_time,
    'Add': add_time,
    'Mult': mult_time,
    'Sub': sub_time,    
    'Bconvup': bconv_up_time,
    'Bconvdown': bconv_down_time
}
transfer_time_dict = {
    'PCIE': pcie_time,
    'HBM': hbm_time,
    'SRAM': sram_time
}

compute_endtime = 0
PCIE_endtime = 0
HBM_endtime = 0

with open(file_path, newline='') as csvfile:
    reader = csv.reader(csvfile, delimiter=',', skipinitialspace=True)
    
    for row in reader:
        if row[0]=='D':
            depend_time = 0
            if memory_dict.get(row[2]) is not None: 
                depend_time = memory_dict[row[2]]
            if memory_dict.get(row[3]) is not None:
                depend_time = max(depend_time, memory_dict[row[3]])
            
            if row[1]=='PCIE':
                for end_time in PCIE_hop_dict.irange(minimum=depend_time):
                    start_time = PCIE_hop_dict[end_time][0]
                    if depend_time < start_time:
                        depend_time = start_time
                    length = PCIE_hop_dict[end_time][1]
                    result = end_time - length

                    memory_dict[row[2]] = depend_time + transfer_time_dict[row[1]]
                    memory_dict[row[3]] = depend_time + transfer_time_dict[row[1]]

                    if math.isnan(result): # inf - inf = nan 
                        del PCIE_hop_dict[end_time]
                        PCIE_hop_dict[float('inf')] = [depend_time + transfer_time_dict[row[1]], float('inf')] # back hop
                        if depend_time - start_time - transfer_time_dict[row[1]] >= 0:
                            PCIE_hop_dict[depend_time] = [start_time, depend_time - start_time] # front hop
                        break
                    else:
                        del PCIE_hop_dict[end_time]
                        if length - transfer_time_dict[row[1]] - transfer_time_dict[row[1]] >= 0:
                            PCIE_hop_dict[end_time] = [depend_time + transfer_time_dict[row[1]], length - transfer_time_dict[row[1]]] # back hop
                        if depend_time - start_time - transfer_time_dict[row[1]] >= 0:
                            PCIE_hop_dict[depend_time] = [start_time, depend_time - start_time] # front hop
                        break
            else: # HBM or SRAM
                for end_time in HBM_hop_dict.irange(minimum=depend_time):
                    start_time = HBM_hop_dict[end_time][0]
                    if depend_time < start_time:
                        depend_time = start_time
                    length = HBM_hop_dict[end_time][1]
                    if length < transfer_time_dict[row[1]]:
                        continue
                    result = end_time - length
                    memory_dict[row[2]] = depend_time + transfer_time_dict[row[1]]
                    memory_dict[row[3]] = depend_time + transfer_time_dict[row[1]]

                    if math.isnan(result): # inf - inf = nan 
                        del HBM_hop_dict[end_time]
                        HBM_hop_dict[float('inf')] = [depend_time + transfer_time_dict[row[1]], float('inf')] # back hop
                        if depend_time - start_time - transfer_time_dict['SRAM'] >= 0:
                            HBM_hop_dict[depend_time] = [start_time, depend_time - start_time] # front hop
                        break
                    else:
                        del HBM_hop_dict[end_time]
                        if length - transfer_time_dict[row[1]] - transfer_time_dict['SRAM'] >= 0:
                            HBM_hop_dict[end_time] = [depend_time + transfer_time_dict[row[1]], length - transfer_time_dict[row[1]]] # back hop
                        if depend_time - start_time - transfer_time_dict['SRAM'] >= 0:
                            HBM_hop_dict[depend_time] = [start_time, depend_time - start_time] # front hop
                        break
        
        elif row[0]=='C':
            depend_time = 0
            if memory_dict.get(row[2]) is not None:
                depend_time = memory_dict[row[2]]
            if memory_dict.get(row[3]) is not None:
                depend_time = max(depend_time, memory_dict[row[3]])
            if memory_dict.get(row[4]) is not None:
                depend_time = max(depend_time, memory_dict[row[4]])

            for end_time in compute_hop_dict.irange(minimum=depend_time):
                start_time = compute_hop_dict[end_time][0]
                if depend_time < start_time:
                    depend_time = start_time
                length = compute_hop_dict[end_time][1]
                if length < compute_time_dict[row[1]]:
                    continue
                result = end_time - length
                
                memory_dict[row[2]] = depend_time + compute_time_dict[row[1]]
                if row[3] != '0':
                    memory_dict[row[3]] = depend_time + compute_time_dict[row[1]]
                if row[4] != '0':
                    memory_dict[row[4]] = depend_time + compute_time_dict[row[1]]

                if math.isnan(result): # inf - inf = nan 
                    del compute_hop_dict[end_time]
                    compute_hop_dict[float('inf')] = [depend_time + compute_time_dict[row[1]], float('inf')] # back hop
                    if depend_time - start_time - compute_time_dict['Sub'] >= 0:
                        compute_hop_dict[depend_time] = [start_time, depend_time - start_time] # front hop
                    break
                else:
                    del compute_hop_dict[end_time]
                    if length - compute_time_dict[row[1]] - compute_time_dict['Sub'] >= 0:
                        compute_hop_dict[end_time] = [depend_time + compute_time_dict[row[1]], length - compute_time_dict[row[1]]] # back hop
                    if depend_time - start_time - compute_time_dict['Sub'] >= 0:
                        compute_hop_dict[depend_time] = [start_time, depend_time - start_time] # front hop
                    break
                    
                    


# print(memory_dict)
print(PCIE_hop_dict)
print(HBM_hop_dict)
print(compute_hop_dict)
# print(PCIE_hop_dict.keys()[-1])
last_key, last_values = compute_hop_dict.peekitem(-1)
print('reordering')
print(f"compute_endtime : {last_values[0]} ns")
last_key, last_values = PCIE_hop_dict.peekitem(-1)
print(f"PCIE_endtime    : {last_values[0]} ns")
last_key, last_values = HBM_hop_dict.peekitem(-1)
print(f"HBM_endtime     : {last_values[0]} ns")

            
with open(file_path, newline='') as csvfile:
    reader = csv.reader(csvfile, delimiter=',', skipinitialspace=True)        
    for row in reader:
        if row[0]=='D':
            if row[1]=='PCIE':
                PCIE_endtime = PCIE_endtime + transfer_time_dict[row[1]]
            else :
                HBM_endtime = HBM_endtime + transfer_time_dict[row[1]]
        elif row[0]=='C':
            compute_endtime = compute_endtime + compute_time_dict[row[1]]

print('ideal')
print(f"compute_endtime : {compute_endtime} ns")
print(f"PCIE_endtime    : {PCIE_endtime} ns")
print(f"HBM_endtime     : {HBM_endtime} ns")

# print(f"compute_endtime : {compute_endtime/1000000} ms")
# print(f"PCIE_endtime    : {PCIE_endtime/1000000} ms")
# print(f"HBM_endtime     : {HBM_endtime/1000000} ms")