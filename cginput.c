/*
	File:	cginput.c
	Author:	John van Groningen
	At:		University of Nijmegen
*/

#define BINARY_ABC 0
#define COMPATIBLE_DESC 0

#include <string.h>

#include "cgport.h"

#include <stdio.h>
#if defined (ANSI_C) || defined (__MWERKS__)
#	include <stdlib.h>
#else
	extern double atof (char *s);
#endif
#include <setjmp.h>

#ifndef THINK_C
# define PROFILING
#endif

#ifdef PROFILING
# define IF_PROFILING(a,b) a
#else
# define IF_PROFILING(a,b) b
#endif

#include "cg.h"
#include "cgcodep.h"
#include "cginput.h"

#include "MAIN_CLM.d"

#ifdef THINK_C
# include "system.h"
# ifdef _MACUSER_
#  undef stderr
#  define stderr StdError
# endif
#endif

#ifdef UNDEF_GETC
# undef getc
#endif

struct instruction {
	char *	instruction_name;
#if defined  (THINK_C) || defined (__cplusplus)
	int (*	instruction_parse_function)(struct instruction *);
	void (*	instruction_code_function)(...);
#else
	int (*	instruction_parse_function)();
	void (*	instruction_code_function)();
#endif
};

typedef struct instruction *InstructionP;

static FILE *abc_file;
static int last_char;

int line_number;

#ifdef applec
static unsigned char *alpha_num_table;
#else
static unsigned char alpha_num_table[257];
#endif

#define	init_memory_allocate(size)	malloc(size)

static jmp_buf exit_abc_parser_buffer;

#ifdef MAIN_CLM
# define abc_parser_error_i error_i
# define abc_parser_error_si error_si
#else

static void exit_abc_parser (void)
{
	longjmp (exit_abc_parser_buffer, 1);
}

static void abc_parser_error_i (char *message, int i)
{
	fprintf (stderr,message,i);
	exit_abc_parser ();
}

static void abc_parser_error_si (char *message, char *s,int i)
{
	fprintf (stderr,message,s,i);
	exit_abc_parser ();
}
#endif

static void initialize_alpha_num_table()
{
	int i;
	static unsigned char extra_alpha_nums[]="@#$%^&*-+/=<>\\_~.?\'\"|`!:{};";

#ifdef applec
	alpha_num_table=init_memory_allocate (257);
#endif
		
	for (i=0; i<257; ++i)
		alpha_num_table[i]=0;
		
	for (i='a'; i<='z'; ++i)
		alpha_num_table[i+1]=1;
	for (i='A'; i<='Z'; ++i)
		alpha_num_table[1+i]=1;
	for (i='0'; i<='9'; ++i)
		alpha_num_table[1+i]=1;
	for (i=0; i<sizeof (extra_alpha_nums)-1; ++i)
		alpha_num_table[1+extra_alpha_nums[i]]=1;
}

#define is_alpha_num_character(c) (alpha_num_table[(c)+1])

#define is_digit_character(c) ((unsigned)((c)-'0')<(unsigned)10)

static void initialize_file_parsing (VOID)
{
	line_number=1;

	last_char=getc (abc_file);
}

static int skip_spaces_and_tabs (VOID)
{
	if (last_char==' ' || last_char=='\t'){
		do
			last_char=getc (abc_file);
		while (last_char==' ' || last_char=='\t');
		return 1;
	} else
		return 0;
}

#if !BINARY_ABC
#define is_newline(c) ((c)=='\xa' || (c)=='\xd')
#else
#define is_newline(c) ((c)=='\xa' || (c)=='\xd' || (c)>=136)
#endif

static void skip_to_end_of_line (VOID)
{
	while (last_char!='\xa' && last_char!='\xd' && last_char!=EOF)
/*	while (!is_newline (last_char) && last_char!=EOF) */
		last_char=getc (abc_file);
}

#define MAX_STRING_LENGTH 200

typedef char STRING [MAX_STRING_LENGTH+1];

static int parse_instruction_string (char *string)
{
	int length;
	
	length=0;
	if (is_alpha_num_character (last_char)){
		do {
			if (length<MAX_STRING_LENGTH)
				string[length++] = last_char;
			last_char=getc (abc_file);
		} while (is_alpha_num_character (last_char));
	}
	string[length]='\0';
	
	skip_spaces_and_tabs();
	
	if (length==0)
		return 0;
	if (length==MAX_STRING_LENGTH)
		warning_i ("Instruction too long, extra characters ignored at line %d\n",line_number);
	return 1;
}

static int parse_unsigned_integer (LONG *integer_p)
{
	LONG integer;
	
	if (!is_digit_character (last_char))
		abc_parser_error_i ("Integer expected at line %d\n",line_number);
		
	integer=last_char-'0';
	last_char=getc (abc_file);
	
	while (is_digit_character (last_char)){
		integer*=10;
		integer+=last_char-'0';
		last_char=getc (abc_file);
	}
	
	skip_spaces_and_tabs();
	
	*integer_p=integer;
	
	return 1;
}

static int parse_integer (LONG *integer_p)
{
	LONG integer;
	int minus_sign;
	
	minus_sign=0;
	if (last_char=='+' || last_char=='-'){
		if (last_char=='-')
			minus_sign=!minus_sign;
		last_char=getc (abc_file);
	}
	
	if (!is_digit_character (last_char))
		abc_parser_error_i ("Integer expected at line %d\n",line_number);
		
	integer=last_char-'0';
	last_char=getc (abc_file);
	
	while (is_digit_character (last_char)){
		integer*=10;
		integer+=last_char-'0';
		last_char=getc (abc_file);
	}
	
	skip_spaces_and_tabs();
	
	if (minus_sign)
		integer=-integer;
	*integer_p=integer;
	
	return 1;
}

static int parse_hexadecimal_number (int *n_p)
{
	register int n;

	if (last_char!='0')
		return 0;
	last_char=getc (abc_file);
	
	if (last_char!='x')
		return 0;
	last_char=getc (abc_file);
	
	if (is_digit_character (last_char))
		n=last_char-'0';
	else if ((unsigned)((last_char & ~0x20)-'A')<(unsigned)6)
		n=(last_char & ~0x20)-('A'-10);
	else
		return 0;
	
	for (;;){
		last_char=getc (abc_file);
		
		if (is_digit_character (last_char))
			n=(n<<4)+last_char-'0';
		else if ((unsigned)((last_char & ~0x20)-'A')<(unsigned)6)
			n=(n<<4)+(last_char & ~0x20)-('A'-10);
		else {
			*n_p=n;
			return 1;
		}
	}
}

static int parse_0_or_1 (int *integer_p)
{
#if COMPATIBLE_DESC
	*integer_p=0;
	return 1;
#else
	int integer;
	
	if (!(last_char=='0' || last_char=='1'))
		abc_parser_error_i ("0 or 1 expected at line %d\n",line_number);
		
	integer=last_char-'0';
	
	last_char=getc (abc_file);
	skip_spaces_and_tabs();
	
	*integer_p=integer;
	
	return 1;
#endif
}

static char real_string [MAX_STRING_LENGTH+1];
static int real_string_length;

static void next_real_character()
{
	if (real_string_length<MAX_STRING_LENGTH)
		real_string[real_string_length++]=last_char;
	last_char=getc (abc_file);
}

static int parse_and_copy_digits()
{
	if (!is_digit_character (last_char))
		abc_parser_error_i ("Digit expected in real at line %d\n",line_number);

	next_real_character();
			
	while (is_digit_character (last_char))
		next_real_character();
	
	return 1;
}

static int parse_real (DOUBLE *real_p)
{	
	real_string_length=0;

	if (last_char=='+' || last_char=='-'){
		if (last_char=='-')
			real_string[real_string_length++]='-';
		last_char=getc (abc_file);
	}
	
	if (!parse_and_copy_digits())
		return 0;
	
	if (last_char=='.'){
		next_real_character();
		
		if (!parse_and_copy_digits())
			return 0;
	} else if (last_char!='e' && last_char!='E')
		abc_parser_error_i ("'.' or 'E' expected in real at line %d\n",line_number);

	if (last_char=='e' || last_char=='E'){
		next_real_character();
		
		if (last_char=='+' || last_char=='-'){
			if (last_char=='-')
				real_string[real_string_length++]='-';
			last_char=getc (abc_file);
		}
		
		if (!parse_and_copy_digits())
			return 0;
	}
	
	if (real_string_length==MAX_STRING_LENGTH)
		abc_parser_error_i ("Real denotation too long at line %d\n",line_number);

	real_string[real_string_length]='\0';
	
	skip_spaces_and_tabs();
	
	*real_p=atof (real_string);
	
	return 1;
}

static int parse_boolean (int *boolean_p)
{
	if (last_char=='F'){
		if ((last_char=getc (abc_file))=='A' && (last_char=getc (abc_file))=='L' &&
				(last_char=getc (abc_file))=='S' && (last_char=getc (abc_file))=='E'){
			last_char=getc (abc_file);
			skip_spaces_and_tabs();
			*boolean_p=0;
			return 1;
		}
	} else if (last_char=='T'){
		if ((last_char=getc (abc_file))=='R' && (last_char=getc (abc_file))=='U' &&
				(last_char=getc (abc_file))=='E'){
			last_char=getc (abc_file);
			skip_spaces_and_tabs();
			*boolean_p=1;
			return 1;
		}
	}
	abc_parser_error_i ("Boolean expected at line %d\n",line_number);
	return 0;
}

