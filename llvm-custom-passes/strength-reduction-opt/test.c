#include <stdio.h>

int main(int argv, char** argc)
{
	int a;
	a = a*16;
	a = 8*a;
	a = a*5;
	a = a/128;
	a = 8/a;
	a = a/3;
	return a*8;
}
