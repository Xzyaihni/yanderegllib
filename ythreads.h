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



class PassArg
{
public:
	virtual ~PassArg() = default;
};

template<typename T>
class ArgCore : public PassArg
{
public:
	ArgCore(T arg) : arg(arg) {};

	T arg;
};

class PoolBase
{
public:
	PoolBase() {};
	virtual ~PoolBase() = default;
};


template<typename F, typename... A>
class PoolCore : public PoolBase
{
private:
	//nice one liner x3
	using fArgType = std::conditional_t<std::is_member_function_pointer<F>::value, typename YanTemplates::element_tuple<1, A...>::type, typename YanTemplates::element_tuple<0, A...>::type>;

public:
	~PoolCore() {exit_threads();};
	
	PoolCore(int numThreads, F callFunc, A... args);
	
	void run(PassArg* arg);
	
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



class YanderePool;

class TypeHolder
{
public:
	virtual ~TypeHolder() = default;
	
	virtual void run(PassArg* arg) = 0;
	virtual void exit_threads() = 0;
};

template<class T>
class TypeTemplate : public TypeHolder
{
public:
	TypeTemplate(T* typePtr) : _typePtr(typePtr) {};

	void run(PassArg* arg) {_typePtr->run(arg);};
	void exit_threads() {_typePtr->exit_threads();};
	
private:
	T* _typePtr;
};

class YanderePool
{
public:
	YanderePool() {};
	
	template<typename F, typename... A>
	YanderePool(int numThreads, F callFunc, A... args);
	
	template<typename A>
	void run(A arg);
	
	void exit_threads();

private:
	std::unique_ptr<TypeHolder> _poolHolder;
	std::unique_ptr<PoolBase> _tPool;
};


template<typename F, typename... A>
YanderePool::YanderePool(int numThreads, F callFunc, A... args)
{
	_tPool = std::make_unique<PoolCore<F, A...>>(numThreads, callFunc, args...);
	_poolHolder = std::make_unique<TypeTemplate<PoolCore<F, A...>>>(dynamic_cast<PoolCore<F, A...>*>(_tPool.get()));
}

template<typename A>
void YanderePool::run(A arg)
{
	std::unique_ptr<PassArg> pArg = std::make_unique<ArgCore<A>>(arg);
	_poolHolder->run(pArg.get());
}

inline void YanderePool::exit_threads()
{
	_poolHolder->exit_threads();
}



//you have to provide some random arguments of the correct type for it to set it up
template<typename F, typename... A>
PoolCore<F, A...>::PoolCore(int numThreads, F callFunc, A... args) : _callFunc(callFunc), _threadNum(numThreads)
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
		_threadsVec.emplace_back(std::thread(&PoolCore::work_func, this));
	}
}


template<typename F, typename... A>
void PoolCore<F, A...>::run(PassArg* pArg)
{
	std::unique_lock<std::mutex> lock(_waitMutex);
	
	//static casting!! not type safe (but i think its faster than dynamic cast?)
	_argsQueue.emplace(std::move(static_cast<ArgCore<fArgType>*>(pArg)->arg));
	
	_condVar.notify_one();
}

template<typename F, typename... A>
void PoolCore<F, A...>::exit_threads()
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
void PoolCore<F, A...>::work_func()
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
