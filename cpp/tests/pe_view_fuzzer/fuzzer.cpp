#include "utilities/pe_view.hpp"

#include <cstdint>
#include <cstdlib>

// Fuzzer for pe_view.hpp
//
// polymeric 2026

// Note that PeView uses native pointer and size types for computing file offsets.
// It cannot completely map an image sized larger than this process' native address space.
const size_t PTR_SIZE = sizeof(uintptr_t);
const size_t PTR_REPLICATES = 4;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // llll... offset_to_query(PTR_SIZE)[0]
    // ...
    // llll... offset_to_query(PTR_SIZE)[PTR_REPLICATES - 1]
    // ....    pe_data
    // ..

    // Constrain the first PTR_SIZE * PTR_REPLICATES of Data to be offsets to query
    if(Size < PTR_SIZE * PTR_REPLICATES) {
        return -1;
    }
    uintptr_t *offsets_to_query = (uintptr_t *)Data;

    // The rest of the Data bytes belong to the PE file
    const uint8_t *pe_data = &Data[PTR_SIZE * PTR_REPLICATES];
    size_t pe_size = Size - PTR_SIZE * PTR_REPLICATES;

    PeView pe_view;
    pe_view.open(pe_data, pe_size);

    for(size_t i = 0; i < PTR_REPLICATES; i++) {
        pe_view.file_offset_to_rva(offsets_to_query[i]);
    }

    return 0;
}
