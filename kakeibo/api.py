import csv
import uvicorn
import os
import shutil
import time

from datetime import date, datetime
from fastapi import FastAPI, UploadFile
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

import kakeibo.receipt

api = FastAPI()

app = FastAPI()
app.mount('/api', api)
app.mount('/', StaticFiles(directory='static', html=True))


class Entry(BaseModel):
	id: int | None
	date: date
	amount: int
	remark: str
	category: str


class Withdrawal(BaseModel):
	id: int


@api.get('/ping')
def ping():
	return 'OK'


last_id = 0

def generate_id():
	global last_id
	id = max(int(time.time()), last_id + 1)
	last_id = id
	return id


def log_entry(*row):
	with open('log.tsv', 'a') as log:
		writer = csv.writer(log, dialect='excel-tab')
		writer.writerow(row)


@api.post('/send')
def send(entry: Entry):
	id = entry.id or generate_id()
	log_entry(
		entry.date.isoformat(),
		entry.category,
		entry.amount,
		None, # TODO
		entry.remark,
		id,
		datetime.now().isoformat(),
		None, # TODO Écrire l’auteur.
	)

	return { 'id': id }


@api.post('/withdraw')
def withdraw(withdrawal: Withdrawal):
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
		datetime.now().isoformat(),
		None, # TODO Écrire l’auteur.
	)

	return {}


@api.post('/upload')
def upload(picture: UploadFile):
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
