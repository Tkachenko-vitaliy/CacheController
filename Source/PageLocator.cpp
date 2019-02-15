#include "PageLocator.h"
#include "CacheException.h"

using namespace cache;

void PageLocator::setType(LocatorType locatorType)
{
	if (type_ == locatorType)
	{
		return;
	}

	type_ = locatorType;

	switch (type_)
	{
	case cache::LOCATOR_HASH_MAP:
	{
		for (auto item : tree_)
		{
			if (item.first >= hash_.size())
			{
				hash_.resize(item.first + 1, INVALID_SLOT);
				hash_.back() = item.second;
			}
		}
		tree_.clear();
	}
	break;
	case cache::LOCATOR_BIN_TREE:
	{
		for (PageNumber page = 0; page < hash_.size(); page++)
		{
			if (hash_[page] != INVALID_SLOT)
				tree_.insert({ page, hash_[page] });
		}
		hash_.clear();
	}
	break;
	}
}

LocatorType PageLocator::getType() const
{
	return type_;
}

SlotIndex PageLocator::get(PageNumber page) const
{
	SlotIndex index = INVALID_SLOT;

	switch (type_)
	{
	case LOCATOR_HASH_MAP:
	{
		if (page < hash_.size())
		{
			index = hash_[page];
		}
	}
	break;
	case LOCATOR_BIN_TREE:
	{
		auto it = tree_.find(page);
		if (it != tree_.end())
		{
			index = it->second;
		}
	}
	break;
	}

	return index;
}

void PageLocator::set(PageNumber page, SlotIndex descriptor)
{
	switch (type_)
	{
	case LOCATOR_HASH_MAP:
	{
		if (page >= hash_.size())
		{
			if (hashLimit_ > 0) 
			{
				if ((page + 1) * sizeof(SlotIndex) > hashLimit_)
				{
					throw cache_exception(ERR_HASH_LIMIT);
				}
			}
			hash_.resize(page + 1, INVALID_SLOT);
		}
		hash_[page] = descriptor;
	}
	break;
	case LOCATOR_BIN_TREE:
	{
		if (descriptor == INVALID_SLOT)
		{
			tree_.erase(page);
		}
		else
		{
			tree_[page] = descriptor;
		}
		
	}
	break;
	}
}

void PageLocator::clear()
{
	hash_.clear();
	tree_.clear();
}

size_t PageLocator::getMemorySize() const
{
	size_t memSize = 0;

	switch (type_)
	{
	case cache::LOCATOR_HASH_MAP:
	{
		memSize = hash_.size() * sizeof(SlotIndex);
	}
	break;
	case cache::LOCATOR_BIN_TREE:
	{
		//We guess (but don't know exact because it depends on tree implementation) tree node has:
		//3 pointers (chiled left, child right, parent);
		//1 byte of color;
		//key and value
		memSize = tree_.size()*(sizeof(void*) * 3 + sizeof(unsigned char) + sizeof(PageNumber) + sizeof(SlotIndex));
	}
	break;
	}

	return memSize;
}

void PageLocator::setHashMemoryLimit(size_t memoryLimit)
{
	hashLimit_ = memoryLimit;
}

size_t PageLocator::getHashMemoryLimit() const
{
	return hashLimit_;
}

PageLocator::iterator PageLocator::begin()
{
	PageLocator::iterator iter(this);

	iter.hashIterator_ = hash_.begin();
	iter.treeIterator_ = tree_.begin();

	if (type_ == LOCATOR_HASH_MAP)
	{
		while (iter.hashIterator_ != hash_.end() && *iter.hashIterator_ == INVALID_SLOT)
		{
			iter.hashIterator_++;
		}
	}

	return iter;
}

PageLocator::iterator PageLocator::end()
{
	PageLocator::iterator iter(this);

	iter.hashIterator_ = hash_.end();
	iter.treeIterator_ = tree_.end();

	return iter;
}

PageLocator::iterator::iterator(PageLocator* locator) :locator_(locator)
{

}

PageLocator::iterator& PageLocator::iterator::operator++()
{
	switch (locator_->type_)
	{
	case LOCATOR_HASH_MAP:
	{
		hashIterator_++;
		while (hashIterator_ != locator_->hash_.end() && *hashIterator_ == INVALID_SLOT)
		{
			hashIterator_++;
		}
	}
	break;
	case LOCATOR_BIN_TREE:
	{
		treeIterator_++;
	}
	break;
	}

	return *this;
}

PageLocator::iterator PageLocator::iterator::operator++ (int)
{
	switch (locator_->type_)
	{
	case LOCATOR_HASH_MAP:
	{
		hashIterator_++;
		while (hashIterator_ != locator_->hash_.end() && *hashIterator_ == INVALID_SLOT)
		{
			hashIterator_++;
		}
	}
	break;
	case LOCATOR_BIN_TREE:
	{
		treeIterator_++;
	}
	break;
	}

	return *this;
}

PageLocator::iterator& PageLocator::iterator::operator--()
{
	switch (locator_->type_)
	{
	case LOCATOR_HASH_MAP:
	{
		hashIterator_--;
		while (hashIterator_ != locator_->hash_.begin() && *hashIterator_ == INVALID_SLOT)
		{
			hashIterator_--;
		}
	}
	break;
	case LOCATOR_BIN_TREE:
	{
		treeIterator_--;
	}
	break;
	}

	return *this;
}

PageLocator::iterator PageLocator::iterator::operator-- (int)
{
	switch (locator_->type_)
	{
	case LOCATOR_HASH_MAP:
	{
		hashIterator_--;
		while (hashIterator_ != locator_->hash_.begin() && *hashIterator_ == INVALID_SLOT)
		{
			hashIterator_--;
		}
	}
	break;
	case LOCATOR_BIN_TREE:
	{
		treeIterator_--;
	}
	break;
	}

	return *this;
}

bool PageLocator::iterator::operator == (iterator const& other) const
{
	bool compare = false;

	switch (locator_->type_)
	{
	case cache::LOCATOR_HASH_MAP:
		compare = this->hashIterator_ == other.hashIterator_;
		break;
	case cache::LOCATOR_BIN_TREE:
		compare = this->treeIterator_ == other.treeIterator_;
		break;
	}

	return compare;
}


bool PageLocator::iterator::operator != (iterator const& other) const
{
	bool compare = false;

	switch (locator_->type_)
	{
	case cache::LOCATOR_HASH_MAP:
		compare = this->hashIterator_ != other.hashIterator_;
		break;
	case cache::LOCATOR_BIN_TREE:
		compare = this->treeIterator_ != other.treeIterator_;
		break;
	}

	return compare;
}

std::pair<PageNumber, SlotIndex>  PageLocator::iterator::operator*() const
{
	std::pair<PageNumber, SlotIndex> descriptor = { INVALID_PAGE, INVALID_SLOT };

	switch (locator_->type_)
	{
	case cache::LOCATOR_HASH_MAP:
		descriptor = { std::distance(locator_->hash_.begin(), hashIterator_), *hashIterator_ };
		break;
	case cache::LOCATOR_BIN_TREE:
		descriptor = { treeIterator_->first, treeIterator_->second };
		break;
	}

	return descriptor;
}