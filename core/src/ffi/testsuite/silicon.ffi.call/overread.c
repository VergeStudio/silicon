#include "ffitest.h"

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>

static int ABI_ATTR fn(unsigned char a, unsigned short b, unsigned int c, unsigned long d)
{
	return (int)(a + b + c + d);
}
#endif

int main(void)
{
#ifdef __linux__
	sffi_cif cif;
	sffi_type *args[MAX_ARGS];
	void *values[MAX_ARGS];
	sffi_arg rint;
	char *m;
	int ps;
	args[0] = &sffi_type_uchar;
	args[1] = &sffi_type_ushort;
	args[2] = &sffi_type_uint;
	args[3] = &sffi_type_ulong;
	CHECK(sffi_prep_cif(&cif, ABI_NUM, 4, &sffi_type_sint, args) == SFFI_OK);
	ps = getpagesize();
	m = mmap(NULL, ps * 3, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	CHECK(m != MAP_FAILED);
	CHECK(mprotect(m, ps, PROT_NONE) == 0);
	CHECK(mprotect(m + ps * 2, ps, PROT_NONE) == 0);
	values[0] = m + ps * 2 - sizeof(unsigned char);
	values[1] = m + ps * 2 - sizeof(unsigned short);
	values[2] = m + ps * 2 - sizeof(unsigned int);
	values[3] = m + ps * 2 - sizeof(unsigned long);
	sffi_call(&cif, SFFI_FN(fn), &rint, values);
	CHECK((int)rint == 0);
	values[0] = m + ps;
	values[1] = m + ps;
	values[2] = m + ps;
	values[3] = m + ps;
	sffi_call(&cif, SFFI_FN(fn), &rint, values);
	CHECK((int)rint == 0);
#endif
	exit(0);
}
