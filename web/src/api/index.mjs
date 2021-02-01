import Api from './Api.mjs';
import process from 'process';
import fs from 'fs';

async function main() {
  const api = new Api();
  const command = process.argv[2];
  const start = process.uptime();

  try {
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
      case 'firmware':
      {
        await api.firmwareUpgrade();
        console.log('firmware upgrade ready');
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
      default:
        throw `invalid arg "${command}"`;
    }    
  } catch(e) {
    console.error('ERROR!!!', e);
  }
  console.log('time', process.uptime() - start);    
}

main()
