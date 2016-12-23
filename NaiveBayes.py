import operator
from numpy import *

def ConvertDataToVec(data):
	vec = [0]*len(data)*11	# [0] * num  of attributes * (0.0 ~ 1.0 = 11)
	attrIdx = 0

	for d in data:
		idx = int(attrIdx * 11) + int(d * 10)
		if idx >= len(vec):
			continue

		vec[idx] = 1
		attrIdx += 1
	return vec

def GetMinMaxRanges(dataSet):
	numpyArr = array(dataSet)
	minValue = numpyArr.min(axis=0)
	maxValue = numpyArr.max(axis=0)
	ranges = maxValue - minValue

	return minValue, maxValue, ranges

class NaiveBayes(object):

	def TrainingDataSet(self, purifiedDataSet, labels, attributes):
		notRichNum = ones(len(purifiedDataSet[0])*11)
		richNum = ones(len(purifiedDataSet[0])*11)

		notRichDenom = 2.0
		richDenom = 2.0

		notRichCount = 0
		richCount = 0

		idx = 0
		for data in purifiedDataSet:
			vec = ConvertDataToVec(data)

			if int(labels[idx]) == 0:
				notRichNum += vec
				notRichDenom += sum(vec)
				notRichCount += 1
			else:
				richNum += vec
				richDenom += sum(vec)
				richCount += 1
			idx += 1

		notRichVect = log(notRichNum / notRichDenom)
		richVect = log(richNum / richDenom)
		notRichPercent = float(notRichCount) / (notRichCount + richCount)

		trainedDataSet = []
		trainedDataSet.append(notRichVect)
		trainedDataSet.append(richVect)
		trainedDataSet.append(notRichPercent)

		return trainedDataSet

	def TestDataSet(self, origDataSet, trainedDataSet, testDataSet, trainedLabels, testLabels, attributes):

		notRichVect, richVect, notRichPercent = trainedDataSet[0:3]

		idx = 0
		errorCount = 0
		totalTest = len(testDataSet)
		currentTest = 0
		targetPercentage = 0.1
		minValue, maxValue, ranges = GetMinMaxRanges(origDataSet)

		for data in testDataSet:
			# normalize
			#for i in range(len(data)):
			#	data[i] = (data[i] - minValue[i]) / (ranges[i])
			#	data[i] = round(data[i], 1)

			vec = ConvertDataToVec(data)

			pNotRich = sum(vec * notRichVect) + log(notRichPercent)
			pRich = sum(vec * richVect) + log(1.0 - notRichPercent)

			if pNotRich > pRich:
				if int(testLabels[idx]) != 0:
					errorCount += 1
			else:
				if int(testLabels[idx]) != 1:
					errorCount += 1
			
			currentTest += 1
			if (currentTest/float(totalTest)) >= targetPercentage:
				print '--- %d percent complete' % (targetPercentage * 100)
				targetPercentage += 0.1
			idx += 1

		return errorCount

	def InspectData(self, origDataSet, trainedDataSet, testData):
		notRichVect, richVect, notRichPercent = trainedDataSet[0:3]
		minValue, maxValue, ranges = GetMinMaxRanges(origDataSet)
		vec = ConvertDataToVec(testData)

		pNotRich = sum(vec * notRichVect) + log(notRichPercent)
		pRich = sum(vec * richVect) + log(1.0 - notRichPercent)

		if pNotRich > pRich:
			return 0
		else:
			return 1
