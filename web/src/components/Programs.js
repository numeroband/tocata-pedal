import Actions from './Actions';
import Footswitches from './Footswitches'
import { makeStyles } from '@material-ui/core/styles';
import { useState, useEffect, useMemo } from 'react';
import EditIcon from '@material-ui/icons/Edit';
import DeleteIcon from '@material-ui/icons/Delete';
import { readProgram, readProgramNames, updateProgram, deleteProgram } from './Api';

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
  Divider,
} from '@material-ui/core';

import {
  Switch,
  Route,
  Redirect,
  useRouteMatch,
  useParams,
  useHistory,
} from "react-router-dom";

const MAX_NAME_LENGTH = 30;

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

function compareObjects(a, b) {
  if (a === b) return true;

  if (typeof a != 'object' || typeof b != 'object' || a == null || b == null) return false;

  const keysA = Object.keys(a), keysB = Object.keys(b);

  if (keysA.length !== keysB.length) return false;

  for (let key of keysA) {
    if (!keysB.includes(key) || !compareObjects(a[key], b[key])) return false;
  }

  return true;
}

function ProgramDialog(props) {
  const { id, program, setProgram, onClose, open } = props
  const classes = useStyles();
  const defaultProgram = useMemo(() => ({
    name: '',
    actions: [],
    fs: [],
  }), [id])
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
          inputProps={{
            maxLength: MAX_NAME_LENGTH,
          }}
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

function DiscardDialog(props) {
  const { discard, onClose, open } = props

  return (
    <Dialog onClose={onClose} aria-labelledby="simple-dialog-title" open={open}>
      <DialogTitle id="simple-dialog-title">Discard changes?</DialogTitle>
      {/* <DialogContent>
        Discard changes?
      </DialogContent> */}
      <DialogActions>
        <Button variant="contained" color="primary" onClick={onClose}>Cancel</Button>
        <Button variant="contained" color="primary" onClick={discard}>Discard
      </Button>
      </DialogActions>
    </Dialog>
  )
}

function Program(props) {
  const { id, program, setProgram } = props
  
  if (!program.name) {
    // Set default MIDI actions for new programs
    program.actions = [{type: 'PC', program: Number(id) + 1}];
  }

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
    <Grid
      container
      direction="column"
      alignItems="flex-start"
    >
      <Actions
        actions={program.actions}
        setActions={actions => setProgram(id, { ...program, actions: actions })}
        title="Program MIDI" />
      <Footswitches footswitches={state.fs ? state.fs : []} setFootswitches={setFootswitches} />
    </Grid>
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
  const [orig, setOrig] = useState(null);
  const [discard, setDiscard] = useState(null);

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
      const prog = await readProgram(id);
      setProgramState(prog);
      setOrig(JSON.parse(JSON.stringify(prog)));
    }

    fetchProgram(programId);
  }, [programId]);

  const modified = () => orig && program &&
    !compareObjects(cleanProgram(orig), cleanProgram(program))

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

  function setProgram(id, program, save = false) {
    if (save) {
      const programClean = cleanProgram(program);
      setOrig(programClean);
      updateProgram(id, programClean);
    }

    if (!program) {
      deleteProgram(id);
    }

    const names = [...programNames];
    names[id] = nameEntry(program ? program.name : null, id);
    setProgramNames(names);
    setProgramState(program ? program : {});
  }

  const handleChange = (event) => {
    const index = event.target.value;
    if (index == programId) {
      return;
    }

    if (!program.name || !modified()) {
      history.push(`${path}/${index}/0`);
      return;
    }

    setDiscard(() => () => {
      setDiscard(null);
      setProgram(programId, orig);
      history.push(`${path}/${index}/0`);
    });
  };

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
          disabled={!program || !program.name}
          onClick={() => setProgram(programId, null)}
        >
          <DeleteIcon />
        </Fab>
        <ProgramDialog id={programId} program={program} setProgram={setProgram} open={editProgram} onClose={() => setEditProgram(false)} />
        <DiscardDialog open={discard != null} discard={discard} onClose={() => setDiscard(null)} />
      </div>
      <Divider/>
      {program ?
        <Program
          id={programId}
          program={program}
          setProgram={setProgram}
        /> :
        <LinearProgress />
      }
      {program && program.name &&
        <Grid>
          <Divider/>
          <Button
            color="primary"
            variant="contained"
            className={classes.root}
            disabled={!modified()}
            onClick={() => setProgram(programId, program, true)}
          >
            Save
          </Button>
          <Button
            color="secondary"
            variant="contained"
            className={classes.root}
            disabled={!modified()}
            onClick={() => setProgram(programId, orig)}
          >
            Discard
          </Button>
        </Grid>
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