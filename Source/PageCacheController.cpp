#include "PageCacheController.h"
#include "CacheAlgorithm.h"
#include "CacheException.h"
#include "PageAddressIterator.h"
#include "PageSlot.h"
#include "PageLocator.h"

#include <assert.h>
#include <stdarg.h>

using namespace cache;

//////////////////////////////////////////////////////////////////////////////
//Helpers
//////////////////////////////////////////////////////////////////////////////

#define TRACE_POINT(what) \
if (callbackTracePoint_) \
{ \
	callbackTracePoint_(what); \
}

#define LOG(format, ...) \
if (callbackLog_) \
{\
	log(format, __VA_ARGS__); \
}

//////////////////////////////////////////////////////////////////////////////////////////
//Main class
//////////////////////////////////////////////////////////////////////////////////////////
PageCacheController::PageCacheController()
{
	pageReplaceAlgoritm.reset(CacheAlgorithm::create(ALG_LRU));
	pageLocator_.reset(new PageLocator);
	cacheBuffer_ = nullptr;
}

PageCacheController::~PageCacheController()
{
	delete cacheBuffer_;
}

void PageCacheController::setStartPageOffset(PageOffset offset) 
{
	startPageOffset_ = offset; 
}

void PageCacheController::enable(bool isEnable) 
{ 
	isEnabled_ = isEnable; 
}

void PageCacheController::setCleanBeforeLoad(bool isCleanBeforeLoad)
{ 
	isCleanBeforeLoad_ = isCleanBeforeLoad; 
}

void PageCacheController::setupPages(PageCount pageCount, PageSize pageSize)
{
	if (pageCount == 0 || pageSize == 0)
	{
		throw cache_exception(ERR_PAGE_COUNT_SIZE);
	}

	pageSlotTable_.clear();
	pageLocator_->clear();
	
	delete cacheBuffer_;
	cacheBuffer_ = new byte_t[pageCount * pageSize];

	if (cacheBuffer_ == nullptr)
	{
		throw cache_exception(ERR_ALLOCATE_BUFFER);
	}

	pageSize_ = pageSize;

	::memset(cacheBuffer_, 0, pageCount * pageSize);

	pageReplaceAlgoritm->setPageCount(pageCount);

	for (PageNumber pageNumber = 0; pageNumber <pageCount; pageNumber++)
	{
		pageSlotTable_.push_back(std::make_unique<PageSlot>());
	}
}

void PageCacheController::read(DataAddress address, DataSize size, void* readBuffer, void* metaData)
{
	if (!isEnabled_)
	{
		readStorage(address, size, readBuffer, metaData);
		return;
	}

	if (cacheBuffer_ == nullptr)
	{
		throw cache_exception(ERR_BUFFER_NOT_ALLOCATED);
	}

	PageAddressIterator pageIterator(pageSize_, startPageOffset_, address, size, readBuffer);

	while (pageIterator.isValid())
	{
		SlotIndex slotIndex = openPage(pageIterator.getPage(), PAGE_READ, metaData);

		if (slotIndex != INVALID_SLOT)
		{
			TRACE_POINT(TRACE_READ_PAGE);
			void* cacheData = calcSlotMemory(slotIndex, pageIterator.getPageOffset());
			::memcpy(pageIterator.getBuffer(), cacheData, pageIterator.getSize());
			closePage(slotIndex, PAGE_READ, metaData);
		}
		else
		{
			//No free pages
			readStorage(pageIterator.getAddress(), pageIterator.getSize(), pageIterator.getBuffer(), metaData);
		}

		pageIterator++;
	}
}

void PageCacheController::write(DataAddress address, DataSize size, const void* writeBuffer, void* metaData)
{
	if (!isEnabled_)
	{
		writeStorage(address, size, writeBuffer, metaData);
		return;
	}

	if (cacheBuffer_ == nullptr)
	{
		throw cache_exception(ERR_BUFFER_NOT_ALLOCATED);
	}

	PageAddressIterator pageIterator(pageSize_, startPageOffset_, address, size, const_cast<void*> (writeBuffer));

	while (pageIterator.isValid())
	{
		SlotIndex slotIndex = openPage(pageIterator.getPage(), PAGE_WRITE, metaData);

		if (slotIndex != INVALID_SLOT)
		{
			TRACE_POINT(TRACE_WRITE_PAGE);
			void* cacheData = calcSlotMemory(slotIndex, pageIterator.getPageOffset());
			::memcpy(cacheData, pageIterator.getBuffer(), pageIterator.getSize());
			closePage(slotIndex, PAGE_WRITE, metaData);

			if (writePolicy_ == WRITE_THROUGH)
			{
				writeStorage(pageIterator.getAddress(), pageIterator.getSize(), pageIterator.getBuffer(), metaData);
			}
		}
		else
		{
			//No free pages
			writeStorage(pageIterator.getAddress(), pageIterator.getSize(), pageIterator.getBuffer(), metaData);
		
		}

		pageIterator++;
	}
}

