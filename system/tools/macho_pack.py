import struct
import sys

MH_MAGIC_64 = 0xFEEDFACF
CPU_TYPE_X86_64 = 0x01000007
CPU_SUBTYPE_X86_64 = 0x80000003
MH_EXECUTE = 2
LC_SEGMENT_64 = 0x19
LC_MAIN = 0x80000028

def pack(code, entry_offset=0):
    segname = b'__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

    total_cmds_size = 72 + 24
    code_off = 32 + total_cmds_size

    header = struct.pack('<IIIIIIII',
        MH_MAGIC_64,
        CPU_TYPE_X86_64,
        CPU_SUBTYPE_X86_64,
        MH_EXECUTE,
        2,
        total_cmds_size,
        0x200085,
        0)

    seg = struct.pack('<II', LC_SEGMENT_64, 72)
    seg += struct.pack('<16s', segname)
    seg += struct.pack('<QQ', 0x2000000, len(code))
    seg += struct.pack('<QQ', code_off, len(code))
    seg += struct.pack('<II', 7, 5)
    seg += struct.pack('<II', 0, 0)

    main_cmd = struct.pack('<II', LC_MAIN, 24)
    main_cmd += struct.pack('<QQ', entry_offset, 0)

    data = header + seg + main_cmd + code
    return data, len(data)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f'uso: {sys.argv[0]} entrada.bin saida.macho [entry_offset]')
        sys.exit(1)
    entry_offset = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    with open(sys.argv[1], 'rb') as f:
        code = f.read()
    macho, size = pack(code, entry_offset)
    with open(sys.argv[2], 'wb') as f:
        f.write(macho)
    print(f'ok: {size} bytes')
