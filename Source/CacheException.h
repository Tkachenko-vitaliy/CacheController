#pragma once

#include <exception>

typedef enum 
{
	ERR_BUFFER_NOT_ALLOCATED	= 0,
	ERR_ADDRESS_OFFSET	= 1,
	ERR_ALLOCATE_BUFFER	= 2,
	ERR_PAGE_COUNT_SIZE	= 3,
	ERR_PAGE_OVERLOADED	= 4,
	ERR_PARAMETER_NAME	= 5,
	ERR_PARAMETER_VALUE	= 6,
	ERR_HASH_LIMIT		= 7
} CacheErrorCode;

class cache_exception : public std::exception
{
public:
	cache_exception(CacheErrorCode errorCode);
	const char* what() const override;
private:
	CacheErrorCode errorCode_;
};

