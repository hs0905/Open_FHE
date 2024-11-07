from sortedcontainers import SortedDict
import math

# SortedDict 생성
sdict = SortedDict()

# 데이터 삽입

sdict[float('inf')] = float('inf')

print(sdict)
# 주어진 key 값 이상의 항목 탐색
key_to_find = 20
for key in sdict.irange(minimum=key_to_find):
    print(f"Key: {key}, Value: {sdict[key]}")

result = float('inf') - float('inf')
print(math.isnan(result))