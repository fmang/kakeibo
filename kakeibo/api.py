import uvicorn
import os

from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

api = FastAPI()

app = FastAPI()
app.mount('/api', api)
app.mount('/', StaticFiles(directory='static', html=True))


@api.get('/ping')
def ping():
	return 'OK'


if __name__ == '__main__':
	ssl_options = {}
	if os.path.exists('ssl'):
		ssl_options['ssl_keyfile'] = 'ssl/key.pem'
		ssl_options['ssl_certfile'] = 'ssl/cert.pem'
	uvicorn.run('kakeibo.api:app', host='0.0.0.0', port=8443, reload=True, **ssl_options)