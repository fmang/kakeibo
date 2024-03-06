import argparse
import importlib.resources
import io
import json
import pickle
import subprocess
import sys
import re

import kakeibo.classifier


DATE_REGEX = re.compile(r'(\d{4})[年／](\d{1,2})[月／](\d{1,2})')
TOTAL_REGEX = re.compile(r'^合(?:計|言十).*￥(\d+)$', re.MULTILINE)


def parse_receipt(text):
	data = {}
	if (m := re.search(DATE_REGEX, text)):
		data['date'] = '%d-%02d-%02d' % (int(m[1]), int(m[2]), int(m[3]))
	if (m := re.search(TOTAL_REGEX, text)):
		data['total'] = int(m[1])
	return data or None


def scan_pictures(*pictures_paths):
	model_path = importlib.resources.files('kakeibo') / 'letters.model'
	with open(model_path, 'rb') as f:
		model = pickle.load(f)

	decoded_io = io.StringIO()
	with subprocess.Popen(['receipt-scanner', '--', *pictures_paths], stdout=subprocess.PIPE, text=True) as scanner:
		kakeibo.classifier.decode(model, input=scanner.stdout, output=decoded_io)

	text = decoded_io.getvalue()
	return [receipt for text_block in text.split('\n\n') if (receipt := parse_receipt(text_block))]


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('pictures', metavar='PICTURE', nargs='+')
	args = parser.parse_args()
	receipts = scan_pictures(*args.pictures)
	json.dump(receipts, sys.stdout, indent='\t')
