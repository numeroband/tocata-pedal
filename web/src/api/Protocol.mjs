import WebUSB from "webusb";
const usb = WebUSB.usb;

const EP_SIZE = 64

class Usb {
  async connect() {
    console.log('Connecting...');
    const filters = [{ vendorId: 0xcafe }];
    this.device_ = await usb.requestDevice({ 'filters': filters });
    await this.device_.open();
    if (this.device_.configuration === null) {
      await this.device_.selectConfiguration(1);
    }
    var interfaces = this.device_.configuration.interfaces;
    interfaces.forEach(element => {
      element.alternates.forEach(elementalt => {
        if (elementalt.interfaceClass == 0xFF) {
          this.interfaceNumber = element.interfaceNumber;
          elementalt.endpoints.forEach(elementendpoint => {
            if (elementendpoint.direction == "out") {
              this.endpointOut = elementendpoint.endpointNumber;
            }
            if (elementendpoint.direction == "in") {
              this.endpointIn = elementendpoint.endpointNumber;
            }
          })
        }
      })
    })

    if (!this.endpointIn || !this.endpointOut) {
      throw 'Cannot find USB device';
    }

    await this.device_.claimInterface(this.interfaceNumber);
    await this.device_.controlTransferOut({
      'requestType': 'class',
      'recipient': 'interface',
      'request': 0x22,
      'value': 0x01,
      'index': this.interfaceNumber
    });

    console.log('Connected');
  }

  async send(data) {
    let pending = data.buffer;
    let buf;
    while (pending) {
      if (pending.byteLength > EP_SIZE) {
        buf = pending.slice(0, EP_SIZE);
        pending = pending.slice(EP_SIZE);
      }
      else
      {
        buf = pending;
        pending = null;
      }
      // console.log('OUT', buf); //.byteLength);
      await this.device_.transferOut(this.endpointOut, buf);
    }
  };

  async receive() {
    const { data } = await this.device_.transferIn(this.endpointIn, EP_SIZE);
    // console.log('IN', data); //.byteLength);
    return data;
  };
}

const LENGTH_OFFSET = 0;
const COMMAND_OFFSET = 2;
const STATUS_OFFSET = 3;
const MSG_HEADER_SIZE = 4;

export default class Protocol {
  constructor() {
    this.usb = new Usb(data => this.onReceive(data));
  }

  connect() {
    this.buffer = new Uint8Array();
    this.responseResolve = null;
    return this.usb.connect();
  }
  
  async receive() {
    let totalLength = Infinity;
    let msg;
    while (this.buffer.byteLength < totalLength) {
      const rcvData = await this.usb.receive();
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
    const resolve = this.responseResolve;
    this.responseResolve = null;
    this.buffer = (this.buffer.byteLength > totalLength) ? this.buffer.slice(totalLength) : new Uint8Array();

    if (status == 0) {
      // console.log('receive', {command, status, data});
      return {command, status, data};
    } else {
      throw `Response with status ${status}`;
    }
  }

  async sendRequest(command, data = new Uint8Array()) {
    const outBuffer = new Uint8Array(MSG_HEADER_SIZE + data.byteLength);
    const msg = new DataView(outBuffer.buffer);
    msg.setUint16(LENGTH_OFFSET, data.byteLength, true);
    msg.setUint8(COMMAND_OFFSET, command);
    msg.setUint8(STATUS_OFFSET, 0);
    outBuffer.set(data, MSG_HEADER_SIZE);
    const promise = new Promise((resolve, reject) => {
      this.responseResolve = resolve;
      this.responseReject = reject;
    });
    await this.usb.send(msg);
    return await this.receive();
  }
}