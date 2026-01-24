/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Threading & Concurrency
 * Sistema de threads inspirado no Java 6
 */

#ifndef KAVA_THREADS_H
#define KAVA_THREADS_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>
#include <vector>
#include <map>
#include <future>
#include <climits>
#include "../gc/gc.h"

namespace Kava {

// Forward declarations
class KavaThread;
class ThreadPool;
class Lock;
class Condition;
class Semaphore;
class CountDownLatch;

// ============================================================
// ESTADO DA THREAD
// ============================================================
enum class ThreadState {
    NEW,
    RUNNABLE,
    BLOCKED,
    WAITING,
    TIMED_WAITING,
    TERMINATED
};

// ============================================================
// RUNNABLE (interface funcional)
// ============================================================
using Runnable = std::function<void()>;

// ============================================================
// KAVA THREAD - Thread gerenciada
// ============================================================
class KavaThread {
private:
    std::thread nativeThread;
    std::string name;
    int priority;
    bool daemon;
    std::atomic<ThreadState> state;
    std::atomic<bool> interruptedFlag;
    Runnable target;
    
    // Stack para a VM
    std::vector<int64_t> vmStack;
    int stackTop;
    
    // Frame de execução atual
    GCObject* currentFrame;
    
    // Grupo de threads
    class ThreadGroup* group;
    
    // Thread local storage
    std::map<int, GCObject*> threadLocals;
    
    // Monitor para sincronização
    std::mutex threadMutex;
    std::condition_variable threadCondition;
    
    static std::atomic<int> threadCounter;
    static thread_local KavaThread* currentThread_;
    
    void run() {
        currentThread_ = this;
        state = ThreadState::RUNNABLE;
        
        if (target) {
            try {
                target();
            } catch (...) {
                // Handle uncaught exception
            }
        }
        
        state = ThreadState::TERMINATED;
    }
    
public:
    static constexpr int MIN_PRIORITY = 1;
    static constexpr int NORM_PRIORITY = 5;
    static constexpr int MAX_PRIORITY = 10;
    
    KavaThread()
        : name("Thread-" + std::to_string(threadCounter++))
        , priority(NORM_PRIORITY)
        , daemon(false)
        , state(ThreadState::NEW)
        , interruptedFlag(false)
        , stackTop(0)
        , currentFrame(nullptr)
        , group(nullptr) {
        vmStack.resize(1024);
    }
    
    explicit KavaThread(Runnable r)
        : KavaThread() {
        target = std::move(r);
    }
    
    KavaThread(Runnable r, const std::string& threadName)
        : KavaThread(std::move(r)) {
        name = threadName;
    }
    
    ~KavaThread() {
        if (nativeThread.joinable()) {
            nativeThread.join();
        }
    }
    
    // Lifecycle
    void start() {
        if (state != ThreadState::NEW) {
            throw std::runtime_error("Thread already started");
        }
        nativeThread = std::thread(&KavaThread::run, this);
    }
    
    void join() {
        if (nativeThread.joinable()) {
            nativeThread.join();
        }
    }
    
    void join(long millis) {
        if (nativeThread.joinable()) {
            // C++ threads don't have timed join, simulate
            join();
        }
    }
    
    void interrupt() {
        interruptedFlag = true;
        threadCondition.notify_all();
    }
    
    bool isInterrupted() const {
        return interruptedFlag;
    }
    
    static bool checkInterrupted() {
        if (currentThread_) {
            bool was = currentThread_->interruptedFlag;
            currentThread_->interruptedFlag = false;
            return was;
        }
        return false;
    }
    
    // Getters/Setters
    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }
    
    int getPriority() const { return priority; }
    void setPriority(int p) { priority = std::clamp(p, MIN_PRIORITY, MAX_PRIORITY); }
    
    bool isDaemon() const { return daemon; }
    void setDaemon(bool d) { daemon = d; }
    
    ThreadState getState() const { return state; }
    bool isAlive() const { 
        return state == ThreadState::RUNNABLE || 
               state == ThreadState::BLOCKED ||
               state == ThreadState::WAITING ||
               state == ThreadState::TIMED_WAITING;
    }
    
    // VM Stack
    void push(int64_t value) {
        vmStack[stackTop++] = value;
    }
    
    int64_t pop() {
        return vmStack[--stackTop];
    }
    
    int64_t peek() const {
        return vmStack[stackTop - 1];
    }
    
    // Thread locals
    void setThreadLocal(int key, GCObject* value) {
        threadLocals[key] = value;
    }
    
