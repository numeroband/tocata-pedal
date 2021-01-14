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
  return action[scheme[action.type].values[0].field];
}

const scheme = {
  'PC': {
    name: 'Program Change',
    values: [
      {
        name: 'Program',
        field: 'program',
        min: 1,
      },
    ],
  },
  'CC': {
    name: 'Control Change',
    values: [
      {
        name: 'Control',
        field: 'control',
        min: 1,
      },
      {
        name: 'Value',
        field: 'value',
      },
    ],
  },
}

function initAction(actionType = 'PC') {
  return scheme[actionType].values.reduce(
    (previous, current) => ({...previous, [current.field]: (current.min || 0)}), 
    {'type': actionType}
  )
}

function ActionDialog(props) {
  const { id, action, setAction, onClose, open } = props;
  const classes = useStyles();
  const [state, setState] = useState(action || initAction('PC'));
  useEffect(() => setState(action || initAction('PC')), [action, open])

  function updateSelect(event) {
    const actionType = event.target.value;
    setState((action && action.type === actionType) ? action : initAction(actionType));
  };

  function updateText(event) {
    setState({ ...state, [event.target.name]: event.target.value });
  };

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
        name={value.field}
        value={state[value.field] === undefined ? value.min : state[value.field]}
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
