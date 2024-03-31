import csv
import uvicorn
import os
import shutil
import time

from datetime import date, datetime
from fastapi import FastAPI, UploadFile, Depends, HTTPException
from fastapi.responses import FileResponse
from fastapi.security import APIKeyCookie
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

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
api_key_cookie = APIKeyCookie(name='kakeibo_api_key')


def authenticate(api_key: str = Depends(api_key_cookie)) -> str:
	"""Authentifie et renvoie l’utilisateur connecté."""
	if user := API_KEYS.get(api_key):
		return user
	else:
		raise HTTPException(401)


@api.get('/connect')
def connect(api_key: str):
	if user := API_KEYS.get(api_key):
		response = FileResponse('static/welcome.html')
		response.set_cookie(key='kakeibo_api_key', value=api_key, secure=True, httponly=True)
		response.set_cookie(key='kakeibo_user', value=user)
		return response
	else:
		raise HTTPException(401)


class Entry(BaseModel):
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
def send(entry: Entry, user: str = Depends(authenticate)):
	id = generate_id()
	log_entry(
		entry.date.isoformat(),
		entry.category,
		entry.amount,
		None, # TODO
		entry.remark,
		id,
		datetime.now().isoformat(timespec='seconds'),
		user,
	)

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
