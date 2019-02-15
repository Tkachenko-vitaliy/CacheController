#include "TestHelper.h"
#include "PageCacheController.h"

namespace cache
{

	bool CheckDescriptor(list_descriptor& descriptors, PageCacheController& cache, unsigned int what)
	{
		std::vector<std::pair<unsigned long, unsigned long>> readInfo;

		if (what & CHEK_STATE)
		{
			cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_STATE);

			if (descriptors.size() != readInfo.size())
			{
				return false;
			}

			size_t length = readInfo.size();

			for (size_t i = 0; i < length; i++)
			{
				if (descriptors[i].state != readInfo[i].second)
				{
					return false;
				}
			}
		}

		if (what & CHECK_PAGE)
		{
			cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_PAGE);

			if (descriptors.size() != readInfo.size())
			{
				return false;
			}

			size_t length = readInfo.size();

			for (size_t i = 0; i < length; i++)
			{
				if (descriptors[i].page != readInfo[i].second)
					return false;
			}
		}

		if (what & CHECK_UNLOAD)
		{
			cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_UNLOAD_PAGE);

			if (descriptors.size() != readInfo.size())
			{
				return false;
			}

			size_t length = readInfo.size();

			for (size_t i = 0; i < length; i++)
			{
				if (descriptors[i].unloadPage != readInfo[i].second)
				{
					return false;
				}
			}
		}

		if (what & CHECK_WAIT_COUNT)
		{
			cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_WAITING_COUNT);

			if (descriptors.size() != readInfo.size())
			{
				return false;
			}

			size_t length = readInfo.size();

			for (size_t i = 0; i < length; i++)
			{
				if (descriptors[i].waitingCount != readInfo[i].second)
				{
					return false;
				}
					
			}
		}

		if (what & DBINFO_DESCRIPTOR_CHANGE)
		{
			cache.getDebugInfo(readInfo, DBINFO_DESCRIPTOR_CHANGE);

			if (descriptors.size() != readInfo.size())
			{
				return false;
			}

			size_t length = readInfo.size();

			for (size_t i = 0; i < length; i++)
			{
				if (descriptors[i].isDirty != readInfo[i].second)
				{
					return false;
				}
			}
		}

		return true;
	}

};