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

#include <iostream>

//design level: spaghetti

//the templates stuff is the main body so i cant separate it into a different file

namespace yanderethreads
{
	namespace yan_templates
	{
		//because the std::conditional_t is evaluated at compile time and not branched
		//i need different versions of functions to always return valid types
		template<bool C, int N, typename... A>
		struct valid_element
		{};

		template<int N, typename... A>
		struct valid_element<true, N, A...>
		{
			typedef typename std::tuple_element<N, std::tuple<A...>>::type type;
		};
		
		template<int N, typename... A>
		struct valid_element<false, N, A...>
		{
			typedef void* type;
		};

		//evil tuple_element be like
		//gets the element in range of the A pack	
		template<int N, typename... A>
		struct element_tuple
		{
			//call element_tuple until it gets below the size of pack and set the type variable to its type
			typedef std::conditional_t<(sizeof...(A)>N), typename valid_element<(sizeof...(A)>N), N, A...>::type, void*> type;
		};
	};


	template<typename F, typename... A>
	class yandere_pool
	{
	private:
		using f_arg_type = std::conditional_t<std::is_member_function_pointer<F>::value,
		typename yan_templates::element_tuple<1, A...>::type,
		typename yan_templates::element_tuple<0, A...>::type>;

	public:
		yandere_pool() {};
		~yandere_pool() {exit_threads();};
		
		yandere_pool(int threads_num, F call_func, A... args);
		
		void run(f_arg_type arg);
		void run();
		
		void exit_threads();
		
	private:
		void work_func();

		mutable std::mutex _wait_mutex;
		std::condition_variable _conditional_var;

		F _call_func;

		using call_type = std::conditional_t<std::is_member_function_pointer<F>::value, typename yan_templates::element_tuple<0, A...>::type, void*>;
		call_type _class_ptr; 
		
		
		std::queue<f_arg_type> _args_queue;
		
		std::vector<std::thread> _threads_vec;

		int _threads_num = 0;
		
		bool _threads_running = true;
		bool _empty = true;
	};


	//you have to provide some random arguments of the correct type for it to set it up
	template<typename F, typename... A>
	yandere_pool<F, A...>::yandere_pool(int threads_num, F call_func, A... args) : _call_func(call_func), _threads_num(threads_num), _empty(false)
	{
		assert(threads_num>0);
		
		if constexpr(std::is_member_function_pointer<F>::value)
		{
			static_assert(sizeof...(args)>0);
			_class_ptr = std::get<0>(std::forward_as_tuple(args...));
		}
		
		_threads_vec.reserve(threads_num);
		for(int i = 0; i < threads_num; ++i)
		{
			_threads_vec.emplace_back(std::thread(&yandere_pool::work_func, this));
		}
	}


	template<typename F, typename... A>
	void yandere_pool<F, A...>::run(f_arg_type arg)
	{
		assert(!_empty);
		std::lock_guard<std::mutex> lock(_wait_mutex);
		
		_args_queue.push(arg);
		
		_conditional_var.notify_one();
	}

	template<typename F, typename... A>
	void yandere_pool<F, A...>::run()
	{
		assert(!_empty);
		std::lock_guard<std::mutex> lock(_wait_mutex);
		
		_args_queue.push(nullptr);
		
		_conditional_var.notify_one();
	}

	template<typename F, typename... A>
	void yandere_pool<F, A...>::exit_threads()
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

	template<typename F, typename... A>
	void yandere_pool<F, A...>::work_func()
	{
		while(true)
		{
			f_arg_type f_arg;
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
			
				if constexpr(std::is_same<f_arg_type, void*>::value)
				{
					//it has no arguments
					(_class_ptr->*_call_func)();
				} else
				{
					//it has arguments
					(_class_ptr->*_call_func)(std::move(f_arg));
				}
			} else
			{
				//non class member function
				
				if constexpr(std::is_same<f_arg_type, void*>::value)
				{
					//it has no arguments
					_call_func();
				} else
				{
					//it has arguments
					_call_func(std::move(f_arg));
				}
			}
		}
	}
};

#endif
