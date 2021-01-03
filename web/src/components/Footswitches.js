import Actions from './Actions';
import {
  FormControlLabel,
  FormGroup,
  Switch,
  MenuItem,
  TextField,
  Grid,
} from '@material-ui/core';
import { makeStyles } from '@material-ui/core/styles';
import {
  Switch as RouteSwitch,
  Route,
  Redirect,
  useRouteMatch,
  useParams,
  useHistory,
} from "react-router-dom";
import { useState, useEffect, useMemo } from 'react';

const ACode = 'A'.charCodeAt(0);
const colors = ['blue', 'purple', 'red', 'yellow', 'green', 'turquoise']

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
  led: {
    margin: theme.spacing(1),
  },
  fsEntry: {
    float: 'left',
    width: 90,
  },
}));

function Led({color}) {
  return (<div style={{
    width: 15,
    height: 15,
    borderWidth: 1,
    borderStyle: 'solid',
    borderRadius: '50%',
    backgroundColor: color,
    float: 'left',
  }}/>);
}

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

  const [onActions, setOnActions] = useState(0)

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
        inputProps={{
          maxLength: 5,
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
        {colors.map((color, index) => <MenuItem key={index} value={color}><Led color={color}/></MenuItem>)}
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
      <div>
        <Actions>On Actions</Actions>
        <Actions>Off Actions</Actions>
      </div>
    </Grid>
  )
}

function FootswitchesSelect(props) {
  const { footswitches, setFootswitches } = props
  const classes = useStyles();
  const {programId, footswitchId} = useParams()
  const history = useHistory();
  // const [footswitchId, setFootswitchId] = useState(0);
  const [footswitchNames, setFootswitchNames] = useState(createNames(footswitches))

  function createNames(footswitches) {
    const NUM_SWITCHES = 6
    const names = footswitches.map(fs => fs ? {name: fs.name, color: fs.color} : null)
    for (let i = footswitches.length; i < NUM_SWITCHES; ++i) {
      names.push(null)
    }
    return names
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

  // useEffect(() => {
  //   setFootswitchNames(createNames(footswitches))
  //   setFootswitchId(0)
  // }, [footswitches]);

  const handleChange = (event) => {
    const index = event.target.value;
    history.push(`/programs/${programId}/${index}`)
    // setFootswitchId(index);
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
        {footswitchNames.map((fs, index) => (          
          <MenuItem key={index} value={index}>
            <div>
              <div className={classes.fsEntry}>{`${String.fromCharCode(ACode + index)} - ${fs ? fs.name : '<EMPTY>'}`}</div>
              {fs ? <Led color={fs.color}/> : <div/>}
            </div>
          </MenuItem>
        ))}
      </TextField>
      <Footswitch id={footswitchId} footswitch={footswitches[footswitchId]} setFootswitch={setFootswitch} />
    </Grid>
  )
}

export default function Footswitches(props) {
  const {footswitches, setFootswitches} = props;
  const { path } = useRouteMatch();

  return (
    <RouteSwitch>
      <Redirect exact from={path} to={`${path}/0/0`}/>
      <Route path={`${path}/:footswitchId`}>
        <FootswitchesSelect footswitches={footswitches} setFootswitches={setFootswitches}/>
      </Route>
    </RouteSwitch>
  );
}