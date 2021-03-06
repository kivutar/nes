#include <unistd.h>
#include "u.h"
#include "compat.h"

ulong
umuldiv(ulong a, ulong b, ulong c)
{
	return ((uvlong)a * (uvlong)b) / c;
}

long
muldiv(long a, long b, long c)
{
	int s;
	long v;

	s = 0;
	if(a < 0) {
		s = !s;
		a = -a;
	}
	if(b < 0) {
		s = !s;
		b = -b;
	}
	if(c < 0) {
		s = !s;
		c = -c;
	}
	v = umuldiv(a, b, c);
	if(s)
		v = -v;
	return v;
}
