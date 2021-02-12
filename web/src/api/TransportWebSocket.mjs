export default class TransportWS {
  constructor(connectionEvent) {
    this.ws = null;
    this.connectionEvent = connectionEvent;
    this.queue = [];
    this.resolve = null;
  }

  get connected() {
    return this.ws != null;
  }

  async connect() {
    const ws = new WebSocket("ws://localhost:9002");
    ws.binaryType = "arraybuffer";

    ws.onopen = e => {
      console.log('onopen', e);
      this.ws = ws;
      this.connectionEvent(true);
    };

    ws.onerror = e => {
      console.log('onerror', e);
      this.ws = null;
      this.connectionEvent(false);
    };    
    
    ws.onmessage = e => {
      this.queue.push(new Uint8Array(e.data));
      if (this.resolve) {
        this.resolve(this.queue.shift());
        this.resolve = null;
      }
    };
  }

  async reconnect() {
    return this.connect();
  }

  async send(data) {
    this.ws.send(data);
  };

  async receive() {
    if (this.queue.length > 0) {
      return this.queue.shift();
    }
    return new Promise((resolve, reject) => {
      this.resolve = resolve;
    });
  };
}
