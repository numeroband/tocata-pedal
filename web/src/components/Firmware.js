import Button from '@material-ui/core/Button';
import { firmwareUpgrade } from './Api';

export default function Firmware() {
  return <Button
    color="primary"
    variant="contained"
    onClick={firmwareUpgrade}
  >
    Firmware upgrade
  </Button>

}