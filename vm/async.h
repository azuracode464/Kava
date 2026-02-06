/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.5 - Async Runtime & Event Loop
 * Event loop proprio para async/await, timers, IO assincrono
 */

#ifndef KAVA_ASYNC_H
#define KAVA_ASYNC_H

#include <vector>
#include <queue>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <map>
#include <memory>

namespace Kava {

// ============================================================
// PROMISE STATE
// ============================================================
enum class PromiseState {
    PENDING,
    FULFILLED,
    REJECTED
};

// ============================================================
// PROMISE - Future value container
// ============================================================
class Promise {
public:
    using Callback = std::function<void(int64_t)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    PromiseState state = PromiseState::PENDING;
    int64_t value = 0;
    std::string error;
    
    std::vector<Callback> thenCallbacks;
    std::vector<ErrorCallback> catchCallbacks;
    
    int promiseId;
    
    void resolve(int64_t val) {
        if (state != PromiseState::PENDING) return;
        state = PromiseState::FULFILLED;
        value = val;
        for (auto& cb : thenCallbacks) {
            cb(val);
        }
    }
    
    void reject(const std::string& err) {
        if (state != PromiseState::PENDING) return;
        state = PromiseState::REJECTED;
        error = err;
        for (auto& cb : catchCallbacks) {
            cb(err);
        }
    }
    
    Promise& then(Callback cb) {
        if (state == PromiseState::FULFILLED) {
            cb(value);
        } else {
            thenCallbacks.push_back(cb);
        }
        return *this;
    }
    
    Promise& onCatch(ErrorCallback cb) {
        if (state == PromiseState::REJECTED) {
            cb(error);
        } else {
            catchCallbacks.push_back(cb);
        }
        return *this;
    }
    
    bool isSettled() const {
        return state != PromiseState::PENDING;
    }
};

// ============================================================
// TIMER - Scheduled callback
// ============================================================
struct Timer {
    int id;
    std::chrono::steady_clock::time_point fireAt;
    std::function<void()> callback;
    int64_t intervalMs;  // 0 = setTimeout, >0 = setInterval
    bool cancelled = false;
    
    bool operator>(const Timer& other) const {
        return fireAt > other.fireAt;
    }
};

// ============================================================
// TASK - Micro/Macro task
// ============================================================
struct Task {
    enum class Type { MICRO, MACRO, IO, TIMER };
    Type type;
    std::function<void()> callback;
    int priority = 0;  // Higher = more important
};

// ============================================================
// EVENT LOOP - Core async runtime
// ============================================================
class EventLoop {
private:
    // Task queues
    std::queue<Task> microtasks;
    std::queue<Task> macrotasks;
    
    // Timer heap (min-heap by fire time)
    std::priority_queue<Timer, std::vector<Timer>, std::greater<Timer>> timers;
    
    // IO completion queue
    std::queue<std::function<void()>> ioCompletions;
    
    // Promises
    std::map<int, std::shared_ptr<Promise>> promises;
    int nextPromiseId = 1;
    int nextTimerId = 1;
    
    // Control
    std::atomic<bool> running{false};
    std::atomic<bool> hasWork{false};
    std::mutex mutex;
    std::condition_variable cv;
    
    // IO thread pool
    std::vector<std::thread> ioThreads;
    std::queue<std::function<void()>> ioTasks;
    std::mutex ioMutex;
    std::condition_variable ioCv;
    std::atomic<bool> ioShutdown{false};
    static constexpr int IO_THREAD_COUNT = 4;
    
    void ioWorker() {
        while (!ioShutdown) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(ioMutex);
                ioCv.wait(lock, [this] { return ioShutdown || !ioTasks.empty(); });
                if (ioShutdown && ioTasks.empty()) return;
                task = std::move(ioTasks.front());
                ioTasks.pop();
            }
            if (task) task();
        }
    }

