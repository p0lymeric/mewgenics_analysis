#pragma once

#include <windows.h>

// Virtual memory utilities
//
// polymeric 2026

// Performs a potentially unsound read behind an address, without risk of triggering a fault.
// "judgment-free read" or, "just f'ing read"
template<class T>
bool jf_read(const void *addr, T *buf) {
    return ReadProcessMemory(GetCurrentProcess(), addr, buf, sizeof(T), NULL);
}

// Read size can be returned in case the user is interested in knowing if the read managed to scrape some data before
// faulting at a page boundary.
// template<class T>
// size_t jf_readn(const void *addr, T *buf) {
//     size_t bytes_read = 0;
//     ReadProcessMemory(GetCurrentProcess(), addr, buf, sizeof(T), &bytes_read);
//     return bytes_read;
// }
