"""
Le lecteur le reçu est capable d’extraire le numéro d’inscription
(T1234567890123), qui nous sert de base pour déduire le magasin. Ce module sert
à traduire ce numéro en nom pour le reçu.

Le site du gouvernement japonais permet d’obtenir la liste de toutes les
entreprises capables d’émettre un ticket de caisse officiel. Les CSV
contiennent notamment le numéro d’inscription et le nom de l’entité.
<https://www.invoice-kohyo.nta.go.jp/download/zenken>

`python -m kakeibo.stores *.csv` permet de bâtir la base de données de données
officielles à partir des CSV du gouvernement.
"""

import csv
import re
import sqlite3
import sys

database = sqlite3.connect("stores.db", isolation_level=None)

# Décrit les indications de statut des entreprises, qu’on voudra effacer.
COMPANY_PREFIX = re.compile(r"(株式|有限)会社")


def fetch(registration):
	pass


def load(files):
	"""Reçoit une liste de noms de fichiers CSV et les charges dans stores.db."""
	database.execute("""
		CREATE TABLE IF NOT EXISTS stores (
			registration TEXT PRIMARY KEY,
			name TEXT
		)
	""")

	for f in files:
		batch = []
		with open(f, newline='') as input:
			for row in csv.reader(input):
				registration = row[1]
				name = re.sub(COMPANY_PREFIX, "", row[18])
				batch.append((registration, name))
		database.executemany("INSERT OR REPLACE INTO stores (registration, name) VALUES (?, ?)", batch)


if __name__ == "__main__":
	load(sys.argv[1:])
