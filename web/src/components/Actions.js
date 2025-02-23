import {
  Grid,
  Avatar,
  Chip,
  Card,
  CardHeader,
  CardContent,
  CardActions,
  Button,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  MenuItem,
} from '@material-ui/core';
import { makeStyles } from '@material-ui/core/styles';
import { useState, useEffect } from 'react';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
  chip: {
    margin: theme.spacing(.5, 0),
  },
  card: {
    padding: theme.spacing(1, 2),
  },
}));

function actionDescription(action) {
  return `${scheme[action.type].description(action)}(${action.channel + 1})`;
}

const scheme = {
  'PC': {
    name: 'Program Change',
    values: [
      {
        name: 'Program',
        min: 0,
      },
    ],
    description: action => `${action.values[0]}`,
  },
  'CC': {
    name: 'Control Change',
    values: [
      {
        name: 'Control',
        min: 0,
      },
      {
        name: 'Value',
      },
    ],
    description: action => `${action.values[0]}:${action.values[1]}`,
  },
}

function initAction(actionType = 'PC', channel = 0) {
  const action = {type: actionType, values: [0, 0], channel}
  scheme[actionType].values.forEach((current, i) => action.values[i] = (current.min || 0));
  return action;
}

function ActionDialog(props) {
  const { id, action, setAction, onClose, open } = props;
  const classes = useStyles();
  const [state, setState] = useState(action || initAction('PC'));
  useEffect(() => setState(action || initAction('PC')), [action, open])

  function updateSelect(event) {
    const actionType = event.target.value;
    setState((action && action.type === actionType) ? action : initAction(actionType, action.channel));
  };

  function updateText(event) {
    const values = [...state.values];
    values[event.target.name] = event.target.value;
    setState({ ...state, values });
  };

  const updateNumberPlusOne = event => setState({ ...state, [event.target.name]: event.target.value - 1});

  function update() {
    setAction(id, state);
    onClose();
  }

  function valueField(id) {
    const value = scheme[state.type].values[id];
    return value &&
      <TextField
        type="number"
        label={value.name}
        className={classes.root}
        name={`${id}`}
        value={state.values[id] === undefined ? value.min : state.values[id]}
        onChange={updateText}
        inputProps={{
          min: value.min === undefined ? 0 : value.min,
          max: value.max === undefined ? 127 : value.max,
        }}
      ></TextField>;
  }

  return (
    <Dialog onClose={onClose} aria-labelledby="simple-dialog-title" open={open}>
      <DialogTitle id="simple-dialog-title">{`MIDI ACTION ${Number(id) + 1}`}</DialogTitle>
      <DialogContent>

        <Grid
          container
          direction="column"
          alignItems="flex-start"
        >
          <TextField
            label="Type"
            select
            name="type"
            value={state.type}
            className={classes.led}
            onChange={updateSelect}
          >
            {Object.keys(scheme).map((actionType, index) => <MenuItem key={index} value={actionType}>{scheme[actionType].name}</MenuItem>)}
          </TextField>
          {valueField(0)}
          {valueField(1)}
          <TextField
            type="number"
            label="Channel"
            className={classes.root}
            name="channel"
            value={state.channel + 1}
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
        <Button variant="contained" color="primary" onClick={update}>Update</Button>
      </DialogActions>
    </Dialog>
  )
}

export default function Actions(props) {
  const { actions, setActions, title } = props
  const classes = useStyles();
  const [editAction, setEditAction] = useState(false);
  const [state, setState] = useState({ actions })
  useEffect(() => setState({ actions }), [actions])

  function edit(id) {
    const action = state.actions ? state.actions[id] : null;
    setState({ ...state, id, action });
    setEditAction(true);
  }

  function add() {
    edit(actions ? actions.length : 0);
  }

  function del(id) {
    const actions = [...state.actions];
    actions.splice(id, 1);
    setState({ ...state, actions });
    setActions(actions);
  }

  function setAction(id, action) {
    const actions = state.actions || [];
    actions[id] = action;
    setState({ actions, id, action });
    setActions(actions);
  }

  return (
    <Card className={classes.root}>
      <CardHeader className={classes.card} title={title} titleTypographyProps={{ variant: 'subtitle1' }} />
      <CardContent className={classes.card}>
        <Grid
          container
          direction="column"
          alignItems="flex-start"
        >
          {actions && actions.length > 0 ? actions.map((action, index) =>
            <Chip
              key={index}
              className={classes.chip}
              avatar={<Avatar>{action.type}</Avatar>}
              variant="outlined"
              label={actionDescription(action)}
              clickable
              onClick={() => edit(index)}
              onDelete={() => del(index)}
            />) : "No actions"}
        </Grid>
      </CardContent>
      <CardActions>
        <Button color="primary" variant="contained" size="small" onClick={add}>Add</Button>
      </CardActions>
      <ActionDialog id={state.id} action={state.action} setAction={setAction} open={editAction} onClose={() => setEditAction(false)} />
    </Card>
  )
}