static int parse_string_character (char *c_p)
{
	if (last_char!='\\'){
		if (last_char==EOF)
			return 0;
		*c_p=last_char;
		last_char=getc (abc_file);
	} else {
		last_char=getc (abc_file);
		if (last_char>='0' && last_char<='7'){
			int c=last_char-'0';
			last_char=getc (abc_file);
			if (last_char>='0' && last_char<='7'){
				c=(c<<3)+(last_char-'0');
				last_char=getc (abc_file);
				if (last_char>='0' && last_char<='7'){
					c=(c<<3)+(last_char-'0');
					last_char=getc (abc_file);
				}
			}
			*c_p=c;
		} else {
			switch (last_char){
				case 'b':	*c_p='\b';	break;
				case 'f':	*c_p='\f';	break;
#if (defined (M68000) && !defined (SUN)) || defined (__MWERKS__) || defined (__MRC__)
				case 'n':	*c_p='\xd';	break;
				case 'r':	*c_p='\xa';	break;
#else
				case 'n':	*c_p='\n';	break;
				case 'r':	*c_p='\r';	break;
#endif
				case 't':	*c_p='\t';	break;
				case EOF:	return 0;
				default:	*c_p=last_char;
			}
			last_char=getc (abc_file);
		}
	}
	return 1;
}

static int parse_character (unsigned char *character_p)
{
	if (last_char!='\'')
		abc_parser_error_i ("Character expected at line %d\n",line_number);

	last_char=getc (abc_file);
	
	if (!parse_string_character ((char*)character_p))
		abc_parser_error_i ("Error in character at line %d\n",line_number);

	if (last_char!='\'')
		abc_parser_error_i ("Character not terminated at line %d\n",line_number);

	last_char=getc (abc_file);
	
	skip_spaces_and_tabs();
	
	return 1;
}

#define append_char(c) if (length<MAX_STRING_LENGTH) {label_string[length] = (c); ++length; };

static int try_parse_label (char *label_string)
{
	int length;
	
	length=0;
	if (is_alpha_num_character (last_char))
		do {
			switch (last_char){
				case '.':
					append_char ('_');
					append_char ('P');
					break;
				case '_':
					append_char ('_');
					append_char ('_');
					break;
				case '*':
					append_char ('_');
					append_char ('M');
					break;
				case '-':
					append_char ('_');
					append_char ('S');
					break;
				case '+':
					append_char ('_');
					append_char ('A');
					break;
				case '=':
					append_char ('_');
					append_char ('E');
					break;
				case '~':
					append_char ('_');
					append_char ('T');
					break;
				case '<':
					append_char ('_');
					append_char ('L');
					break;
				case '>':
					append_char ('_');
					append_char ('G');
					break;
				case '/':
					append_char ('_');
					append_char ('D');
					break;
				case '?':
					append_char ('_');
					append_char ('Q');
					break;
				case '#':
					append_char ('_');
					append_char ('H');
					break;
				case ':':
					append_char ('_');
					append_char ('C');
					break;				
				case '$':
					append_char ('_');
					append_char ('N');
					append_char ('D');
					break;
				case '^':
					append_char ('_');
					append_char ('N');
					append_char ('C');
					break;
				case '@':
					append_char ('_');
					append_char ('N');
					append_char ('T');
					break;
				case '&':
					append_char ('_');
					append_char ('N');
					append_char ('A');
					break;
				case '%':
					append_char ('_');
					append_char ('N');
					append_char ('P');
					break;
				case '\'':
					append_char ('_');
					append_char ('N');
					append_char ('S');
					break;
				case '\"':
					append_char ('_');
					append_char ('N');
					append_char ('Q');
					break;
				case '|':
					append_char ('_');
					append_char ('O');
					break;
				case '\\':
					append_char ('_');
					append_char ('N');
					append_char ('B');
					break;
				case '`':
					append_char ('_');
					append_char ('B');
					break;
				case '!':
					append_char ('_');
					append_char ('N');
					append_char ('E');
					break;
				case ';':
					append_char ('_');
					append_char ('I');
					break;					
				default:
					append_char (last_char);
			}
			last_char=getc (abc_file);
		} while (is_alpha_num_character (last_char));
	
	label_string[length]='\0';
	
	if (length==0)
		return 0;
	if (length==MAX_STRING_LENGTH)
		warning_i ("Label too long, extra characters ignored at line %d\n",line_number);
	return 1;
}

static void parse_label (char *label_string)
{
	if (!try_parse_label (label_string))
		abc_parser_error_i ("Label expected at line %d",line_number);
	skip_spaces_and_tabs();
}

static int try_parse_label_without_conversion (char *label_string)
{
	int length;
	
	length=0;
	if (is_alpha_num_character (last_char))
		do {
			append_char (last_char);
			last_char=getc (abc_file);
		} while (is_alpha_num_character (last_char));
	
	label_string[length]='\0';
	
	if (length==0)
		return 0;
	if (length==MAX_STRING_LENGTH)
		warning_i ("Label too long, extra characters ignored at line %d\n",line_number);
	return 1;
}

static void parse_label_without_conversion (char *label_string)
{
	if (!try_parse_label_without_conversion (label_string))
		abc_parser_error_i ("Label expected at line %d",line_number);
	skip_spaces_and_tabs();
}

static int parse_string (char *string,int *string_length_p)
{
	int length;
	
	if (last_char!='"')
		error_i ("String expected at line %d",line_number);

	last_char=getc (abc_file);
	
	length=0;
	
	while (last_char!='"'){
		char c;
		
		if (!parse_string_character (&c))
			abc_parser_error_i ("Error in string at line %d\n",line_number);

		if (length<MAX_STRING_LENGTH)
			string[length++]=c;
	}
	
	if (length==MAX_STRING_LENGTH)
		warning_i ("String too long, extra characters ignored at line %d\n",line_number);
	
	last_char=getc (abc_file);
	
	string[length]='\0';
	*string_length_p=length;
	
	skip_spaces_and_tabs();
	
	return 1;
}

static int parse_descriptor_string (char *string,int *string_length_p)
{
	int length;
	
	if (last_char!='"')
		error_i ("String expected at line %d",line_number);

	last_char=getc (abc_file);
	
	length=0;
	
	while (last_char!='"'){
		char c;
		
		if (last_char==EOF)
			abc_parser_error_i ("Error in string at line %d\n",line_number);
			
		c=last_char;
		last_char=getc (abc_file);

		if (length<MAX_STRING_LENGTH)
			string[length++]=c;
	}
	
	if (length==MAX_STRING_LENGTH)
		warning_i ("String too long, extra characters ignored at line %d\n",line_number);
	
	last_char=getc (abc_file);
	
	string[length]='\0';
	*string_length_p=length;
	
	skip_spaces_and_tabs();
	
	return 1;
}

static int parse_instruction (InstructionP instruction)
{
	instruction->instruction_code_function();
	return 1;
}

static int parse_instruction_a (InstructionP instruction)
{
	STRING a;
	
	parse_label (a);
	instruction->instruction_code_function (a);
	return 1;
}

#if defined (M68000) || defined (G_POWER)
static int parse_instruction_os_l (InstructionP instruction)
{
	STRING a,s;
	int length;

	if (! (last_char=='"' && parse_string (s,&length))){
		s[0]='\0';
		length = 0;
	}
	
	parse_instruction_string (a);
	instruction->instruction_code_function (s,length,a);
	return 1;
}
#endif

static int parse_instruction_b (InstructionP instruction)
{
	int b;
	
	if (!parse_boolean (&b))
		return 0;
	instruction->instruction_code_function (b);
	return 1;
}

static int parse_instruction_c (InstructionP instruction)
{
	unsigned char c;
	
	if (!parse_character (&c))
		return 0;
	instruction->instruction_code_function (c);
	return 1;
}

static int parse_instruction_i (InstructionP instruction)
{
	LONG i;
	
	if (!parse_integer (&i))
		return 0;
	instruction->instruction_code_function (i);
	return 1;
}

static int parse_instruction_l (InstructionP instruction)
{
	STRING a;
	
	parse_label_without_conversion (a);
	instruction->instruction_code_function (a);
	return 1;
}

static int parse_instruction_x (InstructionP instruction)
{
	int i;
	
	if (!parse_hexadecimal_number (&i))
		return 0;

	do {
		skip_spaces_and_tabs();
		instruction->instruction_code_function (i);
	} while (parse_hexadecimal_number (&i));

	return 1;
}

static int parse_instruction_n (InstructionP instruction)
{
	LONG n;
	
	if (!parse_unsigned_integer (&n))
		return 0;
	
	instruction->instruction_code_function ((int)n);
	return 1;
}

static int parse_instruction_on (InstructionP instruction)
{
	LONG n;
	
	if (!is_digit_character (last_char))
		n=-1;
	else
		if (!parse_unsigned_integer (&n))
			return 0;
	
	instruction->instruction_code_function ((int)n);
	return 1;
}

static int parse_instruction_r (InstructionP instruction)
{
	DOUBLE r;
	
	if (!parse_real (&r))
		return 0;
	instruction->instruction_code_function (r);
	return 1;
}

static int parse_instruction_s (InstructionP instruction)
{
	STRING s;
	int length;
	
	if (!parse_string (s,&length))
		return 0;
	instruction->instruction_code_function (s,length);
	return 1;
}

static int parse_instruction_l_s (InstructionP instruction)
{
	STRING l1,s1;
	int l;
	
	if (!parse_instruction_string (l1) || !parse_string (s1,&l))
		return 0;
	
	instruction->instruction_code_function (l1,s1,l);
	return 1;
}

static int parse_instruction_l_a_s (InstructionP instruction)
{
	STRING l1,l2,s1;
	int l;
	
	if (!parse_instruction_string (l1))
		return 0;
	parse_label (l2);
	if (!parse_string (s1,&l))
		return 0;
	
	instruction->instruction_code_function (l1,l2,s1,l);
	return 1;
}

static int parse_instruction_a_n (InstructionP instruction)
{
	STRING a;
	LONG n;
	
	parse_label (a);
	if (!parse_unsigned_integer (&n))
		return 0;
	instruction->instruction_code_function (a,(int)n);
	return 1;
}

