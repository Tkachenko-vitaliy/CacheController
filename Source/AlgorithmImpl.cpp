#include "AlgorithmImpl.h"
#include "CacheException.h"

using namespace cache;

void CacheAlgorithmQueue::setPageCount(PageCount pageCount)
{
	pageQueue_.clear();
	pageLocator_.clear();

	for (PageNumber page = 0; page < pageCount; page++)
	{
		pageQueue_.push_back(page);
	}

	for (PageQueueLocator locator = pageQueue_.begin(); locator != pageQueue_.end(); locator++)
	{
		pageLocator_.push_back(locator);
	}
}

void CacheAlgorithmQueue::reset()
{
	setPageCount(pageQueue_.size());
}

PageNumber CacheAlgorithmQueue::getReplacePage()
{
	return pageQueue_.front();
}

void CacheAlgorithmQueue::getPageQueue(std::vector<PageNumber>& queue) const
{
	queue.clear();
	for (auto page  : pageQueue_)
	{
		queue.push_back(page);
	}
}

void caFIFO::onPageOperation(PageNumber page, PageOperation pageOperation)
{
	PageQueueLocator currentQueueItem = pageLocator_[page];

	switch (pageOperation)
	{
	case PAGE_REPLACE:
	{
		PageQueueLocator lastQueueItem = pageQueue_.end();
		pageQueue_.splice(lastQueueItem, pageQueue_, currentQueueItem);
	}
	break;
	case PAGE_RESET:
	{
		PageQueueLocator firstQueueItem = pageQueue_.begin();
		pageQueue_.splice(firstQueueItem, pageQueue_, currentQueueItem);
	}
	break;
	}
}

void caLRU::onPageOperation(PageNumber page, PageOperation pageOperation)
{
	PageQueueLocator currentQueueItem = pageLocator_[page];

	switch (pageOperation)
	{
	case PAGE_RESET:
	{
		PageQueueLocator firstQueueItem = pageQueue_.begin();
		pageQueue_.splice(firstQueueItem, pageQueue_, currentQueueItem);
	}
	break;
	case PAGE_READ:
	case PAGE_WRITE:
	case PAGE_REPLACE:
	{
		PageQueueLocator lastQueueItem = pageQueue_.end();
		pageQueue_.splice(lastQueueItem, pageQueue_, currentQueueItem);
	}
	break;
	}
}

void caLFU::onPageOperation(PageNumber page, PageOperation pageOperation)
{
	PageQueueLocator currentQueueItem = pageLocator_[page];

	switch (pageOperation)
	{
	case PAGE_RESET:
	{
		PageQueueLocator firstQueueItem = pageQueue_.begin();
		pageQueue_.splice(firstQueueItem, pageQueue_, currentQueueItem);
	}
	break;
	case PAGE_READ:
	case PAGE_WRITE:
	case PAGE_REPLACE:
	{
		PageQueueLocator nextQueueItem = currentQueueItem;
		nextQueueItem++;
		if (nextQueueItem != pageQueue_.end())
		{
			pageQueue_.splice(currentQueueItem, pageQueue_, nextQueueItem);
		}
		
	}
	break;
	}
}

void caMRU::onPageOperation(PageNumber page, PageOperation pageOperation)
{
	if (pageOperation != PAGE_FLUSH)
	{
		PageQueueLocator currentQueueItem = pageLocator_[page];
		PageQueueLocator firstQueueItem = pageQueue_.begin();
		pageQueue_.splice(firstQueueItem, pageQueue_, currentQueueItem);
	}
}

void caClock::setPageCount(PageCount pageCount)
{
	listPages_.clear();
	listPages_.resize(pageCount, false);
	currentPage_ = 0;
}

void caClock::onPageOperation(PageNumber page, PageOperation pageOperation)
{
	if (pageOperation != PAGE_FLUSH)
	{
		listPages_[page] = true;
	}
}

PageNumber caClock::getReplacePage()
{
	auto getNextPage = [this](PageNumber currentPage) -> PageNumber
	{
		currentPage++;
		if (currentPage == this->listPages_.size())
		{
			currentPage = 0;
		}
		return currentPage;
	};

	PageNumber savedPage = currentPage_;
	currentPage_ = getNextPage(currentPage_);
	if (listPages_[savedPage] == false)
	{
		return savedPage;
	}
	
	while (currentPage_ != savedPage && listPages_[currentPage_] == true)
	{
		listPages_[currentPage_] = false;
		currentPage_ = getNextPage(currentPage_);
	}

	return currentPage_;
}

