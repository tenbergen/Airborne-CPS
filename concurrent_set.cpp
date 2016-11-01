/*
* Implementation of a thread-safe set class in C++ using
*	mutex.
*
*/
#pragma once
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

template <typename T, typename Compare = std::less<T>>
class concurrent_set
{
private:
	set::set<T, Compare> set_;
	std::mutex mutex_;

public:
	typedef typename std::set<T, Compare>::iterator iterator;
	// etc.

	std::pair<iterator, bool>
		insert(const T& val) {
		std::unique_lock<std::mutex> lock(mutex_);
		return set_.insert(val);
	}

	size_type size() const {
		std::unique_lock<std::mutex> lock(mutex_);
		return set_.size();
	}
	// same idea with other functions
};