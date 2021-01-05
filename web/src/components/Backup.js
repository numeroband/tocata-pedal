import {
  Grid,
  Button,
  makeStyles,
} from '@material-ui/core';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Backup(props) {
  const { config, setConfig } = props;
  const classes = useStyles();

  function uploadFile(event) {
    var files = event.target.files;
    if (files.length <= 0) {
      return false;
    }

    const fr = new FileReader();
    fr.onload = e => restore(JSON.parse(e.target.result));
    fr.readAsText(files.item(0));
  }

  function restore(config) {
    if (!config) {
      alert('Invalid configuration')
      return false;
    }
    setConfig(config);
  }

  return (
    <Grid
      container
      direction="column"
      alignItems="flex-start"
    >
      <Button
        color="primary"
        variant="contained"
        className={classes.root}
        href={`data:text/json;charset=utf-8,${encodeURIComponent(JSON.stringify(config, null, 2))}`}
        download="TocataConfig.json"
      >
        Download config
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
          className={classes.root}>
          Restore config
         </Button>
      </label>
    </Grid>
  );
}