    GCObject* getThreadLocal(int key) {
        auto it = threadLocals.find(key);
        return it != threadLocals.end() ? it->second : nullptr;
    }
    
    // Static methods
    static KavaThread* currentThread() {
        return currentThread_;
    }
    
    static void sleep(long millis) {
        if (currentThread_) {
            currentThread_->state = ThreadState::TIMED_WAITING;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
        if (currentThread_) {
            currentThread_->state = ThreadState::RUNNABLE;
        }
    }
    
    static void yield() {
        std::this_thread::yield();
    }
};

// Static initialization
inline std::atomic<int> KavaThread::threadCounter{0};
inline thread_local KavaThread* KavaThread::currentThread_ = nullptr;

// ============================================================
// REENTRANT LOCK - Lock reentrante como Java
// ============================================================
class ReentrantLock {
private:
    std::recursive_mutex mutex;
    std::condition_variable_any condition;
    std::atomic<KavaThread*> owner{nullptr};
    std::atomic<int> holdCount{0};
    bool fair;  // Fairness policy (not fully implemented)
    
public:
    explicit ReentrantLock(bool isFair = false) : fair(isFair) {}
    
    void lock() {
        mutex.lock();
        owner = KavaThread::currentThread();
        holdCount++;
    }
    
    bool tryLock() {
        if (mutex.try_lock()) {
            owner = KavaThread::currentThread();
            holdCount++;
            return true;
        }
        return false;
    }
    
    bool tryLock(long timeoutMillis) {
        auto until = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(timeoutMillis);
        
        while (std::chrono::steady_clock::now() < until) {
            if (tryLock()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
    
    void unlock() {
        if (--holdCount == 0) {
            owner = nullptr;
        }
        mutex.unlock();
    }
    
    bool isLocked() const {
        return holdCount > 0;
    }
    
    bool isHeldByCurrentThread() const {
        return owner == KavaThread::currentThread();
    }
    
    int getHoldCount() const {
        return holdCount;
    }
    
    void await() {
        condition.wait(mutex);
    }
    
    bool await(long timeoutMillis) {
        return condition.wait_for(mutex, std::chrono::milliseconds(timeoutMillis))
               == std::cv_status::no_timeout;
    }
    
    void signal() {
        condition.notify_one();
    }
    
    void signalAll() {
        condition.notify_all();
    }
};

// ============================================================
// SYNCHRONIZED BLOCK HELPER
// ============================================================
class SynchronizedBlock {
private:
    std::recursive_mutex& mutex;
    
public:
    explicit SynchronizedBlock(std::recursive_mutex& m) : mutex(m) {
        mutex.lock();
    }
    
    ~SynchronizedBlock() {
        mutex.unlock();
    }
    
    SynchronizedBlock(const SynchronizedBlock&) = delete;
    SynchronizedBlock& operator=(const SynchronizedBlock&) = delete;
};

// Macro para synchronized
#define SYNCHRONIZED(mutex) SynchronizedBlock _sync_##__LINE__(mutex)

// ============================================================
// SEMAPHORE - Semáforo contável
// ============================================================
class Semaphore {
private:
    std::mutex mutex;
    std::condition_variable condition;
    int permits;
    bool fair;
    
public:
    explicit Semaphore(int initialPermits, bool isFair = false)
        : permits(initialPermits), fair(isFair) {}
    
    void acquire() {
        acquire(1);
    }
    
    void acquire(int n) {
        std::unique_lock<std::mutex> lock(mutex);
        while (permits < n) {
            condition.wait(lock);
        }
        permits -= n;
    }
    
    bool tryAcquire() {
        return tryAcquire(1);
    }
    
    bool tryAcquire(int n) {
        std::unique_lock<std::mutex> lock(mutex);
        if (permits >= n) {
            permits -= n;
            return true;
        }
        return false;
    }
    
    bool tryAcquire(int n, long timeoutMillis) {
        std::unique_lock<std::mutex> lock(mutex);
        auto until = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(timeoutMillis);
        
        while (permits < n) {
            if (condition.wait_until(lock, until) == std::cv_status::timeout) {
                return false;
            }
        }
        permits -= n;
        return true;
    }
    
    void release() {
        release(1);
    }
    
    void release(int n) {
        std::unique_lock<std::mutex> lock(mutex);
        permits += n;
        condition.notify_all();
    }
    
    int availablePermits() const {
        return permits;
    }
};

// ============================================================
// COUNT DOWN LATCH
// ============================================================
class CountDownLatch {
private:
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<int> count;
    
public:
    explicit CountDownLatch(int initialCount) : count(initialCount) {}
    
    void await() {
        std::unique_lock<std::mutex> lock(mutex);
        while (count > 0) {
            condition.wait(lock);
        }
    }
    
    bool await(long timeoutMillis) {
        std::unique_lock<std::mutex> lock(mutex);
        auto until = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(timeoutMillis);
        
        while (count > 0) {
            if (condition.wait_until(lock, until) == std::cv_status::timeout) {
                return count == 0;
            }
        }
        return true;
    }
    
    void countDown() {
        if (--count == 0) {
            std::unique_lock<std::mutex> lock(mutex);
            condition.notify_all();
        }
    }
    
    int getCount() const {
        return count;
    }
};

// ============================================================
// CYCLIC BARRIER
// ============================================================
class CyclicBarrier {
private:
    std::mutex mutex;
    std::condition_variable condition;
    int parties;
    int waiting;
    int generation;
    Runnable barrierAction;
    
public:
    explicit CyclicBarrier(int numParties, Runnable action = nullptr)
        : parties(numParties)
        , waiting(0)
        , generation(0)
        , barrierAction(std::move(action)) {}
    
    int await() {
        std::unique_lock<std::mutex> lock(mutex);
        int gen = generation;
        int index = parties - ++waiting;
        
        if (waiting == parties) {
            // Último a chegar
            if (barrierAction) {
                barrierAction();
            }
            generation++;
            waiting = 0;
            condition.notify_all();
            return 0;
        }
        
        while (generation == gen) {
            condition.wait(lock);
        }
        
        return index;
    }
    
    bool await(long timeoutMillis) {
        std::unique_lock<std::mutex> lock(mutex);
        int gen = generation;
        waiting++;
        
        if (waiting == parties) {
            if (barrierAction) {
                barrierAction();
            }
            generation++;
            waiting = 0;
            condition.notify_all();
            return true;
        }
        
        auto until = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(timeoutMillis);
        
        while (generation == gen) {
            if (condition.wait_until(lock, until) == std::cv_status::timeout) {
                return false;
            }
        }
        
        return true;
    }
    
    void reset() {
        std::unique_lock<std::mutex> lock(mutex);
        generation++;
        waiting = 0;
        condition.notify_all();
    }
    
    int getParties() const { return parties; }
    int getNumberWaiting() const { return waiting; }
};

// ============================================================
// BLOCKING QUEUE
// ============================================================
template<typename T>
class BlockingQueue {
private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable notEmpty;
    std::condition_variable notFull;
    size_t capacity;
    
public:
    explicit BlockingQueue(size_t cap = SIZE_MAX) : capacity(cap) {}
    
    void put(const T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.size() >= capacity) {
            notFull.wait(lock);
        }
        queue.push(item);
        notEmpty.notify_one();
    }
    
    T take() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.empty()) {
            notEmpty.wait(lock);
        }
        T item = queue.front();
        queue.pop();
        notFull.notify_one();
        return item;
    }
    
