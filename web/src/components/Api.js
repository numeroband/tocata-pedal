import Api from '../api/Api.mjs';

const NUM_PROGRAMS = 99;

const urlParams = new URLSearchParams(window.location.search);
const transport = urlParams.get('transport') || 'usb';
const api = (transport === 'ws') ? new Api(WebSocket) : (navigator.usb ? new Api(navigator.usb) : null);

export const isSupported = () => (transport === 'ws' || 'usb' in navigator);
export const isConnected = () => api && api.connected;
export const connect = reconnect => api.connect(reconnect);
export const setConnectionEvent = cb => api.connectionEvent = cb;
export const readConfig = () => api.getConfig();
export const updateConfig = config => api.setConfig(config);
export const deleteConfig = () =>api.deleteConfig();
export const readProgramNames = () => api.getProgramNames();
export const readProgram = id => api.getProgram(id);
export const updateProgram = (id, program) => api.setProgram(id, program);
export const deleteProgram = id => api.deleteProgram(id);
export const version = () => api.version();

export async function readAll(progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start readAll');
  const names = await readProgramNames();
  updateProgress(5);
  const {wifi} = await readConfig();
  updateProgress(5);
  const programIds = names.reduce((ids, name, index) => name ? [...ids, index] : ids, [])
  const programs = [];
  const programProgress = (100 - currentProgress) / programIds.length;
  for (const index in programIds) {
    const id = programIds[index];
    programs[id] = await readProgram(id);
    updateProgress(programProgress);
  }
  progress && progress(100);
  console.log('end readAll');
  return { wifi, programs };
}

export async function updateAll({wifi, programs = []}, progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start updateAll');

  const programProgress = 99 / NUM_PROGRAMS;
  for (let id = 0; id < NUM_PROGRAMS; ++id)
  {
    const program = programs[id];
    await (program ? updateProgram(id, program) : deleteProgram(id));
    updateProgress(programProgress);
  }

  await (wifi ? updateConfig({wifi}) : deleteConfig());
  updateProgress(1);

  console.log('end updateAll');
}

export async function restore(progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start restore');
  const programProgress = 99 / NUM_PROGRAMS;
  for (let id = 0; id < NUM_PROGRAMS; ++id)
  {
    await deleteProgram(id);
    updateProgress(programProgress);
  }
  await deleteConfig();
  updateProgress(1);
  console.log('end restore');
}

export async function restart() {
  console.log('start restart');
  await api.restart();
  console.log('end restart');
}

export async function flash(uf2, progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start flashing');
  await api.flashErase(uf2.flashStart, uf2.flashEnd - uf2.flashStart);
  updateProgress(5);
  const progressUpdate = (100 - currentProgress) / uf2.blocks.length;
  for (const block of uf2.blocks) {
    await api.memWrite(block.address, block.payload);
    updateProgress(progressUpdate);
  }
  console.log('end flashing');
}

export async function verify(uf2, progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start verifying');
  const progressUpdate = 100 / uf2.blocks.length;
  for (const block of uf2.blocks) {
    const payload = await api.memRead(block.address, block.payload.byteLength);
    if (payload.byteLength !== block.payload.byteLength) {
      throw new Error(`Payload length does not match: local ${block.payload.byteLength} != remote ${payload.byteLength}`);
    }
    const local = new Uint8Array(block.payload);
    const remote = new Uint8Array(payload);
    for (let i = 0; i < block.payload.length; ++i)
    {
      if (local[i] !== remote[i]) {
        throw new Error(`Payload difference in addr 0x${(block.address + i).toString(16)} local 0x${local[i].toString(16)} != remote 0x${remote[i].toString(16)}`);
      }
    }
    updateProgress(progressUpdate);
  }
  console.log('end verifying');
}
