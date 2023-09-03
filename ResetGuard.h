#ifndef RESET_GUARD_H
#define RESET_GUARD_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>

class ObjectWasReset : public std::exception {};

class ResetGuard final
{
public:
    using ResetHandler = std::function<void()>;

    /// <summary>
    /// Constructs an object that manages the synchronization of a resettable object.
    /// </summary>
    ResetGuard() = default;

    ~ResetGuard() = default;
    ResetGuard(ResetGuard const&) = delete;
    ResetGuard(ResetGuard&&) = delete;
    ResetGuard& operator=(ResetGuard const&) = delete;
    ResetGuard& operator=(ResetGuard&&) = delete;

    /// <summary>
    /// Locks the internal mutex.
    /// Lock should be called before either strong_guard or weak_guard.
    /// </summary>
    void lock();

    /// <summary>
    /// Unlocks the internal mutex.
    /// Unlock should be called after strong_guard and weak_guard.
    /// </summary>
    void unlock();

    /// <summary>
    /// Checks if the guard requires resetting.
    /// If a reset is required, blocks until the object has been reset and returns.
    /// Use weak_guard if the guarded object will be in the correct state for the operation after reset.
    /// </summary>
    /// <returns>
    /// True if the object was valid on entry, false if the object had to be reset.
    /// </returns>
    bool weak_guard();

    /// <summary>
    /// Checks if the guard requires resetting.
    /// If a reset is required, blocks until the object has been reset and throws ObjectWasReset.
    /// Otherwise, this function returns immediately.
    /// Use strong_guard if the guarded object will be in an incorrect state for the operation after reset.
    /// </summary>
    void strong_guard();

    /// <summary>
    /// Registers a function to be called when the guard is reset.
    /// </summary>
    /// <param name="callback">A function to be called when the guard is reset.</param>
    void registerResetHandler(ResetHandler callback);

    /// <summary>
    /// Resets the guarded object.
    /// </summary>
    void reset();

private:
    enum class reset_state
    {
        valid,
        reset_required,
        ack_expected
    };

    std::atomic<reset_state> m_resetState = reset_state::valid;
    mutable std::mutex m_mutex;
    std::unique_lock<std::mutex> m_lock = std::unique_lock{ m_mutex, std::defer_lock };
    std::condition_variable m_cv;
    ResetHandler m_resetHandler;
};

#endif // RESET_GUARD_H
