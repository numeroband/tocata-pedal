import { connect, isSupported, isConnected, setConnectionEvent, version } from './Api';
import React from 'react';
import PropTypes from 'prop-types';
import AppBar from '@material-ui/core/AppBar';
import Button from '@material-ui/core/Button';
import CssBaseline from '@material-ui/core/CssBaseline';
import Divider from '@material-ui/core/Divider';
import Drawer from '@material-ui/core/Drawer';
import Hidden from '@material-ui/core/Hidden';
import IconButton from '@material-ui/core/IconButton';
import List from '@material-ui/core/List';
import ListItem from '@material-ui/core/ListItem';
import ListItemIcon from '@material-ui/core/ListItemIcon';
import ListItemText from '@material-ui/core/ListItemText';
import MenuIcon from '@material-ui/icons/Menu';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';
import { makeStyles, useTheme } from '@material-ui/core/styles';
import {
  Link as RouterLink,
  MemoryRouter,
  Route,
  Switch,
  Redirect,
} from 'react-router-dom';

const drawerWidth = 240;

const useStyles = makeStyles((theme) => ({
  root: {
    display: 'flex',
    margin: theme.spacing(1),
  },
  drawer: {
    [theme.breakpoints.up('sm')]: {
      width: drawerWidth,
      flexShrink: 0,
    },
  },
  appBar: {
    [theme.breakpoints.up('sm')]: {
      width: `calc(100% - ${drawerWidth}px)`,
      marginLeft: drawerWidth,
    },
  },
  menuButton: {
    marginRight: theme.spacing(2),
    [theme.breakpoints.up('sm')]: {
      display: 'none',
    },
  },
  // necessary for content to be below app bar
  toolbar: theme.mixins.toolbar,
  drawerPaper: {
    width: drawerWidth,
  },
  content: {
    flexGrow: 1,
    padding: theme.spacing(3),
  },
}));

function ListItemLink(props) {
  const { icon, primary, to, index, onClick } = props;

  const renderLink = React.useMemo(
    () => React.forwardRef((itemProps, ref) => <RouterLink to={to} ref={ref} {...itemProps} />),
    [to],
  );

  return (
    <li key={index}>
      <ListItem button component={renderLink} onClick={onClick}>
        {icon ? <ListItemIcon>{icon}</ListItemIcon> : null}
        <ListItemText primary={primary} />
      </ListItem>
    </li>
  );
}

ListItemLink.propTypes = {
  icon: PropTypes.element,
  primary: PropTypes.string.isRequired,
  to: PropTypes.string.isRequired,
};

function newFirmwareAvailable(initial) {
  const {major, minor, subminor} = version();
  const currentVersion = `${major}.${minor}.${subminor}`;
  const latestVersion = process.env.REACT_APP_VERSION;

  console.log('newFirmware initial', initial, 'current', currentVersion, 'latest', latestVersion);
  return currentVersion !== latestVersion
}

function Navigation(props) {
  const currentlyConnected = isConnected();
  const { window, navigation } = props
  const classes = useStyles();
  const theme = useTheme();
  const [mobileOpen, setMobileOpen] = React.useState(false);
  const [usbConnected, setUsbConnected] = React.useState(currentlyConnected);
  const [firmwareAvailable, setFirmwareAvailable] = React.useState(currentlyConnected && newFirmwareAvailable(true));

  if (isSupported()) {
    setConnectionEvent(connected => { 
      console.log(connected ? 'connected' : 'disconnected'); 
      setUsbConnected(wasConnected => {
        if (wasConnected !== connected) {
          setFirmwareAvailable(connected ? newFirmwareAvailable(false) : false);
        }
        return connected;
      });
    });
  }

  React.useEffect(() => {
    if (isSupported()) {
      // Try reconnecting
      connect(true).catch(err => console.log('Cannot reconnect', err));
    }  
  }, [])

  const handleDrawerToggle = () => {
    console.log('handleDrawerToggle', mobileOpen)
    setMobileOpen(!mobileOpen);
  };

  const drawer = (
    <div>
      <div className={classes.toolbar}></div>
      <Divider />
      <List>
        {navigation.map(({ path, name }, index) => (
          <ListItemLink key={index} to={path} primary={name} onClick={() => setMobileOpen(false)} />
        ))}
      </List>
    </div>
  );

  function getTitle({ location }) {
    const currentRoute = navigation.filter(({ path }) => location.pathname.startsWith(path))
    return currentRoute.length > 0 ? currentRoute[0].title : 'TocataPedal'
  }

  const container = window !== undefined ? () => window().document.body : undefined;

  return (
    <MemoryRouter>
      <div className={classes.root}>
        <CssBaseline />
        <AppBar position="fixed" className={classes.appBar}>
          <Toolbar>
            <IconButton
              color="inherit"
              aria-label="open drawer"
              edge="start"
              onClick={handleDrawerToggle}
              className={classes.menuButton}
            >
              <MenuIcon />
            </IconButton>
            <Typography variant="h6" noWrap><Route>{getTitle}</Route></Typography>
            {firmwareAvailable && <Button
              color="secondary"
              variant="contained"
              className={classes.root}
              component={RouterLink} 
              to={'/firmware'}
            >
              New firmware available
            </Button>}
          </Toolbar>
        </AppBar>
        <nav className={classes.drawer} aria-label="mailbox folders">
          {/* The implementation can be swapped with js to avoid SEO duplication of links. */}
          <Hidden smUp implementation="css">
            <Drawer
              container={container}
              variant="temporary"
              anchor={theme.direction === 'rtl' ? 'right' : 'left'}
              open={mobileOpen}
              onClose={handleDrawerToggle}
              classes={{
                paper: classes.drawerPaper,
              }}
              ModalProps={{
                keepMounted: true, // Better open performance on mobile.
              }}
            >
              {drawer}
            </Drawer>
          </Hidden>
          <Hidden xsDown implementation="css">
            <Drawer
              classes={{
                paper: classes.drawerPaper,
              }}
              variant="permanent"
              open
            >
              {drawer}
            </Drawer>
          </Hidden>
        </nav>
        <main className={classes.content}>
          <div className={classes.toolbar} />
          <Switch>
            {navigation.map(({ path, content }, index) => (
              <Route path={path} key={index}>
                {isSupported() ? (usbConnected ? 
                    React.createElement(content, {}) : 
                    <Button
                    color="primary"
                    variant="contained"
                    onClick={() => connect().catch(err => console.log('Cannot connect', err))}
                  >
                    Connect to device
                  </Button>
                ) : <Typography>This browser doesn't support USB connections. Please use Google Chrome.</Typography>
            }
              </Route>
            ))}
            <Redirect to="/programs" />
          </Switch>
        </main>
      </div>
    </MemoryRouter>
  );
}

Navigation.propTypes = {
  /**
   * Injected by the documentation to work in an iframe.
   * You won't need it on your project.
   */
  window: PropTypes.func,
};

export default Navigation;
