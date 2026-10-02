#pragma once

#include <array>
#include <cstddef>
using namespace std;
template <typename T, std::size_t Capacity>
class CircularBuffer {
public:
    bool push(const T& value) {
        if (count_ == Capacity) {
            return false;
        }

        values_[tail_] = value;
        tail_ = (tail_ + 1U) % Capacity;
        ++count_;
        return true;
    }

    bool pop(T& value) {
        if (count_ == 0U) {
            return false;
        }

        value = values_[head_];
        head_ = (head_ + 1U) % Capacity;
        --count_;
        return true;
    }

    bool empty() const {
        return count_ == 0U;
    }

    bool full() const {
        return count_ == Capacity;
    }

private:
    array<T, Capacity> values_{};
    size_t head_ = 0U;
    size_t tail_ = 0U;
    size_t count_ = 0U;
};