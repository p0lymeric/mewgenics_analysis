#include "utilities/pe_view.hpp"

#include <utility>

#include <windows.h>

// Simple PE file view
//
// polymeric 2026

PeView::PeView() :
    file_path(""), file_handle(INVALID_HANDLE_VALUE), mmap_handle(nullptr), mmap_view(nullptr), file_size(0)
{}

PeView::~PeView() {
    this->close();
}

PeView::PeView(PeView &&other) :
    file_path(std::move(other.file_path)), file_handle(other.file_handle), mmap_handle(other.mmap_handle), mmap_view(other.mmap_view), file_size(other.file_size)
{
    other.mmap_view = nullptr;
    other.mmap_handle = nullptr;
    other.file_handle = INVALID_HANDLE_VALUE;
    other.file_size = 0;
}

PeView &PeView::operator=(PeView &&other) {
    if(this != &other) {
        this->close();
        this->file_path = std::move(other.file_path);
        this->mmap_view = other.mmap_view;
        this->mmap_handle = other.mmap_handle;
        this->file_handle = other.file_handle;
        this->file_size = other.file_size;
        other.mmap_view = nullptr;
        other.mmap_handle = nullptr;
        other.file_handle = INVALID_HANDLE_VALUE;
        other.file_size = 0;
    }
    return *this;
}

