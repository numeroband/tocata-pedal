import Midi from './Midi.mjs'
import terminal_kit from 'terminal-kit'
const term = terminal_kit.terminal;

const NUM_SWITCHES = 8;
const MAX_PROGRAMS = 99;
const A_CODE = 'A'.charCodeAt(0);
const COLOR_MAP = {
    'purple': 'magenta',
    'turquoise': 'cyan',
};

export default class Sim {
    constructor(url, getProgram) {
        this.url = url;
        this.getProgram = getProgram;
        this.state = Array(NUM_SWITCHES).fill(false);
        this.expression = 127;
        this.initTerm();

        try {
            this.midi = new Midi();
            this.changeProgram(0);
            this.redraw();
        } catch (e) {
            this.terminating = true;
            term.white(e);
            term.white('. Press any key to exit...');
        }
    }

    quit() {
        term.grabInput(false);
        term.hideCursor(false);
        term.clear();
        this.midi.close();
        process.exit(0);
    }

    changeProgram(id) {
        this.program = this.getProgram(id);
        if (!this.program.fs) {
            return;
        }
        for (let i = 0; i < NUM_SWITCHES; ++i) {
            const fs = this.program.fs[i];
            this.state[i] = (fs ? fs.enabled : false);
        }
        this.midi.sendActions(this.program.actions);
    }

    incExpression() {
        if (this.expression < 127) {
            this.expression = Math.min(this.expression + 5, 127);
            this.midi.sendControl(33 + NUM_SWITCHES, this.expression);    
        }
    }

    decExpression() {
        if (this.expression > 0) {
            this.expression = Math.max(this.expression - 5, 0);
            this.midi.sendControl(33 + NUM_SWITCHES, this.expression);    
        }
    }

    toggleSwitch(name) {
        const code = name.toUpperCase().charCodeAt(0);
        if (code < A_CODE || code >= A_CODE + NUM_SWITCHES) {
            return;
        }
        const id = code - A_CODE;
        const fs = this.program.fs ? this.program.fs[id] : null;
        if (!fs) {
            return;
        }
        this.state[id] = !this.state[id];
        this.midi.sendActions(this.state[id] ? fs.onActions : fs.offActions);
    }

    initTerm() {
        term.on('key', name => {
            if (this.terminating || name === 'CTRL_C') { 
                this.quit();
            }
        
            if (name.length > 1) {
                return;
            }

            if (name === 's') {
                this.midi.sendSysEx([1,2,3,4,5,6,7,8,9,0,1,2,3,4]);
                return;
            }

            if (name === '+' || name === '-') {
                const inc = (name === '+') ? 1 : -1;
                const id = (this.program.id + MAX_PROGRAMS + inc) % MAX_PROGRAMS;
                this.changeProgram(id);
            } else if (name === '<') {
                this.decExpression();
            } else if (name === '>') { 
                this.incExpression();
            } else {
                this.toggleSwitch(name);
            }
        
            this.redraw();
        }) ;
        
        term.grabInput(true);
        term.hideCursor(true);
        term.clear();        
    }
    
    drawSwitch(id, fs, x, y) {
        const name = String.fromCharCode(A_CODE + id);
        const color = (fs ? 
            ((fs.color in COLOR_MAP) ? COLOR_MAP[fs.color] : fs.color) : 
            'gray');
        const brightColor = `bright${color.charAt(0).toUpperCase() + color.slice(1)}`;
        term.moveTo(x, y);
        (fs && this.state[id]) ? term.color(brightColor, '*') : term.color(color, 'o');
        term.moveTo(x, y + 1).white(name);
    }

    drawSwitchName(id, fs, x, y)
    {
        if (!fs) {
            return;
        }

        term.moveTo(x + 1, y);
        const draw = (this.state[id] ? term.bgBrightWhite.black : term.brightWhite)
        draw(' ' + fs.name.padEnd(6));
    }

    drawScreen() {
        const width = 8;
        const height = 3;
        const rows = 2;
        const cols = NUM_SWITCHES / rows;

        term.moveTo(1, 1);
        term.table([['']], {
            hasBorder: true,
            borderChars: 'double' ,
            borderAttr: { color: 'white' } ,
            width: (width + 1) * cols ,
            height: 1 + height * rows ,
        }) ;    

        for (let i = 0; i < NUM_SWITCHES; ++i) {
            const x = 2 + (i % cols) * width;
            const y = 2 + Math.floor(i / cols) * (height + 1);
            this.drawSwitchName(i, this.program.fs ? this.program.fs[i] : null, x, y);
        }
        
        const name = this.program.name ? this.program.name.substring(0, 17) : '<EMPTY>';
        term.moveTo(3, 1 + height).brightWhite(`${String(this.program.id + 1).padStart(2)} - ${name}`);
        return 2 + height * rows;
    }
    
    programUpdated(id) {
        if (id == this.program.id) {
            this.changeProgram(id);
            this.redraw();
        }
    }

    redraw() {
        const width = 9;
        const height = 3;
        const rows = 2;
        const cols = NUM_SWITCHES / rows;

        const pedalY = this.drawScreen();

        term.moveTo(1, pedalY);
        term.table([['']], {
            hasBorder: true,
            borderChars: 'heavy' ,
            borderAttr: { color: 'gray' } ,
            width: width * cols ,
            height: 1 + height * rows ,
        }) ;    

        for (let i = 0; i < NUM_SWITCHES; ++i) {
            const x = 1 + Math.floor(width / 2) + (i % cols) * width;
            const y = pedalY + Math.floor(height / 2) + Math.floor(i / cols) * height;
            this.drawSwitch(i, this.program.fs ? this.program.fs[i] : null, x, y);
        }

        term.moveTo(1, pedalY + height * (rows + 1) - 2);
        term.bar(this.expression / 127, {
            innerSize: cols * width - 1,
            barStyle: term.brightBlack,
        });

        term.moveTo(1, pedalY + height * (rows + 1));
        term.white(`A-${String.fromCharCode(A_CODE + NUM_SWITCHES - 1)} to toggle switch.\n`);
        term.white('+ - to change program.\n');
        term.white('< > to change expression pedal.\n');
        term.white(`Configure in ${this.url}.\n`);
        term.white('Ctrl+C to quit.\n');
    }
}