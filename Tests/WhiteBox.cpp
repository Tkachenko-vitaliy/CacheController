#include "TestSet.h"
#include "TestHelper.h"
#include "PageCacheController.h"

using namespace cache;

class TestCacheWhiteBox : public PageCacheController
{
public:
	TestCacheWhiteBox()
	{
		reset();
	}
	unsigned long countWrite;
	unsigned long countRead;
	DataAddress lastAddressRead;
	DataAddress lastAddressWrite;
	DataSize lastSizeRead;
	DataSize lastSizeWrite;
	bool genReadException;
	bool genWriteException;
	
	void reset()
	{
		countWrite = 0; countRead = 0;
		lastAddressRead = 0; lastAddressWrite = 0;
		lastSizeRead = 0; lastSizeWrite = 0;
		genReadException = false; genWriteException = false; 
	}

protected:
	void readStorage(DataAddress address, DataSize size, void* dataBuffer, void* metaData) override
	{
		countRead++; lastAddressRead = address; lastSizeRead = size;
		if (genReadException)
			throw std::exception();
	}
	void writeStorage(DataAddress address, DataSize size, const void* dataBuffer, void* metaData) override
	{
		countWrite++; lastAddressWrite = address; lastSizeWrite = size;
		if (genWriteException)
			throw std::exception();
	}
};

void TestWhiteBox()
{
	printf("TestWhiteBox\n");

	TestCacheWhiteBox cache;

	const PageCount pageCount = 5;
	const PageSize pageSize = 20;
	const size_t bufferSize = 100;

	char writeBuffer[bufferSize];

	cache.setupPages(pageCount, pageSize);

	std::vector<std::pair<unsigned long, unsigned long>> readInfo;
	std::vector<std::pair<unsigned long, unsigned long>> sampleInfo;

	//Initial state
	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);

	if (readInfo.size() > 0)
		throw TestException("TestWhiteBox");

	list_descriptor descriptors;
	for (size_t i = 0; i < pageCount; i++)
		descriptors.push_back({ STATE_FREE,INVALID_PAGE,INVALID_PAGE,0,0 });

	if (!CheckDescriptor(descriptors, cache, CHECK_ALL))
		throw TestException("TestWhiteBox");

	sampleInfo.clear();
	for (size_t i = 0; i < pageCount; i++)
		sampleInfo.push_back({ i, 0 });

	//Allocate pages
	cache.write(100, 5, writeBuffer);	//Descriptor 0
	cache.write(200, 5, writeBuffer);	//Descriptor 1
	cache.write(300, 5, writeBuffer);	//Descriptor 2
	cache.write(400, 5, writeBuffer);	//Descriptor 3
	cache.write(500, 5, writeBuffer);	//Descriptor 4

	if (cache.countWrite)				//No phisycal write should be executed
		throw TestException("TestWhiteBox");

	//Location table
	sampleInfo.clear();
	sampleInfo.push_back({ 5 , 0 });
	sampleInfo.push_back({ 10 , 1 });
	sampleInfo.push_back({ 15 , 2 });
	sampleInfo.push_back({ 20 , 3 });
	sampleInfo.push_back({ 25 , 4 });
	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	cache.setLocatorType(LOCATOR_BIN_TREE);
	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	cache.setLocatorType(LOCATOR_HASH_MAP);
	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	//Descriptor page
	descriptors.clear();
	descriptors.push_back({STATE_READY,5,INVALID_PAGE,0,1});
	descriptors.push_back({ STATE_READY,10,INVALID_PAGE,0,1 });
	descriptors.push_back({ STATE_READY,15,INVALID_PAGE,0,1 });
	descriptors.push_back({ STATE_READY,20,INVALID_PAGE,0,1 });
	descriptors.push_back({ STATE_READY,25,INVALID_PAGE,0,1 });

	if (!CheckDescriptor(descriptors,cache, CHECK_ALL))
		throw TestException("TestWhiteBox");
	

	//Replace the page
	cache.reset();
	cache.write(1000, 5, writeBuffer);
	if (cache.countWrite != 1)
		throw TestException("TestWhiteBox");
	if (cache.countRead != 1)
		throw TestException("TestWhiteBox");
	if (cache.lastAddressWrite != 100)
		throw TestException("TestWhiteBox");
	if (cache.lastAddressRead != 1000)
		throw TestException("TestWhiteBox");

	sampleInfo.clear();
	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	if (readInfo.size() == 0)
		throw TestException("TestWhiteBox");
	bool bFound = false;
	for (auto item : readInfo)
	{
		if (item.first == 50 && item.second == 0)
		{
			bFound = true; break;
		}
	}
	if (!bFound)
		throw TestException("TestWhiteBox");

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_PAGE);
	if (readInfo.size() == 0)
		throw TestException("TestWhiteBox");
	auto item = readInfo.front();
	if (!(item.first == 0 && item.second == 50))
		throw TestException("TestWhiteBox");

	//Flush
	cache.clear();
	cache.reset();
	sampleInfo.clear();
	for (size_t i = 0; i < pageCount; i++)
		sampleInfo.push_back({ i, STATE_FREE });
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	cache.write(100, 5, writeBuffer);	//Descriptor 0
	cache.write(200, 5, writeBuffer);	//Descriptor 1
	sampleInfo.clear();
	sampleInfo.push_back({ 0, 1 });
	sampleInfo.push_back({ 1, 1 });
	sampleInfo.push_back({ 2, 0 });
	sampleInfo.push_back({ 3, 0 });
	sampleInfo.push_back({ 4, 0 });
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_CHANGE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	sampleInfo.at(0) = { 0,0 };
	sampleInfo.at(1) = { 1,0 };
	cache.flush();
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_CHANGE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	//flush range
	cache.clear();
	cache.reset();
	cache.setupPages(2, 10);
	cache.write(100, 5, writeBuffer);	//Descriptor 0
	cache.write(200, 5, writeBuffer);	//Descriptor 1
	sampleInfo.clear();
	sampleInfo.push_back({ 0, 1 });
	sampleInfo.push_back({ 1, 1 });
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_CHANGE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");
	cache.flush(201, 2);
	sampleInfo[1] = { 1,0 };
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_CHANGE);
	if (readInfo != sampleInfo)
		throw TestException("TestWhiteBox");

	printf("Successfull\n");
}

