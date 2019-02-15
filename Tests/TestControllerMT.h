#pragma once

#include "../Source/PageCacheController.h"
#include "SpinLock.h"
#include "TestLog.h"

#include <list>
#include <cassert>

namespace cache
{

	struct RandomSetup
	{
		unsigned int countRead = 0;
		unsigned int countWrite = 0;
		unsigned int spaceSize = 0;
		unsigned int operationCount = 0;
		unsigned int intervalFlush = 0;
		unsigned int intervalException = 0;
		PageCount pageCount = 0;
		PageSize  pageSize = 0;
		bool fixedAddress = false;
		bool randomSeed = true;
		bool bLog = false;
		unsigned int logRecordLimit = 0;
	};
	
	class TestControllerMT : public PageCacheController
	{
	public:
		void run(const RandomSetup& setup);
		bool isError();
		bool isFinished();
		bool waitFinish(unsigned int timeoutSec);
		bool checkWatchDog();
		bool checkStorage();
		void final();
		void abort();
	protected:
		void readStorage(DataAddress address, DataSize size, void* dataBuffer, void* metaData) override;
		void writeStorage(DataAddress address, DataSize size, const void* dataBuffer, void* metaData) override;
	private:
		std::vector<std::thread> listThread_;
		std::vector<unsigned char> storage_;
		std::vector<unsigned char> sampleData_;
		std::vector<bool> watchDog_;
		std::vector<bool> finishedThread_;
		size_t spaceSize_ = 0;
		size_t operationCount_ = 0;
		size_t intervalFlush_ = 0;
		size_t intervalException_ = 0;
		size_t triggerException_ = 0;
		bool fixedAddress_ = false;
		bool randomSeed_ = true;
		std::atomic<unsigned long> executedOperationCount_ = 0;
		size_t commonOperationCount_ = 0;
		std::atomic_bool stopError_ = false;
		std::atomic_bool run_ = false;
		std::atomic<size_t> threadCount_ = 0;
		std::atomic<size_t> threadReadyCount_ = 0;
		std::condition_variable waitFinish_;
		std::mutex conditionMutex_;
		spin_lock exceptionlock_;
		spin_lock finishLock_;
		bool exceptionRead_ = false;
		bool exceptionWrite_ = false;
		TestLog logFile_;
		bool isLogging_ = false;

		bool verifyData(DataAddress address, std::vector<unsigned char>& data);
		void onFinish(size_t threadIndex);
		void onOperation(size_t threadIndex);
		void setException();


		void threadWrite(size_t index);
		void threadRead(size_t index);
		void threadFlush(size_t index);
	};
};