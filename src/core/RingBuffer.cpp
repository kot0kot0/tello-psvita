#include "RingBuffer.hpp"
#include <cstring>
#include <algorithm>

RingBuffer::RingBuffer(size_t size)
    : m_size(size), m_head(0), m_tail(0) {
    m_buffer = new uint8_t[size];
}

RingBuffer::~RingBuffer() {
    delete[] m_buffer;
}

size_t RingBuffer::availableRead() const {
    size_t head = m_head.load(std::memory_order_acquire);
    size_t tail = m_tail.load(std::memory_order_acquire);
    return (head >= tail) ? (head - tail) : (m_size - tail + head);
}

size_t RingBuffer::availableWrite() const {
    return m_size - availableRead() - 1;
}

size_t RingBuffer::write(const uint8_t* data, size_t size) {
    size_t writable = availableWrite();
    size = std::min(size, writable);

    size_t head = m_head.load(std::memory_order_relaxed);

    size_t first = std::min(size, m_size - head);
    memcpy(m_buffer + head, data, first);
    memcpy(m_buffer, data + first, size - first);

    m_head.store((head + size) % m_size, std::memory_order_release);
    return size;
}

size_t RingBuffer::read(uint8_t* out, size_t size) {
    size_t readable = availableRead();
    size = std::min(size, readable);

    size_t tail = m_tail.load(std::memory_order_relaxed);

    size_t first = std::min(size, m_size - tail);
    memcpy(out, m_buffer + tail, first);
    memcpy(out + first, m_buffer, size - first);

    m_tail.store((tail + size) % m_size, std::memory_order_release);
    return size;
}
