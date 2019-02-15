#pragma once

#include "SpinLock.h"
#include <fstream>

class TestLog
{
public:
	void open();
	void close();
	void setRecordLimit(unsigned long limit);
	void log(const char* message, bool outToConsole = false);
	void logFormat(bool outToConsole, const char* format, ...);
private:
	unsigned long recordLimit_ = 0;
	unsigned long recordCounter_ = 0;
	bool splitted_ = false;
	bool isOpen_ = false;
	spin_lock spinLock_;

	std::ofstream logStream_[2];
	std::ofstream* activeStream_ = nullptr;
	unsigned int numActiveStream_ = 0;

	const char* fileName_[2] = { "CacheLog.txt", "CacheLog1.txt" };
};


