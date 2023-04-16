import Actions from './Actions';
import {
  FormControlLabel,
  Switch,
  MenuItem,
  TextField,
  Grid,
  Fab,
  Button,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
} from '@material-ui/core';
import { makeStyles } from '@material-ui/core/styles';
import EditIcon from '@material-ui/icons/Edit';
import DeleteIcon from '@material-ui/icons/Delete';
import { useState, useMemo, useEffect } from 'react';

const ACode = 'A'.charCodeAt(0);
const colors = ['blue', 'purple', 'red', 'yellow', 'green', 'turquoise']
const MAX_NAME_LENGTH = 5;

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
  led: {
    margin: theme.spacing(1),
  },
  icon: {
    margin: theme.spacing(2, 0, 0, 1),
  },
  fsEntry: {
    float: 'left',
    width: 90,
  },
}));

function Led({ color }) {
  return (<div style={{
    width: 15,
    height: 15,
    borderWidth: 1,
    borderStyle: 'solid',
    borderRadius: '50%',
    backgroundColor: color,
    float: 'left',
  }} />);
}

function FootswitchDialog(props) {
  const { id, footswitch, setFootswitch, onClose, open } = props
  const classes = useStyles();
  const defaultFS = useMemo(() => ({
    name: '',
    color: colors[0],
    enabled: false,
    momentary: false,
    onActions: [],
    offActions: [],
  }), [])
  const [state, setState] = useState({ ...defaultFS, ...footswitch })
  useEffect(() => setState({ ...defaultFS, ...footswitch }), [footswitch, defaultFS]);

  function updateText(event) {
    setState({ ...state, [event.target.name]: event.target.value });
  };

  function updateSwitch(event) {
    setState({ ...state, [event.target.name]: event.target.checked });
  };

  function update() {
    setFootswitch(id, state);
    onClose();
  }

  return (
    <Dialog onClose={onClose} aria-labelledby="simple-dialog-title" open={open}>
      <DialogTitle id="simple-dialog-title">{`FOOTSWITCH ${String.fromCharCode(ACode + Number(id))}`}</DialogTitle>
      <DialogContent>

        <Grid
          container
          direction="column"
          alignItems="flex-start"
        >
          <TextField
            label="Footswitch name"
            className={classes.root}
            name="name"
            value={state.name}
            onChange={updateText}
            inputProps={{
              maxLength: MAX_NAME_LENGTH,
            }}
          ></TextField>
          <TextField
            label="Color"
            select
            name="color"
            value={state.color}
            className={classes.led}
            onChange={updateText}
          >
            {colors.map((color, index) => <MenuItem key={index} value={color}><Led color={color} /></MenuItem>)}
          </TextField>
          <FormControlLabel
            className={classes.root}
            control={<Switch
              checked={state.enabled}
              name="enabled"
              onChange={updateSwitch}
            />}
            label="Default On"
          />
          <FormControlLabel
            className={classes.root}
            control={<Switch
              checked={state.momentary}
              name="momentary"
              onChange={updateSwitch}
            />}
            label="Momentary"
          />
        </Grid>
      </DialogContent>
      <DialogActions>
        <Button variant="contained" color="primary" onClick={onClose}>Cancel</Button>
        <Button variant="contained" color="primary" onClick={update} disabled={!state.name}>Update
      </Button>
      </DialogActions>
    </Dialog>
  )
}

function Footswitch(props) {
  const { id, mode,  footswitch = {}, setFootswitch } = props
  
  return (!footswitch || !footswitch.name) ? <div/> : (
    <Grid
      container
      direction="row"
      alignItems="flex-start"
    >
      <Actions 
        actions={footswitch.onActions} 
        setActions={actions => setFootswitch(id, {...footswitch, onActions: actions})}
        title={(mode === 'scene') ? "Scene MIDI": "On MIDI"}
      />
      {(mode === 'scene') ? <div/> : <Actions 
        actions={footswitch.offActions} 
        setActions={actions => setFootswitch(id, {...footswitch, offActions: actions})}
        title="Off MIDI"/>}
    </Grid>
  )
}

export default function Footswitches(props) {
  const { mode, footswitches, setFootswitches } = props
  const classes = useStyles();
  const [footswitchNames, setFootswitchNames] = useState(createNames(footswitches))
  const [editFS, setEditFS] = useState(false);
  const [footswitchId, setFootswitchId] = useState(0);

  useEffect(() => setFootswitchNames(createNames(footswitches)), [footswitches])

  function createNames(footswitches) {
    const NUM_SWITCHES = 10
    const names = footswitches.map(fs => fs ? { name: fs.name, color: fs.color } : null);
    for (let i = names.length; i < NUM_SWITCHES; ++i) {
      names.push(null)
    }
    return names
  }

  function setFootswitch(id, newFS) {
    const footswitch = newFS ? { ...newFS } : null;
    if (footswitch && !footswitches[id]) {
      // Default MIDI actions
      footswitch.onActions = [{type: 'CC', values: [Number(id) + 32, 127]}]
      footswitch.offActions = [{type: 'CC', values: [Number(id) + 32, 0]}]
    }
  
    if (footswitch) {
      if (!footswitch.enabled) {
        delete footswitch.enabled
      }
      if (!footswitch.momentary) {
        delete footswitch.momentary
      }
      if (footswitch.onActions && footswitch.onActions.length === 0) {
        delete footswitch.onActions
      }
      if (footswitch.offActions && footswitch.offActions.length === 0) {
        delete footswitch.offActions
      }
    }

    footswitches[id] = footswitch
    while (footswitches.length > 0 && !footswitches[footswitches.length - 1]) {
      footswitches.pop()
    }
    setFootswitches(footswitches)
    setFootswitchNames(createNames(footswitches))
  }

  const handleChange = (event) => {
    const index = event.target.value;
    setFootswitchId(index);
  };

  return (
    <Grid>
      <Grid>
        <TextField
          label="Footswitch"
          className={classes.root}
          select
          value={footswitchId}
          onChange={handleChange}
        >
          {footswitchNames.map((fs, index) => (
            <MenuItem key={index} value={index}>
              <div>
                <div className={classes.fsEntry}>{`${String.fromCharCode(ACode + index)} - ${fs ? fs.name : '<EMPTY>'}`}</div>
                {fs ? <Led color={fs.color} /> : <div />}
              </div>
            </MenuItem>
          ))}
        </TextField>
        <Fab
          className={classes.icon}
          size="small"
          color="primary"
          aria-label="edit"
          onClick={() => setEditFS(true)}
        >
          <EditIcon />
        </Fab>
        <Fab
          className={classes.icon}
          size="small"
          color="secondary"
          aria-label="delete"
          onClick={() => setFootswitch(footswitchId, null)}
        >
          <DeleteIcon />
        </Fab>
        <FootswitchDialog id={footswitchId} footswitch={footswitches[footswitchId]} setFootswitch={setFootswitch} open={editFS} onClose={() => setEditFS(false)} />
      </Grid>
      <Footswitch id={footswitchId} mode={mode} footswitch={footswitches[footswitchId]} setFootswitch={setFootswitch} />
    </Grid>
  )
}
