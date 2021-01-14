import express from 'express';
import http from 'http';
import WebSocket from 'ws';
import fs from 'fs'
import child_process from 'child_process';

const MAX_NAME_LENGTH = 30;
const MAX_PROGRAMS = 99;
const EMPTY_NAME = '';
const DELAY = .2;

function sleep(sec) {
  sec && child_process.execSync(`sleep ${sec}`);
}

class Api {
  constructor(dir) {
    this.dir = dir;
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(this.dir);
    }
    this.createEmptyNames();
    for (let id = 0; id < MAX_PROGRAMS; ++id) {
      if (fs.existsSync(this.programPath(id))) {
        const prg = JSON.parse(fs.readFileSync(this.programPath(id), 'utf-8'));
        this.updateNames(id, prg.name);
      }
    }
  }

  programPath(id) {
    return `${this.dir}/prg.${id}.json`;
  }

  get namesPath() {
    return `${this.dir}/names.txt`;
  }

  get configPath() {
    return `${this.dir}/config.json`;
  }

  createEmptyNames() {
    fs.writeFileSync(this.namesPath, EMPTY_NAME.padEnd(MAX_NAME_LENGTH).repeat(MAX_PROGRAMS));
  }

  getConfig(res) {
    console.log('getConfig');
    sleep(DELAY);
    return fs.existsSync(this.configPath) ? res.sendFile(this.configPath) : res.json({});
  } 

  setConfig(config, res) {
    console.log('setConfig', config);
    sleep(DELAY);
    const data = JSON.stringify(config);
    fs.writeFileSync(this.configPath, data);
    return res.json(true);
  }

  delConfig(res) {
    console.log('delConfig');
    sleep(DELAY);
    fs.rmSync(this.configPath, {force: true});
    return res.json(true);
  }

  getProgram(id, res) {
    console.log('getProgram', id);
    sleep(DELAY);
    const fileName = this.programPath(id);
    if (fs.existsSync(fileName)) {
      return res.sendFile(fileName);
    } else {
      return res.json({});
    }
  }

  getPrograms(res) {
    console.log('getPrograms');
    sleep(DELAY);
    return res.sendFile(this.namesPath);
  }

  updateNames(id, name) {
    const namesStr = fs.readFileSync(this.namesPath, 'utf-8');
    const namesList = namesStr.match(/.{1,30}/g)
    namesList[id] = name.substring(0, MAX_NAME_LENGTH).padEnd(MAX_NAME_LENGTH);
    fs.writeFileSync(this.namesPath, namesList.join(''));
  }

  setProgram(id, prg, res) {
    console.log('setProgram', id, prg);
    sleep(DELAY);
    const fileName = this.programPath(id);
    const data = JSON.stringify(prg);
    fs.writeFileSync(fileName, data);
    this.updateNames(id, prg.name);

    return res.json(true);
  }

  delProgram(id, res) {
    console.log('delProgram', id);
    sleep(DELAY);
    const fileName = this.programPath(id);
    fs.rmSync(fileName, {force: true});
    this.updateNames(id, EMPTY_NAME)
    return res.json(true);
  }

  restore(res) {
    console.log('restore');
    sleep(DELAY);
    fs.rmdirSync(this.dir, { recursive: true });
    fs.mkdirSync(this.dir);
    this.createEmptyNames();
    return res.json(true);
  }

  restart(res) {
    console.log('restart');
    sleep(DELAY);
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

app.get('/api/config', (req, res) => {
  return api.getConfig(res);
});

app.post('/api/config', (req, res) => {
  const config = req.body;

  if (!config) {
    return res.status(400).send('Invalid body');
  }

  return api.setConfig(config, res);
});

app.delete('/api/config', (req, res) => {
  return api.delConfig(res);
});

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

  return api.setProgram(id, prg, res);
});

app.delete('/api/programs', (req, res) => {
  const id = req.query.id;
  return (id === undefined) ? res.status(500).send('Invalid id') : api.delProgram(id, res);
});

app.post('/api/restore', (req, res) => {
  return api.restore(res);
});

app.post('/api/restart', (req, res) => {
  return api.restart(res);
});

//start our server
server.listen(process.env.PORT || 8080, () => {
  console.log(`Server started on port ${server.address().port} :)`);
});
