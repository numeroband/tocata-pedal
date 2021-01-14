import Navigation from './components/Navigation';
import Programs from './components/Programs';
import Config from './components/Config';
import Backup from './components/Backup';
import Firmware from './components/Firmware';
import CssBaseline from '@material-ui/core/CssBaseline';
import { useState, useMemo } from 'react';
import { useMediaQuery } from '@material-ui/core';
import { createMuiTheme, ThemeProvider } from '@material-ui/core/styles';

// const client = new WebSocket("ws://" + window.location.host + "/ws");

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

  // useEffect(() => {
  //   client.onopen = () => {
  //     console.log('WebSocket Client Connected');
  //   };
  //   client.onmessage = (event) => {
  //     console.log('received', event)
  //   };
  //   client.onerror = error => console.error(error);
  // }, []);

  const navigation = [
    {
      'path' : '/programs',
      'name' : 'Programs',
      'title' : 'Programs',
      'content' : config => <Programs 
        programs={[]} 
        setPrograms={programs => null} 
        save={savePrograms}
      />
    },
    {
      'path' : '/config',
      'name' : 'Configuration',
      'title' : 'System configuration',
      'content' : config => <Config wifi={null} midi={null} save={saveSystem}/>
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

  async function saveConfig(config) {
    setConfig(config);
    try {
      console.log('Restore');
      await fetch('/api/restore', { 
        method: 'POST',
      })
      for (const id in config.programs) {
        const prg = config.programs[id];
        if (!prg) {
          continue;
        }
        console.log('Sending program', id, prg);
        await fetch(`/api/programs?id=${id}`, { 
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify(prg),          
        })
      }
    } catch (e) {
      console.error('Cannot send program', e);
    }
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
