import { makeStyles } from '@material-ui/core/styles';
import TextField from '@material-ui/core/TextField';
import Button from '@material-ui/core/Button';
import { useState } from 'react';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Config(props) {
  const { wifi, midi, save } = props
  const classes = useStyles();
  const [ssid, setSsid] = useState(wifi ? wifi.ssid : '')
  const [key, setKey] = useState(wifi ? wifi.key : '')

  function sendSave() {
    wifi.ssid = ssid
    wifi.key = key
    save(wifi, midi)
  }

  return (
    <div>
      <div>
        <TextField
          label="SSID"
          className={classes.root}
          value={ssid}
          onChange={event => setSsid(event.target.value)}
        ></TextField>
        <TextField
          label="Key"
          className={classes.root}
          value={key}
          onChange={event => setKey(event.target.value)}
        ></TextField>
      </div>
      <Button
        variant="contained"
        color="primary"
        className={classes.root}
        onClick={sendSave}
      >Save</Button>
    </div>
  )
}