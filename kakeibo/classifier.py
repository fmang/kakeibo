"""
Module de reconnaissance de chiffres et symboles extraits par receipt-scanner.

Pour l’entrainement, il reçoit des CSV de la forme :

	samples/合/4604.png,合,0002500000772700099009908078880508888810090000900966669009300090

La première colonne est le nom de fichier de l’échantillon, la deuxième
l’étiquette, et la 3e le jeu de features. Chaque chiffre représente une feature
distincte, donc il y a autant de chiffres que de features.

Pour le décodage, il reçoit une entrée texte dont les mots correspondent aux
jeux de features qui ont servi à l’entrainement. Il peut y avoir plusieurs mots
par ligne. Chaque mot est remplacé par l’étiquette attribuée par le
classificateur.

	0799970009911990099000950940009949000099990003959911599099999700
	0009999901998799099000993920029999000298991029908998999009990990
	0002999900000110000999910089997105999800199991002999900099992000
"""

import argparse
import collections
import csv
import numpy as np
import pickle
import sklearn.metrics
import sklearn.model_selection
import sklearn.preprocessing
import sklearn.svm
import sys


"""Représente le contenu d’un fichier de modèle."""
Model = collections.namedtuple('Model', 'label_encoder classifier')


def load_data():
	"""Charge depuis l’entrée standard le CSV des échantillons."""
	x = []
	y = []
	for row in csv.reader(sys.stdin):
		label = row[1]
		features = [float(c) / 9 for c in row[2]]
		x.append(features)
		y.append(label)

	label_encoder = sklearn.preprocessing.LabelEncoder()
	x = np.array(x)
	y = label_encoder.fit_transform(y)
	return (label_encoder, x, y)


def train(test_ratio=0):
	"""Entraine le modèle, et le teste si demandé."""
	label_encoder, x_train, y_train = load_data()
	if test_ratio != 0:
		x_train, x_test, y_train, y_test = sklearn.model_selection.train_test_split(x_train, y_train, test_size=test_ratio)
	classifier = sklearn.svm.SVC(gamma=0.05)
	classifier.fit(x_train, y_train)
	if test_ratio != 0:
		predicted = classifier.predict(x_test)
		print(sklearn.metrics.classification_report(y_test, predicted))
	return Model(label_encoder, classifier)


def decode(model):
	"""
	Reçoit sur l’entrée standard des jeux de features (« mots ») séparés
	par des blancs, et remplace chaque mot par la lettre identifiée.
	"""
	for line in sys.stdin:
		words = [[float(c) / 9 for c in word] for word in line.split()]
		x = np.array(words)
		prediction = model.classifier.predict(x)
		letters = model.label_encoder.inverse_transform(prediction)
		print(''.join(letters))


def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('--decode', action='store_true')
	parser.add_argument('--train', action='store_true')
	parser.add_argument('--test', action='store_true')
	parser.add_argument('model', metavar='MODEL', nargs='?')
	args = parser.parse_args()

	try:
		enabled_modes_count = [args.decode, args.train, args.test].count(True)
		if enabled_modes_count == 0:
			raise ValueError('Aucun mode sélectionné.')
		elif enabled_modes_count > 1:
			raise ValueError('Seul un mode peut être sélectionné.')

		if (args.train or args.decode) and args.model is None:
			raise ValueError('Modèle requis.')

	except ValueError as e:
		parser.print_usage()
		print(e, file=sys.stderr)
		sys.exit(2)

	return args


if __name__ == '__main__':
	args = parse_args()
	if args.test:
		train(test_ratio=0.5)
	elif args.train:
		model = train()
		with open(args.model, 'wb') as f:
			pickle.dump(model, f)
	else:
		with open(args.model, 'rb') as f:
			model = pickle.load(f)
		decode(model)
