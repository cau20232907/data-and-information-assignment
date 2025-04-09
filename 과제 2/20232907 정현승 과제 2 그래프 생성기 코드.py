from matplotlib import pyplot as plt


class Chances:
    def __init__(self):
        self.datax = list()
        self.datay = list()
        self.original = list()
        self.MLEvalue = list()
        self.squareParzen = [list(), list(), list()]
        self.roundParzen = [list(), list(), list()]
        self.squareKNN = [list(), list(), list()]
        self.roundKNN = [list(), list(), list()]

    def setDataPoint(self, height, weight):
        self.datax.append(height)
        self.datay.append(weight)

    def setGaussianValues(self, original, MLEvalue):
        self.original.append(original)
        self.MLEvalue.append(MLEvalue)

    def setSquareParzen(self, data1, data2, data3):
        self.squareParzen[0].append(data1)
        self.squareParzen[1].append(data2)
        self.squareParzen[2].append(data3)

    def setRoundParzen(self, data1, data2, data3):
        self.roundParzen[0].append(data1)
        self.roundParzen[1].append(data2)
        self.roundParzen[2].append(data3)

    def setSquareKNN(self, data1, data2, data3):
        self.squareKNN[0].append(data1)
        self.squareKNN[1].append(data2)
        self.squareKNN[2].append(data3)

    def setRoundKNN(self, data1, data2, data3):
        self.roundKNN[0].append(data1)
        self.roundKNN[1].append(data2)
        self.roundKNN[2].append(data3)

    def getGraphMLE(self, name):
        gragh = (plt.figure(figsize=(9, 6))).add_subplot(111, projection='3d')
        gragh.set_xlabel("height")
        gragh.set_ylabel("weight")
        gragh.set_title(name)
        gragh.scatter(self.datax, self.datay, self.MLEvalue, color='y')
        gragh.scatter(self.datax, self.datay, self.original, color='k')
        plt.show()

    def getGraphSquareParzen(self, name):
        gragh = (plt.figure(figsize=(9, 6))).add_subplot(111, projection='3d')
        gragh.set_xlabel("height")
        gragh.set_ylabel("weight")
        gragh.set_title(name)
        gragh.scatter(self.datax, self.datay, self.squareParzen[0], color='r')
        gragh.scatter(self.datax, self.datay, self.squareParzen[1], color='g')
        gragh.scatter(self.datax, self.datay, self.squareParzen[2], color='b')
        gragh.scatter(self.datax, self.datay, self.original, color='k')
        plt.show()

    def getGraphRoundParzen(self, name):
        gragh = (plt.figure(figsize=(9, 6))).add_subplot(111, projection='3d')
        gragh.set_xlabel("height")
        gragh.set_ylabel("weight")
        gragh.set_title(name)
        gragh.scatter(self.datax, self.datay, self.roundParzen[0], color='r')
        gragh.scatter(self.datax, self.datay, self.roundParzen[1], color='g')
        gragh.scatter(self.datax, self.datay, self.roundParzen[2], color='b')
        gragh.scatter(self.datax, self.datay, self.original, color='k')
        plt.show()

    def getGraphSquareKNN(self, name):
        gragh = (plt.figure(figsize=(9, 6))).add_subplot(111, projection='3d')
        gragh.set_xlabel("height")
        gragh.set_ylabel("weight")
        gragh.set_title(name)
        gragh.scatter(self.datax, self.datay, self.squareKNN[0], color='r')
        gragh.scatter(self.datax, self.datay, self.squareKNN[1], color='g')
        gragh.scatter(self.datax, self.datay, self.squareKNN[2], color='b')
        gragh.scatter(self.datax, self.datay, self.original, color='k')
        plt.show()

    def getGraphRoundKNN(self, name):
        gragh = (plt.figure(figsize=(9, 6))).add_subplot(111, projection='3d')
        gragh.set_xlabel("height")
        gragh.set_ylabel("weight")
        gragh.set_title(name)
        gragh.scatter(self.datax, self.datay, self.roundKNN[0], color='r')
        gragh.scatter(self.datax, self.datay, self.roundKNN[1], color='g')
        gragh.scatter(self.datax, self.datay, self.roundKNN[2], color='b')
        gragh.scatter(self.datax, self.datay, self.original, color='k')
        plt.show()


# datax, datay는 순서까지 모두 같아야 함
def ChanceAverage(chances):
    result = Chances()
    result.datax = chances[0].datax
    result.datay = chances[0].datay
    for i in range(len(chances[0].datax)):
        originalValue = 0
        MLEvalue = 0
        squareParzenValues = [0] * 3
        roundParzenValues = [0] * 3
        squareKNNvalues = [0] * 3
        roundKNNvalues = [0] * 3
        for j in range(len(chances)):
            originalValue += chances[j].original[i]
            MLEvalue += chances[j].MLEvalue[i]
            for k in range(len(squareParzenValues)):
                squareParzenValues[k] += chances[j].squareParzen[k][i]
            for k in range(len(roundParzenValues)):
                roundParzenValues[k] += chances[j].roundParzen[k][i]
            for k in range(len(squareKNNvalues)):
                squareKNNvalues[k] += chances[j].squareKNN[k][i]
            for k in range(len(roundKNNvalues)):
                roundKNNvalues[k] += chances[j].roundKNN[k][i]
        result.setGaussianValues(originalValue / len(chances), MLEvalue / len(chances))
        result.setSquareParzen(squareParzenValues[0] / len(chances), squareParzenValues[1] / len(chances),
                               squareParzenValues[2] / len(chances))
        result.setRoundParzen(roundParzenValues[0] / len(chances), roundParzenValues[1] / len(chances),
                              roundParzenValues[2] / len(chances))
        result.setSquareKNN(squareKNNvalues[0] / len(chances), squareKNNvalues[1] / len(chances),
                            squareKNNvalues[2] / len(chances))
        result.setRoundKNN(roundKNNvalues[0] / len(chances), roundKNNvalues[1] / len(chances),
                           roundKNNvalues[2] / len(chances))
    return result


totalValues = list()
rptval = 10
for i in range(3):
    currentValues = list()
    for j in range(rptval):
        fileValue = Chances()
        file = open(f"확률 분포 결과/20232907-{i * rptval + j + 1:02d}.csv", "rt")
        file.readline()
        line = file.readline()
        while line:
            datas = line.split(',')
            if len(datas) < 16:
                break
            fileValue.setDataPoint(float(datas[0]), float(datas[1]))
            fileValue.setGaussianValues(float(datas[2]), float(datas[3]))
            fileValue.setSquareParzen(float(datas[4]), float(datas[5]), float(datas[6]))
            fileValue.setRoundParzen(float(datas[7]), float(datas[8]), float(datas[9]))
            fileValue.setSquareKNN(float(datas[10]), float(datas[11]), float(datas[12]))
            fileValue.setRoundKNN(float(datas[13]), float(datas[14]), float(datas[15]))
            line = file.readline()
        file.close()
        currentValues.append(fileValue)
    totalValues.append(ChanceAverage(currentValues))
samples = [1000, 5000, 10000]
for i in range(len(totalValues)):
    totalValues[i].getGraphMLE(f"graph of {samples[i]} samples: MLE")
    totalValues[i].getGraphSquareParzen(f"graph of {samples[i]} samples: SquareParzen")
    totalValues[i].getGraphRoundParzen(f"graph of {samples[i]} samples: RoundParzen")
    totalValues[i].getGraphSquareKNN(f"graph of {samples[i]} samples: SquareKNN")
    totalValues[i].getGraphRoundKNN(f"graph of {samples[i]} samples: RoundKNN")
