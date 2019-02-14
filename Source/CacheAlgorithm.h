#pragma once

#include "CacheTypes.h"

namespace cache
{
	using namespace std;

	class CacheAlgorithm
	{
	public:
		virtual ~CacheAlgorithm() {}

		virtual void setPageCount(PageCount pageCount) = 0;

		virtual void onPageOperation(PageNumber page, PageOperation pageOperation) = 0;

		virtual PageNumber getReplacePage() = 0;

		virtual void reset() = 0;

		virtual void setParameter(const char* paramName, AlgoritmParameterValue paramValue);

		virtual AlgoritmParameterValue getParameter(const char* paramName) const;

		ReplaceAlgoritm getType() const;

		static CacheAlgorithm* create(ReplaceAlgoritm algoritm);

	private:
		ReplaceAlgoritm type_;
	};

}; //namespace cache
