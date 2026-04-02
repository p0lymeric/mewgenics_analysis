from binaryninja import warp

'''
Run this script in Binary Ninja (File > Run Script...)
against an analyzed database to find function addresses
after a game update.

polymeric 2026
'''

# Desired variable name, WARP GUID pair
symbol_signatures = {
    'ADDRESS_glaiel__SQLSaveFile__BeginSave': 'e073a811-ac5e-5f48-8b6d-472c34e4e0ef',
    'ADDRESS_glaiel__SQLSaveFile__EndSave': '455fdaaf-58a0-5f36-8169-7e85de7ccddb',
    'ADDRESS_glaiel__SQLSaveFile__SQL': '74c83bc6-9e76-5549-8b2c-3b3b53cccaf8',
    'ADDRESS_glaiel__SerializeCatData': '1184393d-db7a-5f40-89f7-d4cb6f23f3fd',
    'ADDRESS_glaiel__CatData_ctor': '7089d0e4-d065-52af-957c-40bb37408c1c',
    'ADDRESS_glaiel__CatData_dtor': '1e47bead-7c70-5cb3-95d3-79473ce939ef',
    'ADDRESS_glaiel__CatData_unk_init': 'cb987a75-507b-50b5-884a-36aeb6bae1c1',
    'ADDRESS_glaiel__CatData_unk_init_bodyparts': 'dfbca3cb-df39-5fc7-9e94-3b59ad621bf4',
    'ADDRESS_glaiel__CatData_breed': 'd6a5fead-b8df-5b2a-81a5-1d34b773ac3c',
}

def main():
    for search_varname, search_warp_sig in symbol_signatures.items():
        found = False
        # could not find a fast, pre-indexed way to search by WARP GUID in BN or WARP documentation
        # instead we'll use good ol' iterative search
        for func in bv.functions:
            func_warp_sig = warp.get_function_guid(func)
            if func_warp_sig is not None and search_warp_sig == func_warp_sig.to_string():
                func_rva = func.start - bv.start
                print(f'inline constexpr uintptr_t {search_varname} = {hex(func_rva)}; // WARP {search_warp_sig}')
                found = True
                break
        if not found:
            # have fun finding the function by hand!
            print(f'inline constexpr uintptr_t {search_varname} = <NOT FOUND>; // WARP {search_warp_sig}')

if __name__ == '__main__':
    main()