static int parse_instruction_b_n (InstructionP instruction)
{
	int b;
	LONG n;
	
	if (!parse_boolean (&b) || !parse_unsigned_integer (&n))
		return 0;
	instruction->instruction_code_function (b,(int)n);
	return 1;
}

static int parse_instruction_c_n (InstructionP instruction)
{
	unsigned char c;
	LONG n;
	
	if (!parse_character (&c) || !parse_unsigned_integer (&n))
		return 0;
	instruction->instruction_code_function (c,(int)n);
	return 1;
}

static int parse_instruction_i_n (InstructionP instruction)
{
	LONG i,n;

	if (!parse_integer (&i) || !parse_unsigned_integer (&n))
		return 0;
	instruction->instruction_code_function (i,(int)n);
	return 1;
}

static int parse_instruction_n_n (InstructionP instruction)
{
	LONG n1,n2;
	
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2))
		return 0;
	instruction->instruction_code_function ((int)n1,(int)n2);
	return 1;
}

static int parse_instruction_r_n (InstructionP instruction)
{
	DOUBLE r;
	LONG n;
	
	if (!parse_real (&r) || !parse_unsigned_integer (&n))
		return 0;
	instruction->instruction_code_function (r,(int)n);
	return 1;
}

static int parse_instruction_s_n (InstructionP instruction)
{
	STRING s;
	int l;
	LONG n;
	
	if (!parse_string (s,&l) || !parse_unsigned_integer (&n))
		return 0;
	instruction->instruction_code_function (s,l,(int)n);
	return 1;
}

static int parse_instruction_a_n_n (InstructionP instruction)
{
	STRING a;
	LONG n1,n2;
	
	parse_label (a);
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2))
		return 0;
	instruction->instruction_code_function (a,(int)n1,(int)n2);
	return 1;
}

static void parse_bit_string (char *s_p)
{
	if (last_char!='0' && last_char!='1'){
		abc_parser_error_i ("0 or 1 expected at line %d\n",line_number);
		*s_p='\0';
	}

	do {
		*s_p++=last_char;
		last_char=getc (abc_file);
	} while (last_char=='0' || last_char=='1');

	*s_p='\0';
}

static int parse_instruction_a_n_n_b (InstructionP instruction)
{
	STRING a1,a2;
	LONG n1,n2;
	
	parse_label (a1);
	if (!parse_unsigned_integer (&n1))
		return 0;
	if (!parse_unsigned_integer (&n2))
		return 0;
	
	parse_bit_string (a2);
	
	skip_spaces_and_tabs();
	
	instruction->instruction_code_function (a1,(int)n1,(int)n2,a2);
	return 1;
}

static int parse_instruction_a_n_n_n_b (InstructionP instruction)
{
	STRING a1,a2;
	LONG n1,n2,n3;
	
	parse_label (a1);
	if (!parse_unsigned_integer (&n1))
		return 0;
	if (!parse_unsigned_integer (&n2))
		return 0;
	if (!parse_unsigned_integer (&n3))
		return 0;
	
	parse_bit_string (a2);
	
	skip_spaces_and_tabs();
	
	instruction->instruction_code_function (a1,(int)n1,(int)n2,(int)n3,a2);
	return 1;
}

static int parse_instruction_n_n_n (InstructionP instruction)
{
	LONG n1,n2,n3;
	
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || 
		!parse_unsigned_integer (&n3))
		return 0;
	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3);
	return 1;
}

static int parse_instruction_n_n_n_n (InstructionP instruction)
{
	LONG n1,n2,n3,n4;
	
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || 
		!parse_unsigned_integer (&n3) || !parse_unsigned_integer (&n4))
		return 0;
	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3,(int)n4);
	return 1;
}

static int parse_instruction_n_n_n_n_n (InstructionP instruction)
{
	LONG n1,n2,n3,n4,n5;
	
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || 
		!parse_unsigned_integer (&n3) || !parse_unsigned_integer (&n4) ||
		!parse_unsigned_integer (&n5))
		return 0;
	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3,(int)n4,(int)n5);
	return 1;
}

static int parse_instruction_n_n_n_n_n_n_n (InstructionP instruction)
{
	LONG n1,n2,n3,n4,n5,n6,n7;
	
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || 
		!parse_unsigned_integer (&n3) || !parse_unsigned_integer (&n4) ||
		!parse_unsigned_integer (&n5) || !parse_unsigned_integer (&n6) ||
		!parse_unsigned_integer (&n7))
		return 0;
	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3,(int)n4,(int)n5,(int)n6,(int)n7);
	return 1;
}

static int parse_instruction_a_n_n_n_n (InstructionP instruction)
{
	STRING a1;
	LONG n1,n2,n3,n4;
	
	parse_label (a1);
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || 
		!parse_unsigned_integer (&n3) || !parse_unsigned_integer (&n4))
		return 0;
	instruction->instruction_code_function (a1,(int)n1,(int)n2,(int)n3,(int)n4);
	return 1;
}

static int parse_instruction_a_n_n_n_n_n (InstructionP instruction)
{
	STRING a1;
	LONG n1,n2,n3,n4,n5;
	
	parse_label (a1);
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || 
		!parse_unsigned_integer (&n3) || !parse_unsigned_integer (&n4) ||
		!parse_unsigned_integer (&n5))
		return 0;
	instruction->instruction_code_function (a1,(int)n1,(int)n2,(int)n3,(int)n4,(int)n5);
	return 1;
}

static int parse_instruction_a_n_a (InstructionP instruction)
{
	STRING a1,a2;
	LONG n1;
	
	parse_label (a1);
	if (!parse_integer (&n1))
		return 0;
	parse_label (a2);
	instruction->instruction_code_function (a1,(int)n1,a2);
	return 1;
}

static int parse_instruction_a_n_a_n (InstructionP instruction)
{
	STRING a1,a2;
	LONG n1,n2;
	
	parse_label (a1);
	if (!parse_integer (&n1))
		return 0;
	parse_label (a2);
	if (!parse_unsigned_integer (&n2))
		return 0;
	instruction->instruction_code_function (a1,(int)n1,a2,(int)n2);
	return 1;
}

static int parse_instruction_a_n_a_n_b (InstructionP instruction)
{
	STRING a1,a2,a3;
	LONG n1,n2;
	
	parse_label (a1);
	if (!parse_integer (&n1))
		return 0;
	parse_label (a2);
	if (!parse_unsigned_integer (&n2))
		return 0;
	
	parse_bit_string (a3);

	instruction->instruction_code_function (a1,(int)n1,a2,(int)n2,a3);
	return 1;
}

static int parse_instruction_a_n_n_a (InstructionP instruction)
{
	STRING a1,a2;
	LONG n1,n2;
	
	parse_label (a1);
	if (!parse_integer (&n1))
		return 0;
	if (!parse_integer (&n2))
		return 0;
	parse_label (a2);
	instruction->instruction_code_function (a1,(int)n1,(int)n2,a2);
	return 1;
}

static int parse_instruction_a_n_n_a_n (InstructionP instruction)
{
	STRING a1,a2;
	LONG n1,n2,n3;
	
	parse_label (a1);
	if (!parse_integer (&n1))
		return 0;
	if (!parse_integer (&n2))
		return 0;
	parse_label (a2);
	if (!parse_unsigned_integer (&n3))
		return 0;

	instruction->instruction_code_function (a1,(int)n1,(int)n2,a2,(int)n3);
	return 1;
}

static int parse_instruction_a_n_n_a_n_b (InstructionP instruction)
{
	STRING a1,a2,a3;
	LONG n1,n2,n3;
	
	parse_label (a1);
	if (!parse_integer (&n1))
		return 0;
	if (!parse_integer (&n2))
		return 0;
	parse_label (a2);
	if (!parse_unsigned_integer (&n3))
		return 0;
	
	parse_bit_string (a3);

	instruction->instruction_code_function (a1,(int)n1,(int)n2,a2,(int)n3,a3);
	return 1;
}

static int parse_directive (InstructionP instruction)
{
	instruction->instruction_code_function();
	return 1;
}

static int parse_directive_n (InstructionP instruction)
{
	LONG n;
	STRING s;

	if (!parse_integer (&n))
		return 0;
	
	parse_label (s);

	skip_spaces_and_tabs();

	if (!is_newline (last_char)){
		STRING s2;

		parse_label (s2);
		instruction->instruction_code_function ((int)n,s,s2);
	} else
		instruction->instruction_code_function ((int)n,s,NULL);

	return 1;
}

static int parse_directive_nu (InstructionP instruction)
{
	LONG n1,n2;
	STRING s;

	if (!parse_integer (&n1) || !parse_integer (&n2))
		return 0;
	
	parse_label (s);

	skip_spaces_and_tabs();

	if (!is_newline (last_char)){
		STRING s2;

		parse_label (s2);
		instruction->instruction_code_function ((int)n1,(int)n2,s,s2);
	} else
		instruction->instruction_code_function ((int)n1,(int)n2,s,NULL);

	return 1;
}

static int parse_directive_n_n_n (InstructionP instruction)
{
	LONG n1,n2,n3;
	
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2)
	||  !parse_unsigned_integer (&n3))
		return 0;
		
	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3);
	return 1;
}

static int parse_directive_n_l (InstructionP instruction)
{
	LONG n;
	STRING s;

	if (!parse_unsigned_integer (&n))
		return 0;

	/* parse the options string as a label */
	parse_label (s);
	
	instruction->instruction_code_function ((int)n,(char *)s);
	return 1;
}

#define SMALL_VECTOR_SIZE 32
#define LOG_SMALL_VECTOR_SIZE 5
#define MASK_SMALL_VECTOR_SIZE 31

static int parse_directive_n_n_t (InstructionP instruction)
{
	LONG n1,n2;
	int i;
/* CHANGED 8-3-98
	static ULONG small_vector,*vector_p;
	static int vector_size=0;
*/	
	static ULONG small_vector;
	ULONG *vector_p;
	int vector_size=0;

	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2))
		return 0;
	
	vector_size=n2;
