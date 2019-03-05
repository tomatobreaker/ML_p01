#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
int main (int argc, char * argv[])
{
	time_t t;
	struct tm * local;
	t = time(NULL);

	local = localtime(&t);
	printf("%d:%d:%d\n", local->tm_year+1900, local->tm_mon,local->tm_mday);
	printf("%d:%d:%d\n", local->tm_hour, local->tm_min, local->tm_sec);
	return 0;
}	
