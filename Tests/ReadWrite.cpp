#include "TestSet.h"
#include "PageCacheController.h"

#include <fstream>
#include <random>

using namespace cache;

typedef unsigned int tested_number_type;

typedef enum { VALUE_INCREMENT = 0, VALUE_SHIFT = 1 } ValueChangeType;

class TestCacheRW : public PageCacheController
{
public:
	void open(const char* fileName, bool createNew = false)
	{
		if (createNew)
		{
			output.open(fileName, ios_base::binary | ios_base::out | ios_base::trunc);
		}
		else
		{
			output.open(fileName, ios_base::binary | ios_base::out | ios_base::in);
			//If we set the mode only ios_base::out, the file is truncate. For the same reason, we use 'fstream' instead of 'ofstream'.
			//Most probably, it is a bug in STL stream implementation
		}

		input.open(fileName, ios_base::binary);

		if (input.fail() || output.fail())
		{
			throw TestException("Error open the file");
		}
	}

	void close()
	{
		input.close();
		output.close();
	}
protected:
	void readStorage(DataAddress address, DataSize size, void* dataBuffer, void* metaData) override
	{
		input.seekg(address);
		input.read((char*)dataBuffer, size);
	}

	void writeStorage(DataAddress address, DataSize size, const void* dataBuffer, void* metaData) override
	{
		output.seekp(address);
		output.write((const char*)dataBuffer, size);
	}

private:
	ifstream input;
	fstream output;
};

void ChangeValue(tested_number_type& value, ValueChangeType changeType)
{
	if (changeType == VALUE_INCREMENT)
	{
		value++;
	}
	else
	{
		value = value << 1;
	}
}

void WriteNumbers(PageCacheController& cache, unsigned int offset, DataAddress startAddress, unsigned int countNumbers, ValueChangeType changeType)
{
	const size_t sizeValue = sizeof(tested_number_type);
	tested_number_type value = 1;
	DataAddress beginAddress = startAddress;
	DataAddress endAddress = startAddress + countNumbers * sizeValue;
	beginAddress += offset; endAddress += offset;
	for (DataAddress i = beginAddress; i < endAddress; i += sizeValue)
	{
		cache.write(i, sizeValue, &value);
		ChangeValue(value, changeType);
	}
}

bool ReadNumbers(PageCacheController& cache, unsigned int offset, DataAddress startAddress, unsigned int countNumbers, ValueChangeType changeType)
{
	const size_t sizeValue = sizeof(tested_number_type);
	tested_number_type value = 1;
	DataAddress beginAddress = startAddress;
	DataAddress endAddress = startAddress + countNumbers * sizeValue;
	beginAddress += offset; endAddress += offset;
	for (DataAddress i = beginAddress; i < endAddress; i += sizeValue)
	{
		unsigned int readValue = 1;
		cache.read(i, sizeValue, &readValue);
		if (value != readValue)
		{
			return false;
		}
		ChangeValue(value, changeType);
	}
	return true;
}

void RWNumbers(unsigned int numberCount, unsigned int pageCount, unsigned int pageSize, unsigned int offset, ValueChangeType changeType)
{
	printf("write numbers: count=%u page=%u page_size=%u offset=%u\n", numberCount, pageCount, pageSize, offset);

	TestCacheRW cache;
	const char* testFile = "test.bin";
	cache.open(testFile, true);
	cache.setupPages(pageCount, pageSize);
	cache.setStartPageOffset(offset);
	WriteNumbers(cache, offset, 0, numberCount, changeType);
	cache.flush();
	cache.close();

	cache.enable(false);
	cache.open(testFile);
	if (!ReadNumbers(cache, offset, 0, numberCount, changeType))
	{
		throw TestException("TestRWNumbers");
	}

	cache.clear();
	cache.enable(true);
	if (!ReadNumbers(cache, offset, 0, numberCount, changeType))
	{
		throw TestException("TestRWNumbers");
	}

	printf("Successfull\n");
}

void RWNumberArray(const tested_number_type numberArray[], unsigned int numberCount, unsigned int pageCount, unsigned int pageSize, unsigned int offset)
{
	printf("write number array: count=%u page=%u page_size=%u offset=%u\n", numberCount, pageCount, pageSize, offset);

	DataAddress address = 0;
	DataSize size = sizeof(tested_number_type) * numberCount;

	TestCacheRW cache;
	const char* testFile = "test.bin";
	cache.open(testFile, true);
	cache.setupPages(pageCount, pageSize);
	cache.setStartPageOffset(offset);
	cache.write(address, size, numberArray);
	cache.flush();
	cache.close();

	cache.enable(false);
	cache.open(testFile);
	if (!ReadNumbers(cache, offset, 0, numberCount, VALUE_INCREMENT))
	{
		throw TestException("RWNumberArray");
	}

	printf("successfull\n");
}


void RWNumbersOnMatrixPages(unsigned int numberCount)
{
	for (unsigned int pageCount = 1; pageCount <= numberCount; pageCount++)
	{
		for (unsigned int pageSize = 1; pageSize <= numberCount; pageSize++)
		{
			RWNumbers(numberCount, pageCount, pageSize, 0, VALUE_INCREMENT);
		}
	}
}

