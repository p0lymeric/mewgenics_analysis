from collections import namedtuple
import re
import hashlib
import pefile

MEWGENICS_EXE_PATH = r'C:\Games\Steam\steamapps\common\Mewgenics\Mewgenics.exe'

'''
Run this script to find symbol addresses after a game update.

polymeric 2026
'''

# Signature patterns were made using the Sigga script for Ghidra and hand-adjusted as needed,
# with an acceptable size range of [32, 128] B and preferring to anchor at function start.

DirectSig = namedtuple('DirectSig', ['pattern', 'offset'])
IndirectSig = namedtuple('IndirectSig', ['pattern', 'offset', 'length', 'signed', 'rip_relative'])

# Desired variable name, location descriptor pair.
signatures = {
    # Functions are located by anchoring on starting bytes if possible, but offsets and indirect references may be used if needed
    'ADDRESS_glaiel__SQLSaveFile__BeginSave': DirectSig('4C 8B DC 53 48 81 EC 80 00 00 00 48 8B D9 83 79 28 00 75 ?? 49 8D 43 B8 49 89 43 08 33 C9 49 89 4B F0', 0),
    'ADDRESS_glaiel__SQLSaveFile__EndSave': DirectSig('4C 8B DC 53 48 81 EC A0 00 00 00 48 8B D9 83 69 28 01 75 ?? 49 8D 43 98 49 89 43 08 33 C9 49 89 4B D0', 0),
    'ADDRESS_glaiel__SQLSaveFile__SQL': DirectSig('48 89 5C 24 18 4C 89 4C 24 20 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC C0 00 00 00', 0),
    'ADDRESS_glaiel__SerializeCatData': DirectSig('40 55 53 56 57 41 56 48 8B EC 48 81 EC 80 00 00 00 45 0F B6 F0 48 8B DA 48 8B F9 C7 45 30 13 00 00 00', 0),
    'ADDRESS_glaiel__CatData_ctor': DirectSig('48 89 4C 24 08 48 83 EC 28 4C 8B C1 45 33 C9 4C 89 49 08 4C 89 49 10 0F 57 C0 0F 11 41 18 4C 89 49 28', 0),
    'ADDRESS_glaiel__CatData_dtor': DirectSig('40 53 48 83 EC 20 48 8B D9 48 81 C1 10 0C 00 00 E8 ?? ?? ?? ?? 48 8D 8B 90 0B 00 00 E8 ?? ?? ??', 0),
    'ADDRESS_glaiel__CatData_unk_init': DirectSig('48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1', 0),
    'ADDRESS_glaiel__CatData_unk_init_bodyparts': DirectSig('40 53 55 56 41 56 41 57 48 83 EC 60 48 8B D9 0F 57 C0 45 33 FF B9 20 00 00 00 0F 11 44 24 40 4C 89 7C 24 50', 0),
    'ADDRESS_glaiel__CatData__breed': DirectSig('48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 28 FF FF FF 48 81 EC 98 01 00 00 0F 29 70 A8 0F 29 78 98 44 0F 29 40 88 44 0F 29 88 78 FF FF FF 44 0F 29 90 68 FF FF FF 44 0F 29 98 58 FF FF FF 44 0F 29 A0 48 FF FF FF 44 0F 29 A8 38 FF FF FF 44 0F 29 B0 28 FF FF FF 44 0F 29 B8 18 FF FF FF 0F 28 F3', 0),
    'ADDRESS_glaiel__HouseCat__unk_remove_from_world': IndirectSig(
        '48 89 5C 24 08 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B F9 48 8B 98 A8 05 00 00 48 8B 88 98 05 00 00 48 8B 47 08 48 8B 50 48 48 8B 92 80 00 00 00 E8 ?? ?? ?? ?? 41 B9 01 00 00 00 4C 8B C0 BA 07 00 00 00 48 8B CB E8 ?? ?? ?? ?? 48 8B 4F 08 48 8B 49 48 E8 ?? ?? ?? ??',
        85, 4, True, True
    ),
    'ADDRESS_maybe_create_stray_catdata_and_register_in_pedigree': DirectSig('48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 30 41 8B F8 48 8B E9 B9 58 0C 00 00 E8', 0),
    'ADDRESS_glaiel__Scene__CreateEntity': DirectSig('40 57 48 83 EC 20 80 B9 B0 04 00 00 00 48 8B F9 74 ?? 33 C0 48 83 C4 20 5F C3 B9 40 00 00 00 48 89 5C 24 38', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_HouseCat_int64': DirectSig('40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 18 01 00 00', 0),
    'ADDRESS_maybe_Director_create_Scene': DirectSig('48 89 5C 24 18 48 89 74 24 20 48 89 54 24 10 57 48 83 EC 20 48 8B FA 48 8B F1 48 8D 0D ?? ?? ??', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_FishingMinigameScene': IndirectSig(
        'E8 ?? ?? ?? ?? EB ?? 48 8D 7B 28 48 8B CB E8 ?? ?? ?? ?? F2 0F 10 73 30 48 8B 0F 0F 28 DE F2 0F 10 43 50',
        1, 4, True, True
    ),
    'ADDRESS_glaiel__Director__DestroyScene': DirectSig('48 89 5C 24 08 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC 80 00 00 00 48 8B FA 4C 8B E9', 0),
    'ADDRESS_glaiel__Scene__AddComponent': DirectSig('48 89 5C 24 18 48 89 6C 24 20 48 89 54 24 10 56 57 41 56 48 83 EC 20 48 8B 02 48 8B F1 48 8B CA', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_RenderCore_int32': DirectSig('40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 0F 84', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_Camera_Component': DirectSig('48 89 6C 24 20 57 48 83 EC 20 80 B9 B0 04 00 00 00 48 8B EA 48 8B F9 74 ?? 33 C0 48 8B 6C 24 48 48 83 C4 20 5F C3 48 89 5C 24 38 48 8D 0D ?? ?? ?? ?? 48 89 74 24 40 E8 ?? ?? ?? ?? 48 89 44 24 30 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 33 D2 41 B8 78 01 00 00', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_Renderer_CStr': DirectSig('40 53 55 41 56 48 83 EC 40 80 B9 B0 04 00 00 00 49 8B E8 4C 8B F2 48 8B D9 74 ?? 33 C0 48 83 C4 40 41 5E 5D 5B C3 48 89 74 24 68 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 70 E8 ?? ?? ?? ?? 48 89 44 24 60 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 08 01 00 00', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_Animator': DirectSig('48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 8B EA 48 8B D9 80 B9 B0 04 00 00 00 74 ?? 33 C0 E9 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 89 44 24 50 48 85 C0 74 ?? 33 D2 41 B8 B8 02 00 00', 0),
    'ADDRESS_glaiel__Scene__CreateComponent_CatParts': DirectSig('48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 8B EA 48 8B D9 80 B9 B0 04 00 00 00 74 ?? 33 C0 E9 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 89 44 24 50 48 85 C0 74 ?? 33 D2 41 B8 90 0A 00 00', 0),

    # Data/TLS references are located through references within functions
    'DATAOFF_glaiel__MewDirector__p_singleton': IndirectSig(
        '48 89 5C 24 10 48 89 4C 24 08 57 48 83 EC 40 48 8B CA 48 8B 05 ?? ?? ?? ?? 48 8B B8 A8 05 00 00',
        21, 4, True, True
    ),
    'DATAOFF_maybe_housecat_component_pool': IndirectSig(
        '40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 18 01 00 00',
        46, 4, True, True
    ),
    'DATAOFF_glaiel__Component___objid_counter': IndirectSig(
        '8B 15 ?? ?? ?? ?? 45 33 C0 80 61 0D 80 89 51 08 C6 41 0C 00 8D 42 01 C7 41 0E 00 00 01 00 89 05',
        2, 4, True, True
    ),
    'TLS0OFF_xoshiro256p_rng_context': IndirectSig(
        '48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1 45 8B F0 48 8B F9 41 BD ?? ?? ?? ??',
        40, 4, False, False
    ),
}

def hex_pattern_to_bytes_regex(pattern):
    pattern_norm = ''.join(pattern.split())
    bytes_str = b''
    for i in range(0, len(pattern_norm), 2):
        pattern_byte = pattern_norm[i:i+2]
        if pattern_byte == '??':
            bytes_str += b'.'
        else:
            bytes_str += re.escape(bytes([int(pattern_byte, 16)]))
    return re.compile(bytes_str, re.DOTALL)

def main():
    pe = pefile.PE(MEWGENICS_EXE_PATH, fast_load=True)

    with open(MEWGENICS_EXE_PATH, 'rb') as f:
        hash = hashlib.sha256(f.read()).hexdigest()
    print(f'inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("{hash}");')

    for search_varname, search_descriptor in signatures.items():
        regexp = hex_pattern_to_bytes_regex(search_descriptor.pattern)
        results = list(re.finditer(regexp, pe.get_memory_mapped_image()))
        if len(results) == 1:
            result_cva = results[0].start()
            result_buffer = results[0].group(0)
            if type(search_descriptor) is DirectSig:
                target_rva = result_cva + search_descriptor.offset
                print(f'inline constexpr uintptr_t {search_varname} = {hex(target_rva)}; // {search_descriptor}')
            else:
                if search_descriptor.offset + search_descriptor.length > len(result_buffer):
                    print(f'// WARNING: indexed past match buffer')
                target_bytes = pe.get_memory_mapped_image()[result_cva+search_descriptor.offset:result_cva+search_descriptor.offset+search_descriptor.length]
                target = int.from_bytes(target_bytes, byteorder='little', signed=search_descriptor.signed)
                if search_descriptor.rip_relative:
                    target += result_cva + search_descriptor.offset + search_descriptor.length
                print(f'inline constexpr uintptr_t {search_varname} = {hex(target)}; // {search_descriptor}')
        elif len(results) > 1:
            print(f'inline constexpr uintptr_t {search_varname} = <MULTIPLE MATCHES>; // {search_descriptor}')
        else:
            print(f'inline constexpr uintptr_t {search_varname} = <NOT FOUND>; // {search_descriptor}')

if __name__ == '__main__':
    main()
