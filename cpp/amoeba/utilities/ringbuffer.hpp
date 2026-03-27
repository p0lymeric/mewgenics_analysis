#include <vector>
#include <optional>

// A bread-and-butter datastructure.
//
// Honestly surprised that most languages with sizable stdlibs
// require you to find a library or roll your own.
//
// polymeric 2026

template<class T>
class Ringbuffer {
public:
    Ringbuffer(size_t initial_capacity = 0) :
        cap(initial_capacity), head(0), tail(0), full(false)
    {
        this->buf.resize(this->cap);
    }

    // Overflow policy is to overwrite the oldest entry
    void push(const T &data) {
        this->buf[head] = data;

        this->head = (this->head + 1) % this->cap;
        if(this->full) {
            this->tail = this->head;
        } else {
            this->full = this->head == this->tail;
        }
    }

    std::optional<T> pop() {
        if(this->head == this->tail && !this->full) {
            return std::nullopt;
        }

        T data = std::move((*this)[0]);

        this->full = false;
        this->head = (this->head + this->cap - 1) % this->cap;

        return data;
    }

    void clear() {
        this->head = 0;
        this->tail = 0;
        this->full = false;
    }

    void resize(size_t new_capacity) {
        std::vector<T>().swap(this->buf);
        this->buf.resize(new_capacity);
        this->cap = new_capacity;
    }

    size_t size() {
        if (this->full) {
            return this->cap;
        } else if(this->head >= this->tail) {
            return this->head - this->tail;
        } else {
            return this->cap - this->tail + this->head;
        }
    }

    size_t capacity() {
        return this->cap;
    }

    T& operator[](size_t idx) {
        return this->buf[(this->head + this->cap - 1 - idx) % this->cap];
    }
private:
    std::vector<T> buf;
    size_t cap;
    size_t head;
    size_t tail;
    bool full;
};
