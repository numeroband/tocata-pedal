import {
  Grid,
  Button,
  LinearProgress,
  makeStyles,
} from '@material-ui/core';
import { useState } from 'react';
import { readAll, updateAll } from './Api';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Backup(props) {
  const classes = useStyles();
  const [progress, setProgress] = useState(-1);

  function uploadFile(event) {
    var files = event.target.files;
    if (files.length <= 0) {
      return false;
    }

    const fr = new FileReader();
    fr.onload = e => restore(JSON.parse(e.target.result));
    fr.readAsText(files.item(0));
  }

  async function restore(config) {
    if (!config) {
      alert('Invalid configuration')
      return false;
    }
    setProgress(0);
    await updateAll(config, setProgress);
    setProgress(-1);
  }

  async function download() {
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
          onClick={download}
          disabled={progress >= 0}
        >
          Backup
      </Button>
        <input
          accept="application/JSON"
          id="restore-file"
          type="file"
          onChange={uploadFile}
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
      </Grid>
      <div>
        {(progress >= 0) && <LinearProgress variant="determinate" value={progress} />}
      </div>
    </div>
  );
}