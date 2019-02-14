#pragma once

#include "CacheTypes.h"

#include <condition_variable>

namespace cache
{
	using namespace std;
	
	class PageSlot
	{
	public:
		PageSlot();

		enum State
		{
			STATE_FREE = 0,
			STATE_READY = 1,
			STATE_LOAD = 2,
			STATE_UNLOAD = 3
		};

		State state = STATE_FREE;
		PageNumber page = INVALID_PAGE;
		PageNumber unloadPage = INVALID_PAGE;
		bool isDirty = false;

		typedef std::unique_lock<std::mutex> locker_t;

		void reset();
		bool isAvailable() const;
		bool isPageUnload(PageNumber checkingPage);
		bool isLoading() const;
		bool canFlush() const;
		void addCapture();
		void releaseCapture();
		unsigned int getCaptureCount() const;
		unsigned int getWaitingCount() const;

		void waitCaptureFree(locker_t& locker);
		void waitUnload(locker_t& locker);
		void waitLoad(locker_t& locker);

		void notifyUnload();
		void notifyLoad();
		void notifyException(std::exception_ptr exception);

	private:
		unsigned int capturedNumber_;
		unsigned int waitingNumber_;
		std::condition_variable cvUnload_;
		std::condition_variable cvLoad_;
		std::condition_variable cvCapture_;
		std::exception_ptr exception_;
	};

}; //namespace cache
