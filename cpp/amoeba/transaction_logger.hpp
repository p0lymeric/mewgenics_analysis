#pragma once

#include <fstream>
#include <filesystem>
#include <chrono>

// version independent header
// "AMOEBA"
// byte: version

// version 0 file
// byte: format: 0 - uncompressed, 1 - framed lz4, others - reserved
// <start of compressed zone>
// record...
// <end of compressed zone>

// records can be classified as full or compact by checking the first byte of the record

// version 0 full record
// byte: c-type (0)
// 3-word: s-type
// dword: reserved
// qword: length
// payload

// version 0 compact record
// byte: c-type (1-255)
// payload

// version 0 full type encodings (s-type for sub-type)
// 0: binary blob
// 1-255: reserved
// 1: utf-8 string

// version 0 compact type encodings (c-type for compact-type)
// data types
// 0: full record
// 1: not available
// 2: int64
// 3: double
// control types
// 253(-3): set timestamp in virtual stream
// 254(-2): select virtual stream
// 255(-1): end of physical stream; reset all virtual stream state

enum class CompactRecordCTypes : uint8_t {
    FullRecord = 0,
    NA = 1,
    Int64 = 2,
    Double = 3,
    SetTimestamp = 253,
    SelectVsid = 254,
    Reset = 255,
};

enum class FullRecordSTypes : uint32_t {
    Blob = 0,
    // 1-255 decode to a CompactRecordType
    StringUtf8 = 256,
};

class TransactionLogger {
public:
    TransactionLogger(std::string file_path) {
        this->file_path = file_path;
        this->file = std::ofstream(file_path, std::ios::binary);
    }
    ~TransactionLogger() {
        this->file.close();
    }

    void write_header() {
        // 6 byte magic
        this->file.write("AMOEBA", 6);
        // version byte
        this->file.put(0);
        // format byte
        this->file.put(0);
    }

    void reset(bool write_full = false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::Reset);
        this->write_compact_record(ctype, nullptr, 0, write_full);
    }

    void select_vsid(uint32_t vsid, bool write_full = false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::SelectVsid);
        this->write_compact_record(ctype, &vsid, 4, write_full, vsid);
    }

    void set_timestamp(uint64_t timestamp_us_epoch, bool write_full=false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::SetTimestamp);
        this->write_compact_record(ctype, &timestamp_us_epoch, 8, write_full);
    }

    void set_timestamp_now(bool write_full=false) {
        uint64_t timestamp_us_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock().now().time_since_epoch()).count();
        this->set_timestamp(timestamp_us_epoch, write_full);
    }

    void write_na(bool write_full=false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::NA);
        this->write_compact_record(ctype, nullptr, 0, write_full);
    }

    void write_int64(uint64_t val, bool write_full=false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::Int64);
        this->write_compact_record(ctype, &val, 8, write_full);
    }

    void write_double(double val, bool write_full=false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::Double);
        this->write_compact_record(ctype, &val, 8, write_full);
    }

    void write_blob_from_file(std::string file_path_to_copy) {
        uint64_t size = std::filesystem::file_size(file_path_to_copy);
        std::ifstream file_to_copy(file_path_to_copy, std::ios::binary);
        const uint32_t type = type_from_stype(FullRecordSTypes::Blob);
        this->write_full_record(type, file_to_copy, size);
    }

    void write_blob(const void *ptr, uint64_t size) {
        const uint32_t type = type_from_stype(FullRecordSTypes::Blob);
        this->write_full_record(type, ptr, size);
    }

    void write_string(std::string string) {
        const uint32_t type = type_from_stype(FullRecordSTypes::StringUtf8);
        this->write_full_record(type, string.data(), string.length());
    }

private:
    std::string file_path;
    std::ofstream file;

    constexpr uint32_t type_from_stype(FullRecordSTypes stype) {
        return static_cast<uint32_t>(stype) << 8;
    }

    void write_full_record(uint32_t type, std::ifstream& ifstream, uint64_t size, uint32_t rsvd=0) {
        this->file.write(reinterpret_cast<const char*>(&type), 4);
        this->file.write(reinterpret_cast<const char*>(&rsvd), 4);
        this->file.write(reinterpret_cast<const char*>(&size), 8);

        // TODO should perform size checks
        this->file << ifstream.rdbuf();

        this->file.flush();
    }

    void write_full_record(uint32_t type, const void* ptr, uint64_t size, uint32_t rsvd=0) {
        this->file.write(reinterpret_cast<const char*>(&type), 4);
        this->file.write(reinterpret_cast<const char*>(&rsvd), 4);
        this->file.write(reinterpret_cast<const char*>(&size), 8);
        this->file.write(static_cast<const char*>(ptr), size);
        this->file.flush();
    }

    void write_compact_record(uint8_t ctype, const void* ptr, uint64_t size, bool write_full=false, uint32_t rsvd=0) {
        if(write_full) {
            uint32_t type = static_cast<uint32_t>(ctype) << 8;
            write_full_record(type, ptr, size, rsvd);
        } else {
            this->file.put(ctype);
            this->file.write(static_cast<const char*>(ptr), size);
            this->file.flush();
        }
    }
};
