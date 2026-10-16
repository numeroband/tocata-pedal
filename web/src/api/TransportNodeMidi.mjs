import { toSysEx, fromSysEx, lastSysEx, kSysExAnyChannel } from "./MidiSysEx.mjs"
import MessageQueue from "./MessageQueue.mjs"

export default class TransportNodeMidi {
  constructor(midi, connectionEvent) {
    this.midiDeviceName = midi.midiDeviceName
    this.midiChannel = midi.midiChannel ?? kSysExAnyChannel
    this.connectionEvent = connectionEvent
    this.midi = midi
    this.input = null
    this.output = null
    this.queue = new MessageQueue()
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
      const buffer = fromSysEx(lastSysEx(message), this.midiChannel)
      if (!buffer) {
        // e.g. the MIDI Identity Reply the board sends after a restart.
        console.log('Ignoring non-Tocata MIDI message', Array.from(message.slice(0, 8)))
        return
      }
      this.queue.push(new DataView(buffer.buffer))
    })
    // Enable SysEx before opening so nothing arriving early is filtered out.
    this.input.ignoreTypes(false, true, true)
    this.input.openPort(inputIndex)
    this.output.openPort(outputIndex)
    // Give the freshly opened ports a moment to settle before the first
    // request, so its reply isn't missed.
    await new Promise(resolve => setTimeout(resolve, 100))
    this.connectionEvent(true);
  }

  async reconnect() {
    return this.connect()
  }

  async send(data) {
    const buffer = new Uint8Array(data.buffer)
    const sysex = toSysEx(buffer, this.midiChannel)
    // console.log('sending', Array.from(sysex))
    await this.output.send(Array.from(sysex))
  };

  receive(timeout) {
    return this.queue.pop(timeout)
  }

  flush() {
    this.queue.clear()
  }
}
