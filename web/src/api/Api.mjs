import Protocol from "./Protocol.mjs";
import {
  parseConfig, 
  serializeConfig,
  parseNames,
  parseProgram, 
  serializeProgram,
  parseAddrPayload,
  serializeAddrPayload,
  serializeAddrLength,
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
const MEM_READ = 0x10;
const MEM_WRITE = 0x11;
const FLASH_ERASE = 0x12;

const NUM_PROGRAMS = 99;

export default class Api {
  constructor(transport) {
    this.protocol = null;
    this.requestQueue = [];
    this.protocol = new Protocol(transport, connected => this.connectionEvent && this.connectionEvent(connected));
  }

  get connected() {
    return this.protocol.connected;
  }

  connect = reconnect => this.protocol.connect(reconnect);

  sendRequest(command, buffer) {
    if (!this.connected) {
      throw new Error('Not connected');
    }
    const request = {command, data: new Uint8Array(buffer)};
    const promise = new Promise((resolve, reject) => {
      request.resolve = resolve;
      request.reject = reject;
    });
    this.requestQueue.push(request);

    const nextRequest = async () => {      
      if (this.requestQueue.length === 0) {
        return;
      }
      const req = this.requestQueue[0];
      try {
        const {data} = await this.protocol.sendRequest(req.command, req.data);
        req.resolve(data.buffer);  
      } catch(e) {
        req.reject(e);
      }
      this.requestQueue.shift();
      nextRequest();
    }

    if (this.requestQueue.length === 1) {
      nextRequest();
    }

    return promise;
  }

  async getConfig() {
    console.log('getConfig');
    const data = await this.sendRequest(GET_CONFIG);
    return parseConfig(data);
  }

  async setConfig(config) {
    console.log('setConfig');
    const data = serializeConfig(config);
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
    const data = serializeProgram({id, program});
    await this.sendRequest(SET_PROGRAM, data);
  }

  async deleteProgram(id) {
    console.log('deleteProgram', id);
    await this.sendRequest(DEL_PROGRAM, new Uint8Array([id]));
  }

  async memRead(address, length) {
    console.log(`memRead ${address.toString(16)} - ${length}`);
    const req = serializeAddrLength({address, length});
    const res = await this.sendRequest(MEM_READ, req);
    const {payload} = parseAddrPayload(res);
    return payload;
  }

  async memWrite(address, payload) {
    console.log(`memWrite ${address.toString(16)} - ${payload.byteLength}`);
    const data = serializeAddrPayload({address, payload});
    await this.sendRequest(MEM_WRITE, data);
  }

  async flashErase(address, length) {
    console.log(`flashErase ${address.toString(16)} - ${length}`);
    const data = serializeAddrLength({address, length});
    await this.sendRequest(FLASH_ERASE, data);
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

  async flashFirmware(uf2) {
    await this.flashErase(uf2.flashStart, uf2.flashEnd - uf2.flashStart);
    for (const block of uf2.blocks) {
      await this.memWrite(block.address, block.payload);
    }
  }

  printBuffer(base, buffer) {
    for (let i = 0; i < buffer.byteLength; i += 16) {
      const length = Math.min(16, buffer.byteLength - i);
      const chunk = new Uint8Array(buffer, i, length);
      const line = Array.prototype.map.call(chunk, x => x.toString(16).toUpperCase().padStart(2, '0')).join(' ');
      const addr = (base + i).toString(16).toUpperCase().padStart(8, '0');
      console.log(`${addr}: ${line}`);
    }
  }

  async verifyFirmware(uf2) {
    for (const block of uf2.blocks) {
      const payload = await this.memRead(block.address, block.payload.byteLength);
      if (payload.byteLength != block.payload.byteLength) {
        throw new Error(`Payload length does not match: local ${block.payload.byteLength} != remote ${payload.byteLength}`);
      }
      const local = new Uint8Array(block.payload);
      const remote = new Uint8Array(payload);
      for (let i = 0; i < block.payload.length; ++i)
      {
        if (local[i] != remote[i]) {
          console.log('local');
          this.printBuffer(block.address, local.buffer);
          console.log('remote');
          this.printBuffer(block.address, remote.buffer);
          throw new Error(`Payload difference in addr 0x${(block.address + i).toString(16)} local 0x${local[i].toString(16)} != remote 0x${remote[i].toString(16)}`);
        }
      }
    }
  }

  async readMemory(addr, length) {
    const result = new Uint8Array(length);
    const chunkSize = 256;
    for (let offset = 0; offset < length; offset += chunkSize) {
      const toRead = Math.min(chunkSize, length - offset);
      const chunk = await this.memRead(addr + offset, toRead);
      result.set(chunk, offset);
    }
    return result;
  }
}
