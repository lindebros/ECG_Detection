#include <stdio.h>
#include <stdlib.h>
#include "sensor.h"

int getNextData(FILE *a)
{
	// Implement me according to the Assignment 1 manual

	int value;

	fscanf(a,"%i",&value);

	return value;
}
