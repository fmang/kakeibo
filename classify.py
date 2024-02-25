#!/usr/bin/env python

import csv
import numpy as np
import sklearn.metrics
import sklearn.preprocessing
import sklearn.svm
import sys

label_encoder = sklearn.preprocessing.LabelEncoder()

def load_data():
	x = []
	y = []
	for row in csv.reader(sys.stdin):
		label = row[1]
		features = [float(c) / 9 for c in row[2]]
		x.append(features)
		y.append(label)

	x = np.array(x)
	y = label_encoder.fit_transform(y)
	return (x, y)

def main():
	x, y = load_data()
	model = sklearn.svm.SVC(gamma=0.001)
	model.fit(x, y)
	predicted = model.predict(x)
	print(sklearn.metrics.classification_report(y, predicted))

if __name__ == '__main__':
	main()