/* CHANGED 6-8-92: one extra */
	if (vector_size+1<=SMALL_VECTOR_SIZE)
		vector_p=&small_vector;
	else
		vector_p=(ULONG*)fast_memory_allocate 
					(((vector_size+1+SMALL_VECTOR_SIZE-1)>>LOG_SMALL_VECTOR_SIZE) * sizeof (ULONG));
	
	i=0;
	while (i!=n2){
		switch (last_char){
			case 'i':	case 'I':
			case 'b':	case 'B':
			case 'c':	case 'C':
			case 'p':	case 'P':
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] &= ~ (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;	
				break;
			case 'f':	case 'F':
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] &= ~ (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] &= ~ (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;
				break;
			case 'r':	case 'R':
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] |= (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] |= (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;	
				break;
			default:
				abc_parser_error_i ("B, C, F, I, P or R expected at line %d\n",line_number);
		}
		last_char=getc (abc_file);
		skip_spaces_and_tabs();
	}
	
	instruction->instruction_code_function ((int)n1,(int)n2,vector_p);
	return 1;
}

static int parse_directive_n_string (InstructionP instruction)
{
	STRING s;
	int l;

	if (!parse_string (s,&l))
		return 0;

	instruction->instruction_code_function (s,l);
	return 1;
}

static int parse_directive_depend (InstructionP instruction)
{
	STRING a;
	int l;
	
	if (!parse_string (a,&l))
		return 0;

	 /* skip optional last modification time */
	if (last_char=='"'){
		STRING t;
		int tl;
	
		parse_string (t,&tl);
	}

	instruction->instruction_code_function (a,l);

#if 0
	while (last_char==','){
		last_char=getc (abc_file);
		skip_spaces_and_tabs();
		if (!parse_string (a,&l))
			return 0;
		instruction->instruction_code_function (a,l);
	}
#endif

	return 1;
}

static int parse_directive_desc (InstructionP instruction)
{
	STRING a1,a2,a3,s;
	int l;
	LONG n;
	int f;

	parse_label (a1);
	parse_label (a2);
	parse_label (a3);
	if (!parse_unsigned_integer (&n) || !parse_0_or_1 (&f) || !parse_descriptor_string (s,&l))
		return 0;
	instruction->instruction_code_function (a1,a2,a3,(int)n,f,s,l);
	return 1;
}

static int parse_directive_descn (InstructionP instruction)
{
	STRING a1,a2,s;
	int l;
	LONG n;
	int f;

	parse_label (a1);
	parse_label (a2);
	if (!parse_unsigned_integer (&n) || !parse_0_or_1 (&f) || !parse_descriptor_string (s,&l))
		return 0;
	instruction->instruction_code_function (a1,a2,(int)n,f,s,l);
	return 1;
}

static int parse_directive_pb (InstructionP instruction)
{
	STRING s;
	int length;
	
	if (!parse_descriptor_string (s,&length))
		return 0;
	instruction->instruction_code_function (s,length);
	return 1;
}

static int parse_directive_record (InstructionP instruction)
{
	STRING a1,a2,s;
	LONG n1,n2;
	int l;
	
	parse_label (a1);
	parse_label (a2);
	if (!parse_unsigned_integer (&n1) || !parse_unsigned_integer (&n2) || !parse_descriptor_string (s,&l))
		return 0;

	instruction->instruction_code_function (a1,a2,(int)n1,(int)n2,s,l);
	return 1;
}

static int parse_directive_module (InstructionP instruction)
{
	STRING a,s;
	int l;

	parse_label (a);
	if (!parse_string (s,&l))
		return 0;
	
	 /* skip optional last modification time */
	if (last_char=='"'){
		STRING t;
		int tl;
	
		parse_string (t,&tl);
	}

	instruction->instruction_code_function (a,s,l);
	return 1;
}

static int parse_directive_string (InstructionP instruction)
{
	STRING a,s;
	int l;

	parse_label (a);
	if (!parse_string (s,&l))
		return 0;
	instruction->instruction_code_function (a,s,l);
	return 1;
}

static int parse_directive_label (InstructionP instruction)
{
	STRING s;
	
	parse_label (s);
	instruction->instruction_code_function (s);
	skip_spaces_and_tabs();
	
	return 1;
}

static int parse_directive_implab (InstructionP instruction)
{
	STRING s;
	
	parse_label (s);
	skip_spaces_and_tabs();

	if (!is_newline (last_char)){
		STRING s2;

		parse_label (s2);
		code_implab_node_entry (s,s2);
	} else
		code_implab (s);

	return 1;
}

static int parse_directive_implib_impobj (InstructionP instruction)
{
	STRING s;
	int l;

	if (!parse_string (s,&l))
		return 0;

	skip_spaces_and_tabs();
	return 1;
}

static int parse_directive_labels (InstructionP instruction)
{
	STRING s;
	
	parse_label (s);
	instruction->instruction_code_function (s);
	skip_spaces_and_tabs();
	
	while (!is_newline (last_char)){
		if (!try_parse_label (s)){
			abc_parser_error_i ("Label expected at line %d\n",line_number);
			return 0;
		}
		skip_spaces_and_tabs();
		instruction->instruction_code_function (s);
		skip_spaces_and_tabs();
	}

	return 1;
}

static int parse_instruction_in_or_out (InstructionP instruction)
{
	char parameters[256],*p;
	
	skip_spaces_and_tabs();
	
	p=parameters;
	while ((last_char & ~0x20)=='A' || (last_char & ~0x20)=='B'){
		long n;
		
		*p++=last_char;
		last_char=getc (abc_file);
		if (!parse_unsigned_integer (&n))
			return 0;

		*p++=n;
		
		if (last_char!=':')
			return 0;
		
		last_char=getc (abc_file);
		
		if ((last_char & ~0x20)=='R' || (last_char & ~0x20)=='I'){
			int c;
					
			c=last_char & ~0x20;

			last_char=getc (abc_file);
			if (!parse_unsigned_integer (&n))
				return 0;
			
			*p++=c;
			*p++=n>>8;
			*p++=n;
		}

		while ((last_char & ~0x20)=='O'){
#ifdef G_POWER
			long d_register;
#endif
			last_char=getc (abc_file);
			if (!parse_unsigned_integer (&n))
				return 0;
#ifdef G_POWER
			if ((last_char & ~0x20)!='D'){
				abc_parser_error_i ("D expected at line %d\n",line_number);
				return 0;
			}
			last_char=getc (abc_file);
			
			if (!parse_unsigned_integer (&d_register))
				return 0;
#endif

			*p++='O';
			*p++=n>>8;
			*p++=n;

#ifdef G_POWER
			*p++=d_register;
#endif
		}
		
		switch (last_char & ~0x20){
			case 'W':
			case 'L':
			case 'U':
			case 'Z':
#ifdef M68000
			case 'B':
#endif
				*p++=last_char & ~0x20;
				last_char=getc (abc_file);
				break;
			case 'S':
				*p++='S';
				last_char=getc (abc_file);
#ifdef G_POWER
				{
					long d_register;

					if ((last_char & ~0x20)!='D'){
						abc_parser_error_i ("D expected at line %d\n",line_number);
						return 0;
					}
					last_char=getc (abc_file);
					
					if (!parse_unsigned_integer (&d_register))
						return 0;

					*p++=d_register;
				}
#endif
				break;
			case 'C':
			{
				long d_register;
				int c;

				*p++='C';
				last_char=getc (abc_file);
				c=last_char & ~0x20;

				if (c!='D' && c!='S'){
					abc_parser_error_i ("D or S expected at line %d\n",line_number);
					return 0;
				}
				while (c=='D' || c=='S'){
					last_char=getc (abc_file);						
					*p++=c;
#ifdef G_POWER					
					if (!parse_unsigned_integer (&d_register))
						return 0;
					*p++=d_register;
#endif
					c=last_char & ~0x20;
				}
				break;
			}
			case 'D':
#ifdef G_POWER
			case 'B':
#endif
			{
				int n;
				
				*p++=last_char & ~0x20;

				last_char=getc (abc_file);
				n=last_char-'0';
				if ((unsigned) n > (unsigned) 7)
					return 0;
				
				*p++=n;
				last_char=getc (abc_file);
				break;
			}
			case 'A':
			{
				int n;
				
				last_char=getc (abc_file);
				n=last_char-'0';
				if ((unsigned) n > (unsigned) 2)
					return 0;

				*p++='A';
				*p++=n;
				last_char=getc (abc_file);
				break;
			}
			default:
				return 0;
		}
		
		skip_spaces_and_tabs();
	}
	*p=0;
	
	instruction->instruction_code_function (parameters);
	
	return 1;
}

#define IN_NAME_TABLE_SIZE 500

struct in_name {
	struct in_name *		in_name_next;
	InstructionP 	in_name_instruction;
};

static struct in_name *in_name_table;

static int instruction_hash (char *s)
{
	int h=0;
	while (*s!='\0')
		h += *s++;
	return h;
}

static void put_instruction_name
#ifdef __cplusplus
	(char *instruction_name,int (*instruction_parse_function)(struct instruction*),void (*instruction_code_function)(...))
#else
	(char *instruction_name,int (*instruction_parse_function)(),void (*instruction_code_function)())
#endif
{
	InstructionP instruction;
	struct in_name *in_name;
	
	instruction=(InstructionP )init_memory_allocate (sizeof (struct instruction));
	instruction->instruction_name=instruction_name;
	instruction->instruction_parse_function=instruction_parse_function;
	instruction->instruction_code_function=instruction_code_function;

	in_name=&in_name_table
		[instruction_hash (instruction->instruction_name) % IN_NAME_TABLE_SIZE];
	
	if (in_name->in_name_instruction!=NULL){
		struct in_name *in_name_link;
		
		in_name_link=in_name;
		while (in_name_link->in_name_next!=NULL)
			in_name_link=in_name_link->in_name_next;
		
		for (in_name=in_name_table; in_name<&in_name_table [IN_NAME_TABLE_SIZE]; ++in_name)
			if (in_name->in_name_instruction==NULL)
				break;
				
		if (in_name==&in_name_table [IN_NAME_TABLE_SIZE])
			error ("Not enough entries in instruction hash table");
			
		in_name_link->in_name_next=in_name;
	}
	
	in_name->in_name_instruction=instruction;
}

