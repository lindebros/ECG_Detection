#include<stdio.h>
#include"sensor.h"

int main()
{
	static const char filename[] = "ECG.txt";

	FILE *file = fopen(filename,"r");

	int test;

	test = getNextData(file);

	printf("%i\n",test);

	test = getNextData(file);


	printf("%i\n",test);

	test = getNextData(file);

		printf("%i\n",test);

	return 0;
}
