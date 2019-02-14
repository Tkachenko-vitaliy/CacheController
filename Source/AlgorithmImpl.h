#pragma once

#include "CacheAlgorithm.h"

#include <vector>
#include <list>
#include <random>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace cache
{
	using namespace std;

	class CacheAlgorithmQueue : public CacheAlgorithm
	{
	public:
		void setPageCount(PageCount pageCount) override;
		PageNumber getReplacePage() override;
		void reset() override;
		virtual void getPageQueue(std::vector<PageNumber>& queue) const;

	protected:
		typedef std::list<PageNumber> PageQueue;
		typedef PageQueue::iterator PageQueueLocator;

		PageQueue pageQueue_;
		std::vector<PageQueueLocator> pageLocator_;
	};

	//First-in, First-out
	class caFIFO : public CacheAlgorithmQueue
	{
	public:
		 void onPageOperation(PageNumber page, PageOperation pageOperation) override;
	};

	//Least recently used 
	class caLRU : public CacheAlgorithmQueue
	{
	public:
		 void onPageOperation(PageNumber page, PageOperation pageOperation) override;
	};

	//Least Frequently Used 
	class caLFU : public CacheAlgorithmQueue
	{
	public:
		 void onPageOperation(PageNumber page, PageOperation pageOperation) override;
	};

	//Most Recently Used 
	class caMRU : public CacheAlgorithmQueue
	{
	public:
		 void onPageOperation(PageNumber page, PageOperation pageOperation) override;
	};

	class caClock: public CacheAlgorithm
	{
	public:
		void setPageCount(PageCount pageCount) override;
		PageNumber getReplacePage() override;
		void onPageOperation(PageNumber page, PageOperation pageOperation) override;
		void reset() override;

	private:
		std::vector<bool> listPages_;
		size_t currentPage_ = 0;
	};

	//Not recently used
	class caNRU : public CacheAlgorithm
	{
	public:
		~caNRU();
		void setPageCount(PageCount pageCount) override;
		void onPageOperation(PageNumber page, PageOperation pageOperation) override;
		PageNumber getReplacePage() override;
		void reset() override;
		void setTimerInterval(unsigned long intervalMillisec);
		void setParameter(const char* paramName, AlgoritmParameterValue paramValue) override;
		AlgoritmParameterValue getParameter(const char* paramName) const override;
	private:
		union UBits
		{
			UBits(char v) : value(v) {}

			struct
			{
				char referenced : 1;
				char modified : 1;
				char reserved : 6;
			} bitValue;

			char value;
		} ;
		std::vector<UBits> listPages_;

		bool isRun_ = false;
		unsigned long timerInterval_ = 200;
		void threadTimer();
		std::thread timer_;
		std::mutex mutexTimer_;
		std::condition_variable cvWaitTimer_;

	};

	class caRandom: public CacheAlgorithm
	{
	public:
		caRandom(unsigned int seedValue = 0);
		void setSeed(unsigned int seedValue = 0);
		void setPageCount(PageCount pageCount) override;
		PageNumber getReplacePage() override;
		void reset() override {}
		void onPageOperation(PageNumber page, PageOperation pageOperation) override {}
		void setParameter(const char* paramName, AlgoritmParameterValue paramValue) override;
		AlgoritmParameterValue getParameter(const char* paramName) const override;
	private:
		std::mt19937 generator_;
		PageCount pageCount_;
		unsigned int seed_;
	};


}; //namespace cache
