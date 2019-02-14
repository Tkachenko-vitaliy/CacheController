#include "CacheException.h"

cache_exception::cache_exception(CacheErrorCode errorCode) : errorCode_(errorCode)
{

}

static const char* g_cErrorList[] =
{
	"Buffer is not allocated", //ERR_BUFFER_NOT_ALLOCATED
	"Address is less then page offset", //ERR_ADDRESS_OFFSET
	"Buffer cannot be allocated", //ERR_ALLOCATE_BUFFER
	"Wrong page count or page size", //ERR_PAGE_COUNT_SIZE
	"Page number is too long", //ERR_PAGE_OVERLOADED
	"Unknown parameter name", // ERR_PARAMETER_NAME
	"Incorrect parameter value", // ERR_PARAMETER_VALUE
};

const char* cache_exception::what() const
{
	return g_cErrorList[errorCode_];
}