void TestWhiteboxException()
{
	printf("TestWhiteboxException\n");

	TestCacheWhiteBox cache;

	const PageCount pageCount = 2;
	const PageSize pageSize = 20;
	const size_t bufferSize = 20;

	char buffer[bufferSize];

	std::vector<std::pair<unsigned long, unsigned long>> readInfo;
	std::vector<std::pair<unsigned long, unsigned long>> sampleInfo;

	cache.setupPages(pageCount, pageSize);

	cache.read(0, 10, buffer);		//Descriptor 0
	cache.write(20, 10, buffer);	//Descriptor 1

	cache.genWriteException = true;
	try
	{
		cache.write(20, 10, buffer);
	}
	catch (const std::exception&)
	{
		cache.genWriteException = false;
	}

	//After exception: Descriptors must not be change
	sampleInfo.push_back({ 0, 0 });
	sampleInfo.push_back({ 1, 1 });

	cache.getDebugInfo(readInfo, DBINFO_LOCATION_TABLE);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_PAGE);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	sampleInfo.clear();
	sampleInfo.push_back({ 0, STATE_READY });
	sampleInfo.push_back({ 1, STATE_READY });

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	sampleInfo.clear();
	sampleInfo.push_back({ 0, 0 });
	sampleInfo.push_back({ 1, 1 });
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_CHANGE);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	sampleInfo.clear();
	sampleInfo.push_back({ 0, 0 });
	sampleInfo.push_back({ 1, 0 });
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_WAITING_COUNT);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	cache.genReadException = true;
	try
	{
		cache.write(100, 10, buffer);
	}
	catch (const std::exception&)
	{
		cache.genReadException = false;
	}

	//After exception: Descriptor 1 must be free
	sampleInfo.clear();
	sampleInfo.push_back({ 0, STATE_FREE });
	sampleInfo.push_back({ 1, STATE_READY });

	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	sampleInfo.clear();
	sampleInfo.push_back({ 0, 0 });
	sampleInfo.push_back({ 1, 0 });
	cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_WAITING_COUNT);
	if (sampleInfo != readInfo)
		throw TestException("TestWhiteboxException");

	printf("Successfull\n");
}