void RWLongNumberArrayOnPages(unsigned int numberCount)
{
	std::unique_ptr<tested_number_type> ptr;
	ptr.reset(new tested_number_type[numberCount]);

	for (unsigned int i = 0; i < numberCount; i++)
	{
		ptr.get()[i] = i + 1;
	}

	TestCacheRW cache;
	const char* testFile = "test.bin";
	cache.open(testFile, true);

	for (unsigned int pageCount = 1; pageCount <= numberCount; pageCount++)
	{
		for (unsigned int pageSize = 1; pageSize <= numberCount; pageSize++)
		{
			RWNumberArray(ptr.get(), numberCount, pageCount, pageSize, 0);
		}
	}
}

void RWChunkArray(unsigned int chunkCount)
{
	printf("write chunk array: chunk count = %u\n", chunkCount);

	struct Chunk
	{
		char n1;
		int16_t n2;
		double n3;

		bool operator == (const Chunk& ch) const
		{
			return n1 == ch.n1 && n2 == ch.n2 && n3 == ch.n3;
		}
	};

	std::mt19937 gen(123456);
	std::uniform_int_distribution<int> u1(0, std::numeric_limits<char>::max());
	std::uniform_int_distribution<int> u2(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
	std::uniform_real_distribution<> u3(1.0, 2.0);

	std::vector<Chunk> sampleVector;

	for (size_t i = 0; i < chunkCount; i++)
	{
		Chunk chunk;
		chunk.n1 = u1(gen);
		chunk.n2 = u2(gen);
		chunk.n3 = u3(gen);
		sampleVector.push_back(chunk);
	}

	unsigned int sizeChunk = sizeof(Chunk);

	TestCacheRW cache;
	const char* testFile = "test.bin";

	for (unsigned int pageCount = 1; pageCount <= chunkCount; pageCount++)
	{
		for (unsigned int pageSize = 1; pageSize <= chunkCount; pageSize++)
		{
			cache.open(testFile, true);
			cache.enable(true);
			cache.setupPages(pageCount, pageSize);
			cache.write(0, sizeof(Chunk)*chunkCount, sampleVector.data());
			cache.flush();
			cache.close();

			cache.enable(false);
			cache.open(testFile);
			std::vector<Chunk> readVector;
			readVector.resize(chunkCount);
			cache.read(0, sizeof(Chunk)*chunkCount, readVector.data());
			if (sampleVector != readVector)
				throw TestException("TestRWChunkArray");

			cache.clear();
			cache.enable(true);
			cache.read(0, sizeof(Chunk)*chunkCount, readVector.data());
			if (sampleVector != readVector)
				throw TestException("TestRWChunkArray");
			cache.close();
		}
	}

	printf("Successfull\n");
}

void TestRW()
{
	RWNumbers(1, 1, sizeof(tested_number_type), 0, VALUE_INCREMENT);		//1 number  on 1 page
	RWNumbers(1, 1, sizeof(tested_number_type), 10, VALUE_INCREMENT);		//1 number  on 1 page with offset
	RWNumbers(4, 1, sizeof(tested_number_type), 0, VALUE_INCREMENT);		//4 numbers on 1 page
	RWNumbers(4, 1, sizeof(tested_number_type), 10, VALUE_INCREMENT);		//4 numbers on 1 page with offset

	RWNumbers(4, 2, sizeof(tested_number_type), 0, VALUE_INCREMENT);		//4 numbers on 2 page
	RWNumbers(4, 2, sizeof(tested_number_type), 10, VALUE_INCREMENT);		//4 numbers on 2 page with offset

	RWNumbers(1, 2, sizeof(tested_number_type) / 2, 0, VALUE_INCREMENT);		//1 numbers on 2 page of half-size
	RWNumbers(1, 2, sizeof(tested_number_type) / 2, 10, VALUE_INCREMENT);	//1 numbers on 2 page of half-size with offset
	RWNumbers(4, 2, sizeof(tested_number_type) / 2, 0, VALUE_INCREMENT);	//4 numbers on 2 page of half-size 
	RWNumbers(4, 2, sizeof(tested_number_type) / 2, 0, VALUE_INCREMENT);	//4 numbers on 2 page of half-size with offset

	RWNumbers(32, 1, sizeof(tested_number_type), 0, VALUE_SHIFT);			//32 number  on 1 page
	RWNumbers(32, 2, sizeof(tested_number_type), 10, VALUE_SHIFT);			//32 number  on 1 page with offset, value shift
	RWNumbers(32, 2, sizeof(tested_number_type) / 2, 0, VALUE_SHIFT);		//32 numbers on 2 page of half-size , value shift
	RWNumbers(32, 2, sizeof(tested_number_type) / 2, 10, VALUE_SHIFT);		//32 numbers on 2 page of half-size with offset, value shift

	RWNumbers(1, 1, 1, 0, VALUE_SHIFT);		//1 number byte-per-byte on 1x1 page, value shift
	RWNumbers(32, 1, 1, 10, VALUE_SHIFT);	//32 numbers byte-per-byte on 1x1 page with offset, value shift

	RWNumbers(32, 4, 1, 0, VALUE_SHIFT);		//32 numbers byte-per-byte on 4x1 page, value shift
	RWNumbers(64, 4, 1, 0, VALUE_SHIFT);		//64 numbers byte-per-byte on 4x1 page, value shift

	RWNumbers(64, 4, 3, 0, VALUE_SHIFT);		//64 numbers byte-per-byte on 4x3 page, value shift

	RWNumbersOnMatrixPages(17); //different pages count with different page size

	RWLongNumberArrayOnPages(13);   //long array on different page count and size

	RWChunkArray(10); //structures
}