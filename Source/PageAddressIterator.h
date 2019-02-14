#pragma once

#include "CacheTypes.h"

namespace cache
{
	class PageAddressIterator
	{
	public:
		PageAddressIterator(PageSize pageSize, PageOffset startPageOffset, DataAddress dataAddress, DataSize dataSize, void* inputBuffer = nullptr);
		void operator++ (int);
		bool isValid();

		PageNumber  getPage() const;
		DataAddress getAddress() const;
		PageOffset  getPageOffset() const;
		DataSize    getSize() const;
		void* getBuffer() const;

	private:
		PageSize pageSize_;
		PageOffset pageOffset_;

		DataAddress currentDataAddress_;
		DataSize currentDataSize_;
		void* currentInputBuffer_;
		PageNumber currentPage_;
		DataSize restDataSize_;
	};

};  //namespace cache