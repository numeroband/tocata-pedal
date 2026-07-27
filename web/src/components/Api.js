import Api from '../api/Api.mjs';

const NUM_PROGRAMS = 99;
const NUM_SETLISTS = 26;

const urlParams = new URLSearchParams(window.location.search);
const transport = urlParams.get('transport') || 'midi';
let transportClass;
switch (transport) {
  case 'midi':
    navigator.midiDeviceName = urlParams.get('device') || 'Tocata Pedal'
    if (urlParams.has('channel')) {
      navigator.midiChannel = parseInt(urlParams.get('channel'), 10)
    }
    transportClass = navigator;
    break;
  default:
    console.error('Wrong transport class', transportClass);
    break;
}
const api = new Api(transportClass);

export const isSupported = () => ('requestMIDIAccess' in navigator);
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
export const readSetlistNames = () => api.getSetlistNames();
export const readSetlist = id => api.getSetlist(id);
export const updateSetlist = (id, setlist) => api.setSetlist(id, setlist);
export const deleteSetlist = id => api.deleteSetlist(id);
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
  const config = await readConfig();
  updateProgress(5);
  const setlistNames = await readSetlistNames();
  updateProgress(5);
  const programIds = names.reduce((ids, name, index) => name ? [...ids, index] : ids, [])
  const setlistIds = setlistNames.reduce((ids, name, index) => name ? [...ids, index] : ids, [])
  const programs = [];
  const setlists = [];
  const itemProgress = (100 - currentProgress) / (programIds.length + setlistIds.length);
  for (const index in programIds) {
    const id = programIds[index];
    programs[id] = await readProgram(id);
    updateProgress(itemProgress);
  }
  for (const index in setlistIds) {
    const id = setlistIds[index];
    setlists[id] = await readSetlist(id);
    updateProgress(itemProgress);
  }
  progress && progress(100);
  console.log('end readAll');
  return { config, programs, setlists };
}

export async function updateAll({config, programs = [], setlists = []}, progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start updateAll');

  const itemProgress = 99 / (NUM_PROGRAMS + NUM_SETLISTS);
  for (let id = 0; id < NUM_PROGRAMS; ++id)
  {
    const program = programs[id];
    await (program ? updateProgram(id, program) : deleteProgram(id));
    updateProgress(itemProgress);
  }

  for (let id = 0; id < NUM_SETLISTS; ++id)
  {
    const setlist = setlists[id];
    await (setlist ? updateSetlist(id, setlist) : deleteSetlist(id));
    updateProgress(itemProgress);
  }

  await (config ? updateConfig(config) : deleteConfig());
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
  const itemProgress = 99 / (NUM_PROGRAMS + NUM_SETLISTS);
  for (let id = 0; id < NUM_PROGRAMS; ++id)
  {
    await deleteProgram(id);
    updateProgress(itemProgress);
  }
  for (let id = 0; id < NUM_SETLISTS; ++id)
  {
    await deleteSetlist(id);
    updateProgress(itemProgress);
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
