#include "TestSet.h"

#include <iostream>

int main()
{
	try
	{
		TestWhiteBox();
		TestAlgoritm();
		TestRW();
		TestWhiteboxException();
		TestWhiteBoxMT();
		TestWhiteBoxExceptionMT();
		TestReadWriteMT();
	}
	catch (const std::exception&)
	{
		std::cout << "FAILED" << std::endl;
		return 1;
	}

	std::cout << "FINISHED" << std::endl;
	return 0;
}