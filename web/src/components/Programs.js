import { makeStyles } from '@material-ui/core/styles';
import MenuItem from '@material-ui/core/MenuItem';
import TextField from '@material-ui/core/TextField';
import Button from '@material-ui/core/Button';
import { useState, useEffect } from 'react';
import Footswitches from './Footswitches'

const NUM_PROGRAMS = 99;

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

function Program(props) {
  const { id, program, save, remove } = props
  const classes = useStyles();
  const [name, setName] = useState('')
  const [footswitches, setFootswitches] = useState([])

  useEffect(() => {
    setName(program ? program.name : '');
    setFootswitches((program && program.fs) ? program.fs : []);
  }, [id, program]) 

  function saveProgram() {
    save(id, {
      name: name
    })
  }

  function removeProgram() {
    remove(id)
  }

  return (
    <div>
      <div>
        <TextField
          label="Program name"
          className={classes.root}
          value={name}
          onChange={event => setName(event.target.value)}
        ></TextField>
      </div>
      <Footswitches footswitches={footswitches}/>
      <Button 
        variant="contained" 
        color="primary" 
        className={classes.root}
        onClick={saveProgram}
        disabled={!name}
      >Save</Button>
      <Button 
        variant="contained" 
        color="secondary" 
        className={classes.root}
        onClick={removeProgram}
        disabled={!name || !program}
      >Remove</Button>
    </div>
    )
}

export default function Programs(props) {
  const { programs, save } = props
  const classes = useStyles();
  const [programId, setProgramId] = useState(0);
  const [programNames, setProgramNames] = useState(createNames(programs))

  function createNames(programs) {
    const programNames = programs.map((prg, index) => `${index + 1} - ${prg ? prg.name : '<EMPTY>'}`);
    for (let i = programs.length + 1; i <= NUM_PROGRAMS; ++i) {
      programNames.push(`${i} - <EMPTY>`);
    }
    return programNames;  
  }

  const handleChange = (event) => {
    const index = event.target.value;
    setProgramId(index);
  };

  function saveProgram(id, program) {
    console.log('saveProgram', id, program)
    programs[id] = program
    setProgramNames(createNames(programs))
    save(programs)
  }

  function removeProgram(id) {
    console.log('removeProgram', id, programs.length)
    programs[id] = null
    while (programs.length > 0 && !programs[programs.length - 1]) {
      programs.pop()
    }
    setProgramNames(createNames(programs))
    save(programs)
  }

  return (
    <div>
      <TextField 
        label="Program"
        className={classes.root}
        select
        value={programId}
        onChange={handleChange}
      >
        {programNames.map((prg, index) => (
          <MenuItem key={index} value={index}>
            {prg}
          </MenuItem>
        ))}
      </TextField>
      <Program id={programId} program={programs[programId]} save={saveProgram} remove={removeProgram}/>
   </div>
    )
}