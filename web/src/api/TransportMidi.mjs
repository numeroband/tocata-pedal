export default class TransportMidi {
  constructor(navigator, connectionEvent) {
    this.navigator = navigator
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
      const buffer = fromSysEx(e.data)
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
    const sysex = toSysEx(buffer)
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

const kSysExPrefix = [0xF0, 0x00, 0x2F, 0x7F];
const kSysExSuffix = [0xF7];
const kSysExMinSize = kSysExPrefix.length + kSysExSuffix.length

function toSysEx(buffer) {
  const totalBits = buffer.length * 8
  const bytesRequired = kSysExMinSize + Math.floor((totalBits + 6) / 7)
  const sysex = new Uint8Array(bytesRequired)
  let bits = 0
  sysex.set(kSysExPrefix)
  let offset = kSysExPrefix.length
  sysex[offset] = 0

  for (let b of buffer) {
    sysex[offset++] |= b >> ++bits
    sysex[offset] = ~0x80 & (b << (7 - bits))
    if (bits === 7) {
      bits = 0
      sysex[++offset] = 0
    }
  }

  if (bits !== 0) {
    ++offset
  }
  
  sysex.set(kSysExSuffix, offset)

  return sysex
}

function fromSysEx(sysex) {
  if (sysex.length < kSysExMinSize) {
    return null
  }
  if (!kSysExPrefix.every((val, i) => sysex[i] === val)) {
    return null
  }

  const bytesRequired = Math.floor(((sysex.length - kSysExMinSize) * 7) / 8)
  const buffer = new Uint8Array(bytesRequired)
  const content = sysex.slice(kSysExPrefix.length, -kSysExSuffix.length)
  let offset = 0
  let bits = 0

  for (let i = 0; i < buffer.length; ++i) {
    let b = content[offset++] << ++bits
    b |= content[offset] >> (7 - bits)
    if (bits === 7) {
      bits = 0;
      ++offset;
    }
    buffer[i] = b
  }

  return buffer
}
