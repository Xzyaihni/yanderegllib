#ifndef YTHREADS_H
#define YTHREADS_H

#include <vector>
#include <thread>
#include <cassert>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <tuple>
#include <functional>

//design level: spaghetti

//the templates stuff is the main body so i cant separate it into a different file

namespace ythreads
{
	template<typename F = void*, typename A = void*, typename B = void*>
	class pool
	{
	public:
		pool() {};
		pool(const int threads_num, F call_func);
		pool(const int threads_num, F call_func, A arg0);
		pool(const int threads_num, F call_func, B arg0,  A arg1);

		pool(const pool&);
		pool(pool&&) noexcept;
		pool& operator=(const pool&);
		pool& operator=(pool&&) noexcept;

		~pool() {exit_threads();};
		
		void run(const A arg);
		void run();
		
		void exit_threads();
		
	private:
		void create_threads(const int num);

		void work_func();

		void copy_members(const pool&);
		void move_members(pool&&) noexcept;

		mutable std::mutex _wait_mutex;
		std::condition_variable _conditional_var;

		F _call_func;

		B _class_ptr;
		
		
		std::queue<A> _args_queue;
		
		std::vector<std::thread> _threads_vec;

		int _threads_num = 0;
		
		bool _threads_running = true;
		bool _empty = true;
	};


	template<typename F, typename A, typename B>
	pool<F, A, B>::pool(const int threads_num, F call_func)
	: _call_func(call_func), _threads_num(threads_num), _empty(false)
	{
		create_threads(threads_num);
	}

	template<typename F, typename A, typename B>
	pool<F, A, B>::pool(const int threads_num, F call_func, A arg0)
	: _call_func(call_func), _threads_num(threads_num), _empty(false)
	{
		create_threads(threads_num);
	}

	template<typename F, typename A, typename B>
	pool<F, A, B>::pool(const int threads_num, F call_func, B arg0,  A arg1)
	: _call_func(call_func), _threads_num(threads_num), _class_ptr(arg0), _empty(false)
	{
		create_threads(threads_num);
	}


	template<typename F, typename A, typename B>
	pool<F, A, B>::pool(const pool& other)
	{
		std::unique_lock<std::mutex> lock(other._wait_mutex);

		copy_members(other);
	}

	template<typename F, typename A, typename B>
	pool<F, A, B>::pool(pool&& other) noexcept
	{
		std::unique_lock<std::mutex> lock(other._wait_mutex);

		move_members(std::move(other));
	}

	template<typename F, typename A, typename B>
	pool<F, A, B>& pool<F, A, B>::operator=(const pool& other)
	{
		if(this != &other)
		{
			std::unique_lock<std::mutex> lock_self(_wait_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock(other._wait_mutex, std::defer_lock);
			std::lock(lock_self, lock);

			copy_members(other);
		}
		return *this;
	}

	template<typename F, typename A, typename B>
	pool<F, A, B>& pool<F, A, B>::operator=(pool&& other) noexcept
	{
		if(this != &other)
		{
			std::unique_lock<std::mutex> lock_self(_wait_mutex, std::defer_lock);
			std::unique_lock<std::mutex> lock(other._wait_mutex, std::defer_lock);
			std::lock(lock_self, lock);

			move_members(std::move(other));
		}
		return *this;
	}

	template<typename F, typename A, typename B>
	void pool<F, A, B>::copy_members(const pool& other)
	{
		_call_func = other._call_func;
		_class_ptr = other._class_ptr;

		_args_queue = other._args_queue;

		_threads_num = other._threads_num;

		_threads_running = other._threads_running;
		_empty = other._empty;

		create_threads(_threads_num);
	}

	template<typename F, typename A, typename B>
	void pool<F, A, B>::move_members(pool&& other) noexcept
	{
		_call_func = other._call_func;
		_class_ptr = other._class_ptr;

		_args_queue = std::move(other._args_queue);

		_threads_num = other._threads_num;

		_threads_running = other._threads_running;
		_empty = other._empty;

		create_threads(_threads_num);
	}


	template<typename F, typename A, typename B>
	void pool<F, A, B>::run(const A arg)
	{
		assert(!_empty);
		std::lock_guard<std::mutex> lock(_wait_mutex);
		
		_args_queue.push(arg);
		
		_conditional_var.notify_one();
	}

	template<typename F, typename A, typename B>
	void pool<F, A, B>::run()
	{
		run(nullptr);
	}

	template<typename F, typename A, typename B>
	void pool<F, A, B>::exit_threads()
	{
		if(_threads_running)
		{
			{
				std::lock_guard<std::mutex> lock(_wait_mutex);
			
				_threads_running = false;
			}
			_conditional_var.notify_all();
		
			for(auto& c_thread : _threads_vec)
			{
				c_thread.join();
			}
		}
	}

	template<typename F, typename A, typename B>
	void pool<F, A, B>::create_threads(const int num)
	{
		assert(num>0);

		_threads_vec.reserve(num);
		for(int i = 0; i < num; ++i)
		{
			_threads_vec.emplace_back(std::thread(&pool::work_func, this));
		}
	}

	template<typename F, typename A, typename B>
	void pool<F, A, B>::work_func()
	{
		while(true)
		{
			A f_arg;
			{
				std::unique_lock<std::mutex> lock(_wait_mutex);
				
				_conditional_var.wait(lock, [this]{return !_args_queue.empty() || !_threads_running;});
				
				if(!_threads_running)
					return;
					
				
				f_arg = std::move(_args_queue.front());
				_args_queue.pop();
			}
			
			
			if constexpr(std::is_member_function_pointer<F>::value)
			{
				//class member function
			
				if constexpr(std::is_same<A, void*>::value)
				{
					//it has no arguments
					std::invoke(_call_func, _class_ptr);
				} else
				{
					//it has arguments
					std::invoke(_call_func, _class_ptr, std::move(f_arg));
				}
			} else
			{
				//non class member function
				
				if constexpr(std::is_same<A, void*>::value)
				{
					//it has no arguments
					std::invoke(_call_func);
				} else
				{
					//it has arguments
					std::invoke(_call_func, std::move(f_arg));
				}
			}
		}
	}
};

#endif
