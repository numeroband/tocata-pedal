import Actions from './Actions';
import Footswitches from './Footswitches'
import { makeStyles } from '@material-ui/core/styles';
import { useState, useEffect, useMemo } from 'react';
import EditIcon from '@material-ui/icons/Edit';
import DeleteIcon from '@material-ui/icons/Delete';
import { readProgram, readProgramNames, updateProgram } from './Api';

import {
  MenuItem,
  TextField,
  Button,
  Fab,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Grid,
  LinearProgress,
} from '@material-ui/core';

import {
  Switch,
  Route,
  Redirect,
  useRouteMatch,
  useParams,
  useHistory,
} from "react-router-dom";

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
  icon: {
    margin: theme.spacing(2, 0, 0, 1),
  },
}));

const nameEntry = (name, index) => `${Number(index) + 1} - ${name ? name : '<EMPTY>'}`

function ProgramDialog(props) {
  const { id, program, setProgram, onClose, open } = props
  const classes = useStyles();
  const defaultProgram = useMemo(() => ({
    name: '',
    actions: [],
    fs: [],
  }), [])
  const [state, setState] = useState({ ...defaultProgram, ...program })
  useEffect(() => setState({ ...defaultProgram, ...program }), [program, defaultProgram])

  function updateText(event) {
    const newState = { ...state, [event.target.name]: event.target.value }
    setState(newState);
  }

  function update() {
    setProgram(id, state);
    onClose();
  }

  return (
    <Dialog onClose={onClose} aria-labelledby="simple-dialog-title" open={open}>
      <DialogTitle id="simple-dialog-title">{`PROGRAM ${Number(id) + 1}`}</DialogTitle>
      <DialogContent>
        <TextField
          label="Name"
          name="name"
          className={classes.root}
          value={state.name}
          onChange={updateText}
        ></TextField>
      </DialogContent>
      <DialogActions>
        <Button variant="contained" color="primary" onClick={onClose}>Cancel</Button>
        <Button variant="contained" color="primary" onClick={update} disabled={!state.name}>Update
      </Button>
      </DialogActions>

    </Dialog>
  )
}

function Program(props) {
  const { id, program, setProgram } = props
  const defaultProgram = useMemo(() => ({
    name: '',
    actions: [],
    fs: [],
  }), [])
  const [state, setState] = useState({ ...defaultProgram, ...program })

  useEffect(() => {
    setState({ ...defaultProgram, ...program })
  }, [program, defaultProgram])

  function setFootswitches(fs) {
    const newProgram = { ...state, fs: fs }
    setState(newProgram)
    setProgram(id, newProgram)
  }

  return !program.name ? <div /> : (
    <div>
      <Grid
        container
        direction="column"
        alignItems="flex-start"
      >
        <Actions
          actions={program.actions}
          setActions={actions => setProgram(id, { ...program, actions: actions })}
          title="Program actions" />
      </Grid>
      <Footswitches footswitches={state.fs ? state.fs : []} setFootswitches={setFootswitches} />
    </div>
  )
}

function ProgramSelect(props) {
  const { path } = props
  const classes = useStyles();
  let { programId } = useParams();
  const history = useHistory();
  const [programNames, setProgramNames] = useState(null);
  const [editProgram, setEditProgram] = useState(false);
  const [program, setProgramState] = useState(null);

  useEffect(() => {
    async function fetchPrograms() {
      setProgramNames(null);
      const names = await readProgramNames();
      setProgramNames(names.map(nameEntry));
    }

    fetchPrograms();
  }, []);

  useEffect(() => {
    async function fetchProgram(id) {
      setProgramState(null);
      setProgramState(await readProgram(id));
    }

    fetchProgram(programId);
  }, [programId]);

  const handleChange = (event) => {
    const index = event.target.value;
    history.push(`${path}/${index}/0`)
  };

  function cleanProgram(program) {
    if (!program) {
      return null;
    }
    if (program.fs && program.fs.length === 0) {
      delete program.fs
    }
    if (program.actions && program.actions.length === 0) {
      delete program.actions
    }
    return program;
  }

  function setProgram(id, program) {
    if (program) {
      updateProgram(id, cleanProgram(program));
    } else {
      deleteProgram(id);
    }

    const names = [...programNames];
    names[id] = nameEntry(program ? program.name : null, id);
    setProgramNames(names);
    setProgramState(program ? program : {});
  }

  function deleteProgram() {
    setProgram(programId, null)
  }

  if (!programNames) {
    return <LinearProgress />;
  }

  return (
    <div>
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
        <Fab
          className={classes.icon}
          size="small"
          color="primary"
          aria-label="edit"
          disabled={!program}
          onClick={() => setEditProgram(true)}
        >
          <EditIcon />
        </Fab>
        <Fab
          className={classes.icon}
          size="small"
          color="secondary"
          aria-label="delete"
          disabled={!program}
          onClick={deleteProgram}
        >
          <DeleteIcon />
        </Fab>
        <ProgramDialog id={programId} program={program} setProgram={setProgram} open={editProgram} onClose={() => setEditProgram(false)} />
      </div>
      {program ?
        <Program
          id={programId}
          program={program}
          setProgram={setProgram}
        /> :
        <LinearProgress />
      }
    </div>
  )
}

export default function Programs(props) {
  const { path } = useRouteMatch();

  return (
    <Switch>
      <Redirect exact from={path} to={`${path}/0/0`} />
      <Route path={`${path}/:programId`}>
        <ProgramSelect path={path} />
      </Route>
    </Switch>
  );
}