public:
    EventLoop() {
        // Start IO thread pool
        for (int i = 0; i < IO_THREAD_COUNT; i++) {
            ioThreads.emplace_back(&EventLoop::ioWorker, this);
        }
    }
    
    ~EventLoop() {
        stop();
        ioShutdown = true;
        ioCv.notify_all();
        for (auto& t : ioThreads) {
            if (t.joinable()) t.join();
        }
    }
    
    // ========================================
    // PROMISE API
    // ========================================
    std::shared_ptr<Promise> createPromise() {
        auto p = std::make_shared<Promise>();
        p->promiseId = nextPromiseId++;
        promises[p->promiseId] = p;
        return p;
    }
    
    std::shared_ptr<Promise> getPromise(int id) {
        auto it = promises.find(id);
        return it != promises.end() ? it->second : nullptr;
    }
    
    void resolvePromise(int id, int64_t value) {
        auto p = getPromise(id);
        if (p) p->resolve(value);
    }
    
    void rejectPromise(int id, const std::string& error) {
        auto p = getPromise(id);
        if (p) p->reject(error);
    }
    
    // ========================================
    // TIMER API
    // ========================================
    int setTimeout(std::function<void()> callback, int64_t delayMs) {
        Timer t;
        t.id = nextTimerId++;
        t.fireAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs);
        t.callback = std::move(callback);
        t.intervalMs = 0;
        
        std::lock_guard<std::mutex> lock(mutex);
        timers.push(t);
        hasWork = true;
        cv.notify_one();
        return t.id;
    }
    
    int setInterval(std::function<void()> callback, int64_t intervalMs) {
        Timer t;
        t.id = nextTimerId++;
        t.fireAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);
        t.callback = std::move(callback);
        t.intervalMs = intervalMs;
        
        std::lock_guard<std::mutex> lock(mutex);
        timers.push(t);
        hasWork = true;
        cv.notify_one();
        return t.id;
    }
    
    // ========================================
    // TASK SCHEDULING
    // ========================================
    void queueMicrotask(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex);
        microtasks.push({Task::Type::MICRO, std::move(task)});
        hasWork = true;
        cv.notify_one();
    }
    
    void queueMacrotask(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex);
        macrotasks.push({Task::Type::MACRO, std::move(task)});
        hasWork = true;
        cv.notify_one();
    }
    
    void queueIO(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(ioMutex);
        ioTasks.push(std::move(task));
        ioCv.notify_one();
    }
    
    void completeIO(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(mutex);
        ioCompletions.push(std::move(callback));
        hasWork = true;
        cv.notify_one();
    }
    
    // ========================================
    // EVENT LOOP EXECUTION
    // ========================================
    void tick() {
        // 1. Process all microtasks
        processMicrotasks();
        
        // 2. Process IO completions
        processIOCompletions();
        
        // 3. Fire ready timers
        processTimers();
        
        // 4. Process one macrotask
        processMacrotask();
    }
    
    void run() {
        running = true;
        while (running && hasPendingWork()) {
            tick();
            
            // Sleep briefly if no work
            if (!hasPendingWork()) {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait_for(lock, std::chrono::milliseconds(1));
            }
        }
    }
    
    void runFor(int64_t maxMs) {
        running = true;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(maxMs);
        while (running && std::chrono::steady_clock::now() < deadline && hasPendingWork()) {
            tick();
        }
    }
    
    void stop() {
        running = false;
        cv.notify_all();
    }
    
    bool hasPendingWork() const {
        return !microtasks.empty() || !macrotasks.empty() || 
               !timers.empty() || !ioCompletions.empty() ||
               hasPendingPromises();
    }
    
    bool hasPendingPromises() const {
        for (const auto& [id, p] : promises) {
            if (!p->isSettled()) return true;
        }
        return false;
    }

private:
    void processMicrotasks() {
        while (!microtasks.empty()) {
            auto task = std::move(microtasks.front());
            microtasks.pop();
            if (task.callback) task.callback();
        }
    }
    
    void processIOCompletions() {
        std::queue<std::function<void()>> completions;
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::swap(completions, ioCompletions);
        }
        while (!completions.empty()) {
            auto cb = std::move(completions.front());
            completions.pop();
            if (cb) cb();
        }
    }
    
    void processTimers() {
        auto now = std::chrono::steady_clock::now();
        while (!timers.empty() && timers.top().fireAt <= now) {
            Timer t = timers.top();
            timers.pop();
            
            if (!t.cancelled && t.callback) {
                t.callback();
                
                // Re-schedule interval timers
                if (t.intervalMs > 0 && !t.cancelled) {
                    t.fireAt = now + std::chrono::milliseconds(t.intervalMs);
                    timers.push(t);
                }
            }
        }
    }
    
    void processMacrotask() {
        if (!macrotasks.empty()) {
            auto task = std::move(macrotasks.front());
            macrotasks.pop();
            if (task.callback) task.callback();
            
            // Process microtasks generated by macrotask
            processMicrotasks();
        }
    }
};

} // namespace Kava

#endif // KAVA_ASYNC_H
