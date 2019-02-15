#pragma once

#include <atomic>

class spin_lock
{
public:
	void lock()
	{
		while (spinlock_.test_and_set(std::memory_order_acquire));
	}
	void unlock()
	{
		spinlock_.clear(std::memory_order_release);
	}
private:
	std::atomic_flag spinlock_ = ATOMIC_FLAG_INIT;
};