void PeView::open(std::filesystem::path path) {
    if(!this->is_opened()) {
        this->file_path = std::move(path);
        this->file_handle = CreateFileW(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if(this->file_handle != INVALID_HANDLE_VALUE) {
            this->mmap_handle = CreateFileMappingW(this->file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
            if(this->mmap_handle != nullptr) {
                this->mmap_view = static_cast<uint8_t *>(MapViewOfFile(mmap_handle, FILE_MAP_READ, 0, 0, 0));
                LARGE_INTEGER size = {};
                if(GetFileSizeEx(this->file_handle, &size)) {
                    this->file_size = static_cast<size_t>(size.QuadPart);
                    if(this->post_open_bounds_check()) {
                        return;
                    }
                }
            }
        }
        this->close();
    }
}

void PeView::close() {
    this->file_size = 0;
    if(this->mmap_view != nullptr) {
        UnmapViewOfFile(this->mmap_view);
        this->mmap_view = nullptr;
    }
    if(this->mmap_handle != nullptr) {
        CloseHandle(this->mmap_handle);
        this->mmap_handle = nullptr;
    }
    if(this->file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(this->file_handle);
        this->file_handle = INVALID_HANDLE_VALUE;
    }
    this->file_path = "";
}

bool PeView::is_opened() const {
    return this->mmap_view != nullptr;
}

std::span<uint8_t> PeView::get_file_span() const {
    if(this->is_opened()) {
        return std::span(this->mmap_view, this->file_size);
    }
    return std::span<uint8_t>();
}

// We use windows.h only for mapping files to virtual memory
// Our PE parser provides its own offsets and expected values instead of leveraging winnt.h's structures
static const size_t PE_DOS_HEADER_SIZE = 64;
static const size_t PE_DOS_HEADER_MAGIC_OFFSET = 0;
static const size_t PE_DOS_HEADER_LFANEW_OFFSET = 60;

static const size_t PE_NT_HEADERS_FIXSIZE = 24; // Signature + File Header
static const size_t PE_NT_HEADERS_SIGNATURE_OFFSET = 0;
static const size_t PE_NT_HEADERS_FILE_HEADER_OFFSET = 4;
static const size_t PE_NT_HEADERS_OPTIONAL_HEADER_OFFSET = 24;

static const size_t PE_NT_HEADERS_FILE_HEADER_NUMBEROFSECTIONS_OFFSET = 2;
static const size_t PE_NT_HEADERS_FILE_HEADER_SIZEOFOPTIONALHEADER_OFFSET = 16;

static const size_t PE_SECTION_HEADER_SIZE = 40;
static const size_t PE_SECTION_HEADER_VIRTUALADDRESS_OFFSET = 12;
static const size_t PE_SECTION_HEADER_SIZEOFRAWDATA_OFFSET = 16;
static const size_t PE_SECTION_HEADER_POINTERTORAWDATA_OFFSET = 20;

static const uint16_t PE_DOS_HEADER_MAGIC_EXPECTED = 0x5A4D;
static const uint32_t PE_NT_HEADERS_SIGNATURE_EXPECTED = 0x00004550;

uintptr_t PeView::file_offset_to_rva(uintptr_t offset) const {
    if(this->is_opened()) {
        std::span<uint8_t> image = this->get_file_span();
        uint8_t *image_data = image.data();

        uint32_t lfanew = *reinterpret_cast<uint32_t *>(image_data + PE_DOS_HEADER_LFANEW_OFFSET);
        uint8_t *nt_headers = image_data + lfanew;

        uint16_t number_of_sections = *reinterpret_cast<uint16_t *>(
            nt_headers +
            PE_NT_HEADERS_FILE_HEADER_OFFSET +
            PE_NT_HEADERS_FILE_HEADER_NUMBEROFSECTIONS_OFFSET
        );

        uint16_t size_of_optional_header = *reinterpret_cast<uint16_t *>(
            nt_headers +
            PE_NT_HEADERS_FILE_HEADER_OFFSET +
            PE_NT_HEADERS_FILE_HEADER_SIZEOFOPTIONALHEADER_OFFSET
        );

        uint8_t *section_table = nt_headers + PE_NT_HEADERS_OPTIONAL_HEADER_OFFSET + size_of_optional_header;

        for(uint16_t i = 0; i < number_of_sections; i++) {
            uint8_t *section = section_table + i * PE_SECTION_HEADER_SIZE;

            uint32_t pointer_to_raw_data = *reinterpret_cast<uint32_t *>(section + PE_SECTION_HEADER_POINTERTORAWDATA_OFFSET);
            uint32_t size_of_raw_data = *reinterpret_cast<uint32_t *>(section + PE_SECTION_HEADER_SIZEOFRAWDATA_OFFSET);
            uint32_t virtual_address = *reinterpret_cast<uint32_t *>(section + PE_SECTION_HEADER_VIRTUALADDRESS_OFFSET);

            if(offset >= pointer_to_raw_data && offset < pointer_to_raw_data + size_of_raw_data) {
                return virtual_address + offset - pointer_to_raw_data;
            }
        }

        // assume the subtractive case is identity mapped (e.g. headers)
        return offset;
        // TODO should return a nullopt for unmapped data
    }
    // FIXME should return a nullopt here
    return 0;
}

bool PeView::post_open_bounds_check() {
    std::span<uint8_t> image = this->get_file_span();
    uint8_t *image_data = image.data();
    size_t image_size = image.size();

    // check DOS header size, magic
    if(image_size < PE_DOS_HEADER_SIZE) {
        return false;
    }
    if(*reinterpret_cast<uint16_t *>(image_data + PE_DOS_HEADER_MAGIC_OFFSET) != PE_DOS_HEADER_MAGIC_EXPECTED) {
        return false;
    }

    // check NT header fixed size, signature
    uint32_t lfanew = *reinterpret_cast<uint32_t *>(image_data + PE_DOS_HEADER_LFANEW_OFFSET);
    if(image_size < lfanew + PE_NT_HEADERS_FIXSIZE) {
        return false;
    }
    uint8_t *nt_headers = image_data + lfanew;
    if(*reinterpret_cast<uint32_t *>(nt_headers + PE_NT_HEADERS_SIGNATURE_OFFSET) != PE_NT_HEADERS_SIGNATURE_EXPECTED) {
        return false;
    }

    // check that the file can fit the optional and section headers
    // (does not check whether the file is large enough to fit the sections described by the section headers)
    uint16_t size_of_optional_header = *reinterpret_cast<uint16_t *>(
        nt_headers +
        PE_NT_HEADERS_FILE_HEADER_OFFSET +
        PE_NT_HEADERS_FILE_HEADER_SIZEOFOPTIONALHEADER_OFFSET
    );

    uint16_t number_of_sections = *reinterpret_cast<uint16_t *>(
        nt_headers +
        PE_NT_HEADERS_FILE_HEADER_OFFSET +
        PE_NT_HEADERS_FILE_HEADER_NUMBEROFSECTIONS_OFFSET
    );
    size_t section_table_offset = lfanew + PE_NT_HEADERS_OPTIONAL_HEADER_OFFSET + size_of_optional_header;
    if(image_size < section_table_offset + number_of_sections * PE_SECTION_HEADER_SIZE) {
        return false;
    }

    return true;
}
