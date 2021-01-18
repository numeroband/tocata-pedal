import fs from 'fs'
import child_process from 'child_process';

const MAX_NAME_LENGTH = 30;
const MAX_PROGRAMS = 99;
const EMPTY_NAME = '';

export default class Api {
  constructor(dir, delayMs = 0, logEnabled = false) {
    this.dir = dir;
    this.delayMs = delayMs;
    this.logEnabled = logEnabled;

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

  get maxPrograms() { 
    return MAX_PROGRAMS;
  }

  log(...args) {
    this.logEnabled && console.log(...args);
  }

  delay() {
    this.delayMs && child_process.execSync(`sleep ${this.delayMs / 1000}`);
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
    this.log('getConfig');
    this.delay();
    return fs.existsSync(this.configPath) ? res.sendFile(this.configPath) : res.json({});
  } 

  setConfig(config, res) {
    this.log('setConfig', config);
    this.delay();
    const data = JSON.stringify(config);
    fs.writeFileSync(this.configPath, data);
    return res.json(true);
  }

  delConfig(res) {
    this.log('delConfig');
    this.delay();
    fs.rmSync(this.configPath, {force: true});
    return res.json(true);
  }

  getProgram(id, res) {
    this.log('getProgram', id);
    this.delay();
    const fileName = this.programPath(id);
    const program = fs.existsSync(fileName) ?
       JSON.parse(fs.readFileSync(fileName, 'utf-8')) : {};

    return res ? res.json(program) : {...program, id};
  }

  getPrograms(res) {
    this.log('getPrograms');
    this.delay();
    return res.sendFile(this.namesPath);
  }

  updateNames(id, name) {
    const namesStr = fs.readFileSync(this.namesPath, 'utf-8');
    const namesList = namesStr.match(/.{1,30}/g)
    namesList[id] = name.substring(0, MAX_NAME_LENGTH).padEnd(MAX_NAME_LENGTH);
    fs.writeFileSync(this.namesPath, namesList.join(''));
  }

  setProgram(id, prg, res) {
    this.log('setProgram', id, prg);
    this.delay();
    const fileName = this.programPath(id);
    const data = JSON.stringify(prg);
    fs.writeFileSync(fileName, data);
    this.updateNames(id, prg.name);

    return res.json(true);
  }

  delProgram(id, res) {
    this.log('delProgram', id);
    this.delay();
    const fileName = this.programPath(id);
    fs.rmSync(fileName, {force: true});
    this.updateNames(id, EMPTY_NAME)
    return res.json(true);
  }

  restore(res) {
    this.log('restore');
    this.delay();
    fs.rmdirSync(this.dir, { recursive: true });
    fs.mkdirSync(this.dir);
    this.createEmptyNames();
    return res.json(true);
  }

  restart(res) {
    this.log('restart');
    this.delay();
    return res.json(true);
  }
}
