#pragma once

#include <semaphore>
#include <limits>
#include <cstddef>

class Semaphore
{
public:
    explicit Semaphore(std::ptrdiff_t initial = 0);
    ~Semaphore() = default;

    void Release(std::ptrdiff_t n = 1);
    void Acquire();

private:
    std::counting_semaphore<std::numeric_limits<std::ptrdiff_t>::max()> m_Sem;
};