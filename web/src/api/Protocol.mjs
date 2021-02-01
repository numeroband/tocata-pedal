const EP_SIZE = 64
const VENDOR_ID = 0xcafe;

class Usb {
  constructor(usb) {
    this.usb = usb;
  }

  async connect(ondisconnect) {
    try {
      await this.reconnect(ondisconnect);
    } catch (error) {
      console.log('Connecting...');
      const filters = [{ vendorId: VENDOR_ID }];
      const device = await this.usb.requestDevice({ 'filters': filters })
      if (!device) {
        throw 'No device selected';
      }
      await this.connectDevice(device, ondisconnect);  
    }
  }

  async reconnect(ondisconnect) {
    console.log('Reconnecting...');
    const devices = await this.usb.getDevices();
    const device = devices.find(dev => { console.log('checking', dev); return dev.vendorId == VENDOR_ID});
    if (!device) {
      throw 'No cached devices found';
    }
    await this.connectDevice(device, ondisconnect);
  }

  async connectDevice(device, ondisconnect) {
    this.device_ = device;
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
      console.log('no usb devie')
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

    this.usb.ondisconnect = event => {
      console.log(event);
      this.device_ = null;
      ondisconnect && ondisconnect();
    };
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
  constructor(usb) {
    this.usb = new Usb(usb);
  }

  connect(reconnect, ondisconnect) {
    this.buffer = new Uint8Array();
    this.responseResolve = null;
    return reconnect ? this.usb.reconnect(ondisconnect) : this.usb.connect(ondisconnect);
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