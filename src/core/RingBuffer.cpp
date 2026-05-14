#include "RingBuffer.h"

#include <algorithm>

RingBuffer::RingBuffer(std::size_t capacity)
    : buffer(capacity, 0.0f), head(0), tail(0), stored(0) {
}

std::size_t RingBuffer::write(const float *data, std::size_t count) {
    if (data == nullptr || count == 0 || buffer.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex);
    
    // Eсли "count" превышает вместимость буфера, по сути можна убрать,
    // но на всякий случай оставлю
    const std::size_t writeCount = std::min(count, buffer.size());

    if (writeCount == 0) {
        return 0;
    }

    const std::size_t firstChunk = std::min(writeCount, buffer.size() - tail);
    std::copy_n(data, firstChunk, buffer.begin() + static_cast<std::ptrdiff_t>(tail));

    const std::size_t secondChunk = writeCount - firstChunk;
    if (secondChunk > 0) {
        std::copy_n(data + firstChunk, secondChunk, buffer.begin());
    }

    tail = (tail + writeCount) % buffer.size();

    if (stored + writeCount > buffer.size()) {
        stored = buffer.size();

        //Теперь будем читать свежие данные
        head = tail; 
    } else {
        stored += writeCount;
    }

    return writeCount;
}



std::size_t RingBuffer::read(float *out, std::size_t count) {
    if (out == nullptr || count == 0 || buffer.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex);

    const std::size_t readable = std::min(count, stored);
    if (readable == 0) {
        return 0;
    }

    const std::size_t firstChunk = std::min(readable, buffer.size() - head);
    std::copy_n(buffer.begin() + static_cast<std::ptrdiff_t>(head), firstChunk, out);

    const std::size_t secondChunk = readable - firstChunk;
    if (secondChunk > 0) {
        std::copy_n(buffer.begin(), secondChunk, out + firstChunk);
    }

    head = (head + readable) % buffer.size();
    stored -= readable;

    return readable;
}

void RingBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    head = 0;
    tail = 0;
    stored = 0;
}

std::size_t RingBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stored;
}

std::size_t RingBuffer::capacity() const {
    return buffer.size();
}

std::size_t RingBuffer::freeSpace() const {
    std::lock_guard<std::mutex> lock(mutex);
    return buffer.size() - stored;
}