export default class UF2 {
  static get FAMILY_RP2040() { return 0xe48bff56 }
  static get FAMILY_RP2XXX_ABSOLUTE() { return 0xe48bff57 }
  static get FAMILY_RP2035_ARM_S() { return 0xe48bff59 }

  constructor(buffer) {
    if (buffer.byteLength === 0 || (buffer.byteLength % 512) !== 0) {
      throw new Error('Invalid UF2 size');
    }
    this.buffer = buffer;
    const numBlocks = buffer.byteLength / 512;
    this.flashStart = Infinity;
    this.flashEnd = 0;
    this.blocks = [];
    for (let i = 0; i < numBlocks; ++i) {
      const block = this.getBlock(i);
      if (block.familyId !== UF2.FAMILY_RP2XXX_ABSOLUTE) {
        this.flashStart = Math.min(this.flashStart, block.address);
        this.flashEnd = Math.max(this.flashEnd, block.address + block.payload.byteLength);
      }
      this.blocks.push(block);
    }
  }

  getBlock(idx) {
    if (idx >= this.numBlocks) {
      throw new Error(`${idx}: Invalid block index`);
    }
    const offset = idx * 512;
    const view = new DataView(this.buffer, offset);
    const firstMagic = view.getUint32(0, true);
    if (firstMagic !== 0x0A324655) {
      throw new Error(`${idx}: Invalid block first magic`);
    }
    const secondMagic = view.getUint32(4, true);
    if (secondMagic !== 0x9E5D5157) {
      throw new Error(`${idx}: Invalid block second magic`);
    }
    const flags = view.getUint32(8, true);
    if ((flags & ~0x8000) !== 0x2000) {
      throw new Error(`${idx}: Invalid block flags (0x${flags.toString(16)})`);
    }
    const payloadSize = view.getUint32(16, true);
    if (payloadSize > 476) {
      throw new Error(`${idx}: Invalid block payload size`);
    }
    const familyId = view.getUint32(28, true);
    if (familyId !== UF2.FAMILY_RP2040 &&
        familyId !== UF2.FAMILY_RP2035_ARM_S &&
        familyId !== UF2.FAMILY_RP2XXX_ABSOLUTE) {
      throw new Error(`${idx}: Invalid block family id (0x${familyId.toString(16)})`);
    }
    const finalMagic = view.getUint32(508, true);
    if (finalMagic !== 0x0AB16F30) {
      throw new Error(`${idx}: Invalid block final magic`);
    }
    return {
      flags,
      familyId,
      address: view.getUint32(12, true),
      payload: new Uint8Array(this.buffer, offset + 32, payloadSize),
    }
  }
}
