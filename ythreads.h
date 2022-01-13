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

//the templates stuff is the main body so i cant separate it into a different file

namespace YanTemplates
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


template <typename F, typename... A>
class YanderePool
{
private:
	//nice one liner x3
	using fArgType = std::conditional_t<std::is_member_function_pointer<F>::value, typename YanTemplates::element_tuple<1, A...>::type, typename YanTemplates::element_tuple<0, A...>::type>;

public:
	~YanderePool() {exit_threads();};
	
	YanderePool(int numThreads, F callFunc, A... args);
	
	void run(fArgType arg);
	
	void exit_threads();

private:
	void work_func();

	std::mutex _waitMutex;
	std::condition_variable _condVar;

	F _callFunc;

	using callType = std::conditional_t<std::is_member_function_pointer<F>::value, typename YanTemplates::element_tuple<0, A...>::type, void*>;
	callType _classPtr; 
	
	
	std::queue<fArgType> _argsQueue;
	
	std::vector<std::thread> _threadsVec;

	int _threadNum = 0;
	
	bool _threadsRunning = true;
};



//you have to provide some random arguments of the correct type for it to set it up
template<typename F, typename... A>
YanderePool<F, A...>::YanderePool(int numThreads, F callFunc, A... args) : _callFunc(callFunc), _threadNum(numThreads)
{
	assert(_threadNum>0);
	
	if constexpr(std::is_member_function_pointer<F>::value)
	{
		static_assert(sizeof...(args)>0);
		_classPtr = std::get<0>(std::forward_as_tuple(args...));
	}
	
	_threadsVec.reserve(numThreads);
	for(int i = 0; i < numThreads; ++i)
	{
		_threadsVec.emplace_back(std::thread(&YanderePool::work_func, this));
	}
}


template<typename F, typename... A>
void YanderePool<F, A...>::run(fArgType arg)
{
	std::unique_lock<std::mutex> lock(_waitMutex);
	
	_argsQueue.emplace(std::move(arg));
	
	_condVar.notify_one();
}

template<typename F, typename... A>
void YanderePool<F, A...>::exit_threads()
{
	if(!_threadsRunning)
		return;

	{
		std::unique_lock<std::mutex> lock(_waitMutex);
	
		_threadsRunning = false;
	}
	_condVar.notify_all();
	
	for(auto& cThread : _threadsVec)
	{
		cThread.join();
	}
}

template<typename F, typename... A>
void YanderePool<F, A...>::work_func()
{
	while(true)
	{
		fArgType fArg;
		{
			std::unique_lock<std::mutex> lock(_waitMutex);
			
			_condVar.wait(lock, [this]{return !_argsQueue.empty() || !_threadsRunning;});
			
			if(!_threadsRunning)
				return;
			
			fArg = _argsQueue.front();
			_argsQueue.pop();
		}
		
		if constexpr(std::is_member_function_pointer<F>::value)
		{
			//class member function
		
			if constexpr(std::is_same<fArgType, void*>::value)
			{
				//it has no arguments
				(_classPtr->*_callFunc)();
			} else
			{
				//it has arguments
				(_classPtr->*_callFunc)(fArg);
			}
		} else
		{
			//non class member function
			
			if constexpr(std::is_same<fArgType, void*>::value)
			{
				//it has no arguments
				_callFunc();
			} else
			{
				//it has arguments
				_callFunc(fArg);
			}
		}
	}
}

#endif
