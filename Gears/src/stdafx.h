// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#if defined(_WIN32)
	#include <windows.h>
#elif __linux__
	#define WM_USER                         0x0400
#endif

// reference additional headers your program requires here
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

using uint = unsigned int;

#define GEARS_SHARED_CREATE( T )	\
	template<typename... Args>	\
	inline static boost::shared_ptr<T> create(Args... args)	\
	{	\
	return boost::shared_ptr<T>(new T(args...));	\
	}	\
	using P =  boost::shared_ptr<T>;	\
	using CP =  boost::shared_ptr<T const>;	\
	using W =  boost::weak_ptr<T>

#define GEARS_SHARED_CREATE_WITH_GETSHAREDPTR_SUB( T )	\
	private: \
	boost::weak_ptr<T> weakPtrForGetSharedPtr; \
	protected: \
	void setWeakPtrForGetSharedPtr(boost::weak_ptr<T> w) {weakPtrForGetSharedPtr = w; __super::setWeakPtrForGetSharedPtr(w);} \
	public: \
	template<typename... Args>	\
	inline static boost::shared_ptr<T> create(Args... args)	\
	{	\
		boost::shared_ptr<T> p(new T(args...)); \
		p->setWeakPtrForGetSharedPtr(p); \
		return p;	\
	}	\
	inline boost::shared_ptr<T> getSharedPtr(){ return weakPtrForGetSharedPtr.lock();} \
	using P = boost::shared_ptr<T>;	\
	using CP =  boost::shared_ptr<T const>;	\
	using W = boost::weak_ptr<T>

#define GEARS_SHARED_CREATE_WITH_GETSHAREDPTR( T )	\
	private: \
	boost::weak_ptr<T> weakPtrForGetSharedPtr; \
	protected:	\
	void setWeakPtrForGetSharedPtr(boost::weak_ptr<T> w) {weakPtrForGetSharedPtr = w;} \
	public: \
	template<typename... Args>	\
	inline static boost::shared_ptr<T> create(Args... args)	\
	{	\
		boost::shared_ptr<T> p(new T(args...)); \
		p->setWeakPtrForGetSharedPtr(p); \
		return p;	\
	}	\
	inline boost::shared_ptr<T> getSharedPtr(){ return weakPtrForGetSharedPtr.lock();} \
	using P = boost::shared_ptr<T>;	\
	using CP =  boost::shared_ptr<T const>;	\
	using W = boost::weak_ptr<T>

#ifdef __linux__
#include <ctime>
#include <cerrno>
	void Sleep(uint msec);
#endif