from math import sqrt
from numpy.random import randn

meanOfHeight = 182.9
meanOfWeight = 67.7
stdOfHeight = sqrt(92)
stdOfWeight = sqrt(72)
datalen = [1000, 5000, 10000]
rptval = 10
for i in range(len(datalen)):
    for j in range(rptval):
        heights = randn(datalen[i]) * stdOfHeight + meanOfHeight
        weights = randn(datalen[i]) * stdOfWeight + meanOfWeight
        file = open(f"데이터 세트/20232907-{i * rptval + j + 1:02d}.csv", "xt")
        file.write(f"z,{datalen[i]},{2}\n")
        file.write("index,height,weight\n")
        for k in range(datalen[i]):
            file.write(f"{k + 1},{heights[k]},{weights[k]}\n")
        file.close()
