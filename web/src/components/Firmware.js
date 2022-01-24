import {
  Grid,
  Button,
  LinearProgress,
  makeStyles,
} from '@material-ui/core';
import { flash, verify, restart, version } from './Api';
import UF2 from '../api/UF2.mjs'
import { useState } from 'react';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 170,
  },
}));

export default function Firmware() {
  const classes = useStyles();
  const [progress, setProgress] = useState(-1);

  function handleFirmware(event) {
    var files = event.target.files;
    if (files.length <= 0) {
      return false;
    }

    setProgress(0);
    const fr = new FileReader();
    fr.onload = async e => {
      try {
        const uf2 = new UF2(e.target.result);
        await flash(uf2, p => setProgress(p / 2));
        await verify(uf2, p => setProgress(100 / 2 + p / 2));
        await restart();
      } catch (error) {
        console.error(error);
      }
      setProgress(-1);
    }
    fr.readAsArrayBuffer(files.item(0));
  }

  async function handleRestart() {
    setProgress(0);
    try {
      await restart();
    } catch (error) {
      console.error(error);
    }
    setProgress(-1);
  }

  async function updateToCurrent() {
    setProgress(0);
    try {
      const fwDownload = await fetch(`/tocata-pedal/TocataPedal-${latestVersion}.uf2`)
      const fwBlob = await fwDownload.blob()
      const uf2 = new UF2(await fwBlob.arrayBuffer())
      setProgress(25);
      await flash(uf2, p => setProgress(p / 2));
      await verify(uf2, p => setProgress(100 / 2 + p / 2));
      await restart();
    } catch (error) {
      console.error(error);
    }
    setProgress(-1);
  }

  const {major, minor, subminor} = version();
  const latestVersion = process.env.REACT_APP_VERSION;

  return (
    <div>
      <Grid
        container
        direction="column"
        alignItems="flex-start"
      >
        <h3>Version: {major}.{minor}.{subminor} - Latest: {latestVersion}</h3>
        <Button
          color="primary"
          variant="contained"
          className={classes.root}
          onClick={updateToCurrent}
          disabled={progress >= 0}
        >
          Update to latest
        </Button>
        <input
          accept=".uf2"
          id="restore-file"
          type="file"
          onChange={handleFirmware}
          hidden
        />
        <label htmlFor="restore-file">
          <Button
            color="primary"
            variant="contained"
            component="span"
            disabled={progress >= 0}
            className={classes.root}
          >
            Update to file...
          </Button>
        </label>
        <Button
          color="secondary"
          variant="contained"
          className={classes.root}
          onClick={handleRestart}
          disabled={progress >= 0}
        >
          Restart
        </Button>
      </Grid>
      <div>
        {(progress >= 0) && <LinearProgress variant="determinate" value={progress} />}
      </div>
    </div>
  )
}