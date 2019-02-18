#include "TestControllerMT.h"

#include <random>
#include <iostream>

using namespace cache;

void TestControllerMT::run(const RandomSetup& setup)
{
	if (setup.bLog)
	{
		logFile_.open();
		logFile_.setRecordLimit(setup.logRecordLimit);

		setDebugCallbackLog([this](const char* message)
		{
			this->logFile_.log(message);
		});
		setDebugLogEndline(true);
	}

	setupPages(setup.pageCount, setup.pageSize);

	spaceSize_ = setup.spaceSize;
	operationCount_ = setup.operationCount;
	intervalFlush_ = setup.intervalFlush;
	intervalException_ = setup.intervalException;
	triggerException_ = intervalException_;
	executedOperationCount_ = 0;
	threadCount_ = setup.countRead + setup.countWrite;
	commonOperationCount_ = threadCount_ * operationCount_;
	if (intervalFlush_) threadCount_++;
	threadReadyCount_ = 0;
	stopError_ = false;
	run_ = false;
	fixedAddress_ = setup.fixedAddress;
	randomSeed_ = setup.randomSeed;
	isLogging_ = setup.bLog;

	storage_.resize(setup.spaceSize);
	sampleData_.resize(setup.spaceSize);

	listThread_.clear(); listThread_.resize(threadCount_);
	watchDog_.clear(); watchDog_.resize(threadCount_, false);
	finishedThread_.clear(); finishedThread_.resize(threadCount_, false);

	unsigned char counter = 1;
	for (size_t i = 0; i < setup.spaceSize; i++)
	{
		storage_[i] = 0;
		sampleData_[i] = counter;
		counter++;
		if (counter == 0)
			counter = 1;
	}

	size_t index = 0;

	for (size_t i = 0; i < setup.countRead; i++)
	{
		listThread_[index] = std::thread(&TestControllerMT::threadRead, this, index);
		index++;

	}
	for (size_t i = 0; i < setup.countWrite; i++)
	{
		listThread_[index] = std::thread(&TestControllerMT::threadWrite, this, index);
		index++;
	}

	if (intervalFlush_)
	{
		listThread_[index] = std::thread(&TestControllerMT::threadFlush, this, index);
		index++;
	}

	while (threadReadyCount_ != threadCount_) { std::this_thread::yield(); }
	run_ = true;
}

bool  TestControllerMT::isError()
{
	return stopError_;
}

bool  TestControllerMT::isFinished()
{
	std::lock_guard<spin_lock> lock(finishLock_);

	for (auto item : finishedThread_)
	{
		if (item == false)
			return false; //not all task finished
	}

	return true;
}

bool TestControllerMT::waitFinish(unsigned int timeoutSec)
{
	std::unique_lock<std::mutex> lk(conditionMutex_);
	std::cv_status status = waitFinish_.wait_for(lk, std::chrono::seconds(timeoutSec));

	return status != std::cv_status::timeout;
}

bool  TestControllerMT::checkWatchDog()
{
	for (size_t i = 0; i < watchDog_.size(); i++)
	{
		if (watchDog_[i] == false && finishedThread_[i] == false)
		{
			logFile_.logFormat(true, "thread %u does not response\n", i);
			return false;
		}
		else
		{
			watchDog_[i] = false;
		}
	}

	return true;
}


bool  TestControllerMT::checkStorage()
{
	unsigned char counter = 1;
	for (size_t i = 0; i < storage_.size(); i++)
	{
		if (storage_[i] != 0)
		{
			if (storage_[i] != counter)
			{
				return false;
			}
		}

		counter++;
		if (counter == 0)
			counter = 1;
	}
	return true;
}

void  TestControllerMT::final()
{
	for (auto &thread : listThread_)
	{
		thread.join();
	}
	logFile_.close();
	exceptionWrite_ = false;
	exceptionRead_ = false;
}

void  TestControllerMT::abort()
{
	stopError_ = true;
	logFile_.close();
	std::terminate();
}

void TestControllerMT::readStorage(DataAddress address, DataSize size, void* dataBuffer, void* metaData)
{
	if (exceptionRead_)
	{
		exceptionRead_ = false;
		logFile_.logFormat(false, "read exception id=%u\n", (size_t)metaData);
		throw std::exception();
	}
	if (address + size > spaceSize_)
		size = spaceSize_ - (DataSize)address;
	memcpy(dataBuffer, &storage_[(DataSize)address], size);
}

void TestControllerMT::writeStorage(DataAddress address, DataSize size, const void* dataBuffer, void* metaData)
{
	if (exceptionWrite_)
	{
		exceptionWrite_ = false;
		logFile_.logFormat(false, "write exception id=%u\n", (size_t)metaData);
		throw std::exception();
	}
	if (address + size > spaceSize_)
		size = spaceSize_ - (DataSize)address;
	memcpy(&storage_[(DataSize)address], dataBuffer, size);
}

