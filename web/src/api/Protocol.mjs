import TransportNodeMidi  from './TransportNodeMidi.mjs';
import TransportMidi  from './TransportMidi.mjs';

const LENGTH_OFFSET = 0;
const COMMAND_OFFSET = 2;
const STATUS_OFFSET = 3;
const MSG_HEADER_SIZE = 4;
const DEFAULT_TIMEOUT_MS = 3000;

export default class Protocol {
  constructor(transport, connectionEvent) {
    if ("requestMIDIAccess" in transport) {
      this.transport = new TransportMidi(transport, connectionEvent);
    } else if ("createReadStream" in transport) {
      this.transport = new TransportNodeMidi(transport, connectionEvent);
    } else {
      throw new Error('Unsupported transport');
    }
    this.buffer = new Uint8Array();
  }

  get connected() {
    return this.transport.connected;
  }

  connect = reconnect => reconnect ? this.transport.reconnect() : this.transport.connect();
  version = _ => this.transport.version();
  
  // Reads one framed response, giving up once `deadline` (Date.now() ms) passes.
  async receive(deadline) {
    let totalLength = Infinity;
    let msg;
    while (this.buffer.byteLength < totalLength) {
      let rcvData;
      try {
        rcvData = await this.transport.receive(deadline - Date.now());
      } catch (e) {
        this.buffer = new Uint8Array();
        throw e;
      }
      const oldBuffer = this.buffer;
      this.buffer = new Uint8Array(oldBuffer.byteLength + rcvData.byteLength);
      this.buffer.set(oldBuffer);
      this.buffer.set(new Uint8Array(rcvData.buffer), oldBuffer.byteLength);
      msg = new DataView(this.buffer.buffer);
      const length = (this.buffer.byteLength > 2) ? msg.getUint16(LENGTH_OFFSET, true) : Infinity;
      totalLength = MSG_HEADER_SIZE + length;
    }

    const command = msg.getUint8(COMMAND_OFFSET);
    const status = msg.getUint8(STATUS_OFFSET);
    const data = this.buffer.slice(MSG_HEADER_SIZE, totalLength);
    this.buffer = (this.buffer.byteLength > totalLength) ? this.buffer.slice(totalLength) : new Uint8Array();
    return {command, status, data};
  }

  // Sends a request and waits up to `timeout` ms for its response, rejecting
  // with a TimeoutError if none arrives. The device echoes the request's
  // command, so responses to other commands (left over from an earlier,
  // timed-out request) are skipped, as are those `accept` returns false for.
  async sendRequest(command, data = new Uint8Array(), {timeout = DEFAULT_TIMEOUT_MS, accept} = {}) {
    const outBuffer = new Uint8Array(MSG_HEADER_SIZE + data.byteLength);
    const msg = new DataView(outBuffer.buffer);
    msg.setUint16(LENGTH_OFFSET, data.byteLength, true);
    msg.setUint8(COMMAND_OFFSET, command);
    msg.setUint8(STATUS_OFFSET, 0);
    outBuffer.set(data, MSG_HEADER_SIZE);
    // Only one request is in flight, so anything received before it was sent
    // can't be its response.
    this.transport.flush();
    this.buffer = new Uint8Array();
    await this.transport.send(msg);
    const deadline = Date.now() + timeout;
    for (;;) {
      const res = await this.receive(deadline);
      if (res.command !== command || (accept && res.status === 0 && !accept(res))) {
        console.log(`Ignoring stale response (command 0x${res.command.toString(16)}) to command 0x${command.toString(16)}`);
        continue;
      }
      if (res.status !== 0) {
        throw new Error(`Response with status ${res.status}`);
      }
      return res;
    }
  }
}
