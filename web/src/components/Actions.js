import {
  FormControlLabel,
  FormGroup,
  Switch,
  MenuItem,
  TextField,
  Grid,
  Button,
  Badge,
} from '@material-ui/core';
import { makeStyles } from '@material-ui/core/styles';
import { useState, useEffect, useMemo } from 'react';

const useStyles = makeStyles((theme) => ({
  root: {
    margin: theme.spacing(1),
    minWidth: 120,
  },
}));

export default function Actions(props) {  
  const classes = useStyles();
  // const {actions, setActions} = props
  const [actions, setActions] = useState(0)

  return (
    <Badge badgeContent={actions} color="primary">
      <Button
        variant="outlined"
        className={classes.root}
        onClick={() => setActions(a => a + 1)}
      >{props.children}</Button>
    </Badge>
  )
}
