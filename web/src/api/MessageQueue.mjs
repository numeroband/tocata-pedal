export class TimeoutError extends Error {
  constructor(message) {
    super(message)
    this.name = 'TimeoutError'
  }
}

// Incoming messages waiting to be read by a single consumer.
export default class MessageQueue {
  constructor() {
    this.messages = []
    this.waiter = null
  }

  push(message) {
    if (this.waiter) {
      const waiter = this.waiter
      this.waiter = null
      waiter(message)
    } else {
      this.messages.push(message)
    }
  }

  // Resolves with the next message, or rejects with a TimeoutError after
  // `timeout` ms (no timeout when undefined).
  pop(timeout) {
    if (this.messages.length > 0) {
      return Promise.resolve(this.messages.shift())
    }
    return new Promise((resolve, reject) => {
      let timer = null
      const waiter = message => {
        clearTimeout(timer)
        resolve(message)
      }
      if (timeout !== undefined) {
        timer = setTimeout(() => {
          if (this.waiter === waiter) {
            this.waiter = null
          }
          reject(new TimeoutError(`No response after ${timeout} ms`))
        }, Math.max(0, timeout))
      }
      this.waiter = waiter
    })
  }

  // Drops messages nobody asked for yet (unsolicited or late replies).
  clear() {
    this.messages = []
  }
}
