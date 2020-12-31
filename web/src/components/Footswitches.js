import {
  FormControlLabel,
  FormGroup,
  Switch,
  MenuItem,
  TextField,
  Grid,
} from '@material-ui/core';
import { makeStyles } from '@material-ui/core/styles';
import { useState, useEffect, useMemo } from 'react';

const ACode = 'A'.charCodeAt(0);
const NUM_SWITCHES = 6
const colors = ['blue', 'purple', 'red', 'yellow', 'green', 'turquoise']

const useStyles = makeStyles((theme) => {
  const styles = {
    root: {
      margin: theme.spacing(1),
      minWidth: 120,
    },
    color: {
      margin: theme.spacing(1)
    }
  }
  colors.forEach(c => styles[c] = {
    width: 17,
    height: 17,
    borderWidth: 1,
    borderStyle: 'solid',
    borderRadius: '50%',
    backgroundColor: c,
    margin: 'auto',
  })
  return styles
});

function Footswitch(props) {
  const { id, footswitch, setFootswitch } = props
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

  useEffect(() => {
    setState({ ...defaultFS, ...footswitch })
  }, [footswitch, defaultFS])

  function updateText(event) {
    const newState = { ...state, [event.target.name]: event.target.value }
    setState(newState);
    setFootswitch(id, newState.name ? newState : null);
  };

  function updateSwitch(event) {
    const newState = { ...state, [event.target.name]: event.target.checked }
    setState(newState);
    setFootswitch(id, newState.name ? newState : null);
  };

  return (
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
      ></TextField>
      <TextField
        label="Color"
        select
        name="color"
        value={state.color}
        className={classes.color}
        onChange={updateText}
      >
        {colors.map((color, index) => <MenuItem key={index} value={color}><div className={classes[color]} /></MenuItem>)}
      </TextField>
      <FormGroup row className={classes.root}>
        <FormControlLabel
          control={<Switch
            checked={state.enabled}
            name="enabled"
            onChange={updateSwitch}
          />}
          label="Default On"
        />
        <FormControlLabel
          control={<Switch
            checked={state.momentary}
            name="momentary"
            onChange={updateSwitch}
          />}
          label="Momentary"
        />
      </FormGroup>
    </Grid>
  )
}

export default function Footswitches(props) {
  const { footswitches, setFootswitches } = props
  const classes = useStyles();
  const [footswitchId, setFootswitchId] = useState(0);
  const [footswitchNames, setFootswitchNames] = useState(createNames(footswitches))

  function createNames(footswitches) {
    const footswitchNames = footswitches.map((fs, index) => `${String.fromCharCode(ACode + index)} - ${fs ? fs.name : '<EMPTY>'}`);
    for (let i = footswitches.length; i < NUM_SWITCHES; ++i) {
      footswitchNames.push(`${String.fromCharCode(ACode + i)} - <EMPTY>`);
    }
    return footswitchNames;
  }

  function setFootswitch(id, newFS) {    
    const footswitch = newFS ? {...newFS} : null
    if (footswitch) {
      if (!footswitch.enabled) {
        delete footswitch.enabled
      }
      if (!footswitch.momentary) {
        delete footswitch.momentary
      }
      if (footswitch.onActions.length === 0) {
        delete footswitch.onActions
      }
      if (footswitch.offActions.length === 0) {
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

  useEffect(() => {
    setFootswitchNames(createNames(footswitches))
    setFootswitchId(0)
  }, [footswitches]);

  const handleChange = (event) => {
    const index = event.target.value;
    setFootswitchId(index);
  };

  return (
    <Grid>
      <TextField
        label="Footswitch"
        className={classes.root}
        select
        value={footswitchId}
        onChange={handleChange}
      >
        {footswitchNames.map((prg, index) => (
          <MenuItem key={index} value={index}>
            {prg}
          </MenuItem>
        ))}
      </TextField>
      <Footswitch id={footswitchId} footswitch={footswitches[footswitchId]} setFootswitch={setFootswitch} />
    </Grid>
  )
}