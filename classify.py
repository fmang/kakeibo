#!/usr/bin/env python

import csv
import numpy as np
import sklearn.metrics
import sklearn.model_selection
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
	x_train, x_test, y_train, y_test = sklearn.model_selection.train_test_split(x, y, test_size=0.5)
	model = sklearn.svm.SVC(gamma=0.05)
	model.fit(x_train, y_train)
	predicted = model.predict(x_test)
	print(sklearn.metrics.classification_report(y_test, predicted))

if __name__ == '__main__':
	main()
