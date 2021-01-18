import {
  Grid,
  Button,
  LinearProgress,
  makeStyles,
} from '@material-ui/core';
import { useState } from 'react';
import { readAll, updateAll, restore, restart } from './Api';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Backup() {
  const classes = useStyles();
  const [progress, setProgress] = useState(-1);

  function handleRestore(event) {
    var files = event.target.files;
    if (files.length <= 0) {
      return false;
    }

    const fr = new FileReader();
    fr.onload = async e => {
      const config = JSON.parse(e.target.result);
      if (!config) {
        alert('Invalid configuration')
        return false;
      }
      setProgress(0);
      await updateAll(config, setProgress);
      setProgress(-1);
    }
    fr.readAsText(files.item(0));
  }

  async function handleBackup() {
    setProgress(0);
    const config = await readAll(setProgress);
    var element = document.createElement('a');
    element.setAttribute('href', `data:text/json;charset=utf-8,${encodeURIComponent(JSON.stringify(config, null, 2))}`);
    element.setAttribute('download', 'TocataConfig.json');
    element.style.display = 'none';

    document.body.appendChild(element);

    element.click();

    document.body.removeChild(element);
    setProgress(-1);
  }

  async function handleReset() {
    setProgress(0);
    await restore(setProgress);
    setProgress(-1);
  }

  async function handleRestart() {
    setProgress(0);
    await restart(setProgress);
    setProgress(-1);
  }

  return (
    <div>
      <Grid
        container
        direction="column"
        alignItems="flex-start"
      >
        <Button
          color="primary"
          variant="contained"
          className={classes.root}
          onClick={handleBackup}
          disabled={progress >= 0}
        >
          Backup
        </Button>
        <input
          accept="application/JSON"
          id="restore-file"
          type="file"
          onChange={handleRestore}
          hidden
        />
        <label htmlFor="restore-file">
          <Button
            color="secondary"
            variant="contained"
            component="span"
            disabled={progress >= 0}
            className={classes.root}>
            Restore
         </Button>
        </label>
        <Button
          color="primary"
          variant="contained"
          className={classes.root}
          onClick={handleReset}
          disabled={progress >= 0}
        >
          Reset
        </Button>
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
  );
}