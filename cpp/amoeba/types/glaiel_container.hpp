#pragma once

#include <cstdint>

// Reconstructions of Mewgenics structures.
//
// Containers library.
//
// polymeric 2026

template<typename T, uint32_t S>
struct PodBufferPreallocated { // Mewgenics
    uint32_t capacity;
    uint32_t size;
    union {
        // prealloc for stack placement
        T buf[S];
        T *ptr;
    } u;

    const T *begin() const {
        if(this->capacity <= S) {
            return &this->u.buf[0];
        } else {
            return this->u.ptr;
        }
    }

    const T *end() const {
        if(this->capacity <= S) {
            return &this->u.buf[this->size];
        } else {
            return this->u.ptr + this->size;
        }
    }

    T *begin() {
        if(this->capacity <= S) {
            return &this->u.buf[0];
        } else {
            return this->u.ptr;
        }
    }

    T *end() {
        if(this->capacity <= S) {
            return &this->u.buf[this->size];
        } else {
            return this->u.ptr + this->size;
        }
    }
};

template<typename T>
struct podvector { // Wookash stream
    uint32_t capacity_;
    uint32_t size_;

    T *data_;

    const T *begin() const {
        return this->data_;
    }

    const T *end() const {
        return this->data_ + this->size_;
    }

    T *begin() {
        return this->data_;
    }

    T *end() {
        return this->data_ + this->size_;
    }
};

template<typename T>
struct flatset { // Wookash stream
    podvector<T> sorted_;
    podvector<T> back_;
    podvector<T> unsorted_;
    podvector<T> append_;
    bool needs_flatten;
};

template<typename T, int32_t C>
struct ConstEvalArray {
    T data[C];
    int32_t size;
};
