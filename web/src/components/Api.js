const MAX_NAME_LENGTH = 30;
const EMPTY_NAME = ' '.repeat(MAX_NAME_LENGTH);
const NUM_PROGRAMS = 99;

export async function readConfig() {
  console.log('readConfig');
  const response = await fetch('/api/config');
  return await response.json();
}

export async function updateConfig(config) {
  console.log('updateConfig', config);
  await fetch('/api/config', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(config),
  })
}

export async function deleteConfig() {
  console.log('deleteConfig');
  await fetch('/api/config', {
    method: 'DELETE'
  })
}

export async function readProgramNames() {
  console.log('readProgramNames');
  const response = await fetch('/api/programs');
  const namesStr = await response.text();
  const namesList = namesStr.match(/.{1,30}/g);
  return namesList.map(name => name === EMPTY_NAME ? null : name);
}

export async function readProgram(id) {
  console.log('readProgram', id);
  const response = await fetch(`/api/programs?id=${id}`);
  return await response.json();
}

export async function updateProgram(id, program) {
  console.log('updateProgram', id, program);
  await fetch(`/api/programs?id=${id}`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(program),
  })
}

export async function deleteProgram(id) {
  console.log('deleteProgram', id);
  await fetch(`/api/programs?id=${id}`, {
    method: 'DELETE'
  })
}

async function sendRestart() {
  console.log('restart');
  await fetch('/api/restart', {
    method: 'POST'
  })
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
