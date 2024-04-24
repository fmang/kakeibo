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

En plus des données du gouvernement, on a le fichier stores.tsv pour retenir en
plus les magasins entrés à la main, avec leur catégorie. Ça permet d’écraser ou
de compléter des données inadaptées.
"""


import csv
import dbm
import re
import sys


def load_custom():
	"""
	Charge la base des magasins connus sous forme de dict { inscription:
	(catégorie, nom) }. Le numéro d’inscription est le la forme
	T0123456789.
	"""
	stores = {}
	try:
		with open('stores.tsv', newline='') as data:
			for row in csv.reader(data, dialect='tsv'):
				stores[row[0]] = (row[1], row[2])
	except FileNotFoundError:
		pass
	return stores


# Charge à l’initialisation la liste des magasins connus. Cette liste est mise
# à jour à chaque fois que l’API enregistre un nouveau magasin.
CUSTOM_DATABASE = load_custom()

def remember(registration, category, name):
	"""
	Ajoute le magasin au fichier stores.tsv, à moins qu’il soit déjà connu
	avec les mêmes propriétés.
	"""
	if CUSTOM_DATABASE.get(registration) == (category, name):
		return

	with open('stores.tsv', 'a', newline='') as data:
		writer = csv.writer(data, dialect='tsv')
		writer.writerow((registration, category, name))

	CUSTOM_DATABASE[registration] = (category, name)


# Pseudo-dict { 登録番号: nom } des magasins d’après les données du gouvernement.
OFFICIAL_DATABASE = dbm.open("stores", "c")

# Motif des indications de statut redondantes dans le nom des magasins.
COMPANY_STATUS = re.compile(r'株式会社|有限会社|コーポレーション')

def import_official(file):
	"""Reçoit une liste de noms de fichiers CSV et les charges dans stores.db."""
	with open(file, newline='') as input:
		for row in csv.reader(input):
			registration = row[1]
			name = row[18]
			if name:
				OFFICIAL_DATABASE[registration] = name


def fetch(registration):
	"""
	Renvoie la paire (category, name) d’un magasin à partir de son numéro
	d’enregistrement. Tente en priorité les données personnalisées, et
	retombe sinon sur la base officielle.
	"""
	if pair := CUSTOM_DATABASE.get(registration):
		return pair
	try:
		name = OFFICIAL_DATABASE[registration].decode()
		name = re.sub(COMPANY_STATUS, '', name)
		return (None, name)
	except KeyError:
		return (None, None)


if __name__ == "__main__":
	for file in sys.argv[1:]:
		import_official(file)
