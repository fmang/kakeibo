import argparse
import importlib.resources
import io
import json
import pickle
import subprocess
import sys
import re

import kakeibo.classifier


DATE_REGEX = re.compile(r'\b(20\d\d)\D([01]?\d)\D([0123]?\d)\b')

# 言偏 est un peu difficile à lire par sa complexité, mais il est peu probable
# qu’on se trouve sur le +, ou qu’une autre lettre vienne s’intercaler.
TOTAL_REGEX = re.compile(r'^合(?:計|言十|�十).*￥(\d+(?:\D?\d{3})*)$', re.MULTILINE)

# Répertorie les magasins connus sous forme de triplet (catégorie, nom, regex).
# La regex prendra typiquement le numéro de téléphone qui fournit un critère
# facilement repérable en utilisant le modèle de chiffres, plus fiable que la
# reconnaissance de lettres.
STORES = [
	['日常', 'ドコカノミセ', re.compile(r'01\D2345\D6789')],
]


def parse_receipt(text):
	data = {}

	if (m := re.search(DATE_REGEX, text)):
		data['date'] = '%d-%02d-%02d' % (int(m[1]), int(m[2]), int(m[3]))

	if (m := re.search(TOTAL_REGEX, text)):
		data['amount'] = int(''.join(filter(str.isdigit, m[1])))

	for (category, name, regex) in STORES:
		if re.search(regex, text):
			data['category'] = category
			data['remark'] = name
			break

	return data or None


def scan_pictures(*pictures_paths):
	model_path = importlib.resources.files('kakeibo') / 'letters.model'
	with open(model_path, 'rb') as f:
		model = pickle.load(f)

	decoded_io = io.StringIO()
	with subprocess.Popen(['./receipt-scanner', '--', *pictures_paths], stdout=subprocess.PIPE, text=True) as scanner:
		kakeibo.classifier.decode(model, input=scanner.stdout, output=decoded_io)

	text = decoded_io.getvalue()
	return [receipt for text_block in text.split('\n\n') if (receipt := parse_receipt(text_block))]


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--format', choices=['json', 'tsv'], default='json')
	parser.add_argument('pictures', metavar='PICTURE', nargs='+')
	args = parser.parse_args()
	receipts = scan_pictures(*args.pictures)

	if args.format == 'json':
		json.dump(receipts, sys.stdout, indent='\t', ensure_ascii=False)
		print()
	elif args.format == 'tsv':
		for r in receipts:
			print(f"%s\t%s" % (r.get('date', ''), r.get('amount', '')))