static void put_instructions_in_table (void)
{
	put_instruction_name ("acosR",			parse_instruction,			code_acosR );
	put_instruction_name ("add_args",		parse_instruction_n_n_n,	code_add_args );
	put_instruction_name ("addI",			parse_instruction,			code_addI );
	put_instruction_name ("addR",			parse_instruction,			code_addR );
	put_instruction_name ("andB",			parse_instruction,			code_andB );
	put_instruction_name ("and%",			parse_instruction,			code_and );
	put_instruction_name ("asinR",			parse_instruction,			code_asinR );
	put_instruction_name ("atanR",			parse_instruction,			code_atanR );
	put_instruction_name ("build",			parse_instruction_a_n_a,	code_build );
	put_instruction_name ("buildB",			parse_instruction_b,		code_buildB );
	put_instruction_name ("buildC",			parse_instruction_c,		code_buildC );
	put_instruction_name ("buildI",			parse_instruction_i,		code_buildI );
	put_instruction_name ("buildR",			parse_instruction_r,		code_buildR );
	put_instruction_name ("buildAC",		parse_instruction_s,		code_buildAC );
	put_instruction_name ("buildB_b",		parse_instruction_n,		code_buildB_b );
	put_instruction_name ("buildC_b",		parse_instruction_n,		code_buildC_b );
	put_instruction_name ("buildF_b",		parse_instruction_n,		code_buildF_b );
	put_instruction_name ("buildI_b",		parse_instruction_n,		code_buildI_b );
	put_instruction_name ("buildR_b",		parse_instruction_n,		code_buildR_b );
	put_instruction_name ("buildh",			parse_instruction_a_n,		code_buildh );
	put_instruction_name ("build_r",		parse_instruction_a_n_n_n_n,code_build_r );
	put_instruction_name ("build_u",		parse_instruction_a_n_n_a,	code_build_u );
	put_instruction_name ("catS",			parse_instruction_n_n_n,	code_catS );
#if defined (M68000) || defined (G_POWER)
	put_instruction_name ("call",			parse_instruction_os_l,		code_call );
#endif
	put_instruction_name ("ccall",			parse_instruction_l_s,		code_ccall );
	put_instruction_name ("centry",			parse_instruction_l_a_s,	code_centry );
	put_instruction_name ("cmpS",			parse_instruction_n_n,		code_cmpS );
	put_instruction_name ("CtoAC",			parse_instruction,			code_CtoAC );
	put_instruction_name ("copy_graph",		parse_instruction_n,		code_copy_graph );
	put_instruction_name ("cosR",			parse_instruction,			code_cosR );
	put_instruction_name ("code_channelP",	parse_instruction,			code_channelP );
	put_instruction_name ("create",			parse_instruction_on,		code_create );
	put_instruction_name ("create_array",	parse_instruction_a_n_n,	code_create_array );
	put_instruction_name ("create_array_",	parse_instruction_a_n_n,	code_create_array_ );
	put_instruction_name ("create_channel",	parse_instruction_a,		code_create_channel );
	put_instruction_name ("currentP",		parse_instruction,			code_currentP );
	put_instruction_name ("CtoI",			parse_instruction,			code_CtoI );
	put_instruction_name ("decI",			parse_instruction,			code_decI );
	put_instruction_name ("del_args",		parse_instruction_n_n_n,	code_del_args );
	put_instruction_name ("divI",			parse_instruction,			code_divI );
	put_instruction_name ("divR",			parse_instruction,			code_divR );
	put_instruction_name ("entierR",		parse_instruction,			code_entierR );
	put_instruction_name ("eqB",			parse_instruction,			code_eqB );
	put_instruction_name ("eqB_a",			parse_instruction_b_n,		code_eqB_a );
	put_instruction_name ("eqB_b",			parse_instruction_b_n,		code_eqB_b );
	put_instruction_name ("eqC",			parse_instruction,			code_eqC );
	put_instruction_name ("eqC_a",			parse_instruction_c_n,		code_eqC_a );
	put_instruction_name ("eqC_b",			parse_instruction_c_n,		code_eqC_b );
	put_instruction_name ("eqD_b",			parse_instruction_a_n,		code_eqD_b );
	put_instruction_name ("eqI",			parse_instruction,			code_eqI );
	put_instruction_name ("eqI_a",			parse_instruction_i_n,		code_eqI_a );
	put_instruction_name ("eqI_b",			parse_instruction_i_n,		code_eqI_b );
	put_instruction_name ("eqR",			parse_instruction,			code_eqR );
	put_instruction_name ("eqR_a",			parse_instruction_r_n,		code_eqR_a );
	put_instruction_name ("eqR_b",			parse_instruction_r_n,		code_eqR_b );
	put_instruction_name ("eqAC_a",			parse_instruction_s,		code_eqAC_a );
	put_instruction_name ("eq_desc",		parse_instruction_a_n_n,	code_eq_desc );
	put_instruction_name ("eq_desc_b",		parse_instruction_a_n,		code_eq_desc_b );
	put_instruction_name ("eq_nulldesc",	parse_instruction_a_n,		code_eq_nulldesc );
	put_instruction_name ("eq_symbol",		parse_instruction_n_n,		code_eq_symbol );
	put_instruction_name ("exit_false",		parse_instruction_a,		code_exit_false );
	put_instruction_name ("expR",			parse_instruction,			code_expR );
	put_instruction_name ("fill",			parse_instruction_a_n_a_n,	code_fill );
	put_instruction_name ("fill1",			parse_instruction_a_n_n_b,	code_fill1 );
	put_instruction_name ("fill2",			parse_instruction_a_n_n_b,	code_fill2 );
	put_instruction_name ("fill3",			parse_instruction_a_n_n_b,	code_fill3 );
	put_instruction_name ("fill1_r",		parse_instruction_a_n_n_n_b,code_fill1_r );
	put_instruction_name ("fill2_r",		parse_instruction_a_n_n_n_b,code_fill2_r );
	put_instruction_name ("fill3_r",		parse_instruction_a_n_n_n_b,code_fill3_r );
	put_instruction_name ("fillcaf",		parse_instruction_a_n_n,	code_fillcaf );
	put_instruction_name ("fillcp",			parse_instruction_a_n_a_n_b,code_fillcp );
	put_instruction_name ("fillcp_u",		parse_instruction_a_n_n_a_n_b,code_fillcp_u );
	put_instruction_name ("fill_u",			parse_instruction_a_n_n_a_n,code_fill_u );
	put_instruction_name ("fillh",			parse_instruction_a_n_n,	code_fillh );
	put_instruction_name ("fillB",			parse_instruction_b_n,		code_fillB );
	put_instruction_name ("fillB_b",		parse_instruction_n_n,		code_fillB_b );
	put_instruction_name ("fillC",			parse_instruction_c_n,		code_fillC );
	put_instruction_name ("fillC_b",		parse_instruction_n_n,		code_fillC_b );
	put_instruction_name ("fillF_b",		parse_instruction_n_n,		code_fillF_b );
	put_instruction_name ("fillI",			parse_instruction_i_n,		code_fillI );
	put_instruction_name ("fillI_b",		parse_instruction_n_n,		code_fillI_b );
	put_instruction_name ("fillR",			parse_instruction_r_n,		code_fillR );
	put_instruction_name ("fillR_b",		parse_instruction_n_n,		code_fillR_b );
	put_instruction_name ("fill_a",			parse_instruction_n_n,		code_fill_a );
	put_instruction_name ("fill_r",			parse_instruction_a_n_n_n_n_n, code_fill_r );
	put_instruction_name ("getWL",			parse_instruction_n,		code_dummy );
	put_instruction_name ("get_desc_arity",	parse_instruction_n,		code_get_desc_arity );
	put_instruction_name ("get_node_arity",	parse_instruction_n,		code_get_node_arity );
	put_instruction_name ("gtC",			parse_instruction,			code_gtC );
	put_instruction_name ("gtI",			parse_instruction,			code_gtI );
	put_instruction_name ("gtR",			parse_instruction,			code_gtR );
	put_instruction_name ("halt",			parse_instruction,			code_halt );
	put_instruction_name ("in",				parse_instruction_in_or_out, code_in );
	put_instruction_name ("incI",			parse_instruction,			code_incI );
#if defined (I486) || defined (sparc)
	put_instruction_name ("instruction",	parse_instruction_i, 		code_instruction );
#else
	put_instruction_name ("instruction",	parse_instruction_x, 		code_instruction );
#endif
	put_instruction_name ("is_record",		parse_instruction_n,		code_is_record );
	put_instruction_name ("ItoC",			parse_instruction,			code_ItoC );
	put_instruction_name ("ItoP",			parse_instruction,			code_ItoP );
	put_instruction_name ("ItoR",			parse_instruction,			code_ItoR );
	put_instruction_name ("jmp",			parse_instruction_a,		code_jmp );
	put_instruction_name ("jmp_eval",		parse_instruction,			code_jmp_eval );
	put_instruction_name ("jmp_eval_upd",	parse_instruction,			code_jmp_eval_upd );
	put_instruction_name ("jmp_false",		parse_instruction_a,		code_jmp_false );
	put_instruction_name ("jmp_true",		parse_instruction_a,		code_jmp_true );
	put_instruction_name ("jsr",			parse_instruction_a,		code_jsr );
	put_instruction_name ("jsr_eval",		parse_instruction_n,		code_jsr_eval );
	put_instruction_name ("lnR",			parse_instruction,			code_lnR );
	put_instruction_name ("log10R",			parse_instruction,			code_log10R );
	put_instruction_name ("ltC",			parse_instruction,			code_ltC );
	put_instruction_name ("ltI",			parse_instruction,			code_ltI );
	put_instruction_name ("ltR",			parse_instruction,			code_ltR );
	put_instruction_name ("modI",			parse_instruction,			code_remI );
	put_instruction_name ("mulI",			parse_instruction,			code_mulI );
	put_instruction_name ("mulR",			parse_instruction,			code_mulR );
	put_instruction_name ("negR",			parse_instruction,			code_negR );
	put_instruction_name ("new_ext_reducer",parse_instruction_a_n,		code_new_ext_reducer );
	put_instruction_name ("new_int_reducer",parse_instruction_a_n,		code_new_int_reducer );
	put_instruction_name ("newP",			parse_instruction,			code_newP );
#ifdef __MRC__
	put_instructions_in_table2();
}
static void put_instructions_in_table2 (void)
{
#endif
	put_instruction_name ("no_op",			parse_instruction,			code_no_op );
	put_instruction_name ("notB",			parse_instruction,			code_notB );
	put_instruction_name ("not%",			parse_instruction,			code_not );
	put_instruction_name ("orB",			parse_instruction,			code_orB );
	put_instruction_name ("or%",			parse_instruction,			code_or );
	put_instruction_name ("out",			parse_instruction_in_or_out, code_out );
	put_instruction_name ("pop_a",			parse_instruction_n,		code_pop_a );
	put_instruction_name ("pop_b",			parse_instruction_n,		code_pop_b );
	put_instruction_name ("powR",			parse_instruction,			code_powR );
	put_instruction_name ("print",			parse_instruction_s,		code_print );
	put_instruction_name ("printD",			parse_instruction,			code_printD );
	put_instruction_name ("print_char",		parse_instruction,			code_print_char );
	put_instruction_name ("print_int",		parse_instruction,			code_print_int );
	put_instruction_name ("print_real",		parse_instruction,			code_print_real );
#if 0
	put_instruction_name ("print_r_arg",	parse_instruction_n,		code_print_r_arg );
#endif
	put_instruction_name ("print_sc",		parse_instruction_s,		code_print_sc );
	put_instruction_name ("print_symbol",	parse_instruction_n,		code_print_symbol );
	put_instruction_name ("print_symbol_sc",parse_instruction_n,		code_print_symbol_sc );
	put_instruction_name ("pushcaf",		parse_instruction_a_n_n,	code_pushcaf );
#ifdef FINALIZERS
	put_instruction_name ("push_finalizers",parse_instruction,			code_push_finalizers );
#endif
	put_instruction_name ("pushA_a",		parse_instruction_n,		code_pushA_a );
	put_instruction_name ("pushB",			parse_instruction_b,		code_pushB );
	put_instruction_name ("pushB_a",		parse_instruction_n,		code_pushB_a );
	put_instruction_name ("pushC",			parse_instruction_c,		code_pushC );
	put_instruction_name ("pushC_a",		parse_instruction_n,		code_pushC_a );
	put_instruction_name ("pushD",			parse_instruction_a,		code_pushD );
	put_instruction_name ("pushD_a",		parse_instruction_n,		code_pushD_a );
	put_instruction_name ("pushF_a",		parse_instruction_n,		code_pushF_a );
	put_instruction_name ("pushI",			parse_instruction_i,		code_pushI );
	put_instruction_name ("pushI_a",		parse_instruction_n,		code_pushI_a );
	put_instruction_name ("pushL",			parse_instruction_l,		code_pushL );
	put_instruction_name ("pushR",			parse_instruction_r,		code_pushR );
	put_instruction_name ("pushR_a",		parse_instruction_n,		code_pushR_a );
	put_instruction_name ("pushzs",			parse_instruction_s,		code_pushzs );
	put_instruction_name ("push_a",			parse_instruction_n,		code_push_a );
	put_instruction_name ("push_b",			parse_instruction_n,		code_push_b );
	put_instruction_name ("push_a_b",		parse_instruction_n,		code_push_a_b );	
	put_instruction_name ("push_arg",		parse_instruction_n_n_n,	code_push_arg );
	put_instruction_name ("push_arg_b",		parse_instruction_n,		code_push_arg_b );
	put_instruction_name ("push_args",		parse_instruction_n_n_n,	code_push_args );
	put_instruction_name ("push_args_u",	parse_instruction_n_n_n,	code_push_args_u );
	put_instruction_name ("push_array",		parse_instruction_n,		code_pushA_a );
	put_instruction_name ("push_arraysize",	parse_instruction_a_n_n,	code_push_arraysize );
	put_instruction_name ("push_b_a",		parse_instruction_n,		code_push_b_a );	
	put_instruction_name ("push_node",		parse_instruction_a_n,		code_push_node );
	put_instruction_name ("push_node_u",	parse_instruction_a_n_n,	code_push_node_u );
	put_instruction_name ("push_a_r_args",	parse_instruction,			code_push_a_r_args );
	put_instruction_name ("push_t_r_a",		parse_instruction_n,		code_push_t_r_a );
	put_instruction_name ("push_t_r_args",	parse_instruction,			code_push_t_r_args );
	put_instruction_name ("push_r_args",	parse_instruction_n_n_n,	code_push_r_args );
	put_instruction_name ("push_r_args_a",	parse_instruction_n_n_n_n_n, code_push_r_args_a );
	put_instruction_name ("push_r_args_b",	parse_instruction_n_n_n_n_n, code_push_r_args_b );
	put_instruction_name ("push_r_args_u",	parse_instruction_n_n_n,	code_push_r_args_u );
	put_instruction_name ("push_r_arg_t",	parse_instruction,			code_push_r_arg_t );
	put_instruction_name ("push_r_arg_u",	parse_instruction_n_n_n_n_n_n_n, code_push_r_arg_u );
	put_instruction_name ("push_wl_args",	parse_instruction_n_n_n,	code_push_args );
	put_instruction_name ("putWL",			parse_instruction_n,		code_dummy );
	put_instruction_name ("randomP",		parse_instruction,			code_randomP );
	put_instruction_name ("release",		parse_instruction,			code_release );
	put_instruction_name ("remI",			parse_instruction,			code_remI );
	put_instruction_name ("replace",		parse_instruction_a_n_n,	code_replace );
	put_instruction_name ("repl_arg",		parse_instruction_n_n,		code_repl_arg );
	put_instruction_name ("repl_args",		parse_instruction_n_n,		code_repl_args );
	put_instruction_name ("repl_args_b",	parse_instruction,			code_repl_args_b );
	put_instruction_name ("repl_r_args",	parse_instruction_n_n,		code_repl_r_args );
	put_instruction_name ("repl_r_args_a",	parse_instruction_n_n_n_n,	code_repl_r_args_a );
	put_instruction_name ("rtn",			parse_instruction,			code_rtn );
	put_instruction_name ("RtoI",			parse_instruction,			code_RtoI );
	put_instruction_name ("select",			parse_instruction_a_n_n,	code_select );
	put_instruction_name ("send_graph",		parse_instruction_a_n_n,	code_send_graph );
	put_instruction_name ("send_request",	parse_instruction_n,		code_send_request );
	put_instruction_name ("set_continue",	parse_instruction_n,		code_set_continue );
	put_instruction_name ("set_defer",		parse_instruction_n,		code_set_defer );
	put_instruction_name ("set_entry",		parse_instruction_a_n,		code_set_entry );
#ifdef FINALIZERS
	put_instruction_name ("set_finalizers",	parse_instruction,			code_set_finalizers );
#endif
	put_instruction_name ("setwait",		parse_instruction_n,		code_dummy );
	put_instruction_name ("shiftl%",		parse_instruction,			code_shiftl );
	put_instruction_name ("shiftr%",		parse_instruction,			code_shiftr );
	put_instruction_name ("shiftrU",		parse_instruction,			code_shiftrU );
	put_instruction_name ("sinR",			parse_instruction,			code_sinR );
	put_instruction_name ("sliceS",			parse_instruction_n_n,		code_sliceS );
	put_instruction_name ("sqrtR",			parse_instruction,			code_sqrtR );
	put_instruction_name ("stop_reducer",	parse_instruction,			code_stop_reducer );
	put_instruction_name ("subI",			parse_instruction,			code_subI );
#ifndef M68000
	put_instruction_name ("addIo",			parse_instruction,			code_addIo );
	put_instruction_name ("mulIo",			parse_instruction,			code_mulIo );
	put_instruction_name ("subIo",			parse_instruction,			code_subIo );
#endif
	put_instruction_name ("subR",			parse_instruction,			code_subR );
	put_instruction_name ("suspend",		parse_instruction,			code_suspend );
	put_instruction_name ("tanR",			parse_instruction,			code_tanR );
	put_instruction_name ("testcaf",		parse_instruction_a,		code_testcaf );
	put_instruction_name ("update_a",		parse_instruction_n_n,		code_update_a );
	put_instruction_name ("updatepop_a",	parse_instruction_n_n,		code_updatepop_a );
	put_instruction_name ("update_b",		parse_instruction_n_n,		code_update_b );
	put_instruction_name ("updatepop_b",	parse_instruction_n_n,		code_updatepop_b );
	put_instruction_name ("updateS",		parse_instruction_n_n,		code_updateS );
	put_instruction_name ("update",			parse_instruction_a_n_n,	code_update );
	put_instruction_name ("xor%",			parse_instruction,			code_xor );
	put_instruction_name (".caf",			parse_instruction_a_n_n,	code_caf );
	put_instruction_name (".code",			parse_directive_n_n_n,		code_dummy	);
	put_instruction_name (".comp",			parse_directive_n_l,		code_comp	);
	put_instruction_name (".d",				parse_directive_n_n_t,		code_d );
	put_instruction_name (".depend",		parse_directive_depend,		code_depend );
	put_instruction_name (".desc",			parse_directive_desc,		code_desc );
	put_instruction_name (".descn",			parse_directive_descn,		code_descn );
	put_instruction_name (".descexp",		parse_directive_desc,		code_descexp );
	put_instruction_name (".end",			parse_directive,			code_dummy );
	put_instruction_name (".endinfo",		parse_directive,			code_dummy	);
	put_instruction_name (".export",		parse_directive_labels,		code_export );
	put_instruction_name (".keep",			parse_instruction_n_n,		code_keep );
	put_instruction_name (".inline",		parse_directive_label,		code_dummy );
	put_instruction_name (".impdesc",		parse_directive_label,		code_impdesc );
	put_instruction_name (".implab",		parse_directive_implab,		code_implab );
	put_instruction_name (".implib",		parse_directive_implib_impobj,	code_dummy );
	put_instruction_name (".impobj",		parse_directive_implib_impobj,	code_dummy );
	put_instruction_name (".module",		parse_directive_module,		code_module );
	put_instruction_name (".n",				parse_directive_n,			code_n );
	put_instruction_name (".nu",			parse_directive_nu,			code_nu );
	put_instruction_name (".newlocallabel",	parse_directive_label,		code_newlocallabel );
	put_instruction_name (".n_string",		parse_directive_n_string,	code_n_string );
	put_instruction_name (".o",				parse_directive_n_n_t,		code_o );

	put_instruction_name (".pb",			parse_directive_pb,			IF_PROFILING (code_pb,code_dummy) );
	put_instruction_name (".pd",			parse_directive,			IF_PROFILING (code_pd,code_dummy) );
	put_instruction_name (".pe",			parse_directive,			IF_PROFILING (code_pe,code_dummy) );
	put_instruction_name (".pl",			parse_directive,			IF_PROFILING (code_pl,code_dummy) );
	put_instruction_name (".pld",			parse_directive,			IF_PROFILING (code_pld,code_dummy) );
	put_instruction_name (".pn",			parse_directive,			IF_PROFILING (code_pn,code_dummy) );
	put_instruction_name (".pt",			parse_directive,			IF_PROFILING (code_pt,code_dummy) );

	put_instruction_name (".record",		parse_directive_record,		code_record );
	put_instruction_name (".start",			parse_directive_label,		code_start );
	put_instruction_name (".string",		parse_directive_string,		code_string );
}

