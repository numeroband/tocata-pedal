const MAX_NAME_LENGTH = 30;
const EMPTY_NAME = ' '.repeat(MAX_NAME_LENGTH);

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

export async function restore() {
  console.log('restore');
  await fetch('/api/restore', {
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

export async function updateAll({wifi, programs}, progress) {
  let currentProgress = 0;
  function updateProgress(value) {
    currentProgress += value;
    progress && progress(currentProgress);
  }
  
  console.log('start updateAll');
  // await restore();
  updateProgress(5);
  if (wifi) {
    await updateConfig({wifi});
  }
  updateProgress(5);
  if (programs) {
    const programIds = programs.reduce((ids, name, index) => name ? [...ids, index] : ids, [])
    const programProgress = (100 - currentProgress) / programIds.length;
    for (const index in programIds) {
      const id = programIds[index];
      await updateProgram(id, programs[id]);
      updateProgress(programProgress);
    }
  }
  console.log('end updateAll');
}
