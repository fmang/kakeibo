"""
Le fichier principal du projet kakeibo est le journal (log.tsv) qui contient
les colonnes suivantes :

1. La date de l’opération au format ISO 8601.
2. La catégorie de l’opération (quotidien, facture, …).
3. Le montant pour リク, négatif si c’est une dépense et positif pour un gain.
4. Le montant pour あん, suivant la même convention.
5. Un commentaire. Habituellement, le nom du magasin.
6. L’ID de l’opération.
7. La date de l’enregistrement.
8. L’auteur de l’opération.

Les 5 premiers champs proviennent du tableur utilisé historiquement, et
représente le cœur du journal. Les 3 autres champs sont des métadonnées d’API.
Si plusieurs lignes ont le même ID, seule la dernière est prise en compte.

Le journal est en append-only. Pour changer une opération, il faut en émettre
une nouvelle avec le même ID. Pour supprimer une opération, il faut générer une
ligne avec les 5 premiers champs vides et l’ID de l’opération à supprimer.

Après déduplication des lignes vides, et en supprimant les données API, on
obtient le livre de compte final.
"""

import csv
import sys


def log(*row):
	"""Ajoute une entrée au journal."""
	with open('log.tsv', 'a', newline='') as log:
		writer = csv.writer(log, dialect='excel-tab')
		writer.writerow(row)


def compile():
	"""
	Compile le journal en un livre de comptes représenté par une liste de
	quintuplets. Les métadonnées comme l’ID, la date d’ajout et l’auteur
	sont omises. Quand deux lignes ont le même ID, la dernière l’emporte.
	"""
	entries = [] # Liste de quintuplets pour le livre compilé.
	api_entries = {} # Association { ID: quintuplet } pour la déduplication.
	with open('log.tsv', newline='') as log:
		for row in csv.reader(log, dialect='excel-tab'):
			gist = row[0:5]
			if len(row) > 5 and (id := row[5]):
				if any(gist):
					api_entries[id] = gist
				# La suppression est modélisée en ajoutant une
				# ligne avec les 5 premières colonnes vides,
				# puis l’ID de la ligne à supprimer.
				else:
					del api_entries[id]
			elif any(gist):
				# Les lignes sans ID sont finales.
				entries.append(gist)

	entries.extend(api_entries.values())
	entries.sort()
	return entries


def export_tsv(output):
	"""Exporte le livre compilé au format TSV."""
	writer = csv.writer(output, dialect='excel-tab')
	writer.writerow(('日付', '部類', 'リク', 'あん', '備考'))
	for entry in compile():
		writer.writerow(entry)


if __name__ == "__main__":
	export_tsv(sys.stdout)
