#pragma once

#include <exception>

class TestException : public std::exception
{
public:
	TestException(const char* stringMessage) : message(stringMessage) {}
	const char* what() const override { return message; }
private:
	const char* message;
};


void TestWhiteBox();
void TestRW();
void TestWhiteboxException();
void TestWhiteBoxMT();
void TestWhiteBoxExceptionMT();
void TestReadWriteMT();
void TestAlgoritm();
