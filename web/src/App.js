import 'fontsource-roboto';
import './App.css';
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
      'content' : config => <Programs programs={config.programs} save={savePrograms}/>
    },
    {
      'path' : '/config',
      'name' : 'Configuration',
      'title' : 'System configuration',
      'content' : config => <Config wifi={config.wifi} midi={config.midi} save={saveConfig}/>
    },
    {
      'path' : '/backup',
      'name' : 'Backup',
      'title' : 'Backup / Restore',
      'content' : config => <Backup config={config}/>
    },
    {
      'path' : '/firmware',
      'name' : 'Firmware',
      'title' : 'Firmware update',
      'content' : config => <Firmware config={config}/>
    }
  ]
  
  function savePrograms(programs) {
    setConfig(config => {
      config.programs = programs
      return config
    })
    console.log('Save config', config)
  }

  function saveConfig(wifi, midi) {
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
