#include "../Source/PageCacheController.h"
#include "TestSet.h"
#include "TestHelper.h"

#include <atomic>
#include <future>

using namespace cache;

class TestCacheMTWhiteBox : public PageCacheController
{
public:
	

	TestCacheMTWhiteBox()
	{
		reset();

		setDebugTracePoint([this](DebugTracePoint tracePoint)
		{
			if (tracePoint == this->expectedTracePoint_)
			{
				this->isTracePointOccured_ = true;
			}

			if (tracePoint == this->holdTracePoint_)
			{
				isTracePointHold_ = true;
			}

			while (isTracePointHold_)
			{
				std::this_thread::yield();
			}
		}
		);
	}

	void reset()
	{
		lastAddressRead = 0; lastAddressWrite = 0;
		lastSizeRead = 0; lastSizeWrite = 0;
		countWrite = 0; countRead = 0;
		holdRead_ = false;  holdWrite_ = false;
		holdReadStart_ = false; holdWriteStart_ = false;
		outHoldRead_ = false; outHoldWrite_ = false;
		expectedTracePoint_ = TRACE_NEVER_CALLED; isTracePointOccured_ = false;
		exceptionReadStorage_ = false; exceptionWriteStorage_ = false;
		holdTracePoint_ = TRACE_NEVER_CALLED;
		isTracePointHold_ = false;
	}

	void setHoldRead(bool holdRead) { holdRead_= holdRead;  }
	void setHoldWrite(bool holdWrite) { holdWrite_ = holdWrite; }
	void waitHoldRead() { while (!holdReadStart_)   std::this_thread::yield(); }
	void waitHoldWrite() { while (!holdWriteStart_) std::this_thread::yield(); }
	void outHoldRead() { outHoldRead_ = true; }
	void outHoldWrite() { outHoldWrite_ = true;  }
	void setExpectedTracePoint(DebugTracePoint tracePoint) { isTracePointOccured_ = false; expectedTracePoint_ = tracePoint; }
	bool isTracePointOccured() { return isTracePointOccured_; }
	void waitExpectedTracePoint() {	while (!isTracePointOccured_) std::this_thread::yield(); }
	void setExceptionReadStorage(bool isSet) { exceptionReadStorage_ = isSet; }
	void setExceptionWriteStorage(bool isSet) { exceptionWriteStorage_ = isSet; }
	void setHoldOnTracePoint(DebugTracePoint tracePoint) { holdTracePoint_ = tracePoint; isTracePointHold_ = false; }
	void waitHoldTracePoint() {while (!isTracePointHold_)  std::this_thread::yield();}
	void cancelHoldTracePoint() {isTracePointHold_ = false; holdTracePoint_ = TRACE_NEVER_CALLED;}
		
	std::atomic_uint  countWrite;
	std::atomic_uint  countRead;
	std::atomic<DataAddress> lastAddressRead;
	std::atomic<DataAddress> lastAddressWrite;
	std::atomic<DataSize> lastSizeRead;
	std::atomic<DataSize> lastSizeWrite;

private:
	std::atomic_bool  holdRead_;
	std::atomic_bool  holdWrite_;
	std::atomic_bool  holdReadStart_;
	std::atomic_bool  holdWriteStart_;
	std::atomic_bool  outHoldRead_;
	std::atomic_bool  outHoldWrite_;
	std::atomic<DebugTracePoint> expectedTracePoint_;
	bool isTracePointOccured_;
	bool exceptionReadStorage_;
	bool exceptionWriteStorage_;
	DebugTracePoint holdTracePoint_;
	bool isTracePointHold_;
	
	void readStorage(DataAddress address, DataSize size, void* dataBuffer, void* metaData) override
	{
		lastAddressRead = address; lastSizeRead = size;
		countRead++;

		while (holdRead_)
		{
			holdReadStart_ = true;
			std::this_thread::yield();
			if (outHoldRead_)
			{
				outHoldRead_ = false;
				break;
			}
		}
		holdReadStart_ = false;

		if (exceptionReadStorage_)
		{
			throw std::exception();
		}
	}

	void writeStorage(DataAddress address, DataSize size, const void* dataBuffer, void* metaData) override
	{
		lastAddressWrite = address; lastSizeWrite = size;
		countWrite++;

		while (holdWrite_)
		{
			holdWriteStart_ = true;
			std::this_thread::yield();
			if (outHoldWrite_)
			{
				outHoldWrite_ = false;
				break;
			}
		}
		holdWriteStart_ = false;
		
		if (exceptionWriteStorage_)
		{
			throw std::exception();
		}
	}
};

void Verify(const char* testName, TestCacheMTWhiteBox& cache, bool condition)
{
	if (condition == false)
	{
		cache.reset();
		throw TestException(testName);
	}
}

