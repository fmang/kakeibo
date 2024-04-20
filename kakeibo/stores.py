"""
Le lecteur le reçu est capable d’extraire le numéro d’inscription
(T1234567890123), qui nous sert de base pour déduire le magasin. Ce module sert
à traduire ce numéro en nom pour le reçu.

Le site du gouvernement japonais permet d’obtenir la liste de toutes les
entreprises capables d’émettre un ticket de caisse officiel. Les CSV
contiennent notamment le numéro d’inscription et le nom de l’entité.
<https://www.invoice-kohyo.nta.go.jp/download/zenken>

`python -m kakeibo.stores *.csv` permet de bâtir la base de données de données
officielles à partir des CSV du gouvernement. Seuls les 法人 sont supportés.
"""

import dbm
import csv
import re
import sys

database = dbm.open("stores", "c")


def fetch(registration):
	"""Renvoie le nom d’un magasin à partir de son numéro d’enregistrement."""
	try:
		return database[registration].decode()
	except KeyError:
		return registration


def load(file):
	"""Reçoit une liste de noms de fichiers CSV et les charges dans stores.db."""
	with open(file, newline='') as input:
		for row in csv.reader(input):
			registration = row[1]
			name = row[18].replace("株式会社", "").replace("有限会社", "")
			if name:
				database[registration] = name


if __name__ == "__main__":
	for file in sys.argv[1:]:
		load(file)
