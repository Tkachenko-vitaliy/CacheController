#pragma once

#include <stdint.h>
#include <limits>
#include <functional>

namespace cache
{

	typedef uint64_t DataAddress;
	typedef uint32_t DataSize;
	typedef DataSize PageSize;
	typedef DataAddress PageOffset;
	typedef size_t PageCount;
	typedef PageCount PageNumber;

	enum WritePolicy
	{
		WRITE_BACK = 0,
		WRITE_THROUGH = 1
	};

	enum WriteMissPolicy
	{
		WRITE_ALLOCATE = 0,
		WRITE_AROUND = 1
	};

	enum ReplaceAlgoritm
	{
		ALG_LRU = 0,
		ALG_FIFO,
		ALG_LFU,
		ALG_MRU,
		ALG_CLOCK,
		ALG_NRU,
		ALG_RANDOM
	};

	typedef double AlgoritmParameterValue;
	
	enum LocatorType
	{
		LOCATOR_HASH_MAP = 0,
		LOCATOR_BIN_TREE = 1
	};

	struct CacheSettings
	{
		PageCount pageCount;
		PageSize  pageSize;
		WritePolicy writePolicy;
		WriteMissPolicy writeMissPolicy;
		ReplaceAlgoritm replaceAlgoritm;
		LocatorType     locatorType;
		bool isEnabled;
		bool isCleanBeforeLoad;
		size_t hashMemoryLimit;
	};


	struct CacheStatistic
	{
		unsigned long operationCount;
		unsigned long hitCount;
		unsigned long missCount;
		unsigned long directCount;
		unsigned long locatorMemory;
	};

	const PageNumber INVALID_PAGE = std::numeric_limits<PageNumber>::max();
	typedef size_t SlotIndex;
	const SlotIndex INVALID_SLOT = std::numeric_limits<SlotIndex>::max();

	enum PageOperation
	{
		PAGE_READ = 0,
		PAGE_WRITE = 1,
		PAGE_REPLACE = 2,
		PAGE_RESET = 3,
		PAGE_FLUSH = 4
	};
	
	enum DebugInformation
	{
		DBINFO_LOCATION_TABLE = 0,
		DBINFO_DESCRIPTOR_PAGE = 1,
		DBINFO_DESCRIPTOR_STATE = 2,
		DBINFO_DESCRIPTOR_CHANGE = 3,
		DBINFO_DESCRIPTOR_UNLOAD_PAGE = 4,
		DBINFO_DESCRIPTOR_WAITING_COUNT = 5,
	};

	enum DebugTracePoint
	{
		TRACE_NEVER_CALLED = 0,
		TRACE_HIT,
		TRACE_MISS,
		TRACE_REPLACE,
		TRACE_LOAD,
		TRACE_UNLOAD,
		TRACE_WAIT_LOAD,
		TRACE_WAIT_UNLOAD,
		TRACE_ADD_CAPTURE,
		TRACE_RELEASE_CAPTURE,
		TRACE_READ,
		TRACE_WRITE,
		TRACE_READ_PAGE,
		TRACE_WRITE_PAGE
	};

	typedef std::function<void(DebugTracePoint)> CallbackTracePoint;
	typedef std::function<void(const char*)> CallbackLog;

};  //namespace cache