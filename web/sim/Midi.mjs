import midi from 'midi'
import terminal_kit from 'terminal-kit'
const term = terminal_kit.terminal;

const MIDI_IN_PORT = 'TocataPedal IN';
const NUM_SWITCHES = 6;
const MAX_PROGRAMS = 99;
const A_CODE = 'A'.charCodeAt(0);
const colors = ['red', 'green', 'yellow', 'blue', 'magenta', 'cyan'];

export default class Midi {
    constructor(portName = MIDI_IN_PORT) {
        this.output = new midi.Output();

        let outPort;
        const portCount = this.output.getPortCount();
        for (let i = 0; i < portCount; ++i) {
            const name = this.output.getPortName(i);
            if (name.startsWith(portName)) {
                outPort = i;
                break;
            }
        }
        
        if (outPort === undefined) {
            throw `Cannot find '${MIDI_IN_PORT}' MIDI port`;
        }

        this.output.openPort(outPort);
    }

    sendActions(actions) {
        if (!actions) {
            return;
        }

        for (let action of actions) {
            switch (action.type) {
            case 'PC':
                this.sendProgram(action.program);
                break;
            case 'CC':
                this.sendControl(action.control, action.value);
                break;
            default:
                throw `Bad action type: ${action.type}`
                break;                    
            }
        }
    }

    sendProgram(program) {
        this.output.sendMessage([0xC0, program]);
    }

    sendControl(control, value) {
        this.output.sendMessage([0xB0, control, value]);
    }

    close() {
        this.output.closePort();
    }
}