    bool offer(const T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.size() >= capacity) {
            return false;
        }
        queue.push(item);
        notEmpty.notify_one();
        return true;
    }
    
    bool poll(T& item) {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty()) {
            return false;
        }
        item = queue.front();
        queue.pop();
        notFull.notify_one();
        return true;
    }
    
    bool poll(T& item, long timeoutMillis) {
        std::unique_lock<std::mutex> lock(mutex);
        auto until = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(timeoutMillis);
        
        while (queue.empty()) {
            if (notEmpty.wait_until(lock, until) == std::cv_status::timeout) {
                return false;
            }
        }
        item = queue.front();
        queue.pop();
        notFull.notify_one();
        return true;
    }
    
    size_t size() const { return queue.size(); }
    bool isEmpty() const { return queue.empty(); }
};

// ============================================================
// THREAD POOL EXECUTOR
// ============================================================
class ThreadPoolExecutor {
private:
    std::vector<std::thread> workers;
    BlockingQueue<Runnable> tasks;
    std::atomic<bool> shutdownFlag{false};
    std::atomic<int> activeCount{0};
    int corePoolSize;
    int maximumPoolSize;
    
    void workerLoop() {
        while (!shutdownFlag) {
            Runnable task;
            if (tasks.poll(task, 100)) {
                activeCount++;
                try {
                    task();
                } catch (...) {
                    // Handle exception
                }
                activeCount--;
            }
        }
    }
    
public:
    ThreadPoolExecutor(int coreSize, int maxSize = -1)
        : corePoolSize(coreSize)
        , maximumPoolSize(maxSize > 0 ? maxSize : coreSize) {
        
        for (int i = 0; i < corePoolSize; i++) {
            workers.emplace_back(&ThreadPoolExecutor::workerLoop, this);
        }
    }
    
