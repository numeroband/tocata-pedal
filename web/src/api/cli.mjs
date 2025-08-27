import { webusb } from 'usb';
import WebSocket from 'ws'
import Api from './Api.mjs';
import process from 'process';
import fs from 'fs';
import UF2 from './UF2.mjs'


function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms))
}

async function main() {
  const command = process.argv[2];
  const transport = process.env['TOCATA_TRANSPORT'] || 'usb';
  const api = new Api((transport === 'ws') ? WebSocket : webusb);
  const start = process.uptime();

  const connect = _ => new Promise((resolve, reject) => {
    api.connectionEvent = connected => connected && resolve();
    api.connect();
  });

  try {
    await connect();
    switch(command) {
      case 'get-config':
      {
        const path = process.argv[3];
        const config = await api.getConfig();
        const data = JSON.stringify(config, null, 2);
        if (path) {
          fs.writeFileSync(path, data);
        } else {
          console.log(data);
        }
        break;
      }
      case 'set-config':
      {
        const path = process.argv[3];
        const config = JSON.parse(fs.readFileSync(path));
        await api.setConfig(config);
        break;
      }
      case 'del-config':
      {
        await api.deleteConfig();
        break;
      }
      case 'get-names':
      {
        const names = await api.getProgramNames();
        const data = JSON.stringify(names, null, 2);
        console.log(data);
        break;
      }
      case 'get-program':
      {
        const id = Number(process.argv[3]);
        const path = process.argv[4];
        const program = await api.getProgram(id);
        const data = JSON.stringify(program, null, 2);
        if (path) {
          fs.writeFileSync(path, data);
        } else {
          console.log(data);
        }
        break;
      }
      case 'set-program':
      {
        const id = Number(process.argv[3]);
        const path = process.argv[4];
        const program = JSON.parse(fs.readFileSync(path));
        await api.setProgram(id, program);
        break;
      }
      case 'del-program':
      {
        const id = Number(process.argv[3]);
        await api.deleteProgram(id);
        break;
      }
      case 'restart':
      {
        await api.restart();
        console.log('restarting');
        break;
      }
      case 'bootrom':
      {
        await api.bootRom();
        console.log('boot rom ready');
        break;
      }
      case 'backup':
      {
        const path = process.argv[3];
        const backup = await api.backup();
        const data = JSON.stringify(backup, null, 2);
        if (path) {
          fs.writeFileSync(path, data);
        } else {
          console.log(data);
        }
        break;
      }
      case 'restore':
      {
        const path = process.argv[3];
        const backup = JSON.parse(fs.readFileSync(path));
        await api.restore(backup);
        break;
      }
      case 'factory':
      {
        await api.factory();
        break;
      }
      case 'flash':
      {
        const path = process.argv[3];
        const content = fs.readFileSync(path);
        const uf2 = new UF2(content.buffer);
        console.log('Flashing firmware');
        await api.flashFirmware(uf2);
        console.log('Verifying firmware');
        await api.verifyFirmware(uf2);
        await sleep(1000);
        console.log('Restarting');
        await api.restart();
        break;
      }
      case 'read':
      {
        const addr = Number(process.argv[3]);
        const length = Number(process.argv[4]);
        const path = process.argv[5];
        const content = await api.readMemory(addr, length);
        fs.writeFileSync(path, content);
        break;
      }
      case 'write':
      {
        const addr = Number(process.argv[3]);
        const path = process.argv[4];
        const content = fs.readFileSync(path);
        await api.writeMemory(addr, content.buffer);
        break;
      }
      case 'erase':
      {
        const addr = Number(process.argv[3]);
        const length = Number(process.argv[4]);
        await api.flashErase(addr, length);
        break;
      }
      case 'uf2-info':
      {
        const path = process.argv[3];
        const content = fs.readFileSync(path);
        const uf2 = new UF2(content.buffer);
        for (const [index, block] of uf2.blocks.entries()) {
          console.log(`--- Block ${index}:`)
          console.log(`flags=0x${block.flags.toString(16)}`);
          console.log(`family=0x${block.familyId.toString(16)}`);
          console.log(`address=0x${block.address.toString(16)}`);
          console.log(`size=${block.payload.length}`);
          console.log();
        }
        break;
      }
      default:
        throw `invalid arg "${command}"`;
    }    
  } catch(e) {
    console.error('ERROR!!!', e);
  }
  console.log('time', process.uptime() - start);
  process.exit(0);   
}

main()
