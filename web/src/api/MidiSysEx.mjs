const kSysExPrefix = [0xF0, 0x00, 0x2F, 0x7F];
const kSysExSuffix = [0xF7];
const kSysExMinSize = kSysExPrefix.length + kSysExSuffix.length

export function toSysEx(buffer) {
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

export function fromSysEx(sysex) {
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
