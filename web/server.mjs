import express from 'express';
import http from 'http';
import WebSocket from 'ws';
import fs from 'fs'

const MAX_NAME_LENGTH = 30;
const MAX_PROGRAMS = 100;
const EMPTY_NAME = '<EMPTY>';

class Api {
  constructor(dir) {
    this.dir = dir;
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(this.dir);
    }
    if (!fs.existsSync(this.namesPath)) {
      this.createEmptyNames();
    }
  }

  programPath(id) {
    return `${this.dir}/prg.${id}.json`;
  }

  get namesPath() {
    return `${this.dir}/names.txt`;
  }

  createEmptyNames() {
    fs.writeFileSync(this.namesPath, EMPTY_NAME.padEnd(MAX_NAME_LENGTH).repeat(MAX_PROGRAMS));
  }

  getProgram(id, res) {
    const fileName = this.programPath(id);
    if (fs.existsSync(fileName)) {
      return res.sendFile(fileName);
    } else {
      return res.status(404).json({});
    }
  }

  getPrograms(res) {
    return res.sendFile(this.namesPath);
  }

  updateNames(id, name) {
    const namesStr = fs.readFileSync(this.namesPath, 'utf-8');
    const namesList = namesStr.match(/.{1,30}/g)
    namesList[id] = name.substring(0, MAX_NAME_LENGTH).padEnd(MAX_NAME_LENGTH);
    fs.writeFileSync(this.namesPath, namesList.join(''));
  }

  setProgram(id, prg, res) {
    const fileName = this.programPath(id);
    const data = JSON.stringify(prg);
    fs.writeFileSync(fileName, data);
    this.updateNames(id, prg.name);

    return res.json(true);
  }

  delProgram(id, res) {
    const fileName = this.programPath(id);
    fs.rmSync(fileName);
    this.updateNames(id, EMPTY_NAME)
    return res.json(true);
  }

  restore(res) {
    fs.rmdirSync(this.dir, { recursive: true });
    fs.mkdirSync(this.dir);
    this.createEmptyNames();
    return res.json(true);
  }
}

const api = new Api('/tmp/TocataPedal')

const app = express();

//initialize a simple http server
const server = http.createServer(app);

//initialize the WebSocket server instance
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {

  //connection is up, let's add a simple simple event
  ws.on('message', (message) => {

    //log the received message and send it back to the client
    console.log('received: %s', message);
    ws.send(`Hello, you sent -> ${message}`);
  });

  //send immediatly a feedback to the incoming connection    
  ws.send('Hi there, I am a WebSocket server');
});

app.use(express.json());

app.get('/api/programs', (req, res) => {
  const id = req.query.id;
  return (id === undefined) ? api.getPrograms(res) : api.getProgram(id, res);
});

app.post('/api/programs', (req, res) => {
  const id = req.query.id;
  console.log('post programs', id, req.body);
  const prg = req.body;

  if (id === undefined) {
    return res.status(500).send('Invalid id');
  }

  if (!prg) {
    return res.status(500).send('Invalid body');
  }

  return api.setProgram(id, prg, res);
});

app.delete('/api/programs', (req, res) => {
  const id = req.query.id;
  return (id === undefined) ? res.status(500).send('Invalid id') : api.delProgram(id, res);
});

app.post('/api/restore', (req, res) => {
  return api.restore(res);
});

//start our server
server.listen(process.env.PORT || 8080, () => {
  console.log(`Server started on port ${server.address().port} :)`);
});
