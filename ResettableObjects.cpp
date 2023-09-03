#include "ResetGuard.h"
#include <iostream>
#include <mutex>
#include <random>

using namespace std::chrono_literals;

class ICounter
{
public:

    using ResetRequiredHandler = std::function<void()>;

    virtual ~ICounter() = default;
    ICounter(ICounter const&) = delete;
    ICounter(ICounter&&) = delete;
    ICounter& operator=(ICounter const&) = delete;
    ICounter& operator=(ICounter&&) = delete;

    virtual void IncrementAndPrint() = 0;

    virtual void Reset() = 0;

protected:
    ICounter() = default;
};

class Counter : public ICounter
{
public:
    Counter(ResetGuard& resetGuard)
        : m_resetGuard{ resetGuard }
    { }

    void IncrementAndPrint() override
    {
        m_resetGuard.strong_guard();
        std::cout << ++m_counter << std::endl;
    }

    void Reset() override
    {
        m_counter = 0;
    }

private:
    uint64_t m_counter = 0;
    ResetGuard& m_resetGuard;
};

class IntermediateCounter final : public ICounter
{
public:

    IntermediateCounter(ICounter& counter, ResetGuard& resetGuard)
        : m_counter{ counter },
        m_resetGuard{ resetGuard }
    { }

    void IncrementAndPrint() override
    {
        m_resetGuard.strong_guard();
        std::cout << "IncrementAndPrint object: " << std::hex << this << std::dec << std::endl;
        //std::this_thread::sleep_for(1ms);
        m_counter.IncrementAndPrint();
    }

    void Reset() override
    {
        m_counter.Reset();
    }

private:
    ICounter& m_counter;
    ResetGuard& m_resetGuard;
};

auto count(ICounter& counter, ResetGuard& resetGuard)
{
    while (true)
    {
        try
        {
            while (true)
            {
                {
                    auto lock = std::scoped_lock{ resetGuard };
                    counter.IncrementAndPrint();
                }
                //std::this_thread::sleep_for(2ms);
                //std::this_thread::yield();
            }
        }
        catch (ObjectWasReset const&)
        {
            std::cout << "Counter was reset!" << std::endl;
        }
    }
}

int main()
{
    auto resetGuard = ResetGuard{};
    auto counter0 = Counter{ resetGuard };
    auto counter1 = IntermediateCounter{ counter0, resetGuard };
    auto counter2 = IntermediateCounter{ counter1, resetGuard };
    auto counter3 = IntermediateCounter{ counter2, resetGuard };
    auto const onReset = [&counter3]()
    {
        counter3.Reset();
    };
    resetGuard.registerResetHandler(onReset);
    auto countingThread = std::thread{ &count, std::ref(counter3), std::ref(resetGuard) };

    auto rd = std::random_device{};
    auto mt19937 = std::mt19937{ rd() };
    auto const distribution = std::uniform_int_distribution<>{ 500, 5500 };

    while (true)
    {
        auto const millis = distribution(mt19937);
        std::this_thread::sleep_for(std::chrono::microseconds{ millis });
        resetGuard.reset();
    }
}

