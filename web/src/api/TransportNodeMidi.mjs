import { toSysEx, fromSysEx } from "./MidiSysEx.mjs"

export default class TransportNodeMidi {
  constructor(midi, connectionEvent) {
    this.midiDeviceName = midi.midiDeviceName
    this.connectionEvent = connectionEvent
    this.midi = midi
    this.input = null
    this.output = null
    this.queue = []
    this.resolve = null
    this.reject = null
  }

  get connected() {
    return this.input != null
  }

  version = _ => ({
    major: 1,
    minor: 2,
    subminor: 3,
  })

  async connect() {
    console.log('Connecting MIDI ...');
    
    this.input = new this.midi.Input();
    this.output = new this.midi.Output();
    let inputIndex = -1
    for (let i = 0; i < this.input.getPortCount(); ++i) {
      if (this.input.getPortName(i) === this.midiDeviceName) {
        inputIndex = i
        break
      }
    }
    let outputIndex = -1
    for (let i = 0; i < this.output.getPortCount(); ++i) {
      if (this.output.getPortName(i) === this.midiDeviceName) {
        outputIndex = i
        break
      }
    }
    if (inputIndex < 0 || outputIndex < 0) {
      throw new Error(`Cannot find MIDI device '${this.midiDeviceName}'`)
    }

    this.input.on('message', (_, message) => {
      // console.log('Received midi message', message)
      const buffer = fromSysEx(message)
      if (!buffer) {
        console.log('Invalid sysex')
        return
      }
      const data = new DataView(buffer.buffer)
      this.queue.push(data)
      if (this.resolve) {
        this.resolve(this.queue.shift())
        this.resolve = null
        this.reject = null
      }
    })
    this.input.openPort(inputIndex)
    this.input.ignoreTypes(false, true, true)
    this.output.openPort(outputIndex)
    this.connectionEvent(true);
  }

  async reconnect() {
    return this.connect()
  }

  async send(data) {
    const buffer = new Uint8Array(data.buffer)
    const sysex = toSysEx(buffer)
    // console.log('sending', Array.from(sysex))
    await this.output.send(Array.from(sysex))
  };

  async receive() {
    if (this.queue.length > 0) {
      return this.queue.shift();
    }
    return new Promise((resolve, reject) => {
      this.resolve = resolve;
      this.reject = reject;
    });
  };
}
