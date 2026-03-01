// ImHex pattern for files/pedigree

struct ChildPedigree {
    u64 key_child_id;
    u64 val_parent_a_id;
    u64 val_parent_b_id;
    double val_coi;
};

struct ParentCOI {
    u64 key_parent_a_id;
    u64 key_parent_b_id;
    double val_coi;
};

struct Child {
    u64 key_child_id;
};

struct HashMap<T> {
    // Unknown, consistently fixed at 0xF5FFFFFFFFFFFFFF
    u64 unknown_0;
    // Number of entries currently stored in the map
    u64 entries;
    // Maximum entries storable by the map
    // Consistently one smaller than a power-of-two
    u64 capacity;

    // Hash array
    // 0x00-0x7F - occupied, 7-bit hash of the key
    // 0x80 - unoccupied
    u8 hash_table[capacity];
    // End of table marker. Always 0xFF
    u8 hash_table_eot_marker;
    // The first 16 entries of the table are replicated, likely to allow
    // wide probe fetches to seamlessly wrap beyond the last 15B of the table
    // This section is consistently 16 bytes wide and is 0x80-filled
    // on and after the analogous EOT position when capacity < 16
    u8 hash_table_wide_load_rep[16];

    // Data array
    // Given an entry i:
    // a) if hash_table[i] <= 0x7F, data_table[i] is occupied
    // b) if data_table[i] is occupied, stores a key concatenated with value
    T data_table[capacity];

    // Unknown, value appear to vary between instances
    u64 unknown_1;
};

// Maps a u64 cat_id to its parents' IDs and its COI
// Likely referenced to draw a cat's family tree
// and to compute COIs when a kitten is produced
HashMap<ChildPedigree> parents_and_coi_map @ 0x00;

// Maps a Pair(u64 parent_a_id, u64 parent_b_id) to a COI value
// Likely a memopad for caching a couple's COI for future kittens
HashMap<ParentCOI> parents_to_coi_memo_map @ $;

// Stores a set of u64 cat_ids
// Likely the set of all cats in the field/currently living in the house
HashMap<Child> accessible_cats @ $;