void PageCacheController::flush(void* metaData)
{
	for (SlotIndex index = 0; index < pageSlotTable_.size(); index++)
	{
		locker_t locker(synchronizer);

		if (pageSlotTable_[index]->canFlush())
		{
			flushPage(locker, index, metaData);
		}
	}
}

void PageCacheController::flush(DataAddress address, DataSize size, void* metaData)
{
	PageAddressIterator pageIterator(pageSize_, startPageOffset_, address, size);

	while (pageIterator.isValid())
	{
		locker_t locker(synchronizer);
		
		SlotIndex index = pageLocator_->get(pageIterator.getPage());
		if ( index != INVALID_SLOT)
		{
			if (pageSlotTable_[index]->canFlush())
			{
				flushPage(locker, index, metaData);
			}
		}
		pageIterator++;
	}
}

void PageCacheController::flushPage(locker_t& locker, SlotIndex slotIndex, void* metaData)
{
	PageSlot& descriptor = *pageSlotTable_[slotIndex];

	try
	{
		descriptor.isDirty = false;

		descriptor.addCapture(); TRACE_POINT(TRACE_ADD_CAPTURE);
		executeWrite(locker, calcPageAddress(descriptor.page), pageSize_, calcSlotMemory(slotIndex), metaData);
		descriptor.releaseCapture(); TRACE_POINT(TRACE_RELEASE_CAPTURE);

		pageReplaceAlgoritm->onPageOperation(slotIndex, PAGE_FLUSH);
	}
	catch (...)
	{
		descriptor.isDirty = true;
		descriptor.releaseCapture();
		std::rethrow_exception(std::current_exception());
	}
}

void PageCacheController::clear()
{
	if (cacheBuffer_ == nullptr)
	{
		throw cache_exception(ERR_BUFFER_NOT_ALLOCATED);
	}

	for (auto& descriptor : pageSlotTable_)
	{
		descriptor->reset();
	}

	::memset(cacheBuffer_, 0, pageSlotTable_.size() * pageSize_);

	pageReplaceAlgoritm->reset();

	pageLocator_->clear();
}


SlotIndex PageCacheController::openPage(PageNumber pageNumber, PageOperation pageOperation, void* metaData)
{
	locker_t locker(synchronizer);

	operationCount_++;

	SlotIndex searchIndex = pageLocator_->get(pageNumber);
	
	if (searchIndex == INVALID_SLOT)
	{
		searchIndex = miss(pageNumber, pageOperation, locker, metaData);
	}
	else
	{
		searchIndex = hit(searchIndex, pageNumber, pageOperation, locker, metaData);
	}

	return searchIndex;
}

void PageCacheController::closePage(SlotIndex slotIndex, PageOperation pageOperation, void* metaData)
{	
	locker_t locker(synchronizer);

	PageSlot& descriptor = *pageSlotTable_[slotIndex];

	if (writePolicy_ != WRITE_THROUGH && pageOperation == PAGE_WRITE)
	{
		descriptor.isDirty = true;
	}

	descriptor.releaseCapture(); TRACE_POINT(TRACE_RELEASE_CAPTURE);
}

SlotIndex PageCacheController::hit(SlotIndex slotIndex, PageNumber pageNumber, PageOperation pageOperation, locker_t& locker, void* metaData)
{
	TRACE_POINT(TRACE_HIT);

	hitCount_++;

	PageSlot& descriptor = *pageSlotTable_[slotIndex];

	if (descriptor.isPageUnload(pageNumber)) 
	{
		TRACE_POINT(TRACE_WAIT_UNLOAD);

		descriptor.waitUnload(locker);

		SlotIndex index = pageLocator_->get(pageNumber);

		if (index != INVALID_SLOT) //another thread could have already located this page
		{
			slotIndex = hit(index, pageNumber, pageOperation, locker, metaData);
			//We have to repeat a hit, because the page can be in waiting state
		}
		else
		{
			slotIndex = miss(pageNumber, pageOperation, locker, metaData);
		}
	}
	else
	{
		TRACE_POINT(TRACE_WAIT_LOAD);

		if (descriptor.isLoading())
		{
			descriptor.waitLoad(locker);
		}

		markCapture(slotIndex, pageOperation, metaData);
	}

	return slotIndex;
}


