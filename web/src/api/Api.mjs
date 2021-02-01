import Protocol from "./Protocol.mjs";
import {
  parseConfig, 
  serializeConfig,
  parseNames,
  parseProgram, 
  serializeProgram,
} from "./Parsers.mjs";

const RESTART = 1;
const FIRMWARE_UPGRADE = 2;
const GET_CONFIG = 3;
const SET_CONFIG = 4;
const DEL_CONFIG = 5;
const GET_NAMES = 6;
const GET_PROGRAM = 7;
const SET_PROGRAM = 8;
const DEL_PROGRAM = 9;

const NUM_PROGRAMS = 99;

export default class Api {
  constructor() {
    this.protocol = null;
    this.buf = new Uint8Array(512);
  }

  async sendRequest(command, buffer) {
    if (!this.protocol) {
      this.protocol = new Protocol();
      await this.protocol.connect();
    }

    const {data} = await this.protocol.sendRequest(command, new Uint8Array(buffer));
    return data.buffer;
  }

  async getConfig() {
    console.log('getConfig');
    const data = await this.sendRequest(GET_CONFIG);
    return parseConfig(data);
  }

  async setConfig(config) {
    console.log('setConfig');
    const data = serializeConfig(this.buf.buffer, config);
    await this.sendRequest(SET_CONFIG, data);
  }

  async deleteConfig() {
    console.log('deleteConfig');
    await this.sendRequest(DEL_CONFIG);
  }

  async getProgramNames() {
    console.log('getProgramNames');
    const names = [];
    while (names.length < NUM_PROGRAMS) {
      const data = await this.sendRequest(GET_NAMES, new Uint8Array([names.length]));
      const res = parseNames(data);
      names.push(...res.names);
    }
    return names;
  }

  async getProgram(id) {
    console.log('getprogram', id);
    const data = await this.sendRequest(GET_PROGRAM, new Uint8Array([id]));
    const {program} = parseProgram(data);
    return program || {};
  }

  async setProgram(id, program) {
    console.log('setprogram', id);
    const data = serializeProgram(this.buf.buffer, {id, program});
    await this.sendRequest(SET_PROGRAM, data);
  }

  async deleteProgram(id) {
    console.log('deleteProgram', id);
    await this.sendRequest(DEL_PROGRAM, new Uint8Array([id]));
  }

  async restart() {
    await this.sendRequest(RESTART);
  }

  async firmwareUpgrade() {
    await this.sendRequest(FIRMWARE_UPGRADE);
  }

  async backup() {
    const res = await this.getConfig();
    const names = await this.getProgramNames();
    res.programs = [];
    for (const id in names) {
      if (names[id]) {
        res.programs[id] = await this.getProgram(id);
      }
    }
    return res;
  }

  async restore(backup) {
    const programs = backup.programs;
    for (let id = 0; id < NUM_PROGRAMS; ++id) {
      const program = programs[id];
      if (program && program.name) {
        await this.setProgram(id, program);
      } else {
        await this.deleteProgram(id);
      }
    }
    await this.setConfig(backup);
  }

  async factory() {
    for (let id = 0; id < NUM_PROGRAMS; ++id) {
      await this.deleteProgram(id);
    }
    await this.deleteConfig();
  }
}