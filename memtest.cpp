// memotest.cpp : Defines the entry point for the console application.
//
#if defined(_WIN32) || defined(_WIN64)
#include <crtdbg.h>
#endif

#include "memtracer.h"

int main()
{
		
	char* a = new char[2];
	//delete a;

	char* b = (char*)malloc(8);
	//free(b);
	
	dumpAlloc();

#if defined(_WIN32) || defined(_WIN64)
	_CrtDumpMemoryLeaks();
#endif
	
	return 0;
}


