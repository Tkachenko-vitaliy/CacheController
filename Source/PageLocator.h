#pragma once

#include "CacheTypes.h"
#include <vector>
#include <map>

namespace cache
{
	class PageLocator
	{
	public:
		void setType(LocatorType locatorType);
		LocatorType getType() const;
		SlotIndex get(PageNumber page) const;
		void set(PageNumber page, SlotIndex descriptor);
		void clear();
		size_t getMemorySize() const;
		void   setHashMemoryLimit(size_t memoryLimit);
		size_t getHashMemoryLimit() const;

		class iterator : public std::iterator<std::bidirectional_iterator_tag, iterator>
		{
		public:
			iterator& operator++ ();
			iterator  operator++ (int);
			iterator& operator-- ();
			iterator operator-- (int);
			bool operator == (iterator const& other) const;
			bool operator != (iterator const& other) const;
			std::pair<PageNumber, SlotIndex>  operator*() const;

		private:
			friend class PageLocator;
			PageLocator* locator_;
			iterator(PageLocator* locator);

			std::vector<SlotIndex>::iterator hashIterator_ ;
			std::map<PageNumber, SlotIndex>::iterator treeIterator_;
		};

		iterator begin();
		iterator end();

	private:
		std::vector<SlotIndex> hash_;
		std::map<PageNumber, SlotIndex> tree_;
		LocatorType type_ = LOCATOR_HASH_MAP;
		size_t hashLimit_ = 0;
	};

}; //namespace cache


