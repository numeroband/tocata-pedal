import { type } from "os";

const MAX_PRG_NAME_SIZE = 30;
const MAX_FS_NAME_SIZE = 5;
const MAX_NAMES_RESPONSE = 16;
const MAX_ACTIONS = 5;
const MAX_SWITCHES = 6;
const MAX_WIFI_STR_SIZE = 63;

function isEmpty(value) {
  return (
    (value === null) || 
    (typeof(value) == 'object' && value.length === 0) ||
    (typeof(value) == 'boolean' && value === false)
  );
}

const parsers = {
  uint8: (view, offset) => [view.getUint8(offset), offset + 1],
  uint16: (view, offset) => [view.getUint16(offset), offset + 2],
  uint32: (view, offset) => [view.getUint32(offset), offset + 4],
  bool: (view, offset) => [view.getUint8(offset) != 0, offset + 1],
  enum: (view, offset, dict) => [dict[view.getUint8(offset)], offset + 1],
  str: (view, offset, size) => {
    const buf = new Uint8Array(view.buffer, offset, size + 1);
    const end = buf.findIndex(c => c == 0);
    return [String.fromCharCode.apply(null, buf.slice(0, end)), offset + size + 1];
  },
  array: (view, offset, numElems, parser, ...args) => {
    const ret = [];
    for (let i = 0; i < numElems; ++i) {
      let parsed;
      [parsed, offset] = parsers[parser](view, offset, ...args);
      ret[i] = parsed;
    }
    return [ret, offset];
  },
  nArray: (view, offset, ...args) => {
    const numElems = view.getUint8(offset++);
    const [ret, newOffset] = parsers.array(view, offset, ...args);
    return [ret.slice(0, numElems), newOffset];
  },
  struct: (view, offset, scheme) => {
    let ret = {};
    for (const [field, parser, ...args] of scheme.fields) {
      let parsed;
      [parsed, offset] = parsers[parser](view, offset, ...args);
      
      if (!isEmpty(parsed)) {
        ret[field] = parsed;
      }
    }
    return [(scheme.valid && !scheme.valid(ret)) ? null : ret, offset];
  },
};

const serializers = {
  uint8: (view, offset, value) => { view.setUint8(offset, value); return offset + 1 },
  uint16: (view, offset, value) => { view.setUint16(offset, value); return offset + 2 },
  uint32: (view, offset, value) => { view.setUint32(offset, value); return offset + 4 },
  bool: (view, offset, value) => { view.setUint8(offset, value); return offset + 1 },
  enum: (view, offset, value, values) => { view.setUint8(offset, values.findIndex(v => v == value)); return offset + 1 },
  str: (view, offset, value, size) => {
    const buf = new Uint8Array(view, offset, size + 1);
    const length = value ? value.length : 0;
    for (let i = 0; i < length; ++i) {
      view.setUint8(offset + i, value.charCodeAt(i));
    }
    view.setUint8(offset + length, 0);
    return offset + size + 1;
  },
  array: (view, offset, value, numElems, parser, ...args) => {
    const ret = [];
    for (let i = 0; i < numElems; ++i) {
      offset = serializers[parser](view, offset, value ? value[i] : null, ...args);
    }
    return offset;
  },
  nArray: (view, offset, value, ...args) => {
    view.setUint8(offset++, value ? value.length : 0);
    return serializers.array(view, offset, value, ...args);
  },
  struct: (view, offset, value, scheme) => {
    value = (value && (!scheme.valid || scheme.valid(value))) ? value : {};
    for (const [field, parser, ...args] of scheme.fields) {
      offset = serializers[parser](view, offset, value[field], ...args);
    }
    return offset;
  },
};

const messageTypes = [
  null,
  'PC',
  'CC',
  'NO',
  'NF',
];

const colors = [
  'blue',
  'purple',
  'red',
  'yellow',
  'green',
  'turquoise',
];

const wifi = {
  fields: [
    ['ssid', 'str', MAX_WIFI_STR_SIZE],
    ['key', 'str', MAX_WIFI_STR_SIZE],    
  ],
  valid: o => o.ssid
};

const config = {
  fields: [
    ['wifi', 'struct', wifi, o => o.ssid],
  ],
};


const action = {
  fields: [
    ['type', 'enum', messageTypes],
    ['values', 'array', 2, 'uint8'],
  ],
};

const footswitch = {
  fields: [
    ['onActions', 'nArray', MAX_ACTIONS, 'struct', action],
    ['offActions', 'nArray', MAX_ACTIONS, 'struct', action],
    ['name', 'str', MAX_FS_NAME_SIZE],
    ['color', 'enum', colors],
    ['enabled', 'bool'],
    ['default', 'bool'],
    ['momentary', 'bool'],
  ],
  valid: o => o.name
};

const names = {
  fields: [
    ['id', 'uint8'],
    ['names', 'nArray', MAX_NAMES_RESPONSE, 'str', MAX_PRG_NAME_SIZE],
  ],
};

const program = {
  fields: [
    ['name', 'str', MAX_PRG_NAME_SIZE],
    ['fs', 'nArray', MAX_SWITCHES, 'struct', footswitch],
    ['actions', 'nArray', MAX_ACTIONS, 'struct', action],
  ],
  valid: o => o.name
};

const idPlusProgram = { 
  fields: [
    ['id', 'uint8'],
    ['program', 'struct', program],
  ]
};

const parseStruct = (buffer, parser) => parsers.struct(new DataView(buffer), 0, parser)[0];
const serializeStruct = (buffer, value, parser) => buffer.slice(0, serializers.struct(new DataView(buffer), 0, value, parser));

export const parseConfig = buffer => parseStruct(buffer, config);
export const parseNames = buffer => parseStruct(buffer, names);
export const parseProgram = buffer => parseStruct(buffer, idPlusProgram);
export const serializeConfig = value => serializeStruct(new Uint8Array(512).buffer, value, config);
export const serializeProgram = value => serializeStruct(new Uint8Array(512).buffer, value, idPlusProgram);
