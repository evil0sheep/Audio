#ifndef audio_block_queue_h_
#define audio_block_queue_h_

#include "AudioStream.h"


// simple circular buffer queue, code adapted from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
// interface intended to mimic std::queue
class queue<T> {
public:

    queue(size_t capacity) : capacity_(capacity) {
      buffer_ = (T*) malloc(capacity_ * sizeof(T));
      if(buffer_) valid_ = true;
    }

    bool empty() {
        return size_ == 0;
    }

    bool full() const {
        return size_ == capacity_;
    }

    size_t size() const {
        return size_;
    }

    // push() on full queue undefined
    void push(T item)
    {
        if (full())
            return;
        rear_ = (rear_ + 1) % capacity_;
        buffer_[rear_] = item;
        size_ = size_ + 1;
    }

    // pop() on empty queue undefined
    void pop()
    {
        if (empty())
            return;
        int item = buffer_[front_];
        front_ = (front_ + 1) % capacity_;
        size_ = size_ - 1;
        return item;
    }

    // front() on empty queue undefined
    T &front()
    {
        // always positive so safe for empty buffer undefined behavior
        return buffer_[front_];
    }

    // back() on empty queue undefined
    T &back(Queue* queue)
    {
        if (empty())
            return buffer_[0];
        return buffer_[rear_];
    }

    bool valid() const {return valid_};
    size_t dynamic_memory() const {return capacity_ * sizeof(T);}
private:
    ssize_t front_ = 0;
    ssize_t rear_ = -1;
    ssize_t size_ = 0;
    size_t capacity_;
    T* buffer_;
    bool valid_ = false;
};


#endif // audio_block_queue_h_