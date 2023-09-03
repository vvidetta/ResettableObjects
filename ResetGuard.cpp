#include "ResetGuard.h"

using namespace std::chrono_literals;

void ResetGuard::lock()
{
    m_lock.lock();
}

void ResetGuard::unlock()
{
    m_lock.unlock();
}

bool ResetGuard::weak_guard()
{
    if (m_resetState.load() == reset_state::valid)
    {
        return true;
    }

    static auto const ack_expected = [this]
    {
        return m_resetState.load() == reset_state::ack_expected;
    };
    m_cv.wait(m_lock, ack_expected);

    m_resetState.store(reset_state::valid);
    return false;
}

void ResetGuard::strong_guard()
{
    if (!weak_guard())
    {
        throw ObjectWasReset{};
    }
}

void ResetGuard::registerResetHandler(ResetHandler callback)
{
    m_resetHandler = std::move(callback);
}

void ResetGuard::reset()
{
    auto expected = reset_state::valid;
    if (!m_resetState.compare_exchange_strong(expected, reset_state::reset_required))
    {
        return;
    }

    {
        auto lock = std::scoped_lock{ m_mutex };
        m_resetHandler();
        m_resetState.store(reset_state::ack_expected);
    }
    m_cv.notify_all();
}

