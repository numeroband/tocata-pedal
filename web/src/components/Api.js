import Api from '../api/Api.mjs';

const NUM_PROGRAMS = 99;

const api = new Api();

export const isSupported = () => "usb" in navigator;
export const isConnected = () => api.connected;
export const connect = (reconnect, ondisconnect) => api.connect(navigator.usb, reconnect, ondisconnect);

export async function readConfig() {
  return api.getConfig();
}

export async function updateConfig(config) {
  return api.setConfig(config);
}

export async function deleteConfig() {
  return api.deleteConfig();
}

export async function readProgramNames() {
  return api.getProgramNames();
}

export async function readProgram(id) {
  return api.getProgram(id);
}

export async function updateProgram(id, program) {
  return api.setProgram(id, program);
}

export async function deleteProgram(id) {
  return api.deleteProgram(id);
}

async function sendRestart() {
  return api.restart();
}

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

function wait(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

export async function restart(progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start restart');
  await sendRestart();
  updateProgress(5);

  const restartDelay = 5000;
  const progressUpdateMs = 500;
  const progressUpdate = (100 - currentProgress) / (restartDelay / progressUpdateMs);
  console.log(`waiting ${restartDelay / 1000} seconds`);
  let waited = 0;
  while (waited < restartDelay)
  {
    await wait(progressUpdateMs);
    waited += progressUpdateMs;
    updateProgress(progressUpdate);
  }
  console.log('end restart');
}
