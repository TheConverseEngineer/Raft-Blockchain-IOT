#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <queue>
#include <mutex>

/** Simple locking thread-safe queue (fairly innefficient)
 * 
 * Not recommended for any real applications (use a consumer-producer queue instead)
*/
template <class T>
class SyncQueue : protected std::queue<T> {
    private:
        std::mutex m;

    public:
        /** Adds an item to the queue */
        void add(T item) {
            m.lock();
            std::queue<T>::push(item);
            m.unlock();
        }

        /** Gets and deletes the top item from the queue */
        T get() {
            m.lock();
            T output;
            if (!std::queue<T>::empty()) {
                output = std::queue<T>::front();
                std::queue<T>::pop();
            }
            m.unlock();
            return output;
        }

        /** Returns true if the queue is empty */
        bool empty() {
            m.lock();
            bool output = std::queue<T>::empty();
            m.unlock();
            return output;
        }

        void clear() {
            m.lock();
            while (!std::queue<T>::empty()) std::queue<T>::pop();
            m.unlock();
        }
};

#endif