SlotIndex PageCacheController::miss(PageNumber pageNumber, PageOperation pageOperation, locker_t& locker, void* metaData)
{
	TRACE_POINT(TRACE_MISS);

	missCount_++;

	if (writeMissPolicy_ == WRITE_AROUND && pageOperation == PAGE_WRITE)
	{
		return INVALID_SLOT;
	}

	SlotIndex searchSlot = pageReplaceAlgoritm->getReplacePage();

	if (searchSlot != INVALID_SLOT)
	{
		PageSlot& descriptor = *pageSlotTable_[searchSlot];
		if (descriptor.isAvailable())
		{
			replacePage(searchSlot, pageNumber, pageOperation, locker, metaData);
			markCapture(searchSlot, pageOperation, metaData);
		}
		else
		{
			//It might occure that all pages are in replace state
			directCount_++;
			searchSlot = INVALID_SLOT;
		}
	}

	return searchSlot;
}

void PageCacheController::replacePage(SlotIndex slotIndex, PageNumber newPage, PageOperation pageOperation, locker_t& locker, void* metaData)
{
	TRACE_POINT(TRACE_REPLACE);

	PageSlot& descriptor = *pageSlotTable_[slotIndex];

	pageLocator_->set(newPage, slotIndex);

	pageReplaceAlgoritm->onPageOperation(slotIndex, PAGE_REPLACE);

	try
	{
		if (descriptor.state != PageSlot::STATE_FREE)
		{
			descriptor.unloadPage = descriptor.page;
			descriptor.state = PageSlot::STATE_UNLOAD;

			descriptor.waitCaptureFree(locker);

			unloadPage(slotIndex, pageOperation, locker, metaData);

		}

		descriptor.unloadPage = INVALID_PAGE;
		descriptor.state = PageSlot::STATE_LOAD;
		descriptor.page = newPage;

		loadPage(slotIndex, pageOperation, locker, metaData);
	}
	catch (...)
	{
		pageLocator_->set(newPage, INVALID_SLOT);
		descriptor.notifyException(std::current_exception());
		std::rethrow_exception(std::current_exception());
	}
}

void PageCacheController::unloadPage(SlotIndex slotIndex, PageOperation pageOperation, locker_t& locker, void* metaData)
{
	TRACE_POINT(TRACE_UNLOAD);

	PageSlot& descriptor = *pageSlotTable_[slotIndex];

	if (descriptor.isDirty)
	{
		try
		{
			executeWrite(locker, calcPageAddress(descriptor.unloadPage), pageSize_, calcSlotMemory(slotIndex), metaData);
		}
		catch (...)
		{
			descriptor.state = PageSlot::STATE_READY;
			descriptor.unloadPage = INVALID_PAGE;
			std::rethrow_exception(std::current_exception());
		}
		
		descriptor.isDirty = false;
	}

	pageLocator_->set(descriptor.unloadPage, INVALID_SLOT);

	descriptor.notifyUnload();
}

void PageCacheController::loadPage(SlotIndex slotIndex, PageOperation pageOperation, locker_t& locker, void* metaData)
{
	TRACE_POINT(TRACE_LOAD);

	PageSlot& descriptor = *pageSlotTable_[slotIndex];

	if (isCleanBeforeLoad_)
	{
		memset(calcSlotMemory(slotIndex), 0, pageSize_);
	}

	try
	{
		executeRead(locker, calcPageAddress(descriptor.page), pageSize_, calcSlotMemory(slotIndex), metaData);
	}
	catch (...)
	{
		descriptor.reset();
		pageReplaceAlgoritm->onPageOperation(slotIndex, PAGE_RESET);
		std::rethrow_exception(std::current_exception());
	}

	descriptor.state = PageSlot::STATE_READY;
	
	descriptor.notifyLoad();
}

void PageCacheController::markCapture(SlotIndex slotIndex, PageOperation pageOperation, void* metaData)
{
	pageSlotTable_[slotIndex]->addCapture(); TRACE_POINT(TRACE_ADD_CAPTURE);
	pageReplaceAlgoritm->onPageOperation(slotIndex, pageOperation);
}

void PageCacheController::executeWrite(locker_t& locker, DataAddress address, DataSize size, const void* dataBuffer, void* metaData)
{
	TRACE_POINT(TRACE_WRITE);
	locker.unlock();

	try
	{
		writeStorage(address, size, dataBuffer, metaData);
	}
	catch (...)
	{
		locker.lock();
		std::rethrow_exception(std::current_exception());
	}
	
	locker.lock();
}

void PageCacheController::executeRead(locker_t& locker, DataAddress address, DataSize size, void* dataBuffer, void* metaData)
{
	TRACE_POINT(TRACE_READ);
	locker.unlock();

	try
	{
		readStorage(address, size, dataBuffer, metaData);
	}
	catch (const std::exception&)
	{
		locker.lock();
		std::rethrow_exception(std::current_exception());
	}
	
	locker.lock();
}


PageCacheController::byte_t* PageCacheController::calcSlotMemory(SlotIndex slotIndex, PageOffset offset)
{
	return &cacheBuffer_[slotIndex * pageSize_ + offset];
}

