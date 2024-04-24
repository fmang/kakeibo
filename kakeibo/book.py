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

import argparse
import csv
import datetime
import sys


def log(*row):
	"""Ajoute une entrée au journal."""
	with open('log.tsv', 'a', newline='') as log:
		writer = csv.writer(log, dialect='tsv')
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
		for row in csv.reader(log, dialect='tsv'):
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
	writer = csv.writer(output, dialect='tsv')
	writer.writerow(('日付', '部類', 'リク', 'あん', '備考'))
	for entry in compile():
		writer.writerow(entry)


def check_duplicates(book):
	"""
	Affiche un message quuand on détecte un doublon. L’entrée est déjà
	triée, donc il suffit de vérifier que deux lignes consécutives sont
	toujours différentes.
	"""
	previous_row = None
	for current_row in book:
		if current_row == previous_row:
			print("Doublon :", *current_row)
		previous_row = current_row


def check_monthly_operations(book):
	"""
	Valide que les factures mensuelles et les salaires sont bien payés tous
	les mois, et qu’il n’y pas de paiments trop sérrés ou trop espacés.
	"""
	bills = {} # name: [first_date, last_date, count]
	for row in book:
		date, category, riku, anju, remark = row
		date = datetime.date.fromisoformat(date)

		if category == "給料":
			name = "Salaire " + (remark if remark else "Fred" if riku else "Anju" if anju else "ø")
		elif category == "月額":
			if not remark:
				print("Facture inconnue :", *row)
				continue
			name = "Facture " + remark
		else:
			continue

		if bill := bills.get(name):
			gap = date - bill[1]
			if gap > datetime.timedelta(days=45):
				# On laisse 2 semaines de marges pour les factures manuelles.
				print(f"{name} : Aucun paiement entre {bill[1]} et {date}.")
			elif gap < datetime.timedelta(days=15):
				print(f"{name} : Paiements rapprochés le {bill[1]} et {date}.")
			bill[1] = date
			bill[2] += 1
		else:
			bills[name] = [date, date, 1]

	for name, (first_date, last_date, count) in bills.items():
		year_difference = last_date.year - first_date.year
		month_difference = last_date.month - first_date.month
		# Nombre de mois impactés. Entre le 1er janvier et le 1er mars, on considère qu’on a 3 mois.
		month_span = year_difference * 12 + month_difference + 1
		# Dans le cas serré, on peut avoir un paiement le 1er et le 30, soit 2 sur 1 mois.
		# Inversement, on peut avoir un paiement le 30 janvier et le 1er mars, soit 2 sur 3 mois.
		if count < month_span - 1 or count > month_span + 1:
			print(f"{name} : {count} paiements au lieu de {month_span} entre {first_date} et {last_date}.")


def validate():
	"""Compile et valide que le livre ne contient pas de données louches."""
	book = compile()
	check_duplicates(book)
	check_monthly_operations(book)


def summarize():
	"""Parcourt le livre et donne les totaux."""
	riku = 0
	anju = 0
	for row in compile():
		riku += int(row[2] or 0)
		anju += int(row[3] or 0)
	print("リク : ", riku)
	print("あん : ", anju)
	if riku > anju:
		print(f"リク doit {(riku - anju) // 2:,} à あん.")
	elif anju > riku:
		print(f"あん doit {(anju - riku) // 2:,} à リク.")


def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('--validate', action='store_true')
	parser.add_argument('--summarize', action='store_true')
	args = parser.parse_args()
	return args


if __name__ == "__main__":
	args = parse_args()
	if args.validate:
		validate()
	elif args.summarize:
		summarize()
	else:
		export_tsv(sys.stdout)
