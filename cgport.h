
#define FINALIZERS
#undef NEW_APPLY

#if defined (__MWERKS__) || defined (__MRC__)
# define POWER
# ifdef __cplusplus
#  include "cgrenameglobals.h"
# elif !defined (MAKE_MPW_TOOL)
#  define G_POWER
# endif
#endif

#if 0 || defined (MACH_O)
#define ALIGN_C_CALLS
#endif

#if defined (G_POWER) || defined (_WINDOWS_) || defined (LINUX_ELF) || defined (sparc)
# define FUNCTION_LEVEL_LINKING
#endif
                                                                                                                        
#ifdef THINK_C
#	define ANSI_C
#	define WORD int
#	define UWORD unsigned int
#else
#	define WORD short
#	define UWORD unsigned short
#endif

#ifdef GNU_C
#	define ANSI_C
#	undef mc68020
# ifdef SUN_C
#	define VARIABLE_ARRAY_SIZE 1
# else
#	define VARIABLE_ARRAY_SIZE 0
# endif
#else
# if defined (POWER) && !defined (__MRC__) && !defined (__MWERKS__)
#	define VARIABLE_ARRAY_SIZE 0
# else
#	define VARIABLE_ARRAY_SIZE
# endif
#endif

#define LONG long
#define BYTE char
#define ULONG unsigned long
#define UBYTE unsigned char

#define VOID void

#ifdef THINK_C
#	define DOUBLE short double
#else
#	define DOUBLE double
#endif

#ifdef sparc
#else
#	if defined (I486) || defined (G_POWER)
#	else
#		define M68000
#	endif
#endif

#if defined (I486) || (defined (G_POWER) || defined (ALIGN_C_CALLS)) || defined (MACH_O)
# define SEPARATE_A_AND_B_STACK_OVERFLOW_CHECKS
#endif

#ifdef __MWERKS__
int mystrcmp (char *p1,char *p2);
#define strcmp(s1,s2) mystrcmp(s1,s2)
#endif