static void init_instruction_name_table (void)
{
	int i;
	struct in_name *local_in_name_table;
	
	local_in_name_table=(struct in_name*)init_memory_allocate (sizeof (struct in_name)*IN_NAME_TABLE_SIZE);
	
	for (i=0; i<IN_NAME_TABLE_SIZE; ++i){
		local_in_name_table [i].in_name_next=NULL;
		local_in_name_table [i].in_name_instruction=NULL;
	}
	
	in_name_table=local_in_name_table;
	
	put_instructions_in_table();
}

static InstructionP get_instruction (char *s)
{
	struct in_name *in_name;
	
	in_name=&in_name_table [instruction_hash (s) % IN_NAME_TABLE_SIZE];
	
	while (in_name!=NULL && in_name->in_name_instruction!=NULL){
		if (strcmp (in_name->in_name_instruction->instruction_name,s)==0)
			return in_name->in_name_instruction;
			
		in_name=in_name->in_name_next;		
	}
	return NULL;
}

#if !BINARY_ABC
static int parse_line (void)
{
	InstructionP instruction;
	STRING s;
	
	if (!skip_spaces_and_tabs() 
		&& last_char!='.' && last_char !='|' && last_char!='\xd' && last_char!='\xa' && last_char!=EOF)
	{
		parse_label (s);
		skip_spaces_and_tabs();
		code_label (s);
	}

	if (last_char=='|'){
		skip_to_end_of_line();
		if (last_char==EOF)
			return 0;

		if (last_char=='\xd'){
			last_char=getc (abc_file);
			if (last_char=='\xa')
				last_char=getc (abc_file);
			return 1;
		}
		
		last_char=getc (abc_file);
		return 1;
	}

	if (!parse_instruction_string (s)){
		if (last_char=='|')
			skip_to_end_of_line();
		if (last_char==EOF)
			return 0;
		if (last_char=='\xd'){
			last_char=getc (abc_file);
			if (last_char=='\xa')
				last_char=getc (abc_file);
			return 1;
		} else if (last_char=='\xa'){
			last_char=getc (abc_file);
			return 1;
		} else
			abc_parser_error_i ("Instruction expected at line %d\n",line_number);
	}
	
	instruction=get_instruction (s);
	if (instruction==NULL)
		abc_parser_error_si ("Invalid instruction %s at line %d\n",s,line_number);

	/* printf ("%s\n",instruction->instruction_name); */
	
	if (!instruction->instruction_parse_function (instruction))
		return 0;
	
	skip_spaces_and_tabs();
	
	if (last_char=='|')
		skip_to_end_of_line();
	
	if (last_char=='\xd'){
		last_char=getc (abc_file);
		if (last_char=='\xa')
			last_char=getc (abc_file);
	} else if (last_char=='\xa')
		last_char=getc (abc_file);
	else if (last_char!=EOF)
		abc_parser_error_i ("Too many arguments for instruction at line %d\n",line_number);
	
	return 1;	
}
#else
static int parse_line_without_newline (void)
{
	InstructionP instruction;
	STRING s;
	
	if (!skip_spaces_and_tabs() 
		&& last_char!='.' && last_char !='|' && last_char!='\xd' && last_char!='\xa' && last_char!=EOF)
	{
		parse_label (s);
		skip_spaces_and_tabs();
		code_label (s);
	}

	if (last_char=='|'){
		skip_to_end_of_line();
		if (last_char==EOF)
			return 0;

		return 1;
	}

	if (!parse_instruction_string (s)){
		if (last_char=='|')
			skip_to_end_of_line();
		if (last_char==EOF)
			return 0;
		return 1;
	}
	
	instruction=get_instruction (s);
	if (instruction==NULL)
		abc_parser_error_si ("Invalid instruction %s at line %d\n",s,line_number);

	/* printf ("%s\n",instruction->instruction_name); */
	
	if (!instruction->instruction_parse_function (instruction))
		return 0;
	
	skip_spaces_and_tabs();
	
	if (last_char=='|')
		skip_to_end_of_line();
		
	return 1;	
}

