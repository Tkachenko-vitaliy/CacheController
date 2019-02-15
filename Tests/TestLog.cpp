#include "TestLog.h"

#include <stdarg.h>
#include <iostream>
#include <mutex>

void TestLog::open()
{
	recordCounter_ = 0;
	numActiveStream_ = 0;
	activeStream_ = &logStream_[numActiveStream_];
	activeStream_->open(fileName_[numActiveStream_]);
	isOpen_ = true;
	splitted_ = false;
}

void TestLog::log(const char* message, bool outToConsole)
{
	if (!isOpen_)
	{
		return;
	}

	if (recordLimit_ > 0)
	{
		std::lock_guard<spin_lock> lock(spinLock_);
		recordCounter_++;
		if (recordCounter_ > recordLimit_)
		{
			activeStream_->close();
			splitted_ = true;
			if (numActiveStream_ == 0)
			{
				numActiveStream_ = 1;
			}
			else
			{
				numActiveStream_ = 0;
			}
			activeStream_ = &logStream_[numActiveStream_];
			activeStream_->open(fileName_[numActiveStream_]);
			recordCounter_ = 0;
		}
	}

	(*activeStream_) << message;

	if (outToConsole)
	{
		std::cout << message;
	}
}

void TestLog::logFormat(bool outToConsole, const char* format, ...)
{
	if (!isOpen_)
	{
		return;
	}

	const unsigned int sizeMessage = 256;
	char message[sizeMessage];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeMessage, format, args); 
	va_end(args);
	log(message, outToConsole);
}

void TestLog::close()
{
	isOpen_ = false;
	logStream_[0].close();
	logStream_[1].close();

	if (splitted_)
	{
		const char* nameSource; const char* nameDest;

		if (numActiveStream_ == 0)
		{
			nameSource = fileName_[0];
			nameDest = fileName_[1];
		}
		else
		{
			nameSource = fileName_[1];
			nameDest = fileName_[0];
		}

		std::ifstream source(nameSource); std::ofstream dest(nameDest, std::ios::app);

		const size_t length = 512;
		char str[length];
		while (!source.eof())
		{
			source.getline(str, length);
			dest << str << std::endl;
		}

		source.close(); dest.close();

		std::remove(nameSource);
		if (strcmp(nameDest, fileName_[0]) != 0)
		{
			std::rename(nameDest, fileName_[0]);
		}
	}
}

void TestLog::setRecordLimit(unsigned long limit)
{
	recordLimit_ = limit;
}