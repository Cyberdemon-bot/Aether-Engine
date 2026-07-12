#include "aepch.h"
#include "Aether/Core/Semaphore.h"

Semaphore::Semaphore(std::ptrdiff_t initial) 
    : m_Sem(initial) 
{
}

void Semaphore::Release(std::ptrdiff_t n) 
{
    m_Sem.release(n);
}

void Semaphore::Acquire() 
{
    m_Sem.acquire();
}