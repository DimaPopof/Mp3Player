#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cstddef>
#include <mutex>
#include <vector>

class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity);

    std::size_t write(const float *data, std::size_t count);
    std::size_t read(float *out, std::size_t count);

    void clear();

    std::size_t size() const;
    std::size_t capacity() const;
    std::size_t freeSpace() const;

private:
    std::vector<float> buffer;
    std::size_t head;
    std::size_t tail;
    std::size_t stored;
    mutable std::mutex mutex;
};

#endif // RINGBUFFER_H