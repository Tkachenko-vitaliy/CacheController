#include "CacheAlgorithm.h"
#include "AlgorithmImpl.h"
#include "TestSet.h"

#include <memory>
#include <vector>

using namespace cache;

void TestAlgoritm()
{
	printf("TestAlgoritm\n");

	std::unique_ptr<CacheAlgorithm> alg;
	std::vector<PageNumber> sampleInfo;
	std::vector<PageNumber> readInfo;

	alg.reset(CacheAlgorithm::create(ALG_FIFO));
	alg->setPageCount(5);
	alg->onPageOperation(0, PAGE_REPLACE);
	if (alg->getReplacePage() != 1)
		throw TestException("TestAlgoritm");
	sampleInfo = { 1,2,3,4,0 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(0, PAGE_RESET);
	if (alg->getReplacePage() != 0)
		throw TestException("TestAlgoritm");

	alg.reset(CacheAlgorithm::create(ALG_LRU));
	alg->setPageCount(5);
	alg->onPageOperation(0, PAGE_REPLACE);
	if (alg->getReplacePage() != 1)
		throw TestException("TestAlgoritm");
	sampleInfo = { 1,2,3,4,0 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(1, PAGE_READ);
	alg->onPageOperation(2, PAGE_WRITE);
	sampleInfo = { 3,4,0,1,2 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(4, PAGE_RESET);
	if (alg->getReplacePage() != 4)
		throw TestException("TestAlgoritm");

	alg.reset(CacheAlgorithm::create(ALG_LFU));
	alg->setPageCount(5);
	alg->onPageOperation(0, PAGE_READ);
	alg->onPageOperation(0, PAGE_WRITE);
	sampleInfo = { 1,2,0,3,4 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(4, PAGE_RESET);
	sampleInfo = { 4,1,2,0,3 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(3, PAGE_READ);
	sampleInfo = { 4,1,2,0,3 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");

	alg.reset(CacheAlgorithm::create(ALG_MRU));
	alg->setPageCount(5);
	alg->onPageOperation(0, PAGE_REPLACE);
	alg->onPageOperation(4, PAGE_READ);
	alg->onPageOperation(3, PAGE_WRITE);
	sampleInfo = { 3,4,0,1,2 }; ((CacheAlgorithmQueue*)alg.get())->getPageQueue(readInfo);
	if (readInfo != sampleInfo)
		throw TestException("TestAlgoritm");

	PageNumber page;

	alg.reset(CacheAlgorithm::create(ALG_CLOCK));
	alg->setPageCount(5);
	page = alg->getReplacePage();
	if (page != 0)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(1, PAGE_READ);
	alg->onPageOperation(2, PAGE_WRITE);
	page = alg->getReplacePage();
	if (page != 3)
		throw TestException("TestAlgoritm");
	page = alg->getReplacePage();
	alg->onPageOperation(4, PAGE_REPLACE);
	page = alg->getReplacePage();
	if (page != 0)
		throw TestException("TestAlgoritm");

	alg.reset(CacheAlgorithm::create(ALG_NRU));
	alg->setPageCount(4);
	((caNRU*)alg.get())->setTimerInterval(60000);
	page = alg->getReplacePage();
	if (page != 0)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(0, PAGE_WRITE);
	alg->onPageOperation(1, PAGE_READ);
	alg->onPageOperation(2, PAGE_WRITE);
	alg->onPageOperation(3, PAGE_WRITE);
	page = alg->getReplacePage();
	if (page != 1)
		throw TestException("TestAlgoritm");
	((caNRU*)alg.get())->setTimerInterval(60000);
	page = alg->getReplacePage();
	if (page != 1)
		throw TestException("TestAlgoritm");
	alg->onPageOperation(3, PAGE_FLUSH);
	page = alg->getReplacePage();
	if (page != 1)
		throw TestException("TestAlgoritm");

	alg.reset(CacheAlgorithm::create(ALG_RANDOM));
	alg->setPageCount(5);
	page = alg->getReplacePage();
	page = alg->getReplacePage();
	page = alg->getReplacePage();

	printf("Successfull\n");
}