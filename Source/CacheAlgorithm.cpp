#include "CacheAlgorithm.h"
#include "AlgorithmImpl.h"
#include "CacheException.h"

using namespace cache;

CacheAlgorithm* CacheAlgorithm::create(ReplaceAlgoritm algoritm)
{
	CacheAlgorithm* alg = nullptr;

	switch (algoritm)
	{
	case ALG_FIFO:
		alg = new caFIFO; 
		break;
	case ALG_LRU:
		alg = new caLRU;
		break;
	case ALG_LFU:
		alg = new caLFU;
		break;
	case ALG_MRU:
		alg = new caMRU;
		break;
	case ALG_CLOCK:
		alg = new caClock;
		break;
	case ALG_NRU:
		alg = new caNRU;
		break;
	case ALG_RANDOM:
		alg = new caRandom;
		break;
	}
	alg->type_ = algoritm;

	return alg;
}

ReplaceAlgoritm CacheAlgorithm::getType() const
{
	return type_;
}

void CacheAlgorithm::setParameter(const char* paramName, AlgoritmParameterValue paramValue)
{
	throw cache_exception(ERR_PARAMETER_NAME);
}

AlgoritmParameterValue CacheAlgorithm::getParameter(const char* paramName) const
{
	throw cache_exception(ERR_PARAMETER_NAME);
}