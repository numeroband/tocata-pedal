import 'fontsource-roboto';
import './App.css';
import Navigation from './components/Navigation';
import Programs from './components/Programs';
import Config from './components/Config';
import Backup from './components/Backup';
import Firmware from './components/Firmware';
import { useEffect, useState } from 'react';

const navigation = [
  {
    'path' : '/programs',
    'name' : 'Programs',
    'title' : 'Programs',
    'content' : config => <Programs config={config}/>
  },
  {
    'path' : '/config',
    'name' : 'Configuration',
    'title' : 'System configuration',
    'content' : config => <Config config={config}/>
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

export default function App() {
  const [config, setConfig] = useState(null)

  useEffect(() => {
    fetch('/config.json')
    .then(response => response.json())
    .then(setConfig)
  }, [])

  return (
    <Navigation navigation={navigation} config={config}/>
  );
}