void TestWhiteBoxMT()
{
	const char* testName = "TestWhiteBoxMT";

	printf("%s\n", testName);

	TestCacheMTWhiteBox cache;

	std::vector<std::pair<unsigned long, unsigned long>> readInfo;
	std::vector<std::pair<unsigned long, unsigned long>> sampleInfo;
	list_descriptor descriptors;

	auto write = [&cache](DataAddress address, DataSize size) 
	{
		std::unique_ptr<char> buffer(new char[size]);
		cache.write(address, size, buffer.get());
	};
	auto read = [&cache](DataAddress address, DataSize size) 
	{
		std::unique_ptr<char> buffer(new char[size]);
		cache.read(address, size, buffer.get());
	};

	//Simple scenario: single read page, one page replaces other
	cache.setupPages(1, 10);
	read(0, 9); //load page 0
	cache.setHoldRead(true);
	auto f = std::async(std::launch::async, write, 10, 5); //page 1 must replace page 0
	cache.waitHoldRead();

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	Verify(testName, cache, readInfo.size() == 1 && readInfo.front().second == STATE_LOAD);

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_PAGE);
	Verify(testName, cache, readInfo.size() == 1 && readInfo.front().second == 1);

	cache.setHoldRead(false);
	f.get();

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	Verify(testName, cache, readInfo.size() == 1 && readInfo.front().second == STATE_READY);
	Verify(testName, cache, cache.lastAddressRead == 10 && cache.lastSizeRead == 10);

	//2 pages replace, 2 pages wait
	cache.setupPages(2, 10);
	write(0, 10); //write page 0
	read(10, 10);  //read page 1
	cache.setHoldWrite(true);
	auto f1 = std::async(std::launch::async, write, 20, 10); //page 2: must unload and replace page 0
	cache.waitHoldWrite();

	descriptors.clear();
	descriptors.push_back({ STATE_UNLOAD, 0, 0, 0, 1 });
	descriptors.push_back({ STATE_READY, 1, INVALID_PAGE, 0, 0 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	cache.setHoldRead(true);
	auto f2 = std::async(std::launch::async, read, 30, 5); //page 3: must replace page 1
	cache.waitHoldRead();

	sampleInfo.clear();
	sampleInfo.push_back({ 0, 0 });
	sampleInfo.push_back({ 2, 0 });
	sampleInfo.push_back({ 3, 1 });
	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	Verify(testName, cache, sampleInfo == readInfo);

	cache.setExpectedTracePoint(TRACE_WAIT_LOAD);
	auto f3 = std::async(std::launch::async, read, 20, 5); // page 2: must wait unload and load
	cache.waitExpectedTracePoint();

	cache.setExpectedTracePoint(TRACE_WAIT_LOAD);
	auto f4 = std::async(std::launch::async, write, 30, 5); //page 3: must wait load
	cache.waitExpectedTracePoint();

	descriptors.clear();
	descriptors.push_back({ STATE_UNLOAD, 0, 0, 1, 1 });
	descriptors.push_back({ STATE_LOAD,  3, INVALID_PAGE, 1, 0 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHEK_STATE| CHECK_PAGE | CHECK_UNLOAD | CHECK_CHANGE));
		

	cache.setHoldRead(false);
	f2.wait(); f4.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_UNLOAD, 0, 0, 1, 1 });
	descriptors.push_back({ STATE_READY,  3, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	cache.setHoldRead(true);
	cache.setHoldWrite(false);
	cache.waitHoldRead();

	descriptors.clear();
	descriptors.push_back({ STATE_LOAD, 2, INVALID_PAGE, 1, 0 });
	descriptors.push_back({ STATE_READY,  3, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	cache.setHoldRead(false);
	f1.wait(); f3.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_READY,  2, INVALID_PAGE, 0, 1 });
	descriptors.push_back({ STATE_READY,  3, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	printf("Successfull\n");
}

void TestWhiteBoxExceptionMT()
{
	const char* testName = "TestWhiteBoxExceptionMT";

	printf("%s\n", testName);

	TestCacheMTWhiteBox cache;
	cache.setupPages(1, 10);

	std::vector<std::pair<unsigned long, unsigned long>> readInfo;
	std::vector<std::pair<unsigned long, unsigned long>> sampleInfo;
	list_descriptor descriptors;

	auto write = [&cache](DataAddress address, DataSize size)
	{
		std::unique_ptr<char> buffer(new char[size]);
		cache.write(address, size, buffer.get());
	};
	auto read = [&cache](DataAddress address, DataSize size)
	{
		std::unique_ptr<char> buffer(new char[size]);
		cache.read(address, size, buffer.get());
	};

	auto writeException = [&cache](DataAddress address, DataSize size)
	{
		std::unique_ptr<char> buffer(new char[size]);
		try
		{
			cache.write(address, size, buffer.get());
		}
		catch (const std::exception&)
		{

		}
		
	};
	auto readException = [&cache](DataAddress address, DataSize size)
	{
		std::unique_ptr<char> buffer(new char[size]);
		try
		{
			cache.read(address, size, buffer.get());
		}
		catch (const std::exception&)
		{

		}
	};

	cache.setExceptionReadStorage(true);
	cache.setExceptionWriteStorage(true);

	writeException(0, 5);

	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	Verify(testName, cache, readInfo.size() == 0);
	
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	Verify(testName, cache, readInfo.size() == 1 && readInfo[0].second == STATE_FREE);

	readException(0, 5);

	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	Verify(testName, cache, readInfo.size() == 0);
	
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	Verify(testName, cache, readInfo.size() == 1 && readInfo[0].second == STATE_FREE);
	
	cache.reset();
	cache.clear();
	write(0, 5);  //page 0
	cache.setExceptionWriteStorage(true);
	auto f = std::async(std::launch::async, write, 10, 5); //page 1 must replace page 0
	f.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_READY, 0, INVALID_PAGE, 0, 1});
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));
	
	cache.setExceptionWriteStorage(false);
	cache.setExceptionReadStorage(true);
	f = std::async(std::launch::async, write, 10, 5); //page 1 must replace page 0
	f.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_FREE, INVALID_PAGE, INVALID_PAGE, 0, 0 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	Verify(testName, cache, readInfo.size() == 0);

	//Check: 2 threads with access the same page
	cache.reset();
	write(0, 5);  //page 0
				  //check exception on write
	cache.setHoldWrite(true); 
	auto f1  = std::async(std::launch::async, write, 10, 5);
	cache.waitHoldWrite();
	cache.setExceptionWriteStorage(true);
	auto f2 = std::async(std::launch::async, write, 10, 5);

	cache.setHoldWrite(false);
	f1.wait(); f2.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_READY, 0, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	cache.reset();
	write(0, 5);  //page 0
				  //check exception on read
	cache.setHoldRead(true); 
	f1 = std::async(std::launch::async, write, 10, 5);
	cache.waitHoldRead();
	cache.setExceptionReadStorage(true);
	f2 = std::async(std::launch::async, write, 10, 5);
	cache.setHoldRead(false);
	f1.wait(); f2.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_FREE, INVALID_PAGE, INVALID_PAGE, 0, 0 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	//check no hang
	cache.setExceptionReadStorage(false); 
	read(10, 5);
	descriptors.clear();
	descriptors.push_back({ STATE_READY, 1, INVALID_PAGE, 0, 0 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	//Check: 2 threads for 2 pages
	
	//exception on write
	cache.reset();
	cache.setupPages(2, 10);
	write(0, 5);  //page 0
	write(10, 5); //page 1
	cache.setHoldWrite(true);
	cache.setExpectedTracePoint(TRACE_REPLACE);
	f1 = std::async(std::launch::async, write, 20, 5); //page 2: must replace page 0
	cache.waitExpectedTracePoint();
	cache.setExpectedTracePoint(TRACE_WAIT_UNLOAD);
	f2 = std::async(std::launch::async, write, 0, 5); //page 0: must wait unload
	cache.waitExpectedTracePoint();
	cache.setExceptionWriteStorage(true);
	cache.setHoldWrite(false);
	f1.wait(); f2.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_READY, 0, INVALID_PAGE, 0, 1 });
	descriptors.push_back({ STATE_READY, 1, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	//exception on read
	cache.reset();
	cache.setupPages(2, 10);
	write(0, 5);  //page 0
	write(10, 5); //page 1
	cache.setHoldRead(true);
	f1 = std::async(std::launch::async, write, 20, 5); //page 2: must replace page 0
	cache.waitHoldRead();
	cache.setExceptionReadStorage(true);
	cache.setExpectedTracePoint(TRACE_WAIT_LOAD);
	f2 = std::async(std::launch::async, write, 20, 5); //page 2: must wait load
	cache.waitExpectedTracePoint();
	
	descriptors.clear();
	descriptors.push_back({ STATE_LOAD, 2, INVALID_PAGE, 1, 0 });
	descriptors.push_back({ STATE_READY, 1, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	cache.setHoldRead(false);
	f1.wait(); 

	descriptors.clear();
	descriptors.push_back({ STATE_FREE, INVALID_PAGE, INVALID_PAGE, 0, 0 });
	descriptors.push_back({ STATE_READY, 1, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHEK_STATE | CHECK_PAGE | CHECK_UNLOAD));
	
	cache.setExceptionReadStorage(false);
	f2.wait();

	descriptors.clear();
	descriptors.push_back({ STATE_FREE, INVALID_PAGE, INVALID_PAGE, 0, 0 });
	descriptors.push_back({ STATE_READY, 1, INVALID_PAGE, 0, 1 });
	Verify(testName, cache, CheckDescriptor(descriptors, cache, CHECK_ALL));

	printf("Successfull\n");
}