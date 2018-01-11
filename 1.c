#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int main (int argc, char*argv[])
{
	typedef struct
	{
		char * name;
		char gender;
		int score;
		int ID;
	}student_info;

	student_info a;
	student_info b;
	a.name = "Michael Liu";
	a.gender = 'M';
	a.score = 90;
	a.ID = 01;
	b.name = "Laura";
	b.gender = 'F';
	b.score = 85;
	b.ID = 02;
	printf("%s %c %d %d \n", a.name, a.gender, a.score, a.ID);
	printf("%s %c %d %d \n", b.name, b.gender, b.score, b.ID);
	return 0;
}
