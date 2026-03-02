from pathlib import Path
from collections import namedtuple
import struct

CatPedigree = namedtuple('CatPedigree', ['child_id', 'parent_a_id', 'parent_b_id', 'coi'])
CatCoiMemo = namedtuple('CatCoiMemo', ['parent_a_id', 'parent_b_id', 'coi'])

class Cat:
    def __init__(self, sql_key, blob):
        self.sql_key = sql_key
        self.blob = blob

    def name(self):
        offset = 12
        name_length = struct.unpack_from('<Q', self.blob, offset=offset)[0]
        offset += 8
        name_bytes = struct.unpack_from(f'<{name_length*2}s', self.blob, offset=offset)[0]
        return name_bytes.decode('utf-16')

    def sex(self):
        offset = 12
        name_length = struct.unpack_from('<Q', self.blob, offset=offset)[0]
        offset += 8 + name_length * 2
        decoration_length = struct.unpack_from('<Q', self.blob, offset=offset)[0]
        offset += 8 + decoration_length
        sex_bytes = struct.unpack_from(f'<i', self.blob, offset=offset)[0]
        if sex_bytes == 0:
            return 'male'
        elif sex_bytes == 1:
            return 'female'
        else:
            return 'neutral'

ParallelHashMapDump = namedtuple('ParallelHashMapDump', ['s_version', 'size', 'capacity', 'hash_table', 'data_table', 'growth_left'])
def scrape_hash_table(buffer, offset, unpack_string, unpack_size, raw_view=False):
    # in older versions of parallel-hashmap, the version field is not present
    # s_version < 2^64-1-10 are decoded as size and subsequent reads are frame-shifted by 1 qword
    # however, presumably, mewgenics never used those older library versions
    s_version, size, capacity = struct.unpack_from('<QQQ', buffer, offset=offset)

    # hash table contains capacity # of hash entries and 1 end of table marker
    # followed by a replicate of the first 16 hash entries
    hash_table_size = capacity + 1 + 16

    # we retrieve the non-EOT hash entries
    hash_table = struct.unpack_from(f'<{capacity}B', buffer, offset=offset + 24)

    # the hashing algorithm digests keys into a range 0x00-0x7F
    # and uses control values outside the range to represent an unoccupied bucket
    # these facts are sufficient to correctly scrape occupied data rows without fully reversing the digest implementation
    data_table = []
    for i in range(capacity):
        if raw_view or hash_table[i] <= 0x7f:
            data_table.append(struct.unpack_from(unpack_string, buffer, offset=offset + 24 + hash_table_size + i * unpack_size))

    # in older versions of parallel-hashmap, growth left is not present
    # presumably, mewgenics has always used versions where growth left is present
    growth_left = struct.unpack_from('<Q', buffer, offset=offset + 24 + hash_table_size + capacity * unpack_size)[0]

    start_of_next_table = offset + 24 + hash_table_size + capacity * unpack_size + 8

    if raw_view:
        return ParallelHashMapDump(s_version, size, capacity, hash_table, data_table, growth_left), start_of_next_table
    else:
        return data_table, start_of_next_table

class DumpedSaveReader:
    def __init__(self, directory):
        self.directory = directory

    def read_cats(self):
        cats = {}
        for cat_file in Path(self.directory, 'cats').iterdir():
            key = int(cat_file.stem)
            value = cat_file.read_bytes()
            cats[key] = Cat(key, value)
        return cats

    def read_pedigree(self, raw_view=False):
        result = Path(self.directory, 'files', 'pedigree.bin').read_bytes()

        start_of_next_table = 0
        pedigree_data, start_of_next_table = scrape_hash_table(result, start_of_next_table, '<qqqd', 32, raw_view=raw_view)
        coi_memo_table, start_of_next_table = scrape_hash_table(result, start_of_next_table, '<qqd', 24, raw_view=raw_view)
        accessible_cats_table, start_of_next_table = scrape_hash_table(result, start_of_next_table, '<q', 8, raw_view=raw_view)

        if raw_view:
            return pedigree_data, coi_memo_table, accessible_cats_table
        else:
            pedigree_dict = {x[0]: CatPedigree._make(x) for x in pedigree_data}
            coi_memo_dict = {x[0]: CatCoiMemo._make(x) for x in coi_memo_table}
            accessible_cats_set = set([x[0] for x in accessible_cats_table])
            return pedigree_dict, coi_memo_dict, accessible_cats_set
