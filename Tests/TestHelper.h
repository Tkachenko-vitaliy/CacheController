#pragma once

#include <vector>

namespace cache
{

	class PageCacheController;

	enum
	{
		STATE_FREE = 0,
		STATE_READY = 1,
		STATE_LOAD = 2,
		STATE_UNLOAD = 3
	}; //The same as it is in PageSlot

	struct TestPageDescriptor
	{
		TestPageDescriptor(int state, unsigned long page, unsigned long unloadPage, unsigned int waitingCount, unsigned int  isDirty)
		{
			this->state = state; this->page = page; this->unloadPage = unloadPage; this->waitingCount = waitingCount; this->isDirty = isDirty;
		}
		unsigned int state;
		unsigned long page;
		unsigned long unloadPage = 0;
		unsigned int waitingCount = 0;
		unsigned int isDirty;
	};

	enum
	{
		CHEK_STATE = 0x01, CHECK_PAGE = 0x02, CHECK_UNLOAD = 0x04, CHECK_WAIT_COUNT = 0x08, CHECK_CHANGE = 0x10, CHECK_ALL = 0xFF
	};

	typedef std::vector<TestPageDescriptor> list_descriptor;

	bool CheckDescriptor(list_descriptor& descriptors, PageCacheController& cache, unsigned int what);

};