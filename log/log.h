#pragma once

#include <time.h>
#include <string>
#include <iostream>
#include <stdarg.h>

using namespace std;

#define DEBUG

class LogPrinter {
private:
	static void printPre() {
		time_t _time = time(NULL);
		tm* utc_time = localtime(&_time);
		char buf[40];
		strftime(buf, 40, "[ %Y/%m.%d/ %H:%M:%S ]: ", utc_time);
		cout << buf;

//		buf = NULL;
	}
public:

	static void output(char *s) {
		printPre();
		cout << s << endl;
	}
	static void output(string s) {
		printPre();
		cout << s << endl;
	}
	static void output(const char* format, ...) {
		printPre();
		va_list va;
		va_start(va, format);
		vprintf(format, va);
		printf("\n");
		va_end(va);
	}
	static void outputD(char *s) {
#ifdef DEBUG
			output(s);
#endif
	}
	static void outputD(string s) {
#ifdef DEBUG
			output(s);
#endif
	}
	static void outputD(const char* format, ...) {
#ifdef DEBUG
			printPre();
			va_list va;
			va_start(va, format);
			vprintf(format, va);
			printf("\n");
			va_end(va);
#endif
	}
};
