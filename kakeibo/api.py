import csv
import uvicorn
import os
import shutil
import time

from datetime import date, datetime
from fastapi import FastAPI, UploadFile, Depends, HTTPException
from fastapi.responses import FileResponse, RedirectResponse
from fastapi.security import APIKeyQuery
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from typing import Annotated

import kakeibo.receipt

api = FastAPI()

app = FastAPI()
app.mount('/api', api)
app.mount('/', StaticFiles(directory='static', html=True))


def load_api_keys():
	"""
	Les clés d’API autorisées sont listées dans le fichier api-keys à la
	racine du répertoire d’exécution. Le premier mot de chaque ligne
	désigne la clé, le deuxième l’utilisateur, et le reste est commentaire.
	"""
	with open('api-keys') as db:
		return { fields[0] : fields[1] for line in db if (fields := line.split()) }


API_KEYS = load_api_keys()


def authenticate(api_key: str = Depends(APIKeyQuery(name="key"))) -> str:
	"""Authentifie et renvoie l’utilisateur connecté."""
	if user := API_KEYS.get(api_key):
		return user
	else:
		raise HTTPException(401)


class Entry(BaseModel):
	date: date
	riku: int | None
	anju: int | None
	remark: str
	category: str
	registration: str


class Withdrawal(BaseModel):
	id: int


last_id = 0

def generate_id():
	global last_id
	id = max(int(time.time()), last_id + 1)
	last_id = id
	return id


def log_entry(*row):
	with open('log.tsv', 'a', newline='') as log:
		writer = csv.writer(log, dialect='excel-tab')
		writer.writerow(row)


@api.post('/send')
def send(entry: Entry, user: str = Depends(authenticate)):
	"""Enregistre la transaction dans le journal des opérations."""
	id = generate_id()
	log_entry(
		entry.date.isoformat(),
		entry.category,
		entry.riku,
		entry.anju,
		entry.remark,
		id,
		datetime.now().isoformat(timespec='seconds'),
		user,
	)

	if entry.registration:
		kakeibo.receipt.remember_store(entry.registration, entry.category, entry.remark)

	return { 'id': id }


@api.post('/withdraw')
def withdraw(withdrawal: Withdrawal, user: str = Depends(authenticate)):
	"""
	Ajoute une ligne avec tous les champs vide sauf l’ID et les métadonnées
	API pour marquer la pierre tombale.
	"""
	log_entry(
		None,
		None,
		None,
		None,
		None,
		withdrawal.id,
		datetime.now().isoformat(timespec='seconds'),
		user,
	)

	return {}


@api.get('/download')
def download(user: str = Depends(authenticate)):
	"""
	Compile le journal en rapport TSV. Les métadonnées comme l’ID, la date
	d’ajout et l’auteur sont omises. Quand deux lignes ont le même ID, la
	dernière l’emporte. Le fichier généré est ensuite archivé dans
	downloads/ puis on propose à l’utilisateur de le télécharger.
	"""
	entries = {}
	with open('log.tsv', newline='') as log:
		reader = csv.reader(log, dialect='excel-tab')
		for index, row in enumerate(reader, start=1):
			# Quand une ligne n’a pas d’ID, on lui en génère un
			# artificiel. Il doit être unique et ne pas faire
			# conflit avec les vrais ID.
			id = row[5] if len(row) >= 6 else -index

			# Le journal modélise la suppression en ajoutant une
			# ligne avec les 5 premières colonnes vides, puis l’ID
			# de la ligne à supprimer.
			gist = row[0:5]
			if any(gist):
				entries[id] = gist
			else:
				del entries[id]

	os.makedirs('downloads', exist_ok=True)
	report_name = f"kakeibo-{date.today().isoformat()}.tsv"
	report_path = f"downloads/{report_name}"
	with open(report_path, 'w', newline='') as report:
		writer = csv.writer(report, dialect='excel-tab')
		for entry in sorted(entries.values()):
			writer.writerow(entry)

	return FileResponse(report_path, filename=report_name)


@api.post('/upload')
def upload(picture: UploadFile, user: str = Depends(authenticate)):
	os.makedirs('uploads', exist_ok=True)
	destination = f"uploads/{generate_id()}"
	with open(destination, "wb") as output:
		shutil.copyfileobj(picture.file, output)
	picture.file.close()

	return { 'receipts': kakeibo.receipt.scan_pictures(destination) }


if __name__ == '__main__':
	ssl_options = {}
	if os.path.exists('ssl'):
		ssl_options['ssl_keyfile'] = 'ssl/key.pem'
		ssl_options['ssl_certfile'] = 'ssl/cert.pem'
	uvicorn.run('kakeibo.api:app', host='0.0.0.0', port=8443, reload=True, **ssl_options)
