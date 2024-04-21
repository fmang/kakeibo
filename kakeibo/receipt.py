"""
Intègre le programme receipt-scanner pour fournir une interface qui reçoit une
image et renvoie une liste de JSON au format suivant :

	{
		"date": "2024-04-21",
		"amount": 1234,
		"registration": "T1234567890123",
		"category": "日常",
		"remark": "某店"
	}
"""


import argparse
import importlib.resources
import io
import json
import pickle
import subprocess
import sys
import re

import kakeibo.classifier
import kakeibo.stores


DATE_REGEX = re.compile(r'\b(20\d\d)\D([01]?\d)\D([0123]?\d)\b')

# Le total est identifé par la ligne qui commence par 合.
TOTAL_REGEX = re.compile(r'^合.*￥(\d+(?:\D?\d{3})*)$', re.MULTILINE)

# 登録番号 inscrit sur le reçu. Il est de la forme T0123456789123.
REGISTRATION_REGEX = re.compile(r'\bT\d{13}\b')


def parse_receipt(text):
	data = {}

	if m := re.search(DATE_REGEX, text):
		data['date'] = '%d-%02d-%02d' % (int(m[1]), int(m[2]), int(m[3]))

	if m := re.search(TOTAL_REGEX, text):
		data['amount'] = int(''.join(filter(str.isdigit, m[1])))

	if m := re.search(REGISTRATION_REGEX, text):
		data['registration'] = m[0]
		data['category'], data['remark'] = kakeibo.stores.fetch(m[0])

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
