import { makeStyles } from '@material-ui/core/styles';
import { 
  FormControlLabel, 
  FormGroup,
  Switch,
  MenuItem,
  TextField,
  Grid,
 } from '@material-ui/core';
import { useState, useEffect } from 'react';

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
  const [name, setName] = useState('')
  const [color, setColor] = useState(colors[0])
  const [momentary, setMomentary] = useState(false)
  const [enabled, setEnabled] = useState(false)

  useEffect(() => {
    setName(footswitch ? footswitch.name : '')
    setColor(footswitch ? footswitch.color : colors[0])
  }, [id, footswitch])

  // useEffect(() => setFootswitch(id, {name, color, momentary, enabled}))

  return (
    <Grid
      container
      direction="column"
      alignItems="flex-start"
    >
      <TextField
        label="Footswitch name"
        className={classes.root}
        value={name}
        onChange={e => setName(e.target.value)}
      ></TextField>
      <TextField
        label="Color"
        select
        value={color}
        className={classes.color}
        onChange={event => {
          setColor(event.target.value)
        }}
      >
        {colors.map((color, index) => <MenuItem key={index} value={color}><div className={classes[color]} /></MenuItem>)}
      </TextField>
      <FormGroup row className={classes.root}>
        <FormControlLabel
          control={<Switch checked={enabled} onChange={e => setEnabled(e.target.checked)} />}
          label="Default On"
        />
        <FormControlLabel
          control={<Switch checked={momentary} onChange={e => setMomentary(e.target.checked)} />}
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

  function setFootswitch(id, footswitch) {
    footswitches[id] = footswitch
    while (footswitches.length > 0 && !footswitches[footswitches.length - 1]) {
      footswitches.pop()
    }
    console.log(footswitches)
  }

  useEffect(() => {
    setFootswitchNames(createNames(footswitches))
    setFootswitchId(0)
  }, [footswitches]);

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
      <Footswitch id={footswitchId} footswitch={footswitches[footswitchId]} setFootswitch={setFootswitch}/>
    </Grid>
  )
}