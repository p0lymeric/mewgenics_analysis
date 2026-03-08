from collections import namedtuple
from enum import Enum
import struct
import lz4.frame

'''
Reads an Amoeba transaction log file written by amoeba.dll
and parses structures from its sql(2) and savefile(3) channels.

polymeric 2026
'''

AmoebaControl = namedtuple('AmoebaControl', ['kind', 'x'])

class SqlStreamInterpreterState(Enum):
    READY = 0
    HALT = 1
    AWAIT_QUERY_STRING = 2
    AWAIT_KEY = 3
    AWAIT_VAL = 4

class SqlStreamInterpreter:
    def __init__(self):
        self.reset()

    def reset(self):
        self.timestamp = 0
        self.state = SqlStreamInterpreterState.READY
        self.buf_params_count = 0
        self.buf_query_string = ''
        self.buf_key = ''
        self.buf_val = ''
        self.buf_params = {}

    def message(self, value):
        if type(value) is AmoebaControl:
            if value.kind == 'set_timestamp':
                self.timestamp = value.x
            return None

        if self.state == SqlStreamInterpreterState.READY:
            if type(value) is int:
                self.buf_params_count = value
                self.buf_params = {}
                self.state = SqlStreamInterpreterState.AWAIT_QUERY_STRING
            else:
                self.state = SqlStreamInterpreterState.HALT
        elif self.state == SqlStreamInterpreterState.HALT:
            return None
        elif self.state == SqlStreamInterpreterState.AWAIT_QUERY_STRING:
            if type(value) is str:
                self.buf_query_string = value
                if self.buf_params_count > 0:
                    self.state = SqlStreamInterpreterState.AWAIT_KEY
                else:
                    self.state = SqlStreamInterpreterState.READY
            else:
                self.state = SqlStreamInterpreterState.HALT
        elif self.state == SqlStreamInterpreterState.AWAIT_KEY:
            if type(value) is str:
                self.buf_key = value
                self.state = SqlStreamInterpreterState.AWAIT_VAL
            else:
                self.state = SqlStreamInterpreterState.HALT
        elif self.state == SqlStreamInterpreterState.AWAIT_VAL:
            self.buf_params[self.buf_key] = value
            self.buf_params_count -= 1
            if self.buf_params_count > 0:
                self.state = SqlStreamInterpreterState.AWAIT_KEY
            else:
                self.state = SqlStreamInterpreterState.READY
        else:
            raise Exception

        if self.state == SqlStreamInterpreterState.READY:
            response = (self.buf_query_string, self.buf_params)
            assert(self.buf_params_count == 0)
            return self.timestamp, response

class SavefileStreamInterpreterState(Enum):
    READY = 0
    HALT = 1
    AWAIT_FILENAME = 2
    AWAIT_BLOB = 3

class SavefileStreamInterpreter:
    def __init__(self):
        self.reset()

    def reset(self):
        self.timestamp = 0
        self.state = SavefileStreamInterpreterState.READY
        self.buf_modified = 0
        self.buf_filename = ''
        self.buf_blob = ''

    def message(self, value):
        if type(value) is AmoebaControl:
            if value.kind == 'set_timestamp':
                self.timestamp = value.x
            return None

        if self.state == SavefileStreamInterpreterState.READY:
            if type(value) is int:
                self.buf_modified = value
                self.state = SavefileStreamInterpreterState.AWAIT_FILENAME
            else:
                self.state = SavefileStreamInterpreterState.HALT
        elif self.state == SavefileStreamInterpreterState.HALT:
            return None
        elif self.state == SavefileStreamInterpreterState.AWAIT_FILENAME:
            if type(value) is str:
                self.buf_filename = value
                self.state = SavefileStreamInterpreterState.AWAIT_BLOB
            else:
                self.state = SavefileStreamInterpreterState.HALT
        elif self.state == SavefileStreamInterpreterState.AWAIT_BLOB:
            if type(value) is bytes:
                self.buf_blob = value
                self.state = SavefileStreamInterpreterState.READY
            else:
                self.state = SavefileStreamInterpreterState.HALT
        else:
            raise Exception

        if self.state == SavefileStreamInterpreterState.READY:
            response = (self.buf_modified, self.buf_filename, self.buf_blob)
            self.buf_blob = ''
            return self.timestamp, response

def read_compact(f, ctype):
    if ctype == 1:
        return None
    elif ctype == 2:
        return int.from_bytes(f.read(8), 'little', signed=False)
    elif ctype == 3:
        return struct.unpack('<d', f.read(8))[0]
    elif ctype == 253:
        return AmoebaControl('set_timestamp', int.from_bytes(f.read(8), 'little', signed=False))
    elif ctype == 254:
        return AmoebaControl('select_vsid', int.from_bytes(f.read(4), 'little', signed=False))
    elif ctype == 255:
        return AmoebaControl('reset', None)
    else:
        raise Exception(ctype)

def read_full(f, stype, length):
    if stype == 0:
        return f.read(length)
    elif stype > 0 and stype < 256:
        return read_compact(f, stype)
    elif stype == 256:
        return f.read(length).decode('utf-8')
    else:
        raise Exception(stype)

def main():
    ssi = SqlStreamInterpreter()
    svi = SavefileStreamInterpreter()
    with open('test.tlog.lz4', 'rb') as f:
        header = f.read(8)
        magic, version, fmt = struct.unpack('<6sbb', header)
        assert(magic == b'AMOEBA')
        assert(version == 0)
        assert(fmt == 1)

        with lz4.frame.open(f, 'rb') as f:
            current_vsid = 0
            while ctype := f.read(1):
                ctype = int.from_bytes(ctype, 'little', signed=False)
                if ctype == 0:
                    # full
                    stype = int.from_bytes(f.read(3), 'little', signed=False)
                    f.read(4) #reserved
                    length = int.from_bytes(f.read(8), 'little', signed=False)
                    rec = read_full(f, stype, length)
                else:
                    # compact
                    rec = read_compact(f, ctype)
                # print(rec)
                if type(rec) is AmoebaControl:
                    if rec.kind == 'select_vsid':
                        current_vsid = rec.x

                if current_vsid == 2 or type(rec) is AmoebaControl:
                    response = ssi.message(rec)
                    if response:
                        if not response[1][0].startswith('SELECT'):
                            print(response[1][0])
                        # print(response)
                if current_vsid == 3 or type(rec) is AmoebaControl:
                    response = svi.message(rec)
                    if response:
                        print(f'wrote {response[1][1]}')
                        # print(response)


if __name__ == '__main__':
    main()
