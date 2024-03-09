from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

api = FastAPI()

app = FastAPI()
app.mount('/api', api)
app.mount('/', StaticFiles(directory='static', html=True))


@api.get('/ping')
def ping():
	return 'OK'