    ~ThreadPoolExecutor() {
        shutdownNow();
    }
    
    void execute(Runnable task) {
        if (shutdownFlag) {
            throw std::runtime_error("ThreadPool is shutdown");
        }
        tasks.put(std::move(task));
    }
    
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        execute([task]() { (*task)(); });
        return result;
    }
    
    void initiateShutdown() {
        shutdownFlag = true;
    }
    
    void shutdownNow() {
        shutdownFlag = true;
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    bool isShutdown() const { return shutdownFlag; }
    int getActiveCount() const { return activeCount; }
    int getPoolSize() const { return workers.size(); }
    size_t getQueueSize() const { return tasks.size(); }
    
    void awaitTermination() {
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    bool awaitTermination(long timeoutMillis) {
        auto until = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(timeoutMillis);
        
        for (auto& worker : workers) {
            if (worker.joinable()) {
                auto remaining = until - std::chrono::steady_clock::now();
                if (remaining.count() <= 0) return false;
                // C++ threads don't support timed join directly
                worker.join();
            }
        }
        return true;
    }
};

// ============================================================
// EXECUTORS FACTORY (como Java)
// ============================================================
class Executors {
public:
    static ThreadPoolExecutor* newFixedThreadPool(int nThreads) {
        return new ThreadPoolExecutor(nThreads);
    }
    
    static ThreadPoolExecutor* newCachedThreadPool() {
        return new ThreadPoolExecutor(0, INT_MAX);
    }
    
    static ThreadPoolExecutor* newSingleThreadExecutor() {
        return new ThreadPoolExecutor(1);
    }
};

// ============================================================
// ATOMIC WRAPPERS (para variáveis atômicas)
// ============================================================
template<typename T>
class AtomicInteger {
private:
    std::atomic<T> value;
    
public:
    AtomicInteger(T initial = 0) : value(initial) {}
    
    T get() const { return value.load(); }
    void set(T newValue) { value.store(newValue); }
    
    T getAndSet(T newValue) { return value.exchange(newValue); }
    
    bool compareAndSet(T expected, T newValue) {
        return value.compare_exchange_strong(expected, newValue);
    }
    
    T getAndIncrement() { return value.fetch_add(1); }
    T getAndDecrement() { return value.fetch_sub(1); }
    T incrementAndGet() { return ++value; }
    T decrementAndGet() { return --value; }
    
    T getAndAdd(T delta) { return value.fetch_add(delta); }
    T addAndGet(T delta) { return value += delta; }
    
    operator T() const { return get(); }
};

using AtomicInt = AtomicInteger<int>;
using AtomicLong = AtomicInteger<long>;
using AtomicBool = std::atomic<bool>;

// ============================================================
// READ-WRITE LOCK
// ============================================================
class ReadWriteLock {
private:
    std::mutex mutex;
    std::condition_variable readCond;
    std::condition_variable writeCond;
    int readers = 0;
    int writers = 0;
    int waitingWriters = 0;
    
public:
    void readLock() {
        std::unique_lock<std::mutex> lock(mutex);
        while (writers > 0 || waitingWriters > 0) {
            readCond.wait(lock);
        }
        readers++;
    }
    
    void readUnlock() {
        std::unique_lock<std::mutex> lock(mutex);
        readers--;
        if (readers == 0) {
            writeCond.notify_one();
        }
    }
    
    void writeLock() {
        std::unique_lock<std::mutex> lock(mutex);
        waitingWriters++;
        while (readers > 0 || writers > 0) {
            writeCond.wait(lock);
        }
        waitingWriters--;
        writers++;
    }
    
    void writeUnlock() {
        std::unique_lock<std::mutex> lock(mutex);
        writers--;
        readCond.notify_all();
        writeCond.notify_one();
    }
    
    // RAII helpers
    class ReadGuard {
        ReadWriteLock& rwlock;
    public:
        ReadGuard(ReadWriteLock& l) : rwlock(l) { rwlock.readLock(); }
        ~ReadGuard() { rwlock.readUnlock(); }
    };
    
    class WriteGuard {
        ReadWriteLock& rwlock;
    public:
        WriteGuard(ReadWriteLock& l) : rwlock(l) { rwlock.writeLock(); }
        ~WriteGuard() { rwlock.writeUnlock(); }
    };
    
    ReadGuard readGuard() { return ReadGuard(*this); }
    WriteGuard writeGuard() { return WriteGuard(*this); }
};

} // namespace Kava

#endif // KAVA_THREADS_H
