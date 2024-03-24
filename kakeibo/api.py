import csv
import uvicorn
import os
import time

from datetime import date, datetime
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

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


@api.get('/ping')
def ping():
	return 'OK'


last_id = 0

def generate_id():
	global last_id
	id = max(int(time.time()), last_id + 1)
	last_id = id
	return id


@api.post('/send')
def send(entry: Entry):
	id = entry.id or generate_id()
	row = [
		entry.date.isoformat(),
		entry.category,
		entry.amount,
		None, # TODO
		entry.remark,
		id,
		datetime.now().isoformat(),
		None, # TODO Écrire l’auteur.
	]

	with open('log.tsv', 'a') as log:
		writer = csv.writer(log, dialect='excel-tab')
		writer.writerow(row)

	return { 'id': id }


if __name__ == '__main__':
	ssl_options = {}
	if os.path.exists('ssl'):
		ssl_options['ssl_keyfile'] = 'ssl/key.pem'
		ssl_options['ssl_certfile'] = 'ssl/cert.pem'
	uvicorn.run('kakeibo.api:app', host='0.0.0.0', port=8443, reload=True, **ssl_options)