struct binary_instruction {
#if defined  (THINK_C) || defined (__cplusplus)
	int (*	instruction_parse_function)(struct binary_instruction *);
	void (*	instruction_code_function)(...);
#else
	int (*	instruction_parse_function)();
	void (*	instruction_code_function)();
#endif
};

typedef struct binary_instruction *BInstructionP;

static int parse_binary_unsigned_integer (LONG *integer_p)
{
	LONG integer;
	
	if (last_char==EOF)
		abc_parser_error_i ("Integer expected at line %d\n",line_number);
	
	if (last_char<128)
		integer=last_char-64;
	else {
		int shift;
		
		integer=last_char & 127;
		shift=7;
		
		last_char=getc (abc_file);
		if (last_char==EOF)
			abc_parser_error_i ("Integer expected at line %d\n",line_number);
		
		while (last_char>=128){
			integer+=(last_char & 127)<<shift;
			shift+=7;
	
			last_char=getc (abc_file);
			if (last_char==EOF)
				abc_parser_error_i ("Integer expected at line %d\n",line_number);
		}
	
		integer+=(last_char-64) << shift;
	}

	last_char=getc (abc_file);
	*integer_p=integer;
	
	return 1;
}

static void parse_binary_label (char *label_string)
{
	if (!try_parse_label (label_string))
		abc_parser_error_i ("Label expected at line %d",line_number);

	if (last_char==' ' || last_char=='\t')
		last_char=getc (abc_file);
}

static int parse_binary_instruction_ (BInstructionP instruction)
{
	instruction->instruction_code_function ();
	return 1;
}

static int parse_binary_instruction_a (BInstructionP instruction)
{
	STRING a1;
	
	parse_binary_label (a1);
	instruction->instruction_code_function (a1);
	return 1;
}

static int parse_binary_instruction_an (BInstructionP instruction)
{
	STRING a1;
	LONG n1;
	
	parse_binary_label (a1);
	if (!parse_binary_unsigned_integer (&n1))
		return 0;
	instruction->instruction_code_function (a1,(int)n1);
	return 1;
}

static int parse_binary_instruction_ana (BInstructionP instruction)
{
	STRING a1,a2;
	LONG n1;
	
	parse_binary_label (a1);
	if (!parse_binary_unsigned_integer (&n1))
		return 0;
	parse_binary_label (a2);
	instruction->instruction_code_function (a1,(int)n1,a2);
	return 1;
}

static int parse_binary_instruction_anan (BInstructionP instruction)
{
	STRING a1,a2;
	LONG n1,n2;
	
	parse_binary_label (a1);
	if (!parse_binary_unsigned_integer (&n1))
		return 0;
	parse_binary_label (a2);
	if (!parse_binary_unsigned_integer (&n2))
		return 0;
	instruction->instruction_code_function (a1,(int)n1,a2,(int)n2);
	return 1;
}

static int parse_binary_instruction_ann (BInstructionP instruction)
{
	STRING a1;
	LONG n1,n2;
	
	parse_binary_label (a1);
	if (!parse_binary_unsigned_integer (&n1))
		return 0;
	if (!parse_binary_unsigned_integer (&n2))
		return 0;
	instruction->instruction_code_function (a1,(int)n1,(int)n2);
	return 1;
}

static int parse_binary_instruction_i (BInstructionP instruction)
{
	LONG i1;
	
	if (!parse_binary_unsigned_integer (&i1))
		return 0;

	instruction->instruction_code_function (i1);
	return 1;
}

