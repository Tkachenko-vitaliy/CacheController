#pragma once

#include "CacheTypes.h"

#include <vector>
#include <limits>
#include <mutex>

namespace cache
{
	using namespace std;

	class CacheAlgorithm;
	class PageSlot;
	class PageLocator;

	class PageCacheController
	{
	public:
		PageCacheController();
		virtual ~PageCacheController();

		void setupPages(PageCount pageCount, PageSize pageSize);
		void setStartPageOffset(PageOffset offset);

		void read(DataAddress address, DataSize size, void* readBuffer, void* metaData = nullptr);
		void write(DataAddress address, DataSize size, const void* writeBuffer, void* metaData = nullptr);
		void flush(void* metaData = nullptr);
		void flush(DataAddress address, DataSize size, void* metaData = nullptr);
		void clear();

		void enable(bool isEnable);
		void setCleanBeforeLoad(bool isCleanBeforeLoad);

		void setReplaceAlgoritm(ReplaceAlgoritm algoritm);
		void setAlgoritmParameter(const char* paramName, AlgoritmParameterValue paramValue);
		AlgoritmParameterValue getAlgoritmParameter(const char* paramName) const;

		void setLocatorType(LocatorType type);

		void setWritePolicy(WritePolicy policy);
		void setWriteMissPolicy(WriteMissPolicy policy);

		CacheStatistic getStatistic() const;
		void resetStatistic();

		CacheSettings getSettings() const;

		void getDebugInfo(std::vector<std::pair<unsigned long, unsigned long>>& debugInfo, DebugInformation what) const;
		void setDebugTracePoint(CallbackTracePoint tracePoint);
		void setDebugCallbackLog(CallbackLog log);
		void setDebugLogEndline(bool isInsertEndline);

	protected:
		virtual void readStorage(DataAddress address, DataSize size, void* dataBuffer, void* metaData) {}
		virtual void writeStorage(DataAddress address, DataSize size, const void* dataBuffer, void* metaData) {}

	private:
		typedef unsigned char byte_t;

		PageSize pageSize_ = 0;
		PageOffset startPageOffset_ = 0;
		byte_t* cacheBuffer_ = nullptr;
		bool isEnabled_ = true;
		bool isCleanBeforeLoad_ = false;
		bool isInsertEndline_ = false;
		WritePolicy writePolicy_ = WRITE_BACK;
		WriteMissPolicy writeMissPolicy_ = WRITE_ALLOCATE;

		std::vector<std::unique_ptr<PageSlot>> pageSlotTable_;
		std::unique_ptr<PageLocator> pageLocator_;
		std::unique_ptr<CacheAlgorithm> pageReplaceAlgoritm;

		mutable std::mutex  synchronizer;
		typedef std::unique_lock<std::mutex> locker_t;

		CallbackTracePoint callbackTracePoint_;
		CallbackLog callbackLog_;

		unsigned long operationCount_ = 0;
		unsigned long hitCount_ = 0;
		unsigned long missCount_ = 0;
		unsigned long directCount_ = 0;

		SlotIndex openPage(PageNumber pageNumber, PageOperation pageOperation, void* metaData);
		void closePage(SlotIndex slotIndex, PageOperation pageOperation, void* metaData); //metaData
		SlotIndex hit(SlotIndex slotIndex, PageNumber pageNumber, PageOperation pageOperation, locker_t& locker, void* metaData);
		SlotIndex miss(PageNumber pageNumber, PageOperation pageOperation, locker_t& locker, void* metaData);
		void markCapture(SlotIndex slotIndex, PageOperation pageOperation, void* metaData); //metaData
		void replacePage(SlotIndex slotIndex, PageNumber newPage, PageOperation pageOperation, locker_t& locker, void* metaData); //pageOperation
		void unloadPage(SlotIndex slotIndex, PageOperation pageOperation, locker_t& locker, void* metaData); //pageOperation
		void loadPage(SlotIndex slotIndex, PageOperation pageOperation, locker_t& locker, void* metaData); //pageOperation
		void executeWrite(locker_t& locker, DataAddress address, DataSize size, const void* dataBuffer, void* metaData);
		void executeRead(locker_t& locker, DataAddress address, DataSize size, void* dataBuffer, void* metaData);
		void flushPage(locker_t& locker, SlotIndex slotIndex, void* metaData);
		byte_t* calcSlotMemory(SlotIndex slotIndex, PageOffset offset = 0);
		DataAddress calcPageAddress(PageNumber page);

		void log(const char* strFormat, ...);
	};

}; //namespace cache