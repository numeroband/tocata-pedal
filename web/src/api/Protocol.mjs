import TransportUsb from './TransportUsb.mjs';
import TransportWebSocket from './TransportWebSocket.mjs';
import TransportMidi  from './TransportMidi.mjs';

const LENGTH_OFFSET = 0;
const COMMAND_OFFSET = 2;
const STATUS_OFFSET = 3;
const MSG_HEADER_SIZE = 4;

export default class Protocol {
  constructor(transport, connectionEvent) {
    if ("requestDevice" in transport) {
      this.transport = new TransportUsb(transport, connectionEvent);
    } else if ("requestMIDIAccess" in transport) {
      this.transport = new TransportMidi(transport, connectionEvent);
    } else {
      this.transport = new TransportWebSocket(transport, connectionEvent);
    }
    this.buffer = new Uint8Array();
  }

  get connected() {
    return this.transport.connected;
  }

  connect = reconnect => reconnect ? this.transport.reconnect() : this.transport.connect();
  version = _ => this.transport.version();
  
  async receive() {
    let totalLength = Infinity;
    let msg;
    while (this.buffer.byteLength < totalLength) {
      const rcvData = await this.transport.receive();
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
    const data = this.buffer.slice(MSG_HEADER_SIZE);
    this.buffer = (this.buffer.byteLength > totalLength) ? this.buffer.slice(totalLength) : new Uint8Array();

    if (status === 0) {
      // console.log('receive', {command, status, data});
      return {command, status, data};
    } else {
      throw new Error(`Response with status ${status}`);
    }
  }

  async sendRequest(command, data = new Uint8Array()) {
    const outBuffer = new Uint8Array(MSG_HEADER_SIZE + data.byteLength);
    const msg = new DataView(outBuffer.buffer);
    msg.setUint16(LENGTH_OFFSET, data.byteLength, true);
    msg.setUint8(COMMAND_OFFSET, command);
    msg.setUint8(STATUS_OFFSET, 0);
    outBuffer.set(data, MSG_HEADER_SIZE);
    await this.transport.send(msg);
    return await this.receive();
  }
}