static int parse_binary_instruction_in (BInstructionP instruction)
{
	LONG i1,n1;
	
	if (!parse_binary_unsigned_integer (&i1) || !parse_binary_unsigned_integer (&n1))
		return 0;

	instruction->instruction_code_function (i1,(int)n1);
	return 1;
}

static int parse_binary_instruction_n (BInstructionP instruction)
{
	LONG n1;
	
	if (!parse_binary_unsigned_integer (&n1))
		return 0;

	instruction->instruction_code_function ((int)n1);
	return 1;
}

static int parse_binary_instruction_nn (BInstructionP instruction)
{
	LONG n1,n2;
	
	if (!parse_binary_unsigned_integer (&n1) || !parse_binary_unsigned_integer (&n2))
		return 0;

	instruction->instruction_code_function ((int)n1,(int)n2);
	return 1;
}

static int parse_binary_instruction_nnn (BInstructionP instruction)
{
	LONG n1,n2,n3;
	
	if (!parse_binary_unsigned_integer (&n1) || !parse_binary_unsigned_integer (&n2) || !parse_binary_unsigned_integer (&n3))
		return 0;

	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3);
	return 1;
}

static int parse_binary_instruction_nnnn (BInstructionP instruction)
{
	LONG n1,n2,n3,n4;
	
	if (!parse_binary_unsigned_integer (&n1) || !parse_binary_unsigned_integer (&n2) || !parse_binary_unsigned_integer (&n3)
		|| !parse_binary_unsigned_integer (&n4))
		return 0;

	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3,(int)n4);
	return 1;
}

static int parse_binary_instruction_nnnnn (BInstructionP instruction)
{
	LONG n1,n2,n3,n4,n5;
	
	if (!parse_binary_unsigned_integer (&n1) || !parse_binary_unsigned_integer (&n2) || !parse_binary_unsigned_integer (&n3)
		|| !parse_binary_unsigned_integer (&n4) || !parse_binary_unsigned_integer (&n5))
		return 0;

	instruction->instruction_code_function ((int)n1,(int)n2,(int)n3,(int)n4,(int)n5);
	return 1;
}

static int parse_binary_directive_label (BInstructionP instruction)
{
	STRING a1;
	
	parse_binary_label (a1);
	instruction->instruction_code_function (a1);
	return 1;
}

static int parse_binary_directive_nnt (BInstructionP instruction)
{
	LONG n1,n2;
	int i;
	static ULONG small_vector;
	ULONG *vector_p;
	int vector_size=0;
	
	if (!parse_binary_unsigned_integer (&n1) || !parse_binary_unsigned_integer (&n2))
		return 0;
	
	vector_size=n2;
	if (vector_size+1<=SMALL_VECTOR_SIZE)
		vector_p=&small_vector;
	else
		vector_p=(ULONG*)fast_memory_allocate 
					(((vector_size+1+SMALL_VECTOR_SIZE-1)>>LOG_SMALL_VECTOR_SIZE) * sizeof (ULONG));
	
	i=0;
	while (i!=n2){
		switch (last_char){
			case 'i':	case 'I':
			case 'b':	case 'B':
			case 'c':	case 'C':
			case 'p':	case 'P':
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] &= ~ (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;	
				break;
			case 'f':	case 'F':
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] &= ~ (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] &= ~ (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;
				break;
			case 'r':	case 'R':
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] |= (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;
				vector_p[i>>LOG_SMALL_VECTOR_SIZE] |= (1<< (i & MASK_SMALL_VECTOR_SIZE));
				i+=1;	
				break;
			default:
				abc_parser_error_i ("B, C, F, I, P or R expected at line %d\n",line_number);
		}
		last_char=getc (abc_file);
	}
	
	instruction->instruction_code_function ((int)n1,(int)n2,vector_p);
	return 1;
}

static int parse_binary_directive_implab (BInstructionP instruction)
{
	STRING s;
	
	parse_binary_label (s);

	if (!is_newline (last_char)){
		STRING s2;

		parse_binary_label (s2);
		code_implab_node_entry (s,s2);
	} else
		code_implab (s);

	return 1;
}

static int parse_binary_directive_n (BInstructionP instruction)
{
	LONG n;
	STRING s;

	if (!parse_binary_unsigned_integer (&n))
		return 0;
	
	parse_binary_label (s);

	if (!is_newline (last_char)){
		STRING s2;

		parse_binary_label (s2);
		instruction->instruction_code_function ((int)n,s,s2);
	} else
		instruction->instruction_code_function ((int)n,s,NULL);

	return 1;
}

static struct binary_instruction instruction_table[]={
	{ parse_binary_instruction_ana,		code_build },
	{ parse_binary_instruction_an,		code_buildh },
	{ parse_binary_instruction_i,		code_buildI },
	{ parse_binary_instruction_n,		code_buildB_b },
	{ parse_binary_instruction_n,		code_buildC_b },
	{ parse_binary_instruction_n,		code_buildI_b },
	{ parse_binary_instruction_n,		code_buildR_b },
	{ parse_binary_instruction_n,		code_buildF_b },
	{ parse_binary_instruction_ann,		code_eq_desc },
	{ parse_binary_instruction_an,		code_eqD_b },
	{ parse_binary_instruction_in,		code_eqI_a },
	{ parse_binary_instruction_in,		code_eqI_b },
	{ parse_binary_instruction_anan,	code_fill },
	{ parse_binary_instruction_ann,		code_fillh },
	{ parse_binary_instruction_in,		code_fillI },
	{ parse_binary_instruction_nn,		code_fillB_b },
	{ parse_binary_instruction_nn,		code_fillC_b },
	{ parse_binary_instruction_nn,		code_fillF_b },
	{ parse_binary_instruction_nn,		code_fillI_b },
	{ parse_binary_instruction_nn,		code_fillR_b },
	{ parse_binary_instruction_nn,		code_fill_a },
	{ parse_binary_instruction_a,		code_jmp },
	{ parse_binary_instruction_a,		code_jmp_false },
	{ parse_binary_instruction_a,		code_jmp_true },
	{ parse_binary_instruction_a,		code_jsr },
	{ parse_binary_instruction_n,		code_jsr_eval },
	{ parse_binary_instruction_n,		code_pop_a },
	{ parse_binary_instruction_n,		code_pop_b },
	{ parse_binary_instruction_n,		code_pushB_a },
	{ parse_binary_instruction_n,		code_pushC_a },
	{ parse_binary_instruction_n,		code_pushI_a },
	{ parse_binary_instruction_n,		code_pushF_a },
	{ parse_binary_instruction_n,		code_pushR_a },
	{ parse_binary_instruction_a,		code_pushD },
	{ parse_binary_instruction_i,		code_pushI },
	{ parse_binary_instruction_n,		code_push_a },
	{ parse_binary_instruction_n,		code_push_b },	
	{ parse_binary_instruction_nnn,		code_push_arg },
	{ parse_binary_instruction_nnn,		code_push_args },
	{ parse_binary_instruction_nnn,		code_push_args_u },
	{ parse_binary_instruction_an,		code_push_node },
	{ parse_binary_instruction_ann,		code_push_node_u },
	{ parse_binary_instruction_nnn,		code_push_r_args },
	{ parse_binary_instruction_nnnnn,	code_push_r_args_a },
	{ parse_binary_instruction_nnnnn,	code_push_r_args_b },
	{ parse_binary_instruction_nnn,		code_push_r_args_u },
	{ parse_binary_instruction_nn,		code_repl_arg },
	{ parse_binary_instruction_nn,		code_repl_args },
	{ parse_binary_instruction_nn,		code_repl_r_args },
	{ parse_binary_instruction_nnnn,	code_repl_r_args_a },
	{ parse_binary_instruction_,		code_rtn },
	{ parse_binary_instruction_nn,		code_update_a },
	{ parse_binary_instruction_nn,		code_update_b },
	{ parse_binary_instruction_nn,		code_updatepop_a },
	{ parse_binary_instruction_nn,		code_updatepop_b },
	{ parse_binary_directive_nnt,		code_d },
	{ parse_binary_directive_nnt,		code_o },
	{ parse_binary_directive_label,		code_impdesc },
	{ parse_binary_directive_implab,	code_implab },
	{ parse_binary_directive_n,			code_n }
	};

static int parse_binary_instruction (void)
{
	struct binary_instruction *instruction_p;
	int instruction_table_index;
		
	instruction_table_index=last_char-136;
	instruction_p=&instruction_table[instruction_table_index];
	
	if (instruction_table_index >= sizeof(instruction_table)/sizeof(struct binary_instruction)){
		abc_parser_error_i ("Invalid instruction at line %d\n",line_number);
		return 0;
	}
	
	last_char=getc (abc_file);
	
	return instruction_p->instruction_parse_function (instruction_p);
}
#endif

int parse_file (FILE *file)
{
	if (setjmp (exit_abc_parser_buffer)==0){
		abc_file=file;
		
		initialize_file_parsing();

#if !BINARY_ABC
		while (parse_line())
			++line_number;
#else
	parse_line_without_newline();
	for (;;){
		if (last_char>=136){
			if (!parse_binary_instruction())
				break;		
		} else {
			if (last_char=='\xd'){
				last_char=getc (abc_file);
				if (last_char=='\xa')
					last_char=getc (abc_file);
			} else if (last_char=='\xa')
				last_char=getc (abc_file);
			else if (last_char!=EOF)
				abc_parser_error_i ("Error in abc file at line %d\n",line_number);

			++line_number;			

			if (last_char>=136){
				if (!parse_binary_instruction())
					break;
			} else
				if (!parse_line_without_newline())
					break;
		}
	}
#endif

		return 0;
	} else
		return 1;
}

static int parser_initialized=0;

void initialize_parser (void)
{
	if (!parser_initialized){
		init_instruction_name_table();
		initialize_alpha_num_table();
		parser_initialized=1;
	}
}