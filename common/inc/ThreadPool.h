#pragma once
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <thread>
#include <future>
#include <utility>
#include <memory>
#include <condition_variable>

class ThreadWorkerPool;

template <typename T> class ThreadSafeQueue
{
public:
	ThreadSafeQueue() {};
	~ThreadSafeQueue() {};
	ThreadSafeQueue(const ThreadSafeQueue& other) 
	{
		std::lock<std::mutex> lock(other.m_mutex);
		m_queue = other.m_queue;
	}
	bool empty() const
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}
	size_t size() const
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_queue.size();
	}
	void enqueue(T& task)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.push(task);
	}
	bool dequeue(T& task)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_queue.empty())
			return false;
		task = m_queue.front();
		m_queue.pop();
		return true;
	}
private:
	mutable std::mutex m_mutex;
	std::queue<T> m_queue;
};

class ThreadWorkerPool
{
public:
	//constructors
	ThreadWorkerPool(size_t num_of_threads):
		m_threads( std::vector<std::thread>( num_of_threads ) ), m_needToShutdown( false ){}
	ThreadWorkerPool(const ThreadWorkerPool& other) = delete;
	ThreadWorkerPool(ThreadWorkerPool&& other) = delete;
	ThreadWorkerPool & operator=(const ThreadWorkerPool &) = delete;
	ThreadWorkerPool & operator=(ThreadWorkerPool &&) = delete;
public:
	void Init();
	void Shutdown();
	
	//add job task to be executed
    //for general case this method returns future object
    //to be used in some client code
    //however, in this project returned future is not used at all
    //this is templated method to work with general case
    //variable arguments that are passed into functional F
	template<typename F, typename...Args>
	auto AddJob(F&& f, Args&&... args) -> std::future<decltype(f(args...))> 
    {
		//at first step we transfrom F&& f object into
        //functional object with no arguments, but it return the same as f
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		
        //wrap func into packaged task to be performed async
        //and using future object
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        //translate packaged task object into lambda
        //that translated into function<void(void)>
        //to be pushed into safe queue 
		std::function<void()> wrapper_func = [task_ptr]() 
        {
			(*task_ptr)();  
		};

		m_queue.enqueue(wrapper_func);
		//wake up worker thread if one of them is waiting for cond var
        m_conditionalVar.notify_one();
		return task_ptr->get_future();
	}
public:
	bool m_needToShutdown;
	ThreadSafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_mutex;
	std::condition_variable m_conditionalVar;
private:
	class WorkingThreadWrapper
	{
	public:
		WorkingThreadWrapper(ThreadWorkerPool* pool, size_t id) : m_pool(pool), m_Id(id) {};
		
        //this main thread function, see implementation of ThreadWorkerPool::Init()
        void operator()();
	private:
		ThreadWorkerPool* m_pool;
		size_t m_Id;
	};
};