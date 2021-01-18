import express from 'express';
import http from 'http';
// import WebSocket from 'ws';
import Api from './Api.mjs';
import os from 'os';
import path from 'path';
import Sim from './Sim.mjs';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const api = new Api(path.join(os.homedir(), '.tocata-sim'));
let sim;

const app = express();

//initialize a simple http server
const server = http.createServer(app);

// //initialize the WebSocket server instance
// const wss = new WebSocket.Server({ server });

// wss.on('connection', (ws) => {

//   //connection is up, let's add a simple simple event
//   ws.on('message', (message) => {

//     //log the received message and send it back to the client
//     console.log('received: %s', message);
//     ws.send(`Hello, you sent -> ${message}`);
//   });

//   //send immediatly a feedback to the incoming connection    
//   ws.send('Hi there, I am a WebSocket server');
// });

function toIndex(req) {
  req.url = '/index.html';
  req.next('route');  
}

app.get('/programs', toIndex);
app.get('/config', toIndex);
app.get('/backup', toIndex);
app.get('/firmware', toIndex);

app.use(express.static(path.join(__dirname, '..', 'build')));
app.use(express.json());

app.get('/api/config', (req, res) => api.getConfig(res));

app.post('/api/config', (req, res) => {
  const config = req.body;

  if (!config) {
    return res.status(400).send('Invalid body');
  }

  return api.setConfig(config, res);
});

app.delete('/api/config', (req, res) => api.delConfig(res));

app.get('/api/programs', (req, res) => {
  const id = req.query.id;
  return (id === undefined) ? api.getPrograms(res) : api.getProgram(id, res);
});

app.post('/api/programs', (req, res) => {
  const id = req.query.id;
  const prg = req.body;

  if (id === undefined) {
    return res.status(400).send('Invalid id');
  }

  if (!prg) {
    return res.status(400).send('Invalid body');
  }

  const ret = api.setProgram(id, prg, res);
  sim.programUpdated(Number(id));
  return ret;
});

app.delete('/api/programs', (req, res) => {
  const id = req.query.id;
  return (id === undefined) ? res.status(500).send('Invalid id') : api.delProgram(id, res);
});

app.post('/api/restore', (req, res) => api.restore(res));

app.post('/api/restart', (req, res) => api.restart(res));

//start our server
server.listen(process.env.PORT || 8080, () => {
  const url = `http://localhost:${server.address().port}`
  sim = new Sim(url, id => api.getProgram(id));
});
