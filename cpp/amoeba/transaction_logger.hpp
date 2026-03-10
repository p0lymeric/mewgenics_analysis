#pragma once

#include "debug_console.hpp"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <chrono>

#include "lz4frame.h"

// A binary logger for muxing separate data streams into a single file.
//
// polymeric 2026

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
// 1-255: aliases to compact type encodings
// 256: utf-8 string

// version 0 compact type encodings (c-type for compact-type)
// data types
// 0: full record
// 1: not available
// 2: int64
// 3: double
// control types
// 253(-3): set timestamp (signed microseconds since Unix epoch)
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
    TransactionLogger(std::string file_path, bool use_lz4) {
        this->file_path = file_path;
        this->file = std::ofstream(file_path, std::ios::binary); // TODO can throw
        this->stream_buffer.resize(this->CHUNK_SIZE);
        this->lz4_preferences = LZ4F_INIT_PREFERENCES;
        if(use_lz4) {
            this->format = 1;
            LZ4F_createCompressionContext(&this->lz4_cctx, LZ4F_VERSION); // TODO can fail
            this->compress_buffer.resize((std::max)(static_cast<size_t>(LZ4F_HEADER_SIZE_MAX), LZ4F_compressBound(this->CHUNK_SIZE, &this->lz4_preferences)));
        } else {
            this->format = 0;
            this->lz4_cctx = nullptr;
        }

        write_header();
    }

    ~TransactionLogger() {
        write_footer();

        // nullptr check is internal to the library function
        LZ4F_freeCompressionContext(this->lz4_cctx);
        this->file.close();
    }

    void reset(bool write_full = false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::Reset);
        this->write_compact_record(ctype, nullptr, 0, write_full);
    }

    void select_vsid(uint32_t vsid, bool write_full = false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::SelectVsid);
        this->write_compact_record(ctype, &vsid, 4, write_full, vsid);
    }

    void set_timestamp(int64_t timestamp_us_unix_epoch, bool write_full=false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::SetTimestamp);
        this->write_compact_record(ctype, &timestamp_us_unix_epoch, 8, write_full);
    }

    void set_timestamp_now(bool write_full=false) {
        int64_t timestamp_us_unix_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock().now().time_since_epoch()).count();
        this->set_timestamp(timestamp_us_unix_epoch, write_full);
    }

    void write_na(bool write_full=false) {
        const uint32_t ctype = static_cast<uint8_t>(CompactRecordCTypes::NA);
        this->write_compact_record(ctype, nullptr, 0, write_full);
    }

    void write_int64(int64_t val, bool write_full=false) {
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
    const size_t CHUNK_SIZE = 64 * 1024;
    std::string file_path;
    std::ofstream file;
    uint8_t format;
    std::vector<char> stream_buffer;
    std::vector<char> compress_buffer;
    LZ4F_cctx *lz4_cctx;
    LZ4F_preferences_t lz4_preferences;

    void write_header() {
        // 6 byte magic
        this->file.write("AMOEBA", 6);
        // version byte
        this->file.put(0);
        // format byte
        this->file.put(this->format);

        if(this->format == 1) {
            size_t written_size = LZ4F_compressBegin(this->lz4_cctx, this->compress_buffer.data(), this->compress_buffer.size(), &this->lz4_preferences);
            if(LZ4F_isError(written_size)) {
                // TODO somehow signal error
                DPRINTFMTPRE("lz4 - error - {}", LZ4F_getErrorName(written_size));
            }
            this->file.write(this->compress_buffer.data(), written_size);
        }
    }

    void write_footer() {
        if(this->format == 1) {
            size_t written_size = LZ4F_compressEnd(this->lz4_cctx, this->compress_buffer.data(), this->compress_buffer.size(), nullptr);
            if(LZ4F_isError(written_size)) {
                // TODO somehow signal error
                DPRINTFMTPRE("lz4 - error - {}", LZ4F_getErrorName(written_size));
            }
            this->file.write(this->compress_buffer.data(), written_size);
        }
    }

    void write_wrapped(const char* ptr, uint64_t size) {
        if(this->format == 1) {
            uint64_t remaining_size = size;
            uint64_t size_to_stream = CHUNK_SIZE;
            while(remaining_size > 0) {
                if(remaining_size < this->CHUNK_SIZE) {
                    size_to_stream = remaining_size;
                    remaining_size = 0;
                } else {
                    remaining_size -= CHUNK_SIZE;
                }
                size_t written_size = LZ4F_compressUpdate(this->lz4_cctx, this->compress_buffer.data(), this->compress_buffer.size(), ptr, size_to_stream, nullptr);
                ptr += size_to_stream;
                if(LZ4F_isError(written_size)) {
                    // TODO somehow signal error
                    DPRINTFMTPRE("lz4 - error - {}", LZ4F_getErrorName(written_size));
                }
                if(written_size > 0) {
                    this->file.write(this->compress_buffer.data(), written_size);
                }
            }
        } else {
            this->file.write(reinterpret_cast<const char*>(ptr), size);
        }
    }

    void write_wrapped_ifstream(std::ifstream& ifstream, uint64_t size) {
        uint64_t remaining_size = size;
        uint64_t size_to_stream = CHUNK_SIZE;
        while(remaining_size > 0) {
            if(remaining_size < this->CHUNK_SIZE) {
                size_to_stream = remaining_size;
                remaining_size = 0;
            } else {
                remaining_size -= CHUNK_SIZE;
            }
            ifstream.read(stream_buffer.data(), size_to_stream);
            if(this->format == 1) {
                size_t written_size = LZ4F_compressUpdate(this->lz4_cctx, this->compress_buffer.data(), this->compress_buffer.size(), stream_buffer.data(), size_to_stream, nullptr);
                if(LZ4F_isError(written_size)) {
                    // TODO somehow signal error
                    DPRINTFMTPRE("lz4 - error - {}", LZ4F_getErrorName(written_size));
                }
                if(written_size > 0) {
                    this->file.write(this->compress_buffer.data(), written_size);
                }
            } else {
                this->file.write(stream_buffer.data(), size_to_stream);
            }
        }
    }

    constexpr uint32_t type_from_stype(FullRecordSTypes stype) {
        return static_cast<uint32_t>(stype) << 8;
    }

    void write_full_record(uint32_t type, std::ifstream& ifstream, uint64_t size, uint32_t rsvd=0) {
        this->write_wrapped(reinterpret_cast<const char*>(&type), 4);
        this->write_wrapped(reinterpret_cast<const char*>(&rsvd), 4);
        this->write_wrapped(reinterpret_cast<const char*>(&size), 8);
        this->write_wrapped_ifstream(ifstream, size);
    }

    void write_full_record(uint32_t type, const void* ptr, uint64_t size, uint32_t rsvd=0) {
        this->write_wrapped(reinterpret_cast<const char*>(&type), 4);
        this->write_wrapped(reinterpret_cast<const char*>(&rsvd), 4);
        this->write_wrapped(reinterpret_cast<const char*>(&size), 8);
        this->write_wrapped(static_cast<const char*>(ptr), size);
    }

    void write_compact_record(uint8_t ctype, const void* ptr, uint64_t size, bool write_full=false, uint32_t rsvd=0) {
        if(write_full) {
            uint32_t type = static_cast<uint32_t>(ctype) << 8;
            write_full_record(type, ptr, size, rsvd);
        } else {
            this->write_wrapped(reinterpret_cast<const char*>(&ctype), 1);
            this->write_wrapped(static_cast<const char*>(ptr), size);
        }
    }
};
