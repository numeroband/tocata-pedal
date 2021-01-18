import Navigation from './components/Navigation';
import Programs from './components/Programs';
import Config from './components/Config';
import Backup from './components/Backup';
import Firmware from './components/Firmware';
import CssBaseline from '@material-ui/core/CssBaseline';
import { useMemo } from 'react';
import { useMediaQuery } from '@material-ui/core';
import { createMuiTheme, ThemeProvider } from '@material-ui/core/styles';

// const client = new WebSocket("ws://" + window.location.host + "/ws");

export default function App() {
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
      'content' : Programs,
    },
    {
      'path' : '/config',
      'name' : 'Configuration',
      'title' : 'System configuration',
      'content' : Config,
    },
    {
      'path' : '/backup',
      'name' : 'Backup',
      'title' : 'Backup / Restore',
      'content' : Backup,
    },
    {
      'path' : '/firmware',
      'name' : 'Firmware',
      'title' : 'Firmware update',
      'content' : Firmware,
    }
  ]
  
  return (
    <ThemeProvider theme={theme}>
      <CssBaseline/>
      <Navigation navigation={navigation}/>
    </ThemeProvider>
  );
}
