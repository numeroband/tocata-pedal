const EP_SIZE = 64
const VENDOR_ID = 0xcafe;
const IFACE_CLASS = 0xFF;

export default class TransportUsb {
  constructor(usb, connectionEvent) {
    this.usb = usb;
    this.device = null;
    this.connectionEvent = connectionEvent;
    usb.onconnect = event => this.onconnect(event);
    usb.ondisconnect = event => this.ondisconnect(event);
  }

  get connected() {
    return this.device != null;
  }

  version = _ => ({
    major: this.device ? this.device.deviceVersionMajor : 0,
    minor: this.device ? this.device.deviceVersionMinor : 0,
    subminor: this.device ? this.device.deviceVersionSubminor : 0,
  })
  
  async onconnect(event) {
    if (this.connected || event.device.vendorId !== VENDOR_ID) {
      return;
    }

    try {
      console.log('Device found. Reconnecting...');
      await this.connectDevice(event.device);
    } catch(err) {
      console.log('Cannot connect', err);
    }
  }

  async ondisconnect(event) {
    if (!this.device || this.device !== event.device)
    {
      return;
    }
    
    this.device = null;
    this.connectionEvent && this.connectionEvent(false);
  }

  async connect() {
    try {
      await this.reconnect();
    } catch (error) {
      console.log('Connecting...');
      const filters = [{ vendorId: VENDOR_ID }];
      const device = await this.usb.requestDevice({ 'filters': filters })
      if (!device) {
        throw new Error('No device selected');
      }
      await this.connectDevice(device);  
    }
  }

  async reconnect() {
    if (this.connected) {
      return;
    }
    console.log('Reconnecting...');
    const devices = await this.usb.getDevices();
    const device = devices.find(dev => dev.vendorId === VENDOR_ID);
    if (!device) {
      throw new Error('No cached devices found');
    }
    await this.connectDevice(device);
  }

  async connectDevice(device) {
    await device.open();
    if (device.configuration === null) {
      await device.selectConfiguration(1);
    }
    const interfaces = device.configuration.interfaces;
    interfaces.forEach(element => {
      element.alternates.forEach(elementalt => {
        if (elementalt.interfaceClass !== IFACE_CLASS) {
          return;
        }
        this.interfaceNumber = element.interfaceNumber;        
        elementalt.endpoints.forEach(elementendpoint => {
          if (elementendpoint.direction === 'out') {
            this.endpointOut = elementendpoint.endpointNumber;
          }
          if (elementendpoint.direction === 'in') {
            this.endpointIn = elementendpoint.endpointNumber;
          }
        });
      });
    });

    if (!this.endpointIn || !this.endpointOut) {
      throw new Error('Cannot find USB device');
    }

    await device.claimInterface(this.interfaceNumber);
    await device.controlTransferOut({
      'requestType': 'class',
      'recipient': 'interface',
      'request': 0x22,
      'value': 0x01,
      'index': this.interfaceNumber
    });

    this.device = device;
    this.connectionEvent && this.connectionEvent(true);
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
      await this.device.transferOut(this.endpointOut, buf);
    }
  };

  async receive() {
    const { data } = await this.device.transferIn(this.endpointIn, EP_SIZE);
    // console.log('IN', data); //.byteLength);
    return data;
  };
}
