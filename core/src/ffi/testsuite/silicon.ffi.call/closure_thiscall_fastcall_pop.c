#include "ffitest.h"

#if defined(__i386__) && defined(__GNUC__) && !defined(__APPLE__)

static uint64_t received;
static int ran;

static void
cb (sffi_cif *cif, void *resp, void **args, void *userdata)
{
  (void) cif; (void) resp; (void) userdata;
  received = *(uint64_t *) args[cif->nargs - 1];
  ran++;
}

static int
esp_delta (void *code, uint64_t stackarg, unsigned ecxv)
{
  unsigned delta;
  unsigned lo = (unsigned) stackarg;
  unsigned hi = (unsigned) (stackarg >> 32);
  __asm__ volatile (
      "movl %[lo], %%eax\n\t"       
      "movl %[hi], %%edx\n\t"       
      "movl %[code], %%edi\n\t"
      "movl %[ecxv], %%ecx\n\t"     
      "movl %%esp, %%esi\n\t"       
      "andl $-16, %%esp\n\t"        
      "subl $8, %%esp\n\t"          
      "pushl %%edx\n\t"             
      "pushl %%eax\n\t"             
      "calll *%%edi\n\t"
      "movl %%esi, %%eax\n\t"       
      "andl $-16, %%eax\n\t"        
      "subl $8, %%eax\n\t"
      "subl %%esp, %%eax\n\t"       
      "movl %%esi, %%esp\n\t"       
      "movl %%eax, %[delta]\n\t"
      : [delta] "=m" (delta)
      : [lo] "m" (lo), [hi] "m" (hi), [code] "m" (code), [ecxv] "m" (ecxv)
      : "memory", "cc", "eax", "ecx", "edx", "esi", "edi");
  return (int) delta;
}

static int
check_abi (sffi_abi abi, unsigned nargs, sffi_type **atypes, unsigned ecx)
{
  sffi_cif cif;
  sffi_closure *closure;
  void *code;
  int delta;

  closure = sffi_closure_alloc (sizeof (sffi_closure), &code);
  CHECK (closure != NULL);
  CHECK (sffi_prep_cif (&cif, abi, nargs, &sffi_type_void, atypes) == SFFI_OK);
  CHECK (sffi_prep_closure_loc (closure, &cif, cb, NULL, code) == SFFI_OK);

  ran = 0;
  received = 0;
  delta = esp_delta (code, 0x1122334455667788ULL, ecx);

  CHECK (ran == 1);
  CHECK (received == 0x1122334455667788ULL);
  sffi_closure_free (closure);
  return delta;
}

int
main (void)
{
  sffi_type *fastcall_args[1] = { &sffi_type_uint64 };
  sffi_type *thiscall_args[2] = { &sffi_type_pointer, &sffi_type_uint64 };
  int d;

  d = check_abi (SFFI_FASTCALL, 1, fastcall_args, 0);
  printf ("FASTCALL uint64 esp delta: %d\n", d);
  CHECK (d == 0);

  d = check_abi (SFFI_THISCALL, 2, thiscall_args, 0xdeadbeef);
  printf ("THISCALL this+uint64 esp delta: %d\n", d);
  CHECK (d == 0);

  exit (0);
}

#else

int
main (void)
{

  exit (0);
}

#endif