DataAddress PageCacheController::calcPageAddress(PageNumber page)
{
	return page * pageSize_ + startPageOffset_;
}

void PageCacheController::setWritePolicy(WritePolicy policy)
{
	writePolicy_ = policy;
}

void PageCacheController::setWriteMissPolicy(WriteMissPolicy policy)
{
	writeMissPolicy_ = policy;
}

void PageCacheController::setReplaceAlgoritm(ReplaceAlgoritm algoritm)
{
	pageReplaceAlgoritm.reset(CacheAlgorithm::create(algoritm));
	pageReplaceAlgoritm->setPageCount(pageSlotTable_.size());
}

void PageCacheController::setAlgoritmParameter(const char* paramName, AlgoritmParameterValue paramValue)
{
	pageReplaceAlgoritm->setParameter(paramName, paramValue);
}

AlgoritmParameterValue PageCacheController::getAlgoritmParameter(const char* paramName) const
{
	return pageReplaceAlgoritm->getParameter(paramName);
}

void PageCacheController::setLocatorType(LocatorType type)
{
	pageLocator_->setType(type);
}

CacheStatistic PageCacheController::getStatistic() const
{
	std::lock_guard<std::mutex> lock(synchronizer);

	CacheStatistic statistic;

	statistic.operationCount = operationCount_;
	statistic.hitCount = hitCount_;
	statistic.missCount = missCount_;
	statistic.directCount = directCount_;
	statistic.locatorMemory = pageLocator_->get_memory_size();

	return statistic;
}

void PageCacheController::resetStatistic()
{
	std::lock_guard<std::mutex> lock(synchronizer);

	operationCount_ = 0;
	hitCount_ = 0;
	missCount_ = 0;
	directCount_ = 0;
}

CacheSettings PageCacheController::getSettings() const
{
	CacheSettings settings;

	settings.pageCount = pageSlotTable_.size();
	settings.pageSize = pageSize_;
	settings.writePolicy = writePolicy_;
	settings.writeMissPolicy = writeMissPolicy_;
	settings.replaceAlgoritm = pageReplaceAlgoritm->getType();
	settings.locatorType = pageLocator_->getType();
	settings.isEnabled = isEnabled_;
	settings.isCleanBeforeLoad = isCleanBeforeLoad_;

	return settings;
}

void PageCacheController::getDebugInfo(std::vector<std::pair<unsigned long, unsigned long>>& debugInfo, DebugInformation what) const
{
	debugInfo.clear();

	switch (what)
	{
	case DBINFO_LOCATION_TABLE:
	{
		for (auto item : *pageLocator_)
		{
			debugInfo.push_back(item);
		}
	}
	break;

	case DBINFO_DESCRIPTOR_PAGE:
	{
		for (size_t i = 0; i < pageSlotTable_.size(); i++)
		{
			debugInfo.push_back({i,pageSlotTable_[i]->page});
		}
	}
	break;

	case DBINFO_DESCRIPTOR_STATE:
	{
		for (size_t i = 0; i < pageSlotTable_.size(); i++)
		{
			debugInfo.push_back({ i,pageSlotTable_[i]->state });
		}
	}
	break;

	case DBINFO_DESCRIPTOR_CHANGE:
	{
		for (size_t i = 0; i < pageSlotTable_.size(); i++)
		{
			debugInfo.push_back({ i,pageSlotTable_[i]->isDirty ? 1 : 0 });
		}
	}
	break;

	case DBINFO_DESCRIPTOR_UNLOAD_PAGE:
	{
		for (size_t i = 0; i < pageSlotTable_.size(); i++)
		{
			debugInfo.push_back({ i,pageSlotTable_[i]->unloadPage });
		}
	}
	break;

	case DBINFO_DESCRIPTOR_WAITING_COUNT:
	{
		for (size_t i = 0; i < pageSlotTable_.size(); i++)
		{
			debugInfo.push_back({ i,pageSlotTable_[i]->getWaitingCount() });
		}
	}
	break;
	}
}

void PageCacheController::setDebugTracePoint(CallbackTracePoint tracePoint)
{
	callbackTracePoint_ = tracePoint;
}

void PageCacheController::setDebugCallbackLog(CallbackLog log)
{
	callbackLog_ = log;
}

void PageCacheController::setDebugLogEndline(bool isInsertEndline)
{
	isInsertEndline_ = isInsertEndline;
}

void PageCacheController::log(const char* strFormat, ...)
{
	//Don't call this method directly, use a macro LOG instead
	const size_t size_buffer = 255;
	char message[size_buffer];
	va_list args;
	va_start(args, strFormat);
	int length = vsnprintf(message, size_buffer, strFormat, args);
	va_end(args);

	if (isInsertEndline_)
	{
		message[length] = '\n';
		message[length + 1] = '\0';
	}
	callbackLog_(message);
}