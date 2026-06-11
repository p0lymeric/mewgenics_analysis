#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

// Simple PE file view
//
// polymeric 2026

class PeView {
public:
    PeView();
    ~PeView();
    PeView(const PeView &other) = delete;
    PeView &operator=(const PeView &other) = delete;
    PeView(PeView &&other);
    PeView &operator=(PeView &&other);

    void open(std::filesystem::path path);
    void close();

    bool is_opened() const;

    std::span<uint8_t> get_file_span() const;
    uintptr_t file_offset_to_rva(uintptr_t offset) const;

private:
    using HANDLE = void *;
    std::filesystem::path file_path;
    HANDLE file_handle;
    HANDLE mmap_handle;
    uint8_t *mmap_view;
    size_t file_size;

    bool post_open_bounds_check();
};
