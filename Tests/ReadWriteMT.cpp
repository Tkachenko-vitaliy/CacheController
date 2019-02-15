#include "PageCacheController.h"
#include "TestSet.h"
#include "TestControllerMT.h"

using namespace cache;

void ReadWriteMT(const RandomSetup& setup)
{
	printf("TestReadWriteMT: pCount=%u pSize=%u space=%u read=%u write=%u op=%u addr=%s ex=%u\n", setup.pageCount, setup.pageSize, setup.spaceSize, setup.countRead, setup.countWrite, setup.operationCount,  setup.fixedAddress ? "fix" : "rand", setup.intervalException);

	TestControllerMT cache;
	
	cache.run(setup);

	while (!cache.isFinished())
	{
		if (cache.waitFinish(5))
		{
			break;
		}

		if (!cache.checkWatchDog())
		{
			printf("ReadWriteMT: some thread hangs\n");
			cache.abort();
		}
	}

	cache.final();

	if (cache.isError())
	{
		throw TestException("ReadWriteMT");
	}
		
	cache.flush();
	if (!cache.checkStorage())
	{
		throw TestException("ReadWriteMT");
	}

	printf("Successfull\n");
}

void TestRW_1_1_Fixed()
{
	RandomSetup setup;

	setup.countRead = 1; setup.countWrite = 1; 
	setup.operationCount = 10;
	setup.fixedAddress = true;
	setup.intervalFlush = 2;

	const PageCount maxPageCount = 7;
	const PageSize maxPageSize = 7;
	const size_t maxSpace = 50;

	for (PageCount pageCount = 1; pageCount <= maxPageCount; pageCount++)
	{
		for (PageSize pageSize = 1; pageSize <= maxPageSize; pageSize++)
		{
			for (size_t space = 1; space <= maxSpace; space++)
			{
				setup.pageCount = pageCount;
				setup.pageSize = pageSize;
				setup.spaceSize = space;

				ReadWriteMT(setup);
			}
		}
	}
}

void TestRW_3_3_Fixed()
{
	RandomSetup setup;

	setup.countRead = 3; setup.countWrite = 3;
	setup.operationCount = 50;
	setup.fixedAddress = true;

	const PageCount maxPageCount = 7;
	const PageSize maxPageSize = 7;
	const size_t maxSpace = 50;

	for (PageCount pageCount = 1; pageCount <= maxPageCount; pageCount++)
	{
		for (PageSize pageSize = 1; pageSize <= maxPageSize; pageSize++)
		{
			for (size_t space = 1; space <= maxSpace; space++)
			{
				setup.pageCount = pageCount;
				setup.pageSize = pageSize;
				setup.spaceSize = space;

				ReadWriteMT(setup);
			}
		}
	}
}

void TestRW_3_3_Random_small()
{
	RandomSetup setup;

	setup.countRead = 3; setup.countWrite = 3;
	setup.operationCount = 50;
	setup.fixedAddress = false;
	setup.randomSeed = true;
	setup.pageCount = 5;
	setup.pageSize = 2;
	setup.spaceSize = 50;
	setup.operationCount = 50;
	setup.intervalFlush = 0;
	setup.intervalException = 0;

	ReadWriteMT(setup);
}

void TestRW_3_3_Random_small_exception()
{
	RandomSetup setup;

	setup.countRead = 3; setup.countWrite = 3;
	setup.operationCount = 50;
	setup.fixedAddress = false;
	setup.randomSeed = true;
	setup.pageCount = 5;
	setup.pageSize = 2;
	setup.spaceSize = 50;
	setup.operationCount = 50;
	setup.intervalFlush = 0;
	setup.intervalException = 5;

	ReadWriteMT(setup);
}

void TestRW_3_3_Random_big()
{
	RandomSetup setup;

	setup.countRead = 3; setup.countWrite = 3;
	setup.operationCount = 50;
	setup.fixedAddress = false;
	setup.randomSeed = true;
	setup.pageCount = 20;
	setup.pageSize = 5;
	setup.spaceSize = 1000;
	setup.operationCount = 1000;
	setup.intervalFlush = 100;
	setup.intervalException = 120;

	ReadWriteMT(setup);
}

void TestRW_3_3_Random_huge()
{
	RandomSetup setup;

	setup.countRead = 3; setup.countWrite = 3;
	setup.fixedAddress = false;
	setup.randomSeed = true;
	setup.pageCount = 20;
	setup.pageSize = 5;
	setup.spaceSize = 1000;
	setup.operationCount = 100000;
	setup.intervalFlush = 100;
	setup.intervalException = 150;
	
	ReadWriteMT(setup);
}

void TestReadWriteMT()
{
	TestRW_1_1_Fixed();
	TestRW_3_3_Fixed();
	TestRW_3_3_Random_small();
	TestRW_3_3_Random_small_exception();
	TestRW_3_3_Random_big();
	TestRW_3_3_Random_huge();
}