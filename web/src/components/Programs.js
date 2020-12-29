import { makeStyles } from '@material-ui/core/styles';
import MenuItem from '@material-ui/core/MenuItem';
import TextField from '@material-ui/core/TextField';
import Button from '@material-ui/core/Button';
import { useState } from 'react';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Programs(props) {
  const { config } = props
  const classes = useStyles();
  const [programId, setProgramId] = useState(0);
  const [program, setProgram] = useState(clone(config.programs[programId]));

  function clone(obj) {
    return obj ? JSON.parse(JSON.stringify(obj)) : {name: ''};
  }

  const handleChange = (event) => {
    console.log('handlechange', event)
    const index = event.target.value;
    setProgramId(index);
    setProgram(clone(config.programs[index]));
  };

  const programNames = config.programs.map((prg, index) => `${index + 1} - ${prg ? prg.name : '<EMPTY>'}`)
  if (programNames[programNames.length - 1]) {
    programNames.push(`${programNames.length + 1} - <EMPTY>`)
  }

  function save() {
    config.programs[programId] = clone(program)
  }

  return (
    <form>
      <div>
        <TextField 
          className={classes.root}
          id="program-combo"
          select
          label="Program"
          value={programId}
          onChange={handleChange}
          SelectProps={{
            native: true,
          }}
        >
          {programNames.map((prg, index) => (
            <option key={index} value={index}>
              {prg}
            </option>
          ))}
        </TextField>
        <TextField
          label="Name"
          id="program-name"
          className={classes.root}
          value={program.name}
          onChange={event => {
            program.name = event.target.value;
            setProgram(program);
          }}
        ></TextField>
      </div>
      <Button 
        variant="contained" 
        color="primary" 
        className={classes.root}
        onClick={save}
      >Save</Button>
    </form>
    )
}