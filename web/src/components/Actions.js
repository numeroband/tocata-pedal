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
import { useState, useMemo, useEffect } from 'react';

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
  }
}));

function actionDescription(action) {
  switch (action.type) {
    case 'PC':
      return action.program
    case 'CC':
      return action.control
    default:
      return 'Unknown';
  }
}

const actionTypes = {
  'PC': 'Program Change',
  'CC': 'Control Change',
}

function ActionDialog(props) {
  const { id, action, setAction, onClose, open } = props;
  const classes = useStyles();
  const defaultAction = useMemo(() => ({
    type: 'PC',
    program: 1,
  }), [])
  const [state, setState] = useState({ ...defaultAction, ...action });
  useEffect(() => setState({...defaultAction, ...action}), [action, defaultAction])

  function updateText(event) {
    setState({ ...state, [event.target.name]: event.target.value });
  };

  function update() {
    setAction(id, state);
    onClose();
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
            onChange={updateText}
          >
            {Object.keys(actionTypes).map((actionType, index) => <MenuItem key={index} value={actionType}>{actionTypes[actionType]}</MenuItem>)}
          </TextField>
          <TextField
            type="number"
            label="Value"
            className={classes.root}
            name="program"
            value={state.program}
            onChange={updateText}
            inputProps={{
              min: 0,
              max: 127,
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
    <div>
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
      </Card>
      <ActionDialog id={state.id} action={state.action} setAction={setAction} open={editAction} onClose={() => setEditAction(false)} />
    </div>
  )
}