void caClock::reset()
{
	setPageCount(listPages_.size());
}

caNRU::~caNRU()
{
	std::unique_lock<std::mutex> lock(mutexTimer_);
	isRun_ = false;
	cvWaitTimer_.notify_all();
	lock.unlock();
	timer_.join();
}

void caNRU::setPageCount(PageCount pageCount)
{
	std::lock_guard<std::mutex> lock(mutexTimer_);
	listPages_.clear();
	listPages_.resize(pageCount, 0);
}

void caNRU::onPageOperation(PageNumber page, PageOperation pageOperation)
{
	if (!timer_.joinable())
	{
		isRun_ = true;
		timer_ = std::thread(&caNRU::threadTimer, this);
	}

	switch (pageOperation)
	{
	case cache::PAGE_READ:
		listPages_[page].bitValue.referenced = 1;
		break;
	case cache::PAGE_WRITE:
		listPages_[page].bitValue.referenced = 1;
		listPages_[page].bitValue.modified = 1;
		break;
	case cache::PAGE_REPLACE:
		listPages_[page].bitValue.modified = 0;
		listPages_[page].bitValue.referenced = 1;
		break;
	case cache::PAGE_RESET:
		listPages_[page].bitValue.referenced = 0;
		listPages_[page].bitValue.modified = 0;
		break;
	case cache::PAGE_FLUSH:
		listPages_[page].bitValue.modified = 0;
		break;
	}
}

PageNumber caNRU::getReplacePage()
{
	unsigned char minValue = 0xFF;
	PageNumber retPage = listPages_.size();
	for (size_t i = 0; i < listPages_.size(); i++)
	{
		if (listPages_[i].value < minValue)
		{
			minValue = listPages_[i].value;
			retPage = i;
			if (minValue == 0)
			{
				break;
			}
		}
	}

	return retPage;
}

void caNRU::reset()
{
	for (auto& item : listPages_)
	{
		item.value = 0;
	}
}

void caNRU::setTimerInterval(unsigned long intervalMillisec)
{
	std::lock_guard<std::mutex> lock(mutexTimer_);
	timerInterval_ = intervalMillisec;
	cvWaitTimer_.notify_all();
}

void caNRU::threadTimer()
{
	while (isRun_)
	{
		std::unique_lock<std::mutex> lock(mutexTimer_);
		cvWaitTimer_.wait_for(lock, std::chrono::milliseconds(timerInterval_));
		for (auto& item : listPages_)
		{
			item.bitValue.referenced = 0;
		}
	}
}

void caNRU::setParameter(const char* paramName, AlgoritmParameterValue paramValue)
{
	if (::strcmp(paramName, "timeout") != 0)
	{
		throw cache_exception(ERR_PARAMETER_NAME);
	}

	if (paramValue <= 0)
	{
		throw cache_exception(ERR_PARAMETER_VALUE);
		setTimerInterval((unsigned long)paramValue);
	}
}

AlgoritmParameterValue caNRU::getParameter(const char* paramName) const
{
	if (::strcmp(paramName, "timeout") != 0)
	{
		throw cache_exception(ERR_PARAMETER_NAME);
	}

	return timerInterval_;
}

caRandom::caRandom(unsigned int seedValue)
{
	seed_ = seedValue;

	if (seed_ == 0)
	{
		std::random_device rd;
		seed_ = rd();
		generator_.seed(seed_);
	}
	else
	{
		generator_.seed(seed_);
	}

	pageCount_ = 0;
}

void caRandom::setSeed(unsigned int seedValue)
{
	seed_ = seedValue;

	if (seed_ == 0)
	{
		std::random_device rd;
		seed_ = rd();
		generator_.seed(seed_);
	}
	else
	{
		generator_.seed(seed_);
	}
}


void caRandom::setPageCount(PageCount pageCount)
{
	pageCount_ = pageCount;
}

PageNumber caRandom::getReplacePage()
{
	std::uniform_int_distribution<PageNumber> numbers(0, pageCount_ - 1);
	return numbers(generator_);
}

void caRandom::setParameter(const char* paramName, AlgoritmParameterValue paramValue)
{
	if (::strcmp(paramName, "seed") != 0)
	{
		throw cache_exception(ERR_PARAMETER_NAME);
	}

	setSeed((unsigned int)paramValue);
}

AlgoritmParameterValue caRandom::getParameter(const char* paramName) const
{
	if (::strcmp(paramName, "seed") != 0)
	{
		throw cache_exception(ERR_PARAMETER_NAME);
	}

	return seed_;
}

