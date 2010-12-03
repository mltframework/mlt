
#include <errno.h> 
#include <time.h> 
#include <windows.h>
#include <pthread.h>

int usleep(unsigned int useconds)
{
	HANDLE timer;
	LARGE_INTEGER due;

	due.QuadPart = -(10 * (__int64)useconds);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &due, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
	return 0;
}


int nanosleep( const struct timespec * rqtp, struct timespec * rmtp )
{
	if (rqtp->tv_nsec > 999999999) {
		/* The time interval specified 1,000,000 or more microseconds. */
		errno = EINVAL;
		return -1;
	}
	return usleep( rqtp->tv_sec * 1000000 + rqtp->tv_nsec / 1000 );
} 

int setenv(const char *name, const char *value, int overwrite)
{
	int result = 1; 
	if (overwrite == 0 && getenv (name))  {
		result = 0;
	} else  {
		result = SetEnvironmentVariable (name,value); 
	 }

	return result; 
} 

