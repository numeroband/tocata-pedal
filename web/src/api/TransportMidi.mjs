import { toSysEx, fromSysEx, kSysExAnyChannel } from "./MidiSysEx.mjs"

export default class TransportMidi {
  constructor(navigator, connectionEvent) {
    this.navigator = navigator
    this.midiChannel = navigator.midiChannel ?? kSysExAnyChannel
    this.connectionEvent = connectionEvent
    this.midi = null
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
    this.midi = await this.navigator.requestMIDIAccess({sysex: true})
    const name = this.navigator.midiDeviceName
    
    console.log('inputs')
    this.midi.inputs.values().forEach(k => console.log(k))
    console.log('outputs')
    this.midi.outputs.values().forEach(k => console.log(k))

    this.input = this.midi.inputs.values().find(x => x.name === name)
    this.output = this.midi.outputs.values().find(x => x.name === name)
    if (!this.input || !this.output) {
      throw new Error(`Cannot find MIDI device '${name}'`)
    }
    this.input.onmidimessage = e => {
      console.log('Received midi message', e.data)
      const buffer = fromSysEx(e.data, this.midiChannel)
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
    }
    this.connectionEvent(true);
  }

  async reconnect() {
    return this.connect()
  }

  async send(data) {
    const buffer = new Uint8Array(data.buffer)
    const sysex = toSysEx(buffer, this.midiChannel)
    console.log('sending', Array.from(sysex))
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
