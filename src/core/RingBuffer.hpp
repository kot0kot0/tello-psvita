#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

class RingBuffer {
public:
    RingBuffer(size_t size);
    ~RingBuffer();

    size_t write(const uint8_t* data, size_t size);
    size_t read(uint8_t* out, size_t size);

    size_t availableRead() const;
    size_t availableWrite() const;

private:
    uint8_t* m_buffer;
    const size_t m_size;

    std::atomic<size_t> m_head;
    std::atomic<size_t> m_tail;
};
