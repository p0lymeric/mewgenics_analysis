#include <vector>
#include <optional>

// A bread-and-butter datastructure.
//
// Honestly surprised that most languages with sizable stdlibs
// require you to find a library or roll your own.
//
// polymeric 2026

template<class T, bool Undo = false>
class Ringbuffer {
public:
    Ringbuffer(size_t initial_capacity = 0) :
        cap(initial_capacity), head(0), tail(0), undo_cursor(0), full_(false), undo_cursor_valid(false)
    {
        this->buf.resize(this->cap);
    }

    // Overflow policy is to overwrite the oldest entry
    void push(const T &data) {
        if constexpr(Undo) {
            this->set_head_to_undo_cursor_and_reattach_cursor();
        }

        this->buf[head] = data;

        this->head = (this->head + 1) % this->cap;
        if(this->full()) {
            this->tail = this->head;
        } else {
            this->full_ = this->head == this->tail;
        }
    }

    std::optional<T> pop() {
        if constexpr(Undo) {
            this->set_head_to_undo_cursor_and_reattach_cursor();
        }

        if(this->empty()) {
            return std::nullopt;
        }

        T data = std::move((*this)[0]);

        this->full_ = false;
        this->head = (this->head + this->cap - 1) % this->cap;

        return data;
    }

    void clear() {
        if constexpr(Undo) {
            // this->undo_cursor = 0; // no need to reset
            this->undo_cursor_valid = false;
        }

        this->head = 0;
        this->tail = 0;
        this->full_ = false;
    }

    void resize(size_t new_capacity) {
        std::vector<T>().swap(this->buf);
        this->buf.resize(new_capacity);
        this->cap = new_capacity;
        // FIXME reposition head/tail, recalculate full_
    }

    size_t size() {
        if (this->full()) {
            return this->cap;
        } else if(this->head >= this->tail) {
            return this->head - this->tail;
        } else {
            return this->cap - this->tail + this->head;
        }
    }

    bool full() {
        return this->full_;
    }

    bool empty() {
        return !this->full() && (this->head == this->tail);
    }

    size_t capacity() {
        return this->cap;
    }

    T& operator[](size_t idx) {
        return this->buf[(this->head + this->cap - 1 - idx) % this->cap];
    }

    void undo_detach() requires Undo {
        this->undo_cursor = this->head;
        this->undo_cursor_valid = true;
    }

    void undo_attach() requires Undo {
        this->undo_cursor_valid = false;
    }

    bool undo_can_step_backward() requires Undo {
        if(this->undo_cursor_valid) {
            return this->undo_effective_size() > 1;
        } else {
            return this->size() > 1;
        }
    }

    bool undo_can_step_forward() requires Undo {
        if(this->undo_cursor_valid) {
            return this->size() != this->undo_effective_size();
        } else {
            return false;
        }
    }

    void undo_step_backward() requires Undo {
        if(!this->undo_cursor_valid) {
            if(this->size() > 1) {
                undo_detach();
            } else {
                return;
            }
        }
        // since we rebase to the cursor on a push/pop/clear, this will never underflow
        size_t undo_cursor_m1 = (this->undo_cursor + this->cap - 1) % this->cap;
        if(undo_cursor_m1 != this->tail) {
            this->undo_cursor = undo_cursor_m1;
        }
    }

    // aka "redo"
    void undo_step_forward() requires Undo {
        if(this->undo_cursor_valid) {
            if(this->size() != this->undo_effective_size()) {
                this->undo_cursor = (this->undo_cursor + 1) % this->cap;
            }
        }
        // since we rebase to the cursor on a push/pop, no need to reattach the cursor
    }

    std::optional<T> undo_peek() requires Undo {
        if(!this->empty()) {
            if(this->undo_cursor_valid) {
                return this->buf[(this->undo_cursor + this->cap - 1) % this->cap];
            } else {
                return this->buf[(this->head + this->cap - 1) % this->cap];
            }
        }
        return std::nullopt;
    }
private:
    std::vector<T> buf;
    size_t cap;
    size_t head;
    size_t tail;
    size_t undo_cursor; // TODO ZST
    bool full_;
    bool undo_cursor_valid; // TODO ZST

    void set_head_to_undo_cursor_and_reattach_cursor() requires Undo {
        if(this->undo_cursor_valid) {
            if(this->undo_cursor != this->head) {
                this->head = this->undo_cursor;
                // If the cursor were equal to head, it may point to the end of the buffer,
                // but we would've fallen out of this case.
                // That is to say, if the undo cursor differs from head, it isn't possible
                // for it to point to the end of a full buffer.
                this->full_ = false;
            }
            this->undo_cursor_valid = false;
        }
    }

    size_t undo_effective_size() requires Undo {
        if (this->full() && this->head == this->undo_cursor) {
            return this->cap;
        } else if(this->undo_cursor >= this->tail) {
            return this->undo_cursor - this->tail;
        } else {
            return this->cap - this->tail + this->undo_cursor;
        }
    }
};
