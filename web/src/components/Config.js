import { makeStyles } from '@material-ui/core/styles';
import TextField from '@material-ui/core/TextField';
import Button from '@material-ui/core/Button';
import { useEffect, useState } from 'react';
import { readConfig, updateConfig } from './Api';
import { LinearProgress } from '@material-ui/core';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Config(props) {
  const classes = useStyles();
  const [config, setConfig] = useState(null);
  const [updating, setUpdating] = useState(false);

  useEffect(() => {
    async function fetchConfig() {
      setConfig(null);
      setConfig(await readConfig());
    }

    fetchConfig();
  }, []);

  async function save() {
    setUpdating(true);
    await updateConfig(config);
    setUpdating(false);
  }

  function updateText(event) {
    const [group, value] = event.target.name.split('.');
    setConfig({ ...config, [group]: {...config[group], [value]: event.target.value} });
  };

  if (!config) {
    return <LinearProgress />;
  }

  return (
    <div>
      <div>
        <TextField
          label="SSID"
          className={classes.root}
          name="wifi.ssid"
          value={config.wifi ? config.wifi.ssid : ''}
          onChange={updateText}
        ></TextField>
        <TextField
          label="Key"
          className={classes.root}
          name="wifi.key"
          value={config.wifi ? config.wifi.key : ''}
          onChange={updateText}
        ></TextField>
      </div>
      <Button
        variant="contained"
        color="primary"
        className={classes.root}
        onClick={save}
        disabled={updating}
      >Save</Button>
    </div>
  )
}