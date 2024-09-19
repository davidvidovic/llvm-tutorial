#include <stdio.h>

void foo(int a)
{
	int b = a + 1;
}

int main()
{
	int a, b, c, d;
	a = 1;
	a = 2;
	b = 4 + a;
	a = 5;
	c = 3;

	if(a < 6)
	{
		b = 1;
	}
	d = c + 19;
	foo(a);
	return a + b;
}
