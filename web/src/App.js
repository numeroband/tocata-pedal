import Navigation from './components/Navigation';
import Programs from './components/Programs';
import Config from './components/Config';
import Backup from './components/Backup';
import Firmware from './components/Firmware';
import CssBaseline from '@material-ui/core/CssBaseline';
import { useEffect, useState, useMemo } from 'react';
import { useMediaQuery } from '@material-ui/core';
import { createMuiTheme, ThemeProvider } from '@material-ui/core/styles';

export default function App() {
  const [config, setConfig] = useState(null)
  const prefersDarkMode = useMediaQuery('(prefers-color-scheme: dark)');

  const theme = useMemo(
    () =>
      createMuiTheme({
        palette: {
          type: prefersDarkMode ? 'dark' : 'light',
        },
      }),
    [prefersDarkMode],
  );

  useEffect(() => {
    fetch('/config.json')
    .then(response => response.json())
    .then(setConfig)
  }, [])

  const navigation = [
    {
      'path' : '/programs',
      'name' : 'Programs',
      'title' : 'Programs',
      'content' : config => <Programs 
        programs={config.programs ? config.programs : []} 
        setPrograms={programs => config.programs = programs} 
        save={savePrograms}
      />
    },
    {
      'path' : '/config',
      'name' : 'Configuration',
      'title' : 'System configuration',
      'content' : config => <Config wifi={config.wifi} midi={config.midi} save={saveSystem}/>
    },
    {
      'path' : '/backup',
      'name' : 'Backup',
      'title' : 'Backup / Restore',
      'content' : config => <Backup config={config} setConfig={saveConfig}/>
    },
    {
      'path' : '/firmware',
      'name' : 'Firmware',
      'title' : 'Firmware update',
      'content' : config => <Firmware config={config}/>
    }
  ]
  
  function savePrograms(programs) {
    return saveConfig({ ...config, programs });
  }

  function saveConfig(config) {
    setConfig(config);
    const str = JSON.stringify(config);
    const bytes = new TextEncoder().encode(str);
    const blob = new Blob([bytes], {
      type: "application/json;charset=utf-8"
    });
    const formData = new FormData();
    formData.append("config", blob, "config.json");
    return fetch('/config.json', { 
      method: 'POST',
      body: formData,
    })
    .then(response => console.log(response))
    .catch(error => console.error(error));
  }

  function saveSystem(wifi, midi) {
    setConfig(config => {
      config.wifi = wifi
      config.midi = midi
      return config
    })
    console.log('Save config', config)
  }

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline/>
      <Navigation navigation={navigation} config={config}/>
    </ThemeProvider>
  );
}