bool TestControllerMT::verifyData(DataAddress address, std::vector<unsigned char>& data)
{
	for (size_t i = 0; i < data.size(); i++)
	{
		if (data[i] != 0)
		{
			if (sampleData_[(DataSize)address + i] != data[i])
			{
				return false;
			}
		}
	}
	return true;
}

void TestControllerMT::onFinish(size_t threadIndex)
{
	std::lock_guard<spin_lock> lock(finishLock_);

	finishedThread_[threadIndex] = true;

	for (auto item : finishedThread_)
	{
		if (item == false)
			return; //not all task finished
	}

	waitFinish_.notify_all();
}

void TestControllerMT::setException()
{
	if (intervalException_ > 0)
	{
		std::lock_guard<spin_lock> lock(exceptionlock_);

		if (executedOperationCount_ > triggerException_)
		{
			exceptionRead_ = true; exceptionWrite_ = true;
			triggerException_ = executedOperationCount_ + intervalException_;
		}
	}
}

void TestControllerMT::onOperation(size_t threadIndex)
{
	watchDog_[threadIndex] = true;
}

void TestControllerMT::threadWrite(size_t index)
{
	std::random_device rd;
	std::mt19937 gen;
	if (randomSeed_)
		gen.seed(rd());
	else
		gen.seed(index);
	std::uniform_int_distribution<> numbers(0, spaceSize_ - 1);

	size_t opCounter = 0;

	threadReadyCount_++;

	while (!run_) { std::this_thread::yield(); }

	while (opCounter < operationCount_)
	{
		DataAddress address;
		DataSize size;

		if (fixedAddress_)
		{
			address = 0;
			size = spaceSize_;
		}
		else
		{
			address = numbers(gen);
			size = numbers(gen) + 1;
		}

		DataSize smallAddress = (DataSize)address;

		try
		{
			if (address + size > spaceSize_)
			{
				DataSize rest = smallAddress + size - spaceSize_;
				write(address, spaceSize_ - smallAddress, &sampleData_[smallAddress], (void*)index);
				write(0, rest, &sampleData_[0], (void*)index);
			}
			else
			{
				write(address, size, &sampleData_[smallAddress], (void*)index);
			}
		}
		catch (const std::exception&)
		{

		}

		opCounter++;
		executedOperationCount_++;

		if (stopError_)
		{
			break;
		}

		onOperation(index);
		setException();
	}

	onFinish(index);
}

void TestControllerMT::threadRead(size_t index)
{
	std::vector<unsigned char> readData;

	std::random_device rd;
	std::mt19937 gen;
	if (randomSeed_)
		gen.seed(rd());
	else
		gen.seed(index);
	std::uniform_int_distribution<> numbers(0, spaceSize_ - 1);

	size_t opCounter = 0;

	threadReadyCount_++;

	while (!run_) {}

	while (opCounter < operationCount_)
	{
		DataAddress address;
		DataSize size;

		if (fixedAddress_)
		{
			address = 0;
			size = spaceSize_;
		}
		else
		{
			address = numbers(gen);
			size = numbers(gen) + 1;
		}

		DataSize smallAddress = (DataSize)address;

		try
		{
			if (address + size > spaceSize_)
			{
				DataSize rest = smallAddress + size - spaceSize_;
				readData.resize(spaceSize_ - smallAddress);
				read(address, spaceSize_ - smallAddress, readData.data(), (void*)index);

				if (!verifyData(address, readData))
				{
					logFile_.log("verification error\n");
					stopError_ = true;
				}

				readData.resize(rest);
				read(0, rest, readData.data(), (void*)index);

				if (!verifyData(0, readData))
				{
					logFile_.log("verification error\n");
					stopError_ = true;
				}
			}
			else
			{
				readData.resize(size);
				read(address, size, readData.data(), (void*)index);

				if (!verifyData(address, readData))
				{
					logFile_.log("verification error\n");
					stopError_ = true;
				}
			}
		}
		catch (const std::exception&)
		{

		}

		opCounter++;
		executedOperationCount_++;

		if (stopError_)
		{
			break;
		}

		onOperation(index);
		setException();
	}

	onFinish(index);
}

void TestControllerMT::threadFlush(size_t index)
{
	size_t nextTrigger = intervalFlush_;
	threadReadyCount_++;

	while (!run_) {}

	while (executedOperationCount_ < commonOperationCount_)
	{
		std::this_thread::yield();

		if (executedOperationCount_ > nextTrigger)
		{
			try
			{
				flush();
			}
			catch (const std::exception&)
			{

			}
			nextTrigger = executedOperationCount_ + intervalFlush_;
		}

		onOperation(index);

		if (stopError_)
			break;

	}

	onFinish(index);
}