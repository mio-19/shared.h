#ifndef _SHARED_H
#define _SHARED_H

#include <mutex>
#include <thread>
using namespace std;

#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
template<class T>
struct SharedIterator;
template<class T>
class Shared {
public:
    std::shared_ptr<T> shared;
    std::mutex lock;
    volatile size_t ver = 0;
    // update function
    void update(std::shared_ptr<T> new_shared) {
        std::lock_guard<std::mutex> locked (lock);
        shared = new_shared;
        ver++;
    }
    void try_update(std::shared_ptr<T> new_shared) {
        if(lock.try_lock()){
            shared = new_shared;
            ver++;
            lock.unlock();
        }
    }
    void copy_update(T new_shared) {
        auto pointer = std::make_shared<T>(new_shared);
        update(pointer);
    }
    // lock & get function
    std::shared_ptr<T> get() {
        std::lock_guard<std::mutex> locked (lock);
        return shared;
    }
    T get_copy() {
        std::lock_guard<std::mutex> locked (lock);
        return *shared;
    }
    SharedIterator<T> begin() {
        Shared<T> &shared = *this;
        return SharedIterator<T>(shared);
    }
    SharedIterator<T> end() {
        Shared<T> &shared = *this;
        SharedIterator<T> end = SharedIterator<T>(shared);
        end.is_end = true;
        return end;
    }
};
template<class T>
struct SharedIterator {
    Shared<T> *shared;
    std::shared_ptr<T> holder;
    size_t ver;
    bool is_end = false;
    explicit SharedIterator(Shared<T> &used_shared) {
        shared = &used_shared;
        std::lock_guard<std::mutex> lock (shared->lock);
        ver = shared->ver;
        holder = shared->shared;
    }
    T &operator*() {
        return *holder;
    }
    std::shared_ptr<T> get() {
        return holder;
    }
    SharedIterator &operator++() {
        while (ver == shared->ver) {
            std::this_thread::yield();
        }
        std::lock_guard<std::mutex> lock (shared->lock);
        ver = shared->ver;
        holder = shared->shared;
        return *this;
    }
    SharedIterator &next() {
        return ++*this;
    }


    friend bool operator== (const SharedIterator& a, const SharedIterator& b) { return a.shared == b.shared && a.is_end == b.is_end && a.ver == b.ver; };
    friend bool operator!= (const SharedIterator& a, const SharedIterator& b) { return !(a == b); };
};

#endif
