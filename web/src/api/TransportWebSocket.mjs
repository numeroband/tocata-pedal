export default class TransportWS {
  constructor(ws, connectionEvent) {
    this.ws = ws;
    this.socket = null;
    this.connectionEvent = connectionEvent;
    this.queue = [];
    this.resolve = null;
    this.reject = null;
  }

  get connected() {
    return this.socket != null;
  }

  version = _ => ({
    major: 1,
    minor: 2,
    subminor: 3,
  })

  async connect() {
    console.log('Socket connect');

    const socket = new this.ws("ws://localhost:9002");
    socket.binaryType = "arraybuffer";

    socket.onopen = e => {
      console.log('onopen');
      this.socket = socket;
      this.connectionEvent(true);
    };

    socket.onerror = e => {
      console.log('onerror');
      this.socket = null;
      this.connectionEvent(false);
    };    

    socket.onclose = e => {
      console.log('onclose');

      while (this.queue.length > 0) {
        this.queue.shift();
      }
  
      if (this.reject) {
        this.reject('Connection closed');
        this.resolve = null;
        this.reject = null;
      }
      this.socket = null;
      this.connectionEvent(false);
    };

    socket.onmessage = e => {
      this.queue.push(new Uint8Array(e.data));
      if (this.resolve) {
        this.resolve(this.queue.shift());
        this.resolve = null;
        this.reject = null;
      }
    };
  }

  async reconnect() {
    return this.connect();
  }

  async send(data) {
    this.socket.send(data);
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
