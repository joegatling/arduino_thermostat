#ifndef EVENT_EMITTER_H
#define EVENT_EMITTER_H

#include <functional>
#include <vector>

/**
 * Generic EventEmitter class that supports multiple subscribers
 * with any function signature.
 * 
 * Usage examples:
 *   EventEmitter<float> tempEvent;
 *   EventEmitter<int, const char*> complexEvent;
 *   EventEmitter<> voidEvent;
 */
template<typename... Args>
class EventEmitter {
public:
    using Callback = std::function<void(Args...)>;
    
    /**
     * Subscribe a callback to this event
     * @param callback Function to call when event is emitted
     * @return Subscription ID (useful for later unsubscribe)
     */
    size_t subscribe(Callback callback) {
        subscribers.push_back(callback);
        return subscribers.size() - 1;
    }
    
    /**
     * Subscribe a lambda to this event
     */
    template<typename Func>
    size_t subscribe(Func callback) {
        subscribers.push_back(Callback(callback));
        return subscribers.size() - 1;
    }
    
    /**
     * Unsubscribe a callback by ID
     */
    void unsubscribe(size_t id) {
        if (id < subscribers.size()) {
            subscribers[id] = nullptr;
        }
    }
    
    /**
     * Emit the event to all subscribers
     */
    void emit(Args... args) {
        for (auto& callback : subscribers) {
            if (callback) {
                callback(args...);
            }
        }
    }
    
    /**
     * Clear all subscribers
     */
    void clear() {
        subscribers.clear();
    }
    
    /**
     * Get number of active subscribers
     */
    size_t subscriberCount() const {
        size_t count = 0;
        for (const auto& cb : subscribers) {
            if (cb) count++;
        }
        return count;
    }
    
private:
    std::vector<Callback> subscribers;
};

#endif // EVENT_EMITTER_H
