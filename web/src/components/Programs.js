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
  Switch,
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
  Prompt,
  useRouteMatch,
  useHistory,
  useLocation,
} from "react-router-dom";

const MAX_NAME_LENGTH = 30;
const DISCARD_PROMPT = 'Are you sure you want to discard the changes made to this program?';
const INVALID_CC_MASK = 0x80;
const isValidCC = cc => ((cc & INVALID_CC_MASK) === 0);
const validCC = cc => (cc & ~INVALID_CC_MASK) & 0xFF;
const invalidCC = cc => (cc | INVALID_CC_MASK) & 0xFF;
const DEFAULT_EXP_CC = 40;
const MODE = Object.freeze({
  stomp: 'stomp',
  scene: 'scene',
});

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
  icon: {
    margin: theme.spacing(2, 0, 0, 1),
  },
  switch: {
    marginTop: 20,
  }
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

function useQuery() {
  return new URLSearchParams(useLocation().search);
}

function ProgramDialog(props) {
  const { id, program, setProgram, onClose, open } = props
  const classes = useStyles();
  const defaultProgram = useMemo(() => ({
    name: '',
    mode: MODE.stomp,
    expression: invalidCC(DEFAULT_EXP_CC),
    expChannel: 0,
    actions: [],
    fs: [],
  }), [])
  const [state, setState] = useState({ ...defaultProgram, ...program })
  useEffect(() => setState({ ...defaultProgram, ...program }), [program, defaultProgram])

  const updateText = event => setState({ ...state, [event.target.name]: event.target.value });
  const updateNumber = event => setState({ ...state, [event.target.name]: Number(event.target.value)});
  const updateNumberPlusOne = event => setState({ ...state, [event.target.name]: event.target.value - 1});
  const updateSwitch = event => {
    const validate = event.target.checked ? validCC : invalidCC;
    setState(state => ({ ...state, [event.target.name]: validate(state.expression)}));
  }

  function update() {
    setProgram(id, state);
    onClose();
  }

  const modeNames = [
    {value: MODE.stomp, name: "Stomp"},
    {value: MODE.scene, name: "Scene"},
  ]

  return (
    <Dialog onClose={onClose} aria-labelledby="simple-dialog-title" open={open}>
      <DialogTitle id="simple-dialog-title">{`PROGRAM ${Number(id) + 1}`}</DialogTitle>
      <DialogContent>
        <Grid
            container
            direction="column"
            alignItems="flex-start"
        >
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
          <TextField
            label="Mode"
            name="mode"
            className={classes.root}
            select
            value={state.mode || MODE.stomp}
            onChange={updateText}
          >
            {modeNames.map((mode, index) => (
              <MenuItem key={index} value={mode.value}>
                {mode.name}
              </MenuItem>
            ))}
          </TextField>
          <div>
            <TextField
              type="number"
              label="Expression CC"
              className={classes.root}
              disabled={!isValidCC(state.expression)}
              name="expression"
              value={validCC(state.expression)}
              onChange={updateNumber}
              inputProps={{
                min: 0,
                max: 127,
              }}
            ></TextField>
            <Switch
                className={classes.switch}
                checked={isValidCC(state.expression)}
                name="expression"
                onChange={updateSwitch}
            />          
          </div>
          <TextField
              type="number"
              label="Exp channel"
              className={classes.root}
              disabled={!isValidCC(state.expression)}
              name="expChannel"
              value={state.expChannel + 1}
              onChange={updateNumberPlusOne}
              inputProps={{
                min: 1,
                max: 16,
              }}
            ></TextField>
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

function Program(props) {
  const { id, program, setProgram } = props
  
  if (!program.name) {
    // Set default MIDI actions for new programs
    program.actions = [{type: 'PC', values: [Number(id)], channel: 0}];
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
      <Footswitches mode={state.mode} footswitches={state.fs ? state.fs : []} setFootswitches={setFootswitches} />
    </Grid>
  )
}

export default function Programs() {
  const classes = useStyles();
  const programId = useQuery().get("id") || 0;
  const history = useHistory();
  const [programNames, setProgramNames] = useState(null);
  const [editProgram, setEditProgram] = useState(false);
  const [program, setProgramState] = useState(null);
  const [orig, setOrig] = useState(null);
  const { path } = useRouteMatch();

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
    if (index === programId) {
      return;
    }

    if (!program.name || !modified()) {
      history.push(`${path}?id=${index}`);
      return;
    }

    if (window.confirm(DISCARD_PROMPT)) {
      setProgram(programId, orig);
      history.push(`${path}?id=${index}`);
    }
  };

  if (!programNames) {
    return <LinearProgress />;
  }

  return (
    <div>
      <Prompt
        when={program && modified()}
        message={ () => DISCARD_PROMPT }
      />
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
            onClick={() => setProgram(programId, JSON.parse(JSON.stringify(orig)))}
          >
            Discard
          </Button>
        </Grid>
      }
    </div>
  )
}
