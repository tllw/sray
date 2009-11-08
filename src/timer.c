#include "timer.h"

#if defined(__APPLE__) && defined(__MACH__)
#ifndef __unix__
#define __unix__	1
#endif
#endif

#if defined(unix) || defined(__unix__)
#include <unistd.h>
#include <sys/time.h>

unsigned long get_msec(void)
{
	static struct timeval tv0;
	struct timeval tv;

	gettimeofday(&tv, 0);

	if(tv0.tv_sec == 0 && tv0.tv_usec == 0) {
		tv0 = tv;
		return 0;
	}
	return (tv.tv_sec - tv0.tv_sec) * 1000 + (tv.tv_usec - tv0.tv_usec) / 1000;
}

void msleep(int msec)
{
	usleep(msec * 1000);
}

#elif defined(WIN32) || defined(__WIN32__)
#include <windows.h>

unsigned long get_msec()
{
	return timeGetTime();
}

void msleep(int msec)
{
	Sleep(msec);
}

#else
#error "don't know how to measure time in your system :("
#endif
