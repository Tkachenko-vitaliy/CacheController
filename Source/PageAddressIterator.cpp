#include "PageAddressIterator.h"
#include "CacheException.h"

using namespace cache;

PageAddressIterator::PageAddressIterator(PageSize pageSize, PageOffset startPageOffset, DataAddress dataAddress, DataSize dataSize, void* inputBuffer)
{
	if (dataAddress < startPageOffset)
	{
		throw cache_exception(ERR_ADDRESS_OFFSET);
	}

	pageSize_ = pageSize;
	currentPage_ = (PageNumber) ((dataAddress - startPageOffset) / pageSize);
	pageOffset_ = dataAddress - (currentPage_ * pageSize_) - startPageOffset;
	currentDataAddress_ = dataAddress;
	currentInputBuffer_ = inputBuffer;

	DataSize restToNextPage = (DataSize) ( ((DataAddress)(currentPage_ + 1)) * pageSize - dataAddress + startPageOffset );

	if (restToNextPage >= dataSize)
	{
		currentDataSize_ = dataSize;
		restDataSize_ = 0;
	}
	else
	{
		currentDataSize_ = restToNextPage;
		restDataSize_ = dataSize - restToNextPage;
	}
}

void PageAddressIterator::operator++(int)
{
	if (restDataSize_ == 0)
	{
		currentDataSize_ = 0;
		return;
	}

	currentPage_++;

	if (currentPage_ == INVALID_PAGE)
	{
		throw cache_exception(ERR_PAGE_OVERLOADED);
	}

	currentDataAddress_ = currentDataAddress_ - pageOffset_ + pageSize_;

	pageOffset_ = 0;

	if (currentInputBuffer_) //input buffer can be not set, we iterate only through page address space
	{
		currentInputBuffer_ = &((char*)currentInputBuffer_)[currentDataSize_];
	}

	if (restDataSize_ > pageSize_)
	{
		currentDataSize_ = pageSize_;
		restDataSize_ -= pageSize_;
	}
	else
	{
		currentDataSize_ = restDataSize_;
		restDataSize_ = 0;
	}
}

bool PageAddressIterator::isValid()
{
	return restDataSize_ > 0 || currentDataSize_ > 0;
}

PageNumber  PageAddressIterator::getPage() const
{ 
	return currentPage_; 
}
DataAddress PageAddressIterator::getAddress() const
{ 
	return currentDataAddress_; 
}

PageOffset PageAddressIterator::getPageOffset() const
{ 
	return pageOffset_; 
}

DataSize PageAddressIterator::getSize() const
{ 
	return currentDataSize_; 
}

void* PageAddressIterator::getBuffer() const
{ 
	return currentInputBuffer_; 
}