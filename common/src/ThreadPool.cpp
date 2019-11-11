#include "stdafx.h"
#include "ThreadPool.h"

void ThreadWorkerPool::Init()
{
	for (size_t i = 0; i < m_threads.size(); ++i)
		m_threads[i] = std::thread(WorkingThreadWrapper(this, i));
}

void ThreadWorkerPool::Shutdown()
{
	m_needToShutdown = true;
	m_conditionalVar.notify_all();
	for (size_t i = 0; i < m_threads.size(); ++i)
	{
		if (m_threads[i].joinable())
			m_threads[i].join();
	}
}

//this main thread function, see implementation of ThreadWorkerPool::Init()
void ThreadWorkerPool::WorkingThreadWrapper::operator()()
{
    std::function<void()> func;
    bool is_dequeued = false;
    while (!m_pool->m_needToShutdown)
    {
        { //this block is required to unlock mutex when we call and execute func()
            std::unique_lock<std::mutex> lock( m_pool->m_mutex );
            if(m_pool->m_queue.empty()) 
            {
                m_pool->m_conditionalVar.wait( lock );
            }
            //if spurious wake up is happened and queue is empty
            //no problem since we check return value is_dequeued
            is_dequeued = m_pool->m_queue.dequeue( func );
        }

        if (is_dequeued)
            func();
    } 
}
