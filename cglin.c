/*
	File:	cglin.c
	Author:	John van Groningen
	At:		University of Nijmegen
*/

#include <stdio.h>
#include "cgport.h"
#include "cg.h"
#include "cgconst.h"
#include "cgiconst.h"
#include "cgrconst.h"
#include "cgtypes.h"

#include "cglin.h"

#include "cgcalc.h"
#include "cgstack.h"

#define g_fhighlow

#define LOAD_X_PARAMETER_REGISTER

#ifdef G_POWER
# define IF_G_POWER(a) a
#else
# define IF_G_POWER(a)
#endif

#define LOAD_X_TO_ADDRESS 1
#define LOAD_X_TO_REGISTER 2

#ifdef sparc
# undef ALIGN_REAL_ARRAYS
# undef LOAD_STORE_ALIGNED_REAL 4
#endif

/* from cgcode.c : */

extern struct basic_block *first_block,*last_block;
extern struct instruction *last_instruction;

extern LABEL *index_error_label;

int local_data_offset;

#pragma segment Code3

#define S2(s,v1,v2) s.v1;s.v2
#define S3(s,v1,v2,v3) s.v1;s.v2;s.v3
#define S4(s,v1,v2,v3,v4) s.v1;s.v2;s.v3;s.v4

#define U2(s,v1,v2) s->v1;s->v2
#define U3(s,v1,v2,v3) s->v1;s->v2;s->v3
#define U4(s,v1,v2,v3,v4) s->v1;s->v2;s->v3;s->v4

IF_G_POWER(struct instruction *last_heap_pointer_update;)

static struct instruction *i_new_instruction (int instruction_code,int arity,int arg_size)
{
	register struct instruction *instruction;
	
	instruction=(struct instruction*)fast_memory_allocate (sizeof (struct instruction)+arg_size);
	
	U4(instruction,	instruction_next=NULL,
					instruction_prev=last_instruction,
					instruction_icode=instruction_code,
					instruction_arity=arity);
	
	if (last_block->block_instructions==NULL)
		last_block->block_instructions=instruction;
	else 
		last_instruction->instruction_next=instruction;
	last_instruction=instruction;
	
	return instruction;
}

static struct instruction *i_new_instruction1 (int instruction_code)
{
	register struct instruction *instruction;
	
	instruction=(struct instruction*)fast_memory_allocate (sizeof (struct instruction)+sizeof (struct parameter));
	
	U4(instruction,	instruction_next=NULL,
					instruction_prev=last_instruction,
					instruction_icode=instruction_code,
					instruction_arity=1);
	
	if (last_block->block_instructions==NULL)
		last_block->block_instructions=instruction;
	else 
		last_instruction->instruction_next=instruction;
	last_instruction=instruction;
	
	return instruction;
}

static struct instruction *i_new_instruction2 (int instruction_code)
{
	register struct instruction *instruction;
	
	instruction=(struct instruction*)fast_memory_allocate (sizeof (struct instruction)+2*sizeof (struct parameter));
	
	U4(instruction,	instruction_next=NULL,
					instruction_prev=last_instruction,
					instruction_icode=instruction_code,
					instruction_arity=2);
	
	if (last_block->block_instructions==NULL)
		last_block->block_instructions=instruction;
	else 
		last_instruction->instruction_next=instruction;
	last_instruction=instruction;
	
	return instruction;
}

void i_add_i_r (LONG value,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IADD);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

void i_add_r_r (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IADD);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

#if defined (G_POWER) || defined (sparc)
static void i_addi_r_r (LONG value,int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction (IADDI,3,3*sizeof (struct parameter));
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);

	S2 (instruction->instruction_parameters[2],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
}
#endif

#if defined (M68000) || defined (G_POWER)
# if defined (M68000)
	static
# endif
void i_and_i_r (LONG value,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IAND);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}
#endif

#ifdef M68000
void i_bmi_l (LABEL *label)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IBMI);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_LABEL,
												parameter_data.l=label);
}

LONG *i_bmi_i (VOID)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IBMI);
	
	instruction->instruction_parameters[0].parameter_type=P_IMMEDIATE;

	return &instruction->instruction_parameters[0].parameter_data.i;
}
#endif

#ifdef G_POWER
void i_bnep_l (LABEL *label)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (IBNEP);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
}
#endif

#if defined (sparc) || defined (I486)
void i_bne_l (LABEL *label)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (IBNE);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
}
#endif

#ifdef M68000
static void i_bmove_pd_pd_r (int reg_1,int reg_2,int reg_3)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction (IBMOVE,3,3*sizeof (struct parameter));
	
	instruction->instruction_parameters[0].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[0].parameter_data.i=reg_1;
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=reg_2;

	instruction->instruction_parameters[2].parameter_type=P_REGISTER;
	instruction->instruction_parameters[2].parameter_data.i=reg_3;
}

static void i_bmove_pi_pi_r (int reg_1,int reg_2,int reg_3)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction (IBMOVE,3,3*sizeof (struct parameter));
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=reg_1;
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=reg_2;

	instruction->instruction_parameters[2].parameter_type=P_REGISTER;
	instruction->instruction_parameters[2].parameter_data.i=reg_3;
}
#endif

#ifdef I486
void i_btst_i_id (LONG i,int offset,int register_1)
{
	register struct instruction *instruction;

	instruction=i_new_instruction2 (IBTST);

	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=i);

	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.i=register_1;
}
#endif

#if defined (sparc) || defined (I486) || defined (G_POWER)
void i_btst_i_r (LONG i,int register_1)
{
	register struct instruction *instruction;

	instruction=i_new_instruction2 (IBTST);

	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=i);

	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}
#endif

static void i_cmp_i_r (LONG value,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (ICMP);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

static void i_cmp_id_r (int offset,int register_1,int register_2)
{
	struct instruction *instruction;
#ifdef G_POWER
	instruction=i_new_instruction2 (ICMPLW);
#else
	instruction=i_new_instruction2 (ICMP);
#endif

	S3(instruction->instruction_parameters[0],	parameter_type=P_INDIRECT,
												parameter_offset=offset,
												parameter_data.i=register_1);
	
	S2(instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

static void i_cmpw_d_id (LABEL *descriptor,int arity,int offset_1,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (ICMPW);
	
	S3(instruction->instruction_parameters[0],	parameter_type=P_DESCRIPTOR_NUMBER,
												parameter_offset=arity,
												parameter_data.l=descriptor);
	
	S3(instruction->instruction_parameters[1],	parameter_type=P_INDIRECT,
												parameter_offset=offset_1,
												parameter_data.i=register_1);
}

#if defined (M68000) || defined (I486)
static void i_exg_r_r (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IEXG);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}
#endif

#if defined (I486) && defined (FP_STACK_OPTIMIZATIONS)
# define set_float_register_parameter(p,r) (p).parameter_type=P_F_REGISTER; (p).parameter_flags=0; (p).parameter_data.i=(r)
#else
# define set_float_register_parameter(p,r) (p).parameter_type=P_F_REGISTER; (p).parameter_data.i=(r)
#endif

#if defined (M68000) || defined (G_POWER)
static void i_extb_r (int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (IEXTB);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}
#endif

#ifdef M68000
void i_ext_r (int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (IEXT);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}
#endif

#ifdef FP_STACK_OPTIMIZATIONS
static void i_fexg_fr_fr (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFEXG);

	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	set_float_register_parameter (instruction->instruction_parameters[1],register_2);
}
#endif

void i_fmove_fr_fr (int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
		
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);	
	set_float_register_parameter (instruction->instruction_parameters[1],register_2);
}

void i_fmove_fr_id (int register_1,int offset,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
	
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
#ifdef ALIGN_REAL_ARRAYS
	instruction->instruction_parameters[1].parameter_flags=0;
#endif
}

#ifdef ALIGN_REAL_ARRAYS
void i_fmove_fr_id_f (int register_1,int offset,int register_2,int flags)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
	
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
	instruction->instruction_parameters[1].parameter_flags=flags;
}
#endif

#ifdef M68000
void i_fmove_fr_pd (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
	
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}

void i_fmove_fr_pi (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
	
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

#ifdef ALIGN_REAL_ARRAYS
static void i_fmove_fr_x (int register_1,int offset,int register_2,int register_3,int flags)
#else
static void i_fmove_fr_x (int register_1,int offset,int register_2,int register_3)
#endif
{
	struct instruction *instruction;
	struct index_registers *index_registers;
	
	instruction=i_new_instruction (IFMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	set_float_register_parameter (instruction->instruction_parameters[0],register_1);

	S3(instruction->instruction_parameters[1],parameter_type=P_INDEXED,parameter_offset=offset,parameter_data.ir=index_registers);
#ifdef ALIGN_REAL_ARRAYS
	instruction->instruction_parameters[1].parameter_flags=flags;
#endif
	
	index_registers->a_reg.r=register_2;
	index_registers->d_reg.r=register_3;
}

void i_fmove_id_fr (int offset,int register_2,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
		
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.i=register_2;
#ifdef ALIGN_REAL_ARRAYS
	instruction->instruction_parameters[0].parameter_flags=0;
#endif

	set_float_register_parameter (instruction->instruction_parameters[1],register_1);
}

#ifdef ALIGN_REAL_ARRAYS
void i_fmove_id_fr_f (int offset,int register_2,int register_1,int flags)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
		
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.i=register_2;
	instruction->instruction_parameters[0].parameter_flags=flags;

	set_float_register_parameter (instruction->instruction_parameters[1],register_1);
}
#endif

#ifdef M68000
void i_fmove_pd_fr (int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
		
	instruction->instruction_parameters[0].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;

	set_float_register_parameter (instruction->instruction_parameters[1],register_2);
}

void i_fmove_pi_fr (int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVE);
		
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;

	set_float_register_parameter (instruction->instruction_parameters[1],register_2);
}
#endif

static void i_fmove_if_fr (DOUBLE r,int register_1)
{
	struct instruction *instruction;
	DOUBLE *rp;
	
	instruction=i_new_instruction2 (IFMOVE);
	rp=(DOUBLE*)fast_memory_allocate (sizeof (DOUBLE));
	
	instruction->instruction_parameters[0].parameter_type=P_F_IMMEDIATE;
	instruction->instruction_parameters[0].parameter_data.r=rp;
	*rp=r;

	set_float_register_parameter (instruction->instruction_parameters[1],register_1);
}

static void i_fmovel_fr_r (int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IFMOVEL);
	
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

#ifdef ALIGN_REAL_ARRAYS
static void i_fmove_x_fr (int offset,int register_1,int register_2,int register_3,int flags)
#else
static void i_fmove_x_fr (int offset,int register_1,int register_2,int register_3)
#endif
{
	struct instruction *instruction;
	struct index_registers *index_registers;
	
	instruction=i_new_instruction (IFMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
#ifdef ALIGN_REAL_ARRAYS
	instruction->instruction_parameters[0].parameter_flags=flags;
#endif	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	set_float_register_parameter (instruction->instruction_parameters[1],register_3);
}

void i_jmp_id (int offset_1,int register_1,int n_a_registers)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJMP);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.reg.r=register_1;
	
	instruction->instruction_parameters[0].parameter_data.reg.u=n_a_registers;
}

#if defined (G_POWER) || defined (I486)
void i_jmp_id_profile (int offset_1,int register_1,int n_a_registers)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJMPP);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.reg.r=register_1;
	
	instruction->instruction_parameters[0].parameter_data.reg.u=n_a_registers;
}
#endif

void i_jmp_l (LABEL *label,int n_a_registers)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJMP);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
	
	instruction->instruction_parameters[0].parameter_offset=n_a_registers;
}

#if defined (G_POWER) || defined (I486)
void i_jmp_l_profile (LABEL *label,int offset)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJMPP);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
	
	instruction->instruction_parameters[0].parameter_offset=offset;
}
#endif

#ifdef I486
void i_jmp_r (int a_reg)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJMP);
	
	instruction->instruction_parameters[0].parameter_type=P_REGISTER;
	instruction->instruction_parameters[0].parameter_data.i=a_reg;
}

void i_jmp_r_profile (int a_reg)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJMPP);
	
	instruction->instruction_parameters[0].parameter_type=P_REGISTER;
	instruction->instruction_parameters[0].parameter_data.i=a_reg;
}
#endif

#if defined (M68000) || defined (I486)
void i_jsr_id (int offset,int register_1,int n_a_registers)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJSR);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.reg.r=register_1;
	
	instruction->instruction_parameters[0].parameter_data.reg.u=n_a_registers;
}

void i_jsr_l (LABEL *label,int n_a_registers)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJSR);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
		
	instruction->instruction_parameters[0].parameter_offset=n_a_registers;
}
#else
void i_jsr_id_id (int offset_1,int register_1,int offset_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IJSR);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;

	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
/*	instruction->instruction_parameters[1].parameter_data.reg.r=B_STACK_POINTER; */
	instruction->instruction_parameters[1].parameter_data.i=offset_2;
}

void i_jsr_l_id (LABEL *label,int offset)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IJSR);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;

	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
/*	instruction->instruction_parameters[1].parameter_data.reg.r=B_STACK_POINTER; */
	instruction->instruction_parameters[1].parameter_data.i=offset;
}

# ifdef G_POWER
	void i_jsr_id_idu (int offset_1,int register_1,int offset_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IJSR);
		
		instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
		instruction->instruction_parameters[0].parameter_offset=offset_1;
		instruction->instruction_parameters[0].parameter_data.i=register_1;
	
		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
/*		instruction->instruction_parameters[1].parameter_data.reg.r=B_STACK_POINTER; */
		instruction->instruction_parameters[1].parameter_data.i=offset_2;
	}

	void i_jsr_l_idu (LABEL *label,int offset)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IJSR);
		
		instruction->instruction_parameters[0].parameter_type=P_LABEL;
		instruction->instruction_parameters[0].parameter_data.l=label;
	
		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
/*		instruction->instruction_parameters[1].parameter_data.reg.r=B_STACK_POINTER; */
		instruction->instruction_parameters[1].parameter_data.i=offset;
	}
# endif
#endif

#ifdef I486
void i_jsr_r (int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (IJSR);
	
	instruction->instruction_parameters[0].parameter_type=P_REGISTER;
	instruction->instruction_parameters[0].parameter_data.reg.r=register_1;
}
#endif

#if defined (sparc) || defined (G_POWER)
# ifdef sparc
void i_call_l (LABEL *label)
# else
void i_call_l (LABEL *label,int frame_size)	
# endif
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IJSR);
	
	S2(instruction->instruction_parameters[0],	parameter_type=P_LABEL,
												parameter_data.l=label);

	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
# ifdef G_POWER
	instruction->instruction_parameters[1].parameter_data.i=frame_size;
# endif
}

# ifdef sparc
void i_call_r (int register_1)
# else
void i_call_r (int register_1,int frame_size)	
# endif
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IJSR);
	
	S2(instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.reg.r=register_1);

	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
# ifdef G_POWER
	instruction->instruction_parameters[1].parameter_data.i=frame_size;
# endif
}
#endif

void i_lea_id_r (int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (ILEA);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.reg.r=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
	instruction->instruction_parameters[1].parameter_data.reg.r=register_2;
}

void i_lea_l_i_r (LABEL *label,int offset,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (ILEA);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.l=label;
	
	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
	instruction->instruction_parameters[1].parameter_data.reg.r=register_1;
}

#if defined (sparc) || defined (I486) || defined (G_POWER)
static void i_lea_x_r (int register_1,int register_2,int register_3)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction (ILEA,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=0;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
	instruction->instruction_parameters[1].parameter_data.i=register_3;
}
#endif

#if defined (G_POWER) || defined (sparc)
static void i_lsli_r_r (LONG value,int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction (ILSLI,3,3*sizeof (struct parameter));
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);

	S2 (instruction->instruction_parameters[2],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
}
#endif

static void i_moveb_id_r (int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEB);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

#if !(defined (M68000) || defined (I486))
static void i_moveb_x_r (int register_1,int register_2,int register_3)
{
	struct instruction *instruction;
	struct index_registers *index_registers;
	
	instruction=i_new_instruction (IMOVEB,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=0;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
	instruction->instruction_parameters[1].parameter_data.i=register_3;
}
#else
static void i_moveb_x_r (int offset,int register_1,int register_2,int register_3)
{
	struct instruction *instruction;
	struct index_registers *index_registers;
	
	instruction=i_new_instruction (IMOVEB,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
	instruction->instruction_parameters[1].parameter_data.i=register_3;
}
#endif

#ifdef M68000
static void i_movem_id (int offset_1,int register_1,int n_arguments,int arguments[])
{
	register struct instruction *instruction;
	register struct parameter *parameter;
	register int argument_number;
	
	instruction=i_new_instruction (IMOVEM,n_arguments+1,(n_arguments+1)*sizeof (struct parameter));
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	parameter=&instruction->instruction_parameters[1];
	
	for (argument_number=0; argument_number<n_arguments; ++argument_number){
		parameter->parameter_type=P_REGISTER;
		parameter->parameter_data.i=arguments[argument_number];
		++parameter;
	}
}

void i_movem_pd (int register_1,int n_arguments,int arguments[])
{
	struct instruction *instruction;
	struct parameter *parameter;
	int argument_number;
	
	instruction=i_new_instruction (IMOVEM,n_arguments+1,(n_arguments+1)*sizeof (struct parameter));
	
	instruction->instruction_parameters[n_arguments].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[n_arguments].parameter_data.i=register_1;
	
	parameter=&instruction->instruction_parameters[0];
	
	for (argument_number=0; argument_number<n_arguments; ++argument_number){
		parameter->parameter_type=P_REGISTER;
		parameter->parameter_data.i=arguments[argument_number];
		++parameter;
	}
}

void i_movem_pi (int register_1,int n_arguments,int arguments[])
{
	struct instruction *instruction;
	struct parameter *parameter;
	int argument_number;
	
	instruction=i_new_instruction (IMOVEM,n_arguments+1,(n_arguments+1)*sizeof (struct parameter));
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	parameter=&instruction->instruction_parameters[1];
	
	for (argument_number=0; argument_number<n_arguments; ++argument_number){
		parameter->parameter_type=P_REGISTER;
		parameter->parameter_data.i=arguments[argument_number];
		++parameter;
	}
}

void i_movem_id_r (int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEM);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}
#endif

void i_movew_id_r (int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEW);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

static void i_move_i_id (LONG i,int offset_1,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=i);
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset_1;
	instruction->instruction_parameters[1].parameter_data.i=register_1;
}

#ifdef M68000
static void i_move_i_pi (LONG i,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=i);
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_1;
}
#endif

void i_move_i_r (LONG i,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=i);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

#if defined (M68000) || defined (I486)
static void i_move_i_x (LONG i,int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=i);

	instruction->instruction_parameters[1].parameter_type=P_INDEXED;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
}

void i_move_id_pd (int offset_1,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);

	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

#ifdef M68000
static void i_move_id_pi (int offset_1,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);

	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

static void i_move_d_r (LABEL *descriptor,int arity,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_DESCRIPTOR_NUMBER;
	instruction->instruction_parameters[0].parameter_offset=arity;
	instruction->instruction_parameters[0].parameter_data.l=descriptor;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

void i_move_id_id (int offset_1,int register_1,int offset_2,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset_2;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}

void i_move_id_r (int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

#ifdef G_POWER
	void i_move_idu_r (int offset,int register_1,int register_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVE);
		
		instruction->instruction_parameters[0].parameter_type=P_INDIRECT_WITH_UPDATE;
		instruction->instruction_parameters[0].parameter_offset=offset;
		instruction->instruction_parameters[0].parameter_data.i=register_1;
		
		instruction->instruction_parameters[1].parameter_type=P_REGISTER;
		instruction->instruction_parameters[1].parameter_data.i=register_2;
	}

	void i_move_id_idu (int offset1,int register_1,int offset2,int register_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVE);
				
		instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
		instruction->instruction_parameters[0].parameter_offset=offset1;
		instruction->instruction_parameters[0].parameter_data.i=register_1;

		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
		instruction->instruction_parameters[1].parameter_offset=offset2;
		instruction->instruction_parameters[1].parameter_data.i=register_2;
	}

	void i_move_r_idu (int register_1,int offset,int register_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVE);
				
		instruction->instruction_parameters[0].parameter_type=P_REGISTER;
		instruction->instruction_parameters[0].parameter_data.i=register_1;

		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
		instruction->instruction_parameters[1].parameter_offset=offset;
		instruction->instruction_parameters[1].parameter_data.i=register_2;
	}

	void i_movew_id_idu (int offset1,int register_1,int offset2,int register_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVEW);
				
		instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
		instruction->instruction_parameters[0].parameter_offset=offset1;
		instruction->instruction_parameters[0].parameter_data.i=register_1;

		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
		instruction->instruction_parameters[1].parameter_offset=offset2;
		instruction->instruction_parameters[1].parameter_data.i=register_2;
	}

	void i_movew_r_idu (int register_1,int offset,int register_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVEW);
				
		instruction->instruction_parameters[0].parameter_type=P_REGISTER;
		instruction->instruction_parameters[0].parameter_data.i=register_1;

		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
		instruction->instruction_parameters[1].parameter_offset=offset;
		instruction->instruction_parameters[1].parameter_data.i=register_2;
	}
#endif

#if defined (M68000) || defined (I486)
static void i_move_id_x (int offset_1,int register_1,int offset_2,int register_2,int register_3)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;

	instruction->instruction_parameters[1].parameter_type=P_INDEXED;
	instruction->instruction_parameters[1].parameter_offset=offset_2;
	instruction->instruction_parameters[1].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_2;
	index_registers->d_reg.r=register_3;
}
#endif

void i_move_l_r (LABEL *label,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);

	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

#ifdef M68000
void i_move_pd_r (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

void i_move_pi_id (int register_1,int offset_2,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset_2;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}

static void i_move_pi_pi (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

#if defined (M68000) | defined (I486)
void i_move_pi_r (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}
#endif

#ifdef M68000
static void i_move_pi_x (int register_1,int offset,int register_2,int register_3)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;

	instruction->instruction_parameters[1].parameter_type=P_INDEXED;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_2;
	index_registers->d_reg.r=register_3;
}
#endif

#ifdef G_POWER
static struct instruction *instruction_move_r_idhp (int register_1,LONG offset)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT_HP;
	instruction->instruction_parameters[1].parameter_data.i=offset;

	return instruction;
}
#endif

#if defined (I486)
	void i_move_r_l (int register_1,LABEL *label)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVE);
		S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
													parameter_data.i=register_1);
		S2 (instruction->instruction_parameters[1],	parameter_type=P_LABEL,
													parameter_data.l=label);
	}
#endif

void i_move_r_id (int register_1,int offset,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}

#if defined (M68000) | defined (I486)
void i_move_r_pd (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

#ifdef M68000
void i_move_r_pi (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

void i_move_r_r (int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

static void i_move_x_r (int offset,int register_1,int register_2,int register_3)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	instruction->instruction_parameters[1].parameter_type=P_REGISTER;
	instruction->instruction_parameters[1].parameter_data.i=register_3;
}

static void i_move_x_id (int offset_1,int register_1,int register_2,int offset_2,int register_3)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset_2;
	instruction->instruction_parameters[1].parameter_data.i=register_3;
}

#if defined (M68000)
static void i_move_x_pi (int offset,int register_1,int register_2,int register_3)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=offset;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_3;
}

static void i_move_x_x (int offset_1,int register_1,int register_2,int offset_2,int register_3,int register_4)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers_1,*index_registers_2;
	
	instruction=i_new_instruction
		(IMOVE,2,2*sizeof (struct parameter)+2*sizeof (struct index_registers));
	index_registers_1=(struct index_registers *)&instruction->instruction_parameters[2];
	index_registers_2=&index_registers_1[1];

	instruction->instruction_parameters[0].parameter_type=P_INDEXED;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.ir=index_registers_1;

	index_registers_1->a_reg.r=register_1;
	index_registers_1->d_reg.r=register_2;

	instruction->instruction_parameters[1].parameter_type=P_INDEXED;
	instruction->instruction_parameters[1].parameter_offset=offset_2;
	instruction->instruction_parameters[1].parameter_data.ir=index_registers_2;
	
	index_registers_2->a_reg.r=register_3;
	index_registers_2->d_reg.r=register_4;
}

void i_movew_id_pd (int offset_1,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEW);

	instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[0].parameter_offset=offset_1;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}

void i_movew_pi_id (int register_1,int offset_2,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEW);
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset_2;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}

void i_movew_pi_r (int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEW);
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

void i_movew_r_pd (int register_1,int register_2)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IMOVEW);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_2;
}
#endif

#ifdef G_POWER
void i_or_i_r (LONG value,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (IOR);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

void i_mtctr (int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction (IMTCTR,1,1*sizeof (struct parameter));	

	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}
#endif

#if ! (defined (sparc) || defined (G_POWER))
	void i_rts (void)
	{
		i_new_instruction (IRTS,0,0);
	}

# ifdef I486
	void i_rts_i (int offset)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction (IRTSI,1,1*sizeof (struct parameter));
		
		S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
													parameter_data.i=offset);
	}
	
	void i_rts_profile (void)
	{
		i_new_instruction (IRTSP,0,0);
	}
# endif
#else
	void i_rts (int offset_1,int offset_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IRTS);
	
		instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
		instruction->instruction_parameters[0].parameter_offset=offset_1;
		instruction->instruction_parameters[0].parameter_data.i=B_STACK_POINTER;
	
		instruction->instruction_parameters[1].parameter_type=P_IMMEDIATE;
		instruction->instruction_parameters[1].parameter_data.i=offset_2;
	}
#endif

#ifdef G_POWER
	void i_rts_c (void)
	{
		i_new_instruction (IRTS,0,0);
	}

	void i_rts_profile (int offset_1,int offset_2)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IRTSP);
	
		instruction->instruction_parameters[0].parameter_type=P_INDIRECT;
		instruction->instruction_parameters[0].parameter_offset=offset_1;
		instruction->instruction_parameters[0].parameter_data.i=B_STACK_POINTER;
	
		instruction->instruction_parameters[1].parameter_type=P_IMMEDIATE;
		instruction->instruction_parameters[1].parameter_data.i=offset_2;
	}

	void i_rts_r (int register1,int offset_1)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IRTS);
	
		instruction->instruction_parameters[0].parameter_type=P_REGISTER;
		instruction->instruction_parameters[0].parameter_data.i=register1;
	
		instruction->instruction_parameters[1].parameter_type=P_IMMEDIATE;
		instruction->instruction_parameters[1].parameter_data.i=offset_1;
	}

	void i_rts_r_profile (int register1,int offset_1)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IRTSP);
	
		instruction->instruction_parameters[0].parameter_type=P_REGISTER;
		instruction->instruction_parameters[0].parameter_data.i=register1;
	
		instruction->instruction_parameters[1].parameter_type=P_IMMEDIATE;
		instruction->instruction_parameters[1].parameter_data.i=offset_1;
	}
#endif

void i_schedule_i (int value)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (ISCHEDULE);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
}

void i_sub_i_r (LONG value,int register_1)
{

	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (ISUB);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

#ifdef M68000
static void i_tstb_pi (int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction1 (ITSTB);
	
	instruction->instruction_parameters[0].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
}
#endif

void i_tst_r (int register_1)
{
	register struct instruction *instruction;

	instruction=i_new_instruction1 (ITST);

	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

void i_word_i (int value)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (IWORD);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
}

#define N_VECTOR_BITS	32

struct register_list {
	ULONG					r_l_vector;
	struct register_list	*r_l_next;
};

struct register_set {
	struct register_list	r_s_list;
	UWORD					r_s_first_free;
	UWORD					r_s_highest;
};

static struct register_set free_dregisters,free_aregisters,free_fregisters;

void get_n_virtual_registers
	(int *n_virtual_a_regs_p,int *n_virtual_d_regs_p,int *n_virtual_f_regs_p)
{
	*n_virtual_a_regs_p=free_aregisters.r_s_highest;
	*n_virtual_d_regs_p=free_dregisters.r_s_highest;
	*n_virtual_f_regs_p=free_fregisters.r_s_highest;
}

static int get_free_register_number (register struct register_set *r_s_p)
{
	struct register_list *r_list;
	register unsigned int bit_n,reg_n;
	ULONG vector;
	
	reg_n=r_s_p->r_s_first_free;
	
	bit_n=reg_n;
	r_list=&r_s_p->r_s_list;
	
	while (bit_n>=N_VECTOR_BITS){
		struct register_list *n_r_list;
		
		bit_n-=N_VECTOR_BITS;
		n_r_list=r_list->r_l_next;
		if (n_r_list==NULL){
			n_r_list=(struct register_list*)fast_memory_allocate (sizeof (struct register_list));
			n_r_list->r_l_next=NULL;
			
			r_list->r_l_next=n_r_list;
		}
		r_list=n_r_list;
	}	
	vector=r_list->r_l_vector;
		
	while (reg_n<r_s_p->r_s_highest){
		if (bit_n==N_VECTOR_BITS){
			bit_n=0;
			r_list=r_list->r_l_next;
			vector=r_list->r_l_vector;
		}
		if (vector & (1<<bit_n)){
			r_list->r_l_vector &= ~(1<<bit_n);
			r_s_p->r_s_first_free=reg_n+1;
			return reg_n;
		}
		++reg_n;
		++bit_n;
	}
	
	if (bit_n==N_VECTOR_BITS){
		r_list=r_list->r_l_next=(struct register_list*)fast_memory_allocate (sizeof (struct register_list));
		r_list->r_l_next=NULL;
		bit_n=0;
	}
	
	r_s_p->r_s_highest=reg_n+1;
	r_s_p->r_s_first_free=reg_n+1;
	r_list->r_l_vector &= ~(1<<bit_n);
	return reg_n;
}

static void free_register_number (struct register_set *r_s_p,unsigned int reg_n)
{
	struct register_list *r_list;
	register unsigned int bit_n;
	
	if (reg_n<r_s_p->r_s_first_free)
		r_s_p->r_s_first_free=reg_n;
	
	bit_n=reg_n;
	r_list=&r_s_p->r_s_list;
	
	while (bit_n>=N_VECTOR_BITS){
		bit_n-=N_VECTOR_BITS;
		r_list=r_list->r_l_next;
	}
	
	r_list->r_l_vector |= (1<<bit_n);
}

static void allocate_register_number (struct register_set *r_s_p,unsigned int reg_n)
{
	struct register_list *r_list;
	register unsigned int bit_n;
	
	bit_n=reg_n;
	r_list=&r_s_p->r_s_list;
	
	while (bit_n>=N_VECTOR_BITS){
		bit_n-=N_VECTOR_BITS;
		r_list=r_list->r_l_next;
	}
	
	r_list->r_l_vector &= ~(1<<bit_n);	
}

static int try_allocate_register_number (int reg_n)
{
	struct register_list *r_list;
	register unsigned int bit_n;
	
	if (is_d_register (reg_n)){
		r_list=&free_dregisters.r_s_list;
		bit_n=d_reg_num (reg_n);
	} else {
		r_list=&free_aregisters.r_s_list;
		bit_n=a_reg_num (reg_n);
	}
	
	while (bit_n>=N_VECTOR_BITS){
		bit_n-=N_VECTOR_BITS;
		r_list=r_list->r_l_next;
	}
	
	if (r_list->r_l_vector & (1<<bit_n)){
		r_list->r_l_vector &= ~(1<<bit_n);
		return 1;
	} else
		return 0;
}

static int try_get_real_dregister_number (VOID)
{
	register struct register_list *r_list;
	register unsigned int reg_n;
	ULONG vector;
	
	reg_n=free_dregisters.r_s_first_free;
	r_list=&free_dregisters.r_s_list;
	vector=r_list->r_l_vector;
		
	while (reg_n<N_DATA_PARAMETER_REGISTERS && reg_n<free_dregisters.r_s_highest){
		if (vector & (1<<reg_n)){
			r_list->r_l_vector &= ~(1<<reg_n);
			free_dregisters.r_s_first_free=reg_n+1;
			return reg_n;
		}
		++reg_n;
	}
	
	if (reg_n>=N_DATA_PARAMETER_REGISTERS)
		return -1;
	
	free_dregisters.r_s_highest=reg_n+1;
	r_list->r_l_vector &= ~(1<<reg_n);
	return reg_n;
}

static int get_dregister (VOID)
{
	return num_to_d_reg (get_free_register_number (&free_dregisters));
}

static void free_dregister (int dregister)
{
	free_register_number (&free_dregisters,d_reg_num (dregister));
}

void allocate_dregister (int dregister)
{
	allocate_register_number (&free_dregisters,d_reg_num (dregister));
}

void free_all_dregisters (VOID)
{
	free_dregisters.r_s_first_free=d_reg_num (REGISTER_D0);
	free_dregisters.r_s_highest=8;
#ifdef I486
		free_dregisters.r_s_list.r_l_vector=
						  (1<<d_reg_num (REGISTER_D0)) | (1<<d_reg_num (REGISTER_D1));
#else
	if (parallel_flag)
		free_dregisters.r_s_list.r_l_vector=
						  (1<<d_reg_num (REGISTER_D0)) | (1<<d_reg_num (REGISTER_D1))
						| (1<<d_reg_num (REGISTER_D2)) | (1<<d_reg_num (REGISTER_D3))
						| (1<<d_reg_num (REGISTER_D4)) | (1<<d_reg_num (REGISTER_D5));
	else
		free_dregisters.r_s_list.r_l_vector=
						  (1<<d_reg_num (REGISTER_D0)) | (1<<d_reg_num (REGISTER_D1))
						| (1<<d_reg_num (REGISTER_D2)) | (1<<d_reg_num (REGISTER_D3))
						| (1<<d_reg_num (REGISTER_D4)) | (1<<d_reg_num (REGISTER_D5))
						| (1<<d_reg_num (REGISTER_D6));
#endif
	free_dregisters.r_s_list.r_l_next=NULL;
}

static int get_aregister (VOID)
{
	return num_to_a_reg (get_free_register_number (&free_aregisters));
}

static void free_aregister (int aregister)
{
	free_register_number (&free_aregisters,a_reg_num (aregister));
}

void allocate_aregister (int aregister)
{
	allocate_register_number (&free_aregisters,a_reg_num (aregister));
}

void free_all_aregisters (VOID)
{
	free_aregisters.r_s_first_free=a_reg_num (REGISTER_A0);
	free_aregisters.r_s_highest=N_REAL_A_REGISTERS;
	free_aregisters.r_s_list.r_l_vector=	  (1<<a_reg_num (REGISTER_A0))
											| (1<<a_reg_num (REGISTER_A1))
#ifndef I486
											| (1<<a_reg_num (REGISTER_A2))
#endif
											;
	free_aregisters.r_s_list.r_l_next=NULL;
}

static int get_fregister (VOID)
{
	return get_free_register_number (&free_fregisters);
}

static void free_fregister (int fregister)
{
	free_register_number (&free_fregisters,fregister);
}

void allocate_fregister (int fregister)
{
	allocate_register_number (&free_fregisters,fregister);
}

void free_all_fregisters (VOID)
{
	free_fregisters.r_s_first_free=REGISTER_FP0;
	free_fregisters.r_s_highest=8;
	free_fregisters.r_s_list.r_l_vector=	(1<<REGISTER_FP0) | (1<<REGISTER_FP1) |
											(1<<REGISTER_FP2) | (1<<REGISTER_FP3) |
											(1<<REGISTER_FP4) | (1<<REGISTER_FP5) |
											(1<<REGISTER_FP6)
#ifndef I486
											| (1<<REGISTER_FP7)
#endif
											;
	free_fregisters.r_s_list.r_l_next=NULL;
}

static void free_register (int reg)
{
	if (is_d_register (reg))
		free_dregister (reg);
	else
		free_aregister (reg);
}

typedef struct address {
					int 	ad_mode;
	union {
		struct {
			union {
					LONG 	ad_register;
					LABEL	  *ad_label;
				struct {
					WORD	ad_areg;
					WORD	ad_dreg;
				} s;
			} u;
					LONG 	ad_offset;
		} s;
					DOUBLE	ad_real;
	} u;
					WORD   *ad_count_p;
					WORD   *ad_count_p2;
					WORD	ad_count;
					WORD	ad_count2;
} ADDRESS;

#define ad_register u.s.u.ad_register
#define ad_label u.s.u.ad_label
#define ad_areg u.s.u.s.ad_areg
#define ad_dreg u.s.u.s.ad_dreg
#define ad_offset u.s.ad_offset
#define ad_real u.ad_real

static void ad_to_parameter (ADDRESS *ad_p,struct parameter *parameter_p)
{
	switch (ad_p->ad_mode){
		case P_REGISTER:
			parameter_p->parameter_type=P_REGISTER;
			parameter_p->parameter_data.i=ad_p->ad_register;
			if (--*ad_p->ad_count_p==0)
				free_register (ad_p->ad_register);
			break;
		case P_IMMEDIATE:
			parameter_p->parameter_type=P_IMMEDIATE;
			parameter_p->parameter_data.i=ad_p->ad_offset;
			break;
		case P_INDIRECT:
			parameter_p->parameter_type=P_INDIRECT;
			parameter_p->parameter_offset=ad_p->ad_offset;
			parameter_p->parameter_data.i=ad_p->ad_register;
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			break;
		case P_DESCRIPTOR_NUMBER:
			parameter_p->parameter_type=P_DESCRIPTOR_NUMBER;
			parameter_p->parameter_offset=ad_p->ad_offset;
			parameter_p->parameter_data.l=ad_p->ad_label;
			break;
		case P_INDEXED:
		{
			struct index_registers *index_registers;
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					index_registers=(struct index_registers*)fast_memory_allocate (sizeof (struct index_registers));
			
					U3(parameter_p,	parameter_type=P_INDEXED,
									parameter_offset=i_ad_p->ad_offset,
									parameter_data.ir=index_registers);
			
					index_registers->a_reg.r=i_ad_p->ad_areg;
					index_registers->d_reg.r=i_ad_p->ad_dreg;

					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
				} else {
					U3(parameter_p,	parameter_type=P_INDIRECT,
									parameter_offset=i_ad_p->ad_offset,
									parameter_data.i=i_ad_p->ad_register);

					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
				}
			} else {
				U2(parameter_p,parameter_type=P_REGISTER,parameter_data.i=i_ad_p->ad_register);

				if (--*i_ad_p->ad_count_p==0)
					free_register (i_ad_p->ad_register);
			}
			break;
		}
		default:
			internal_error_in_function ("ad_to_parameter");
	}
}

static int fad_to_parameter (ADDRESS *ad_p,struct parameter *parameter_p)
{
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
			set_float_register_parameter (*parameter_p,ad_p->ad_register);
			if (--*ad_p->ad_count_p==0)
				/* free_fregister (ad_p->ad_register); */
				return 1;
			break;
		case P_F_IMMEDIATE:
		{
			DOUBLE *rp;
			
			parameter_p->parameter_type=P_F_IMMEDIATE;
			rp=(DOUBLE*)fast_memory_allocate (sizeof (DOUBLE));
			parameter_p->parameter_data.r=rp;
			*rp=ad_p->ad_real;
			break;
		}
		case P_INDIRECT:
			parameter_p->parameter_type=P_INDIRECT;
			parameter_p->parameter_offset=ad_p->ad_offset;
			parameter_p->parameter_data.i=ad_p->ad_register;
#ifdef ALIGN_REAL_ARRAYS
			parameter_p->parameter_flags=0;
#endif
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			break;
		case P_DESCRIPTOR_NUMBER:
			parameter_p->parameter_type=P_DESCRIPTOR_NUMBER;
			parameter_p->parameter_offset=ad_p->ad_offset;
			parameter_p->parameter_data.l=ad_p->ad_label;
			break;
		case P_INDEXED:
		{
			struct index_registers *index_registers;
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					index_registers=(struct index_registers*)fast_memory_allocate (sizeof (struct index_registers));
			
					U3(parameter_p,	parameter_type=P_INDEXED,
									parameter_offset=i_ad_p->ad_offset,
									parameter_data.ir=index_registers);
			
					index_registers->a_reg.r=i_ad_p->ad_areg;
					index_registers->d_reg.r=i_ad_p->ad_dreg;

					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
				} else {
					U3(parameter_p,	parameter_type=P_INDIRECT,
									parameter_offset=i_ad_p->ad_offset,
									parameter_data.i=i_ad_p->ad_register);

					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
				}
#ifdef ALIGN_REAL_ARRAYS
				parameter_p->parameter_flags = load_x_graph->inode_arity & LOAD_STORE_ALIGNED_REAL;
#endif
			} else {
				U2(parameter_p,parameter_type=P_REGISTER,parameter_data.i=i_ad_p->ad_register);

				if (--*i_ad_p->ad_count_p==0)
					free_register (i_ad_p->ad_register);
			}
			break;
		}
		default:
			internal_error_in_function ("ad_to_parameter");
	}
	return 0;
}

static void instruction_ad (int instruction_code,ADDRESS *ad_p)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction1 (instruction_code);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
}

static void instruction_ad_id (int instruction_code,ADDRESS *ad_p,int offset,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);

	instruction->instruction_parameters[1].parameter_type=P_INDIRECT;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.i=register_1;
}

#ifdef G_POWER
	static void i_move_ad_idu (ADDRESS *ad_p,int offset,int register_1)
	{
		struct instruction *instruction;
		
		instruction=i_new_instruction2 (IMOVE);
		
		ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
	
		instruction->instruction_parameters[1].parameter_type=P_INDIRECT_WITH_UPDATE;
		instruction->instruction_parameters[1].parameter_offset=offset;
		instruction->instruction_parameters[1].parameter_data.i=register_1;
	}
#endif

#ifdef M68000
static void instruction_ad_pd (int instruction_code,ADDRESS *ad_p,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
	
	instruction->instruction_parameters[1].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_1;
}

static void instruction_ad_pi (int instruction_code,ADDRESS *ad_p,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
	
	instruction->instruction_parameters[1].parameter_type=P_POST_INCREMENT;
	instruction->instruction_parameters[1].parameter_data.i=register_1;
}
#endif

static void instruction_ad_r (int instruction_code,ADDRESS *ad_p,int register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

static void instruction_i_ad (int instruction_code,LONG value,ADDRESS *ad_p)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_IMMEDIATE,
												parameter_data.i=value);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[1]);
}

static void instruction_d_ad (int instruction_code,LABEL *label,int arity,ADDRESS *ad_p)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	instruction->instruction_parameters[0].parameter_type=P_DESCRIPTOR_NUMBER;
	instruction->instruction_parameters[0].parameter_data.l=label;
	instruction->instruction_parameters[0].parameter_offset=arity;
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[1]);
}

static void instruction_fr_fr (int instruction_code,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	set_float_register_parameter (instruction->instruction_parameters[0],register_1);
	
	set_float_register_parameter (instruction->instruction_parameters[1],register_2);
}

static void instruction_l (int instruction_code,LABEL *label)
{
	register struct instruction *instruction;

	instruction=i_new_instruction1 (instruction_code);
	
	instruction->instruction_parameters[0].parameter_type=P_LABEL;
	instruction->instruction_parameters[0].parameter_data.l=label;
}

static void instruction_ad_r_r (int instruction_code,ADDRESS *ad_p,int register_1,int register_2)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction (instruction_code,3,3*sizeof (struct parameter));
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
	
	S2 (instruction->instruction_parameters[1],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
	
	S2 (instruction->instruction_parameters[2],	parameter_type=P_REGISTER,
												parameter_data.i=register_2);
}

static void instruction_r (int instruction_code,int register_1)
{
	register struct instruction *instruction;

	instruction=i_new_instruction1 (instruction_code);
	
	S2 (instruction->instruction_parameters[0],	parameter_type=P_REGISTER,
												parameter_data.i=register_1);
}

#ifdef M68000
void instruction_pd (int instruction_code,int register_1)
{
	register struct instruction *instruction;

	instruction=i_new_instruction1 (instruction_code);
	
	instruction->instruction_parameters[0].parameter_type=P_PRE_DECREMENT;
	instruction->instruction_parameters[0].parameter_data.i=register_1;
}
#endif

static void instruction_ad_x (int instruction_code,ADDRESS *ad_p,int offset,int register_1,int register_2)
{
	register struct instruction *instruction;
	register struct index_registers *index_registers;
	
	instruction=i_new_instruction
		(instruction_code,2,2*sizeof (struct parameter)+sizeof (struct index_registers));
	index_registers=(struct index_registers *)&instruction->instruction_parameters[2];
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);

	instruction->instruction_parameters[1].parameter_type=P_INDEXED;
	instruction->instruction_parameters[1].parameter_offset=offset;
	instruction->instruction_parameters[1].parameter_data.ir=index_registers;
	
	index_registers->a_reg.r=register_1;
	index_registers->d_reg.r=register_2;
}

static void to_data_addressing_mode (register ADDRESS *ad_p)
{
	if (ad_p->ad_mode==P_REGISTER && is_a_register (ad_p->ad_register)){
		int dreg;
		
		dreg=get_dregister();
		i_move_r_r (ad_p->ad_register,dreg);
		if (--*ad_p->ad_count_p==0)
			free_aregister (ad_p->ad_register);
		ad_p->ad_mode=P_REGISTER;
		ad_p->ad_register=dreg;
		ad_p->ad_count_p=&ad_p->ad_count;
		ad_p->ad_count=1;
	}
}

static void in_register (register ADDRESS *ad_p)
{
	int dreg;
	
	switch (ad_p->ad_mode){
		case P_REGISTER:
			return;
		case P_IMMEDIATE:
			dreg=get_dregister();
			i_move_i_r (ad_p->ad_offset,dreg);
			break;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			dreg=get_dregister();
			i_move_id_r (ad_p->ad_offset,ad_p->ad_register,dreg);
			break;
		case P_DESCRIPTOR_NUMBER:
			dreg=get_dregister();
			i_move_d_r (ad_p->ad_label,ad_p->ad_offset,dreg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
					dreg=get_dregister();
					i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,dreg);
				} else {
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);

					dreg=get_dregister();
					i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,dreg);
				}
				i_ad_p->ad_register=dreg;
			} else {
				dreg=i_ad_p->ad_register;
			}
			break;
		}
		default:
			internal_error_in_function ("in_register");
			return;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=dreg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void in_data_register (register ADDRESS *ad_p)
{
	int dreg;
	
	switch (ad_p->ad_mode){
		case P_REGISTER:
			if (is_d_register (ad_p->ad_register))
				return;
			dreg=get_dregister();
			i_move_r_r (ad_p->ad_register,dreg);
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			break;
		case P_IMMEDIATE:
			dreg=get_dregister();
			i_move_i_r (ad_p->ad_offset,dreg);
			break;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			dreg=get_dregister();
			i_move_id_r (ad_p->ad_offset,ad_p->ad_register,dreg);
			break;
		case P_DESCRIPTOR_NUMBER:
			dreg=get_dregister();
			i_move_d_r (ad_p->ad_label,ad_p->ad_offset,dreg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
					dreg=get_dregister();
					i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,dreg);
				} else {
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
					dreg=get_dregister();
					i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,dreg);
				}
				i_ad_p->ad_register=dreg;
			} else {
				dreg=i_ad_p->ad_register;
				if (!is_d_register (dreg)){
					int areg;
					
					areg=dreg;
					dreg=get_dregister();
					i_move_r_r (areg,dreg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (areg);
				}
			}
			break;
		}
		default:
			internal_error_in_function ("in_data_register");
			return;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=dreg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void in_alterable_data_register (register ADDRESS *ad_p)
{
	int dreg;
	
	switch (ad_p->ad_mode){
		case P_REGISTER:
			if (is_d_register (ad_p->ad_register)){
				if (*ad_p->ad_count_p==1)
					return;
				dreg=get_dregister();
				i_move_r_r (ad_p->ad_register,dreg);
				--*ad_p->ad_count_p;
			} else {
				dreg=get_dregister();
				i_move_r_r (ad_p->ad_register,dreg);
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
			}
			break;
		case P_IMMEDIATE:
			dreg=get_dregister();
			i_move_i_r (ad_p->ad_offset,dreg);
			break;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			dreg=get_dregister();
			i_move_id_r (ad_p->ad_offset,ad_p->ad_register,dreg);
			break;
		case P_DESCRIPTOR_NUMBER:
			dreg=get_dregister();
			i_move_d_r (ad_p->ad_label,ad_p->ad_offset,dreg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);

					dreg=get_dregister();
					i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,dreg);
				} else {
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
					dreg=get_dregister();
					i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,dreg);
				}
				i_ad_p->ad_register=dreg;
			} else {
				dreg=i_ad_p->ad_register;
				
				if (is_d_register (dreg)){
					if (*i_ad_p->ad_count_p>1){
						int old_dreg;
						
						old_dreg=dreg;
						dreg=get_dregister();
						i_move_r_r (old_dreg,dreg);
						--*i_ad_p->ad_count_p;
					}		
				} else {
					int areg;
					
					areg=dreg;
					dreg=get_dregister();
					i_move_r_r (areg,dreg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (areg);
				}
			}
			break;
		}
		default:
			internal_error_in_function ("in_alterable_data register");
			return;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=dreg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void in_address_register (register ADDRESS *ad_p)
{
	int areg;
	
	switch (ad_p->ad_mode){
		case P_REGISTER:
			if (is_a_register (ad_p->ad_register))
				return;
			areg=get_aregister();
			i_move_r_r (ad_p->ad_register,areg);
			if (--*ad_p->ad_count_p==0)
				free_dregister (ad_p->ad_register);
			break;
		case P_IMMEDIATE:
			areg=get_aregister();
			i_move_i_r (ad_p->ad_offset,areg);
			break;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			areg=get_aregister();
			i_move_id_r (ad_p->ad_offset,ad_p->ad_register,areg);
			break;
		case P_DESCRIPTOR_NUMBER:
			areg=get_aregister();
			i_move_d_r (ad_p->ad_label,ad_p->ad_offset,areg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
					areg=get_aregister();
					i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,areg);
				} else {
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
					areg=get_aregister();
					i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,areg);
				}
				i_ad_p->ad_register=areg;
			} else {
				areg=i_ad_p->ad_register;
				if (!is_a_register (areg)){
					int dreg;
					
					dreg=areg;
					areg=get_aregister();
					i_move_r_r (dreg,areg);
					if (--*i_ad_p->ad_count_p==0)
						free_dregister (dreg);
				}
			}
			break;
		}
		default:
			internal_error_in_function ("in_address_register");
			return;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=areg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void in_alterable_address_register (register ADDRESS *ad_p)
{
	int areg;
	
	switch (ad_p->ad_mode){
		case P_REGISTER:
			if (is_a_register (ad_p->ad_register)){
				if (*ad_p->ad_count_p==1)
					return;
				areg=get_aregister();
				i_move_r_r (ad_p->ad_register,areg);
				--*ad_p->ad_count_p;
			} else {
				areg=get_aregister();
				i_move_r_r (ad_p->ad_register,areg);
				if (--*ad_p->ad_count_p==0)
					free_dregister (ad_p->ad_register);
			}
			break;
		case P_IMMEDIATE:
			areg=get_aregister();
			i_move_i_r (ad_p->ad_offset,areg);
			break;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			areg=get_aregister();
			i_move_id_r (ad_p->ad_offset,ad_p->ad_register,areg);
			break;
		case P_DESCRIPTOR_NUMBER:
			areg=get_aregister();
			i_move_d_r (ad_p->ad_label,ad_p->ad_offset,areg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);
				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
					areg=get_aregister();
					i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,areg);
				} else 
				{
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
					areg=get_aregister();
					i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,areg);
				}
				i_ad_p->ad_register=areg;
			} else {
				areg=i_ad_p->ad_register;
				
				if (is_a_register (areg)){
					if (*i_ad_p->ad_count_p>1){
						int old_areg;
						
						old_areg=areg;
						areg=get_aregister();
						i_move_r_r (old_areg,areg);
						--*i_ad_p->ad_count_p;
					}		
				} else {
					int dreg;
					
					dreg=areg;
					areg=get_aregister();
					i_move_r_r (dreg,areg);
					if (--*i_ad_p->ad_count_p==0)
						free_dregister (dreg);
				}
			}
			break;
		}
		default:
			internal_error_in_function ("in_alterable_address_register");
			return;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=areg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void register_node (INSTRUCTION_GRAPH graph,int reg)
{
	graph->instruction_code=GREGISTER;
	graph->instruction_parameters[0].i=reg;
		
	if ((unsigned)a_reg_num (reg)<(unsigned)N_ADDRESS_PARAMETER_REGISTERS)
		global_block.block_graph_a_register_parameter_node[a_reg_num (reg)]=graph;
		
	if ((unsigned)d_reg_num (reg)<(unsigned)N_DATA_PARAMETER_REGISTERS
#ifdef MORE_PARAMETER_REGISTERS
									+ N_ADDRESS_PARAMETER_REGISTERS
#endif
	)
		global_block.block_graph_d_register_parameter_node[d_reg_num (reg)]=graph;
}

static void float_register_node (register INSTRUCTION_GRAPH graph,int reg)
{
	graph->instruction_code=GFREGISTER;
	graph->instruction_parameters[0].i=reg;
			
	if ((unsigned)reg<(unsigned)N_FLOAT_PARAMETER_REGISTERS)
		global_block.block_graph_f_register_parameter_node [reg]=graph;
}

static void linearize_graph (INSTRUCTION_GRAPH,ADDRESS *);

#if defined (G_POWER)
# define SMALL_IMMEDIATE(i) (((short int)i)==((int)i))
#endif
#if defined (sparc)
# define SMALL_IMMEDIATE(i) ((int)i<4096 && (int)i>=-4095)
#endif

static void linearize_dyadic_commutative_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}
	
	if (graph->instruction_d_min_a_cost<=0){
		if (ad_1.ad_mode==P_REGISTER && is_d_register (ad_1.ad_register) && *ad_1.ad_count_p<=1 && 
			!(ad_2.ad_mode==P_REGISTER && is_d_register (ad_2.ad_register) && *ad_2.ad_count_p<=1))
		{
			instruction_ad_r (i_instruction_code,&ad_2,ad_1.ad_register);
			--*ad_1.ad_count_p;
			reg_1=ad_1.ad_register;
		} else {
#ifndef M68000 /* optimization added 18-8-1998 */
			if (ad_2.ad_mode==P_IMMEDIATE){
# if defined (G_POWER) || defined (sparc)
				if (ad_1.ad_mode==P_REGISTER && SMALL_IMMEDIATE (ad_2.ad_offset) && i_instruction_code==IADD){
					if (--*ad_1.ad_count_p==0)
						free_register (ad_1.ad_register);	
					reg_1=get_dregister();								
					i_addi_r_r (ad_2.ad_offset,ad_1.ad_register,reg_1);
				} else
# endif
				{
				in_alterable_data_register (&ad_1);
				instruction_ad_r (i_instruction_code,&ad_2,ad_1.ad_register);
				--*ad_1.ad_count_p;
				reg_1=ad_1.ad_register;
				}
			} else {
#endif
#if defined (G_POWER) || defined (sparc)
			if (ad_1.ad_mode==P_IMMEDIATE && ad_2.ad_mode==P_REGISTER && SMALL_IMMEDIATE (ad_1.ad_offset) && i_instruction_code==IADD){
				if (--*ad_2.ad_count_p==0)
					free_register (ad_2.ad_register);
				reg_1=get_dregister();								
				i_addi_r_r (ad_1.ad_offset,ad_2.ad_register,reg_1);
			} else
#endif

			{
#if 1 /* optimization added 9-1-2000 */
			if (! (ad_1.ad_mode==P_REGISTER && ad_2.ad_mode==P_REGISTER && ad_1.ad_register==ad_2.ad_register &&
				is_d_register (ad_1.ad_register) && *ad_1.ad_count_p==2 && ad_1.ad_count_p==ad_2.ad_count_p))
#endif
				in_alterable_data_register (&ad_2);
			instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
			--*ad_2.ad_count_p;
			reg_1=ad_2.ad_register;
			}
#ifndef M68000
			}
#endif
		} 
	} else {
# if defined (G_POWER) || defined (sparc)
		if (ad_1.ad_mode==P_IMMEDIATE && ad_2.ad_mode==P_REGISTER && SMALL_IMMEDIATE (ad_1.ad_offset) && i_instruction_code==IADD){
			if (--*ad_2.ad_count_p==0)
				free_register (ad_2.ad_register);
			reg_1=get_aregister();								
			i_addi_r_r (ad_1.ad_offset,ad_2.ad_register,reg_1);
		} else if (ad_2.ad_mode==P_IMMEDIATE && ad_1.ad_mode==P_REGISTER && SMALL_IMMEDIATE (ad_2.ad_offset) && i_instruction_code==IADD){
			if (--*ad_1.ad_count_p==0)
				free_register (ad_1.ad_register);	
			reg_1=get_aregister();								
			i_addi_r_r (ad_2.ad_offset,ad_1.ad_register,reg_1);
		} else
# endif

		if ((ad_1.ad_mode==P_REGISTER || ad_1.ad_mode==P_INDIRECT) 
			&& is_a_register (ad_1.ad_register) && *ad_1.ad_count_p<=1)
		{
			in_alterable_address_register (&ad_1);
			instruction_ad_r (i_instruction_code,&ad_2,ad_1.ad_register);
			--*ad_1.ad_count_p;
			reg_1=ad_1.ad_register;
		} else {
			in_alterable_address_register (&ad_2);
			instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
			--*ad_2.ad_count_p;
			reg_1=ad_2.ad_register;
		}
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

static void linearize_dyadic_commutative_data_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	register INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}
	
	if (ad_1.ad_mode==P_REGISTER && (is_a_register (ad_1.ad_register) || *ad_1.ad_count_p<=1) &&
		!(ad_2.ad_mode==P_REGISTER && is_d_register (ad_2.ad_register) && *ad_2.ad_count_p<=1))
	{
		in_alterable_data_register (&ad_1);
		to_data_addressing_mode (&ad_2);
		instruction_ad_r (i_instruction_code,&ad_2,ad_1.ad_register);
		--*ad_1.ad_count_p;
		reg_1=ad_1.ad_register;
	} else {
		in_alterable_data_register (&ad_2);
		to_data_addressing_mode (&ad_1);
		instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
		--*ad_2.ad_count_p;
		reg_1=ad_2.ad_register;
	}
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

#ifdef M68000
static void linearize_eor_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	register INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}
	
	in_alterable_data_register (&ad_2);
	if (ad_1.ad_mode!=P_IMMEDIATE)
		in_data_register (&ad_1);
	instruction_ad_r (IEOR,&ad_1,ad_2.ad_register);
	--*ad_2.ad_count_p;
	reg_1=ad_2.ad_register;
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}
#endif

static int compare_node (INSTRUCTION_GRAPH graph,int i_test_1,int i_test_2)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int mode_1,mode_2;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph_1->instruction_code==GLOAD_DES_ID && graph_1->node_count==1 
		&& graph_2->instruction_code==GLOAD_DES_I && graph_2->node_count==1)
	{
		linearize_graph (graph_1->instruction_parameters[1].p,&ad_1);
		in_address_register (&ad_1);
		if (--*ad_1.ad_count_p==0)
			free_aregister (ad_1.ad_register);
		i_cmpw_d_id (graph_2->instruction_parameters[0].l,graph_2->instruction_parameters[1].i,
					 graph_1->instruction_parameters[0].i,ad_1.ad_register);
		return i_test_2;
	} else
		if (graph_2->instruction_code==GLOAD_DES_ID && graph_2->node_count==1
			&& graph_1->instruction_code==GLOAD_DES_I && graph_1->node_count==1)
		{
			linearize_graph (graph_2->instruction_parameters[1].p,&ad_1);
			in_address_register (&ad_1);
			if (--*ad_1.ad_count_p==0)
				free_aregister (ad_1.ad_register);
			i_cmpw_d_id (graph_1->instruction_parameters[0].l,
						 graph_1->instruction_parameters[1].i,
						 graph_2->instruction_parameters[0].i,ad_1.ad_register);
			return i_test_1;
		} else {
			if (graph->order_left){
				linearize_graph (graph_1,&ad_1);
				linearize_graph (graph_2,&ad_2);
			} else {
				linearize_graph (graph_2,&ad_2);
				linearize_graph (graph_1,&ad_1);
			}

			mode_1=ad_1.ad_mode;
			mode_2=ad_2.ad_mode;

			if (mode_1==P_IMMEDIATE && ! (mode_2==P_IMMEDIATE || mode_2==P_DESCRIPTOR_NUMBER)){
				LONG i;
#ifdef M68000
				int real_dreg_number;
#endif
				i=ad_1.ad_offset;
#ifdef M68000
				if (i<128 && i>=-128 && i!=0 && (real_dreg_number=try_get_real_dregister_number())>=0){
					int dreg;

					dreg=num_to_d_reg (real_dreg_number);
					i_move_i_r (i,dreg);
					instruction_ad_r (ICMP,&ad_2,dreg);
					free_dregister (dreg);
					return i_test_2;
				} else {
#endif
					instruction_i_ad (ICMP,i,&ad_2);
					return i_test_1;
#ifdef M68000
				}
#endif
			} else if (mode_1==P_DESCRIPTOR_NUMBER 
					   && ! (mode_2==P_IMMEDIATE || mode_2==P_DESCRIPTOR_NUMBER)){
				instruction_d_ad (ICMP,ad_1.ad_label,ad_1.ad_offset,&ad_2);
				return i_test_1;
			} else if (mode_2==P_IMMEDIATE 
					   && ! (mode_1==P_IMMEDIATE || mode_1==P_DESCRIPTOR_NUMBER))
			{
				LONG i;
#ifdef M68000
				int real_dreg_number;
#endif
				
				i=ad_2.ad_offset;
#ifdef M68000
				if (i<128 && i>=-128 && i!=0 && (real_dreg_number=try_get_real_dregister_number())>=0){
					int dreg;
					
					dreg=num_to_d_reg (real_dreg_number);
					i_move_i_r (i,dreg);
					instruction_ad_r (ICMP,&ad_1,dreg);
					free_dregister (dreg);
					return i_test_1;
				} else {
#endif
					instruction_i_ad (ICMP,i,&ad_1);
					return i_test_2;
#ifdef M68000
				}
#endif
			} else if (mode_2==P_DESCRIPTOR_NUMBER 
					   && ! (mode_1==P_IMMEDIATE || mode_1==P_DESCRIPTOR_NUMBER)){
				instruction_d_ad (ICMP,ad_2.ad_label,ad_2.ad_offset,&ad_1);
				return i_test_2;
			} else if (mode_2==P_REGISTER 
					   || (mode_1!=P_REGISTER && mode_2==P_INDIRECT && *ad_2.ad_count_p==1)){
				in_register (&ad_2);
				instruction_ad_r (ICMP,&ad_1,ad_2.ad_register);
				if (--*ad_2.ad_count_p==0)
					free_register (ad_2.ad_register);
				return i_test_1;
			} else {	
				in_register (&ad_1);
				instruction_ad_r (ICMP,&ad_2,ad_1.ad_register);
				if (--*ad_1.ad_count_p==0)
					free_register (ad_1.ad_register);
				return i_test_2;
			}
		}
}

enum {
	CEQ,	CNE,	CGT,	CLT,	CGE,	CLE,
	CO,		CNO,	CGTU,	CLTU,	CGEU,	CLEU,
	CFEQ,	CFNE,	CFGT,	CFLT,	CFGE,	CFLE
};

#define is_float_condition(c) ((c)>=CFEQ)

int condition_to_set_instruction[]=
{
	ISEQ,	ISNE,	ISGT,	ISLT,	ISGE,	ISLE,
	ISO,	ISNO,	ISGTU,	ISLTU,	ISGEU,	ISLEU,
	IFSEQ,	IFSNE,	IFSGT,	IFSLT,	IFSGE,	IFSLE
};

static int condition_to_branch_false_instruction[]=
{
	IBNE,	IBEQ,	IBLE,	IBGE,	IBLT,	IBGT,
	IBNO,	IBO,	IBLEU,	IBGEU,	IBLTU,	IBGTU,
	IFBNE,	IFBEQ,	IFBLE,	IFBGE,	IFBLT,	IFBGT
};

static int condition_to_branch_true_instruction[]=
{
	IBEQ,	IBNE,	IBGT,	IBLT,	IBGE,	IBLE,
	IBO,	IBNO,	IBGTU,	IBLTU,	IBGEU,	IBLEU,
	IFBEQ,	IFBNE,	IFBGT,	IFBLT,	IFBGE,	IFBLE
};

static void save_condition (INSTRUCTION_GRAPH graph,int condition)
{
	int reg_1;

	reg_1=get_dregister();
	instruction_r (condition_to_set_instruction[condition],reg_1);

	if (graph->instruction_d_min_a_cost>0){
		int areg;
	
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		reg_1=areg;
	}

	register_node (graph,reg_1);
	--graph->node_count;
}

static int float_compare_node (INSTRUCTION_GRAPH graph,int condition_1,int condition_2);
static int linearize_not_condition (INSTRUCTION_GRAPH graph);

static int linearize_condition (INSTRUCTION_GRAPH graph)
{
	int condition;
	
	switch (graph->instruction_code){
		case GCMP_EQ:
			condition=compare_node (graph,CEQ,CEQ);
			break;
		case GCMP_LT:
			condition=compare_node (graph,CLT,CGT);
			break;
		case GCMP_LTU:
			condition=compare_node (graph,CLTU,CGTU);
			break;
		case GCMP_GT:
			condition=compare_node (graph,CGT,CLT);
			break;
		case GCMP_GTU:
			condition=compare_node (graph,CGTU,CLTU);
			break;
		case GFCMP_EQ:
			condition=float_compare_node (graph,CFEQ,CFEQ);
			break;
		case GFCMP_LT:
			condition=float_compare_node (graph,CFLT,CFGT);
			break;
		case GFCMP_GT:
			condition=float_compare_node (graph,CFGT,CFLT);
			break;
		case GCNOT:
			condition=linearize_not_condition (graph->instruction_parameters[0].p);
			break;
		case GTEST_O:
			if (graph->node_count==0){
#ifdef sparc
				if (graph->inode_arity==0)
					return CNE;
				else
#endif
				return CO;
			} /* else default */
		default:
		{
			ADDRESS ad_1;
			
			linearize_graph (graph,&ad_1);
			
			if (ad_1.ad_mode==P_REGISTER && is_a_register (ad_1.ad_register)){
				/* TST.L An is an illegal instruction, use CMP.L #0,An */
				i_cmp_i_r (0,ad_1.ad_register);
				if (--*ad_1.ad_count_p==0)
					free_aregister (ad_1.ad_register);
			} else {
				if (ad_1.ad_mode==P_IMMEDIATE)	/* TST #d is an illegal instruction */
					in_data_register (&ad_1);
				instruction_ad (ITST,&ad_1);
			}
			return CNE;
		}
	}
	if (graph->node_count>1){
		save_condition (graph,condition);
		condition=CNE;
	}
	return condition;
}

static int linearize_not_condition (INSTRUCTION_GRAPH graph)
{
	int condition;
	
	switch (graph->instruction_code){
		case GCMP_EQ:
			condition=compare_node (graph,CNE,CNE);
			break;
		case GCMP_LT:
			condition=compare_node (graph,CGE,CLE);
			break;
		case GCMP_LTU:
			condition=compare_node (graph,CGEU,CLEU);
			break;
		case GCMP_GT:
			condition=compare_node (graph,CLE,CGE);
			break;
		case GCMP_GTU:
			condition=compare_node (graph,CLEU,CGEU);
			break;
		case GFCMP_EQ:
			condition=float_compare_node (graph,CFNE,CFNE);
			break;
		case GFCMP_LT:
			condition=float_compare_node (graph,CFGE,CFLE);
			break;
		case GFCMP_GT:
			condition=float_compare_node (graph,CFLE,CFGE);
			break;
		case GCNOT:
			condition=linearize_condition (graph->instruction_parameters[0].p);
			break;
		case GTEST_O:
			if (graph->node_count==0){
#ifdef sparc
				if (graph->inode_arity==0)
					return CEQ;
				else
#endif
				return CNO;
			} /* else default */
		default:
		{
			ADDRESS ad_1;
	
			linearize_graph (graph,&ad_1);
			
			if (ad_1.ad_mode==P_REGISTER && is_a_register (ad_1.ad_register)){
				/* TST.L An is an illegal instruction, use CMP.L #0,An */
				/* (MOVE.L An,Dn would be faster */
				i_cmp_i_r (0,ad_1.ad_register);
				if (--*ad_1.ad_count_p==0)
					free_aregister (ad_1.ad_register);
			} else {
				if (ad_1.ad_mode==P_IMMEDIATE)	/* TST #d is an illegal instruction */
					in_data_register (&ad_1);
				instruction_ad (ITST,&ad_1);
			}
			return CEQ;
		}
	}
	if (graph->node_count>1){
		save_condition (graph,condition);
		condition=CEQ;
	}
	return condition;
}

static void condition_to_register (INSTRUCTION_GRAPH graph,int condition,ADDRESS *ad_p)
{
	int reg_1;
	
	reg_1=get_dregister();
	instruction_r (condition_to_set_instruction[condition],reg_1);
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		reg_1=areg;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

static void linearize_compare_operator (int i_test_1,int i_test_2,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	int condition;
	
	condition=compare_node (graph,i_test_1,i_test_2);
	condition_to_register (graph,condition,ad_p);
}

static void linearize_dyadic_non_commutative_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}

# if defined (G_POWER) || defined (sparc)
	if (ad_1.ad_mode==P_IMMEDIATE && ad_2.ad_mode==P_REGISTER && SMALL_IMMEDIATE (-ad_1.ad_offset) && i_instruction_code==ISUB){
		int reg_1;
		
		if (--*ad_2.ad_count_p==0)
			free_register (ad_2.ad_register);	

		if (graph->instruction_d_min_a_cost<=0)
			reg_1=get_dregister();
		else
			reg_1=get_aregister();

		i_addi_r_r (-ad_1.ad_offset,ad_2.ad_register,reg_1);

		ad_p->ad_mode=P_REGISTER;
		ad_p->ad_register=reg_1;
		
		ad_p->ad_count_p=&graph->node_count;
		if (*ad_p->ad_count_p>1)
			register_node (graph,reg_1);
		
		return;
	}
# endif
	
	if (graph->instruction_d_min_a_cost<=0)
		in_alterable_data_register (&ad_2);
	else
		in_alterable_address_register (&ad_2);
	instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
	--*ad_2.ad_count_p;

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=ad_2.ad_register;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,ad_p->ad_register);
}

static void linearize_dyadic_non_commutative_data_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	register INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}
	
	in_alterable_data_register (&ad_2);
	to_data_addressing_mode (&ad_1);
	instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
	--*ad_2.ad_count_p;
	reg_1=ad_2.ad_register;
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

static void linearize_monadic_data_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1;
	ADDRESS ad_1;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	
	linearize_graph (graph_1,&ad_1);
	
	in_alterable_data_register (&ad_1);
	reg_1=ad_1.ad_register;
	instruction_r (i_instruction_code,reg_1);
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

#if defined (I486) || defined (G_POWER)
static void linearize_div_rem_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}

	in_alterable_data_register (&ad_2);
	to_data_addressing_mode (&ad_1);

# ifdef I486
	if (ad_1.ad_mode==P_IMMEDIATE){
		if (i_instruction_code==IDIVU || i_instruction_code==IREMU){
			in_data_register (&ad_1);
			instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);		
		} else {
			int i;
			
			i=ad_1.ad_offset;
			if (i_instruction_code==IMOD && i<0 && i!=0x80000000)
				i=-i;
			
			if ((i & (i-1))==0 && (i_instruction_code==IMOD ? i>1 : i>0))
				instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
			else if (i>1 || (i<-1 && i!=0x80000000)){
				int tmp_reg;
				
				tmp_reg=get_dregister();
				instruction_ad_r_r (i_instruction_code==IDIV ? IDIVI : IREMI,&ad_1,ad_2.ad_register,tmp_reg);
				free_dregister (tmp_reg);
			} else {
				in_data_register (&ad_1);
				instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
			}
		}
	} else {
		if (ad_1.ad_mode==P_INDEXED)
			in_data_register (&ad_1);
		instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
	}
# else
	instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
# endif

	--*ad_2.ad_count_p;
	reg_1=ad_2.ad_register;
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}
#else
static void linearize_mod_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	register INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}
	
	in_alterable_data_register (&ad_2);
	to_data_addressing_mode (&ad_1);

	if (ad_1.ad_mode==P_INDEXED)
		in_data_register (&ad_1);

	if (ad_1.ad_mode==P_REGISTER && is_d_register (ad_1.ad_register)){
		if (*ad_1.ad_count_p>1)
			in_alterable_data_register (&ad_1);
		++*ad_1.ad_count_p;
		reg_1=ad_1.ad_register;
	} else
		reg_1=get_dregister();
	instruction_ad_r_r (IMOD,&ad_1,reg_1,ad_2.ad_register);
	free_dregister (ad_2.ad_register);
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}
#endif

static void linearize_shift_operator (int i_instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	
	if (graph->order_left){
		linearize_graph (graph_1,&ad_1);
		linearize_graph (graph_2,&ad_2);
	} else {
		linearize_graph (graph_2,&ad_2);
		linearize_graph (graph_1,&ad_1);
	}

#if defined (G_POWER) || defined (sparc)
	if (i_instruction_code==ILSL && ad_1.ad_mode==P_IMMEDIATE && (unsigned)ad_1.ad_offset<(unsigned)32){
		in_register (&ad_2);
		if (--*ad_2.ad_count_p==0)
			free_register (ad_2.ad_register);
		
		if (graph->instruction_d_min_a_cost<=0)
			reg_1=get_dregister();
		else
			reg_1=get_aregister();
					
		i_lsli_r_r (ad_1.ad_offset,ad_2.ad_register,reg_1);
	} else {
#endif

	in_alterable_data_register (&ad_2);

#ifdef M68000
	if (ad_1.ad_mode!=P_IMMEDIATE || ad_1.ad_offset<=0 || ad_1.ad_offset>8)
#else
	if (ad_1.ad_mode!=P_IMMEDIATE || ad_1.ad_offset<0 || ad_1.ad_offset>=32)
#endif
		in_data_register (&ad_1);

	instruction_ad_r (i_instruction_code,&ad_1,ad_2.ad_register);
	--*ad_2.ad_count_p;

	reg_1=ad_2.ad_register;
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg_1,areg);
		free_dregister (reg_1);
		reg_1=areg;
	}

#if defined (G_POWER) || defined (sparc)
	}
#endif

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	
	ad_p->ad_count_p=&graph->node_count;
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

static void linearize_integer_o_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_r_p,ADDRESS *ad_f_p,
											void linearize_operator (int,INSTRUCTION_GRAPH,ADDRESS *),int i_instruction_code)
{
	INSTRUCTION_GRAPH test_o_graph;
	int reg_1;

	test_o_graph=graph->instruction_parameters[2].p;

	if (ad_r_p!=NULL){
		linearize_operator (i_instruction_code,graph,ad_r_p);
	} else {
		ADDRESS ad_r;

		linearize_operator (i_instruction_code,graph,&ad_r);
		
		if (graph->node_count>0){
			register_node (graph,ad_r.ad_register);
		} else
			free_register (ad_r.ad_register);
	}

	if (test_o_graph->node_count!=0){
		reg_1=get_dregister();
#ifdef sparc
		instruction_r (condition_to_set_instruction[test_o_graph->inode_arity==0 ? CNE : CO],reg_1);
#else
		instruction_r (condition_to_set_instruction[CO],reg_1);
#endif			
		if (ad_f_p==NULL){
			register_node (test_o_graph,reg_1);
		} else {
			ad_f_p->ad_mode=P_REGISTER;
			ad_f_p->ad_register=reg_1;			
			ad_f_p->ad_count_p=&test_o_graph->node_count;
			
			if (test_o_graph->node_count>1)
				register_node (test_o_graph,reg_1);
		}
	}
}

static void linearize_add_o_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_r_p,ADDRESS *ad_f_p)
{
	linearize_integer_o_operator (graph,ad_r_p,ad_f_p,linearize_dyadic_commutative_operator,
#if defined (sparc) || defined (G_POWER)
		IADDO
#else
		IADD
#endif
		);
}

static void linearize_sub_o_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_r_p,ADDRESS *ad_f_p)
{
	linearize_integer_o_operator (graph,ad_r_p,ad_f_p,linearize_dyadic_non_commutative_operator,
#if defined (sparc) || defined (G_POWER)
		ISUBO
#else
		ISUB
#endif
		);
}

static void linearize_mul_o_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_r_p,ADDRESS *ad_f_p)
{
	linearize_integer_o_operator (graph,ad_r_p,ad_f_p,linearize_dyadic_commutative_data_operator,
#ifdef G_POWER
		IMULO
#else
		IMUL
#endif
		);
}

static void linearize_test_o_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH operator_graph;

	operator_graph=graph->instruction_parameters[0].p;

	switch (operator_graph->instruction_code){
		case GADD_O:
			linearize_add_o_operator (operator_graph,NULL,ad_p);
			break;
		case GMUL_O:
			linearize_mul_o_operator (operator_graph,NULL,ad_p);
			break;
		case GSUB_O:
			linearize_sub_o_operator (operator_graph,NULL,ad_p);
			break;
		default:
			internal_error_in_function ("linearize_test_o_operator");
	}
}

static void linearize_conditional_not_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	int condition;
	
	condition=linearize_not_condition (graph->instruction_parameters[0].p);
	condition_to_register (graph,condition,ad_p);
}

static void linearize_float_graph (INSTRUCTION_GRAPH graph,ADDRESS *ad_p);

#define MAX(a,b) ((a)>(b)?(a):(b))

static ADDRESS *evaluate_arguments (union instruction_parameter arguments[],int n_arguments)
{
	int argument_number;
	char *argument_evaluated;
	ADDRESS *ad_a;
	char small_argument_evaluated[32];

	ad_a=(ADDRESS*)memory_allocate (sizeof (ADDRESS) * n_arguments);
	
	if (n_arguments<32)
		argument_evaluated=small_argument_evaluated;
	else
		argument_evaluated=(char*)memory_allocate (sizeof (char) * n_arguments);
	
	for (argument_number=0; argument_number<n_arguments; ++argument_number)
		argument_evaluated[argument_number]=(arguments[argument_number].p==NULL);
	
	for (;;){
		int first_argument_number;
		INSTRUCTION_GRAPH a_graph_1;
		
		first_argument_number=0;
		while (first_argument_number<n_arguments && argument_evaluated[first_argument_number])
			++first_argument_number;
		
		if (first_argument_number>=n_arguments)
			break;
			
		a_graph_1=arguments[first_argument_number].p;
		
		for (argument_number=first_argument_number+1; argument_number<n_arguments; ++argument_number){
			if (!argument_evaluated[argument_number]){
				INSTRUCTION_GRAPH a_graph_2;
				int i1,i2,u1,u2;
				int a,d;
				
				a_graph_2=arguments[argument_number].p;
				
				a=a_graph_1->i_aregs; d=a_graph_1->i_dregs; i1=AD_REG_WEIGHT (a,d);
				a=a_graph_2->i_aregs; d=a_graph_2->i_dregs; i2=AD_REG_WEIGHT (a,d);
				
				a=a_graph_1->u_aregs; d=a_graph_1->u_dregs; u1=AD_REG_WEIGHT (a,d);
				a=a_graph_2->u_aregs; d=a_graph_2->u_dregs; u2=AD_REG_WEIGHT (a,d);
				
				if (i1<0){
					if (i2<0 && (u2<u1 || (u1==u2 && i2<i1))){
						first_argument_number=argument_number;
						a_graph_1=a_graph_2;
					}
				} else if (i1==0){
					if (i2<0 || (i2==0 && u2<u1)){
						first_argument_number=argument_number;
						a_graph_1=a_graph_2;
					}
				} else {
					if (i2<=0 || (u2-i2>u1-i1 || (u2-i2==u1-i1 && i2<i1))){
						first_argument_number=argument_number;
						a_graph_1=a_graph_2;
					}
				}
			}		
		}
		
		if (a_graph_1->instruction_code==GFHIGH &&
			first_argument_number+1<n_arguments &&
			!argument_evaluated[first_argument_number+1])
		{
			INSTRUCTION_GRAPH next_graph;
			
			next_graph=arguments[first_argument_number+1].p;
			if (next_graph!=NULL && next_graph->instruction_code==GFLOW &&
				a_graph_1->instruction_parameters[0].p==next_graph->instruction_parameters[0].p)
			{
				INSTRUCTION_GRAPH f_graph;
				
				f_graph=a_graph_1->instruction_parameters[0].p;
				arguments[first_argument_number].p=f_graph;
				arguments[first_argument_number+1].p=f_graph;
				
				/* CHANGED 22-10-1999 */
				{
				int f_graph_count;

				f_graph_count=f_graph->node_count;
				if (--a_graph_1->node_count==0)
					--f_graph_count;
				if (--next_graph->node_count==0)
					--f_graph_count;
				f_graph->node_count=f_graph_count+1;
				}				
				/*
				--f_graph->node_count;
				*/
				
				linearize_float_graph (f_graph,&ad_a[first_argument_number]);
				argument_evaluated[first_argument_number]=1;
				argument_evaluated[first_argument_number+1]=1;

				ad_a[first_argument_number].ad_mode+=100;
				continue;
			}
		}

		linearize_graph (a_graph_1,&ad_a[first_argument_number]);
		argument_evaluated[first_argument_number]=1;
	}

	if (argument_evaluated!=small_argument_evaluated)
		memory_free (argument_evaluated);
	
	return ad_a;
}

#ifdef M68000
static void move_float_ad_pi (ADDRESS *ad_p,int areg)
{
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
			i_fmove_fr_pi (ad_p->ad_register,areg);
			if (--*ad_p->ad_count_p==0)
				free_fregister (ad_p->ad_register);
			return;
		case P_INDIRECT:
			if (ad_p->ad_offset==0 && *ad_p->ad_count_p==1){
				i_move_pi_pi (ad_p->ad_register,areg);
				i_move_id_pi (0,ad_p->ad_register,areg);
				*ad_p->ad_count_p=0;
				free_aregister (ad_p->ad_register);
			} else {
				i_move_id_pi (ad_p->ad_offset,ad_p->ad_register,areg);
				i_move_id_pi (ad_p->ad_offset+4,ad_p->ad_register,areg);
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
			}
			return;
		case P_F_IMMEDIATE:
			i_move_i_pi (((LONG*)&ad_p->ad_register)[0],areg);
			i_move_i_pi (((LONG*)&ad_p->ad_register)[1],areg);
			return;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

				if (i_ad_p->ad_mode==P_INDEXED){
					i_move_x_pi (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,areg);
					i_move_x_pi (i_ad_p->ad_offset+(4<<2),i_ad_p->ad_areg,i_ad_p->ad_dreg,areg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
				} else {
					i_move_id_pi (i_ad_p->ad_offset,i_ad_p->ad_register,areg);
					i_move_id_pi (i_ad_p->ad_offset+4,i_ad_p->ad_register,areg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
				}
			} else {
				i_fmove_fr_pi (i_ad_p->ad_register,areg);
				if (--*i_ad_p->ad_count_p==0)
					free_fregister (i_ad_p->ad_register);
			}
			return;
		}
		default:
			internal_error_in_function ("move_float_ad_pi");
	}
}
#endif

#ifndef M68000
LONG offset_from_heap_register;
#endif
#ifdef G_POWER
LONG heap_pointer_offset_in_basic_block;
#endif

void evaluate_arguments_and_free_addresses (union instruction_parameter arguments[],int n_arguments)
{
	ADDRESS *ad_a;
	
	ad_a=evaluate_arguments (arguments,n_arguments);
	memory_free (ad_a);
}

#if defined (FP_STACK_OPTIMIZATIONS) || defined (FMADD)
#define FP_REG_LAST_USE 4
#endif

static void move_float_ad_id (ADDRESS *ad_p,int offset,int areg)
{
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
			i_fmove_fr_id (ad_p->ad_register,offset,areg);
			if (--*ad_p->ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
				last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
				free_fregister (ad_p->ad_register);
			}
			return;
		case P_INDIRECT:
#ifdef M68000
			if (ad_p->ad_offset==0 && *ad_p->ad_count_p==1){
				i_move_pi_id (ad_p->ad_register,offset,areg);
				i_move_id_id (0,ad_p->ad_register,offset+4,areg);
				*ad_p->ad_count_p=0;
				free_aregister (ad_p->ad_register);
			} else {
#endif
				i_move_id_id (ad_p->ad_offset,ad_p->ad_register,offset,areg);
				i_move_id_id (ad_p->ad_offset+4,ad_p->ad_register,offset+4,areg);
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
#ifdef M68000
			}
#endif
			return;
		case P_F_IMMEDIATE:
			i_move_i_id (((LONG*)&ad_p->ad_register)[0],offset,areg);
			i_move_i_id (((LONG*)&ad_p->ad_register)[1],offset+4,areg);
			return;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

				if (i_ad_p->ad_mode==P_INDEXED){
#ifdef sparc
					if (i_ad_p->ad_offset!=0){
						int a_reg,offset1;
												
						if (--*i_ad_p->ad_count_p==0)
							free_aregister (i_ad_p->ad_areg);
						a_reg=get_aregister();			

						offset1=i_ad_p->ad_offset>>2;

						i_lea_x_r (i_ad_p->ad_areg,i_ad_p->ad_dreg,a_reg);
						i_move_id_id (offset1,a_reg,offset,areg);
						i_move_id_id (offset1+4,a_reg,offset+4,areg);
						free_aregister (a_reg);

						if (--*i_ad_p->ad_count_p2==0)
							free_dregister (i_ad_p->ad_dreg);
						return;
					}
#endif
					i_move_x_id (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,offset,areg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
#if defined (M68000) || defined (I486)
					i_move_x_id (i_ad_p->ad_offset+(4<<2),i_ad_p->ad_areg,i_ad_p->ad_dreg,offset+4,areg);
#else
					{
						int a_reg;

						a_reg=get_aregister();			
						i_lea_x_r (i_ad_p->ad_areg,i_ad_p->ad_dreg,a_reg);
						i_move_id_id (4,a_reg,offset+4,areg);
						free_aregister (a_reg);
					}
#endif
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
				} else {
					i_move_id_id (i_ad_p->ad_offset,i_ad_p->ad_register,offset,areg);
					i_move_id_id (i_ad_p->ad_offset+4,i_ad_p->ad_register,offset+4,areg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
				}
			} else {
				i_fmove_fr_id (i_ad_p->ad_register,offset,areg);
				if (--*i_ad_p->ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
					last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
					free_fregister (i_ad_p->ad_register);
				}
			}
			return;
		}
		default:
			internal_error_in_function ("move_float_ad_id");
	}
}

static void linearize_load_graph (register INSTRUCTION_GRAPH load_graph)
{
	if (load_graph!=NULL){
		if (load_graph->node_count>0){
			switch (load_graph->instruction_code){
				case GLOAD:
				{
					int reg;
					
					if (load_graph->instruction_d_min_a_cost<=0)
						reg=get_dregister();
					else
						reg=get_aregister();
						
					i_move_id_r (load_graph->instruction_parameters[0].i,load_graph->instruction_parameters[1].i,reg);
					
					register_node (load_graph,reg);
					break;
				}
				case GFLOAD:
				{
					int reg;
					
					reg=get_fregister();
					
					i_fmove_id_fr (load_graph->instruction_parameters[0].i,load_graph->instruction_parameters[1].i,reg);
					
					float_register_node (load_graph,reg);
					
					break;
				}
				case GREGISTER:
				case GGREGISTER:
				case GFREGISTER:
				case GGFREGISTER:
					break;
				default:
					internal_error_in_function ("linearize_load_graph");
			}
		}
		/* added 24-8-1999 */
		else if (load_graph->instruction_code==GFHIGH || load_graph->instruction_code==GFLOW){
			load_graph=load_graph->instruction_parameters[0].p;
			if (load_graph->node_count>0 && load_graph->instruction_code==GFLOAD){
				int reg;
				
				reg=get_fregister();
				
				i_fmove_id_fr (load_graph->instruction_parameters[0].i,load_graph->instruction_parameters[1].i,reg);
				
				float_register_node (load_graph,reg);			
			}
		}
		/* */
	}
}

static void linearize_create_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p,int result_register)
{
	int argument_number,arity;
	ADDRESS *ad_a;
	int reg_1,heap_offset;
#ifndef M68000
	int reg_1_allocated;
	
	reg_1_allocated=0;
#endif

	arity=graph->inode_arity;

	last_block->block_n_new_heap_cells+=arity;

#ifdef G_POWER
	for (argument_number=0; argument_number<arity; ++argument_number){
		INSTRUCTION_GRAPH a_graph;
		
		a_graph=graph->instruction_parameters[argument_number].p;
		
		if (a_graph!=NULL && a_graph->instruction_code==GCREATE && a_graph->node_count==1)
			a_graph->instruction_code=GCREATE_S;
	}
#endif

	ad_a=evaluate_arguments (graph->instruction_parameters,arity);

#ifndef M68000
# ifdef I486
	if (offset_from_heap_register+(arity<<2) >= 127){
# else
#	ifdef G_POWER
	if (offset_from_heap_register+(arity<<2) >= 32767){
		heap_pointer_offset_in_basic_block+=offset_from_heap_register;
#	else
	if (offset_from_heap_register+(arity<<2) >= 4096){
#	endif
# endif
		i_add_i_r (offset_from_heap_register,HEAP_POINTER);
		IF_G_POWER(last_heap_pointer_update=last_instruction;) 
		offset_from_heap_register=0;
	}

	heap_offset=offset_from_heap_register;
#else
	if (result_register!=0 && try_allocate_register_number (result_register>>1))
		reg_1=result_register>>1;
	else
		if (graph->instruction_d_min_a_cost<=0)
			reg_1=get_dregister();
		else 
			reg_1=get_aregister();

	i_move_r_r (HEAP_POINTER,reg_1);
	heap_offset=0;
#endif

	for (argument_number=0; argument_number<arity; ++argument_number){
		INSTRUCTION_GRAPH a_graph;
		
		a_graph=graph->instruction_parameters[argument_number].p;
		
		if (a_graph!=NULL){
#ifdef M68000
			if (heap_offset>0){
				i_add_i_r (heap_offset,HEAP_POINTER);
				heap_offset=0;
			}
#endif
			if (ad_a[argument_number].ad_mode>=100){
				ad_a[argument_number].ad_mode-=100;
#ifdef M68000
				move_float_ad_pi (&ad_a[argument_number],HEAP_POINTER);
#else
#	ifdef G_POWER
				move_float_ad_id (&ad_a[argument_number],heap_offset+4,HEAP_POINTER);
#	else
				move_float_ad_id (&ad_a[argument_number],heap_offset,HEAP_POINTER);
#	endif
				heap_offset+=8;
#endif
				++argument_number;
			} else {
#ifdef M68000
				instruction_ad_pi (IMOVE,&ad_a[argument_number],HEAP_POINTER);
#else
#	ifdef G_POWER
				if (ad_a[argument_number].ad_mode==P_STORE_HP_INSTRUCTION){
					struct instruction *instruction;

					instruction = (struct instruction*)ad_a[argument_number].ad_offset;

					instruction->instruction_parameters[1].parameter_data.i=
						heap_pointer_offset_in_basic_block+heap_offset+4
						-instruction->instruction_parameters[1].parameter_data.i;
				} else
					instruction_ad_id (IMOVE,&ad_a[argument_number],heap_offset+4,HEAP_POINTER);
#	else
				instruction_ad_id (IMOVE,&ad_a[argument_number],heap_offset,HEAP_POINTER);
#	endif
				heap_offset+=4;
#endif
			}
		} else {
			if (arity>3 && graph->instruction_parameters[0].p!=NULL){
#ifdef M68000
				if (heap_offset>0){
					i_add_i_r (heap_offset,HEAP_POINTER);
					heap_offset=0;
				}
				i_move_r_pi (reg_1,HEAP_POINTER);
#else
				if (!reg_1_allocated){
					if (result_register!=0 && try_allocate_register_number (result_register>>1))
						reg_1=result_register>>1;
					else
						if (graph->instruction_d_min_a_cost<=0)
							reg_1=get_dregister();
						else 
							reg_1=get_aregister();

#	ifdef G_POWER			
					i_lea_id_r (offset_from_heap_register+4+NODE_POINTER_OFFSET,HEAP_POINTER,reg_1);
#	else
					if (offset_from_heap_register==0)
						i_move_r_r (HEAP_POINTER,reg_1);
					else
						i_lea_id_r (offset_from_heap_register,HEAP_POINTER,reg_1);
#	endif
		
					reg_1_allocated=1;
				}

#	ifdef G_POWER
				i_move_r_id (reg_1,heap_offset+4,HEAP_POINTER);
#	else
				i_move_r_id (reg_1,heap_offset,HEAP_POINTER);
#	endif
				heap_offset+=4;
#endif
			} else
				heap_offset+=4;
		}
	}

#ifndef M68000
	if (!reg_1_allocated){
		if (result_register!=0 && try_allocate_register_number (result_register>>1))
			reg_1=result_register>>1;
		else
			if (graph->instruction_d_min_a_cost<=0)
				reg_1=get_dregister();
			else 
				reg_1=get_aregister();

#	ifdef G_POWER
		i_lea_id_r (offset_from_heap_register+4+NODE_POINTER_OFFSET,HEAP_POINTER,reg_1);
#	else
		if (offset_from_heap_register==0)
			i_move_r_r (HEAP_POINTER,reg_1);
		else
			i_lea_id_r (offset_from_heap_register,HEAP_POINTER,reg_1);
#	endif
	}

	offset_from_heap_register=heap_offset;
#else
	if (heap_offset>0)
		i_add_i_r (heap_offset,HEAP_POINTER);
#endif

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	ad_p->ad_count_p=&graph->node_count;

	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
	
	memory_free (ad_a);
}

#ifdef G_POWER
static struct instruction *
#else
static void
#endif
linearize_store_create_operator (int offset,int stack_pointer,INSTRUCTION_GRAPH graph)
{
	int argument_number;
	int arity,heap_offset;
	ADDRESS *ad_a;
#ifdef G_POWER
	struct instruction *instruction;
#endif

	arity=graph->inode_arity;
	
	last_block->block_n_new_heap_cells+=arity;

#ifdef G_POWER
	for (argument_number=1; argument_number<arity; ++argument_number){
		INSTRUCTION_GRAPH a_graph;
		
		a_graph=graph->instruction_parameters[argument_number].p;
		
		if (a_graph!=NULL && a_graph->instruction_code==GCREATE && a_graph->node_count==1)
			a_graph->instruction_code=GCREATE_S;
	}
#endif

	ad_a=evaluate_arguments (graph->instruction_parameters,arity);

	heap_offset=0-NODE_POINTER_OFFSET;	
	argument_number=0;

#ifdef G_POWER
# if NODE_POINTER_OFFSET==0
	if (arity>=1 && graph->instruction_parameters[0].p!=NULL && ad_a[0].ad_mode<100){
		i_move_ad_idu (&ad_a[0],offset_from_heap_register+4,HEAP_POINTER);
		heap_pointer_offset_in_basic_block+=offset_from_heap_register+4;
		offset_from_heap_register=-4;
		heap_offset=4;
		++argument_number;
	} else {
#endif
		if (offset_from_heap_register!=-(4+NODE_POINTER_OFFSET)){
			i_add_i_r (offset_from_heap_register+(4+NODE_POINTER_OFFSET),HEAP_POINTER);
			heap_pointer_offset_in_basic_block+=offset_from_heap_register+(4+NODE_POINTER_OFFSET);
			offset_from_heap_register=-(4+NODE_POINTER_OFFSET);
		}
# if NODE_POINTER_OFFSET==0
	}
#endif
	if (stack_pointer==HEAP_POINTER)
		instruction=instruction_move_r_idhp (HEAP_POINTER,heap_pointer_offset_in_basic_block);
	else {
		i_move_r_id (HEAP_POINTER,offset,stack_pointer);
		instruction=NULL;
	}
	IF_G_POWER (last_heap_pointer_update=last_instruction;) 
#else
# ifndef M68000
	if (offset_from_heap_register>0){
		i_add_i_r (offset_from_heap_register,HEAP_POINTER);
		IF_G_POWER (last_heap_pointer_update=last_instruction;) 
		offset_from_heap_register=0;
	}
# endif
	i_move_r_id (HEAP_POINTER,offset,stack_pointer);
#endif

	for (; argument_number<arity; ++argument_number){
		INSTRUCTION_GRAPH a_graph;
		
		a_graph=graph->instruction_parameters[argument_number].p;
		
		if (a_graph!=NULL){
#ifdef M68000
			if (heap_offset>0){
				i_add_i_r (heap_offset,HEAP_POINTER);
				heap_offset=0;
			}
#endif
			if (ad_a[argument_number].ad_mode>=100){
				ad_a[argument_number].ad_mode-=100;
#ifdef M68000
				move_float_ad_pi (&ad_a[argument_number],HEAP_POINTER);
#else
				move_float_ad_id (&ad_a[argument_number],heap_offset,HEAP_POINTER);
				heap_offset+=8;
#endif
				++argument_number;
			} else {
#ifdef M68000
				instruction_ad_pi (IMOVE,&ad_a[argument_number],HEAP_POINTER);
#else
# ifdef G_POWER
				if (ad_a[argument_number].ad_mode==P_STORE_HP_INSTRUCTION){
					struct instruction *instruction;

					instruction = (struct instruction*)ad_a[argument_number].ad_offset;

					instruction->instruction_parameters[1].parameter_data.i=
						heap_pointer_offset_in_basic_block+heap_offset
						-instruction->instruction_parameters[1].parameter_data.i;
				} else
# endif
				instruction_ad_id (IMOVE,&ad_a[argument_number],heap_offset,HEAP_POINTER);
				heap_offset+=4;
#endif
			}
		} else
			heap_offset+=4;
	}

#ifndef M68000
#	ifdef G_POWER
	offset_from_heap_register=heap_offset-4;
#	else
	offset_from_heap_register=heap_offset;
#	endif
#else
	if (heap_offset>0)
		i_add_i_r (heap_offset,HEAP_POINTER);
#endif

	memory_free (ad_a);

#ifdef G_POWER
	return instruction;
#endif
}

static void linearize_fill_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	int argument_number;
	ADDRESS *ad_a;
	int reg_1,reg_2,n_arguments;
	
	n_arguments=graph->inode_arity;
	while (n_arguments>1 && graph->instruction_parameters[n_arguments-1].p==NULL)
		--n_arguments;
	
	ad_a=evaluate_arguments (graph->instruction_parameters,n_arguments);
	
	if (n_arguments==2 && graph->instruction_parameters[1].p!=NULL){
		in_address_register (&ad_a[0]);
		reg_1=reg_2=ad_a[0].ad_register;
		instruction_ad_id (IMOVE,&ad_a[1],0-NODE_POINTER_OFFSET,reg_2);
		if (--*ad_a[0].ad_count_p==0 && graph->node_count==0)
			free_aregister (reg_2);
	} else if (	n_arguments==3 && graph->instruction_parameters[1].p!=NULL 
				&& graph->instruction_parameters[2].p!=NULL)
	{
		in_address_register (&ad_a[0]);
		reg_1=reg_2=ad_a[0].ad_register;
		
		if (ad_a[1].ad_mode>=100){
			ad_a[1].ad_mode-=100;
			move_float_ad_id (&ad_a[1],0-NODE_POINTER_OFFSET,reg_2);
		} else {		
			instruction_ad_id (IMOVE,&ad_a[1],0-NODE_POINTER_OFFSET,reg_2);
			instruction_ad_id (IMOVE,&ad_a[2],4-NODE_POINTER_OFFSET,reg_2);
		}
		if (--*ad_a[0].ad_count_p==0 && graph->node_count==0)
			free_aregister (reg_2);
	} else {
#ifndef M68000
		int offset;
		
		offset=0-NODE_POINTER_OFFSET;
#endif
		if (ad_a[0].ad_mode==P_REGISTER && is_d_register (ad_a[0].ad_register)
			&& graph->instruction_d_min_a_cost<=0 && graph->node_count>0
			&& *ad_a[0].ad_count_p<=1)
		{
			reg_2=ad_a[0].ad_register;
			reg_1=get_aregister();
			i_move_r_r (reg_2,reg_1);
		} else {
#ifdef M68000
			in_alterable_address_register (&ad_a[0]);
#else
			in_address_register (&ad_a[0]);
#endif

			reg_1=ad_a[0].ad_register;
			reg_2=reg_1;

#ifdef M68000
			if (graph->node_count>0){
				if (graph->instruction_d_min_a_cost<=0)
					reg_2=get_dregister();
				else
					reg_2=get_aregister();
				i_move_r_r (reg_1,reg_2);
			}
#endif
		}
	
		for (argument_number=1; argument_number<n_arguments; ++argument_number){
			register INSTRUCTION_GRAPH a_graph;
			
			a_graph=graph->instruction_parameters[argument_number].p;
			
			if (a_graph!=NULL){
				if (ad_a[argument_number].ad_mode>=100){
					ad_a[argument_number].ad_mode-=100;
#ifdef M68000
					move_float_ad_pi (&ad_a[argument_number],reg_1);
#else
					move_float_ad_id (&ad_a[argument_number],offset,reg_1);
					offset+=8;
#endif
					++argument_number;
				} else {
#ifdef M68000
					if (argument_number!=n_arguments-1)
						instruction_ad_pi (IMOVE,&ad_a[argument_number],reg_1);
					else
						instruction_ad_id (IMOVE,&ad_a[argument_number],0,reg_1);
#else
					instruction_ad_id (IMOVE,&ad_a[argument_number],offset,reg_1);
					offset+=4;
#endif
				}
			} else
#ifdef M68000
				i_add_i_r (4,reg_1);
#else
				offset+=4;
#endif
		}

#ifndef M68000
		if (reg_1!=reg_2)
#endif
		free_aregister (reg_1);
		
		--*ad_a[0].ad_count_p;
	}
	
	ad_p->ad_mode=P_REGISTER;

	if (reg_1==reg_2 && *ad_a[0].ad_count_p>0 && graph->node_count>1){
		int result_reg;

		if (graph->instruction_d_min_a_cost<=0)
			result_reg=get_aregister();
		else
			result_reg=get_dregister();
		
		i_move_r_r (reg_1,result_reg);
		--*ad_a[0].ad_count_p;

		ad_p->ad_register=result_reg;
		ad_p->ad_count_p=&graph->node_count;
		
		register_node (graph,result_reg);
	} else {
		ad_p->ad_register=reg_2;
		ad_p->ad_count_p=&graph->node_count;

		if (*ad_p->ad_count_p>1)
			register_node (graph,reg_2);

		if (reg_1==reg_2 && *ad_a[0].ad_count_p>0){
			ad_p->ad_count_p=ad_a[0].ad_count_p;
			*ad_p->ad_count_p+=graph->node_count;
		}
	}

	memory_free (ad_a);
}

#ifdef ALIGN_REAL_ARRAYS
static void move_float_ad_x (ADDRESS *ad_p,int offset,int areg,int dreg,int flags)
#else
static void move_float_ad_x (ADDRESS *ad_p,int offset,int areg,int dreg)
#endif
{
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
#ifdef ALIGN_REAL_ARRAYS
			i_fmove_fr_x (ad_p->ad_register,offset,areg,dreg,flags);
#else
			i_fmove_fr_x (ad_p->ad_register,offset,areg,dreg);
#endif
			if (--*ad_p->ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
				last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
				free_fregister (ad_p->ad_register);
			}
			return;
#if defined (M68000) || defined (I486)
		case P_INDIRECT:
# ifdef M68000
			if (ad_p->ad_offset==0 && *ad_p->ad_count_p==1){
				i_move_pi_x (ad_p->ad_register,offset,areg,dreg);
				i_move_id_x (0,ad_p->ad_register,offset+(4<<2),areg,dreg);
				*ad_p->ad_count_p=0;
				free_aregister (ad_p->ad_register);
			} else {
# endif
				i_move_id_x (ad_p->ad_offset,ad_p->ad_register,offset,areg,dreg);
				i_move_id_x (ad_p->ad_offset+4,ad_p->ad_register,offset+(4<<2),areg,dreg);
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
# ifdef M68000
			}
# endif
			return;
		case P_F_IMMEDIATE:
			i_move_i_x (((LONG*)&ad_p->ad_register)[0],offset,areg,dreg);
			i_move_i_x (((LONG*)&ad_p->ad_register)[1],offset+(4<<2),areg,dreg);
			return;
#endif
#ifdef M68000
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

				if (i_ad_p->ad_mode==P_INDEXED){
					i_move_x_x (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,offset,areg,dreg);
					i_move_x_x (i_ad_p->ad_offset+(4<<2),i_ad_p->ad_areg,i_ad_p->ad_dreg,offset+(4<<2),areg,dreg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
				} else {
					i_move_id_x (i_ad_p->ad_offset,i_ad_p->ad_register,offset,areg,dreg);
					i_move_id_x (i_ad_p->ad_offset+4,i_ad_p->ad_register,offset+(4<<2),areg,dreg);
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
				}
			} else {
				i_fmove_fr_x (i_ad_p->ad_register,offset,areg,dreg);
				if (--*i_ad_p->ad_count_p==0)
					free_fregister (i_ad_p->ad_register);
			}
			return;
		}
#endif
		default:
			internal_error_in_function ("move_float_ad_x");
	}
}

static void linearize_fill_r_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	ADDRESS ad_a[3];
	int reg_2;
	
	linearize_float_graph (graph->instruction_parameters[2].p,&ad_a[2]);
	linearize_graph (graph->instruction_parameters[0].p,&ad_a[0]);
	linearize_graph (graph->instruction_parameters[1].p,&ad_a[1]);
	
	in_address_register (&ad_a[0]);
	reg_2=ad_a[0].ad_register;
	
	instruction_ad_id (IMOVE,&ad_a[1],0,reg_2);
	move_float_ad_id (&ad_a[2],4,reg_2);
	
	if (graph->node_count==0)
		if (--*ad_a[0].ad_count_p==0)
			free_aregister (reg_2);
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_2;
	ad_p->ad_count_p=&graph->node_count;

	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_2);
}

static void in_float_register (ADDRESS *ad_p);

static void linearize_store_fill_operator (INSTRUCTION_GRAPH s_graph,INSTRUCTION_GRAPH graph)
{
	int argument_number,stack_offset,stack_pointer,n_arguments;
	ADDRESS *ad_a;
	int reg_1;
#ifndef M68000
	int offset;
#endif
		
	n_arguments=graph->inode_arity;
	while (n_arguments>1 && graph->instruction_parameters[n_arguments-1].p==NULL)
		--n_arguments;
	
	ad_a=evaluate_arguments (graph->instruction_parameters,n_arguments);

#ifdef M68000
	if (n_arguments!=2 || graph->instruction_parameters[1].p==NULL)
		in_alterable_address_register (&ad_a[0]);
	else
#endif
		in_address_register (&ad_a[0]);

	reg_1=ad_a[0].ad_register;
					
	stack_offset=s_graph->instruction_parameters[0].i;
	stack_pointer=s_graph->instruction_parameters[1].i;
	
	if (s_graph->instruction_parameters[3].p!=NULL){
		for (argument_number=1; argument_number<n_arguments; ++argument_number){
			if (graph->instruction_parameters[argument_number].p!=NULL){
				ADDRESS *ad_p;
				
				ad_p=&ad_a[argument_number];
				if (ad_p->ad_mode==100 + P_INDIRECT){
					if (ad_p->ad_register==stack_pointer && (ad_p->ad_offset==stack_offset || ad_p->ad_offset-4==stack_offset)){
						ad_p->ad_mode=P_INDIRECT;
						in_float_register (ad_p);
						ad_p->ad_mode+=100;
					}
				} else if (ad_p->ad_mode==P_INDIRECT){
					if (ad_p->ad_register==stack_pointer && ad_p->ad_offset==stack_offset){
						in_register (ad_p);
					}
				}
			}
		}
		linearize_load_graph (s_graph->instruction_parameters[3].p);
	}
		
	i_move_r_id (reg_1,stack_offset,stack_pointer);

#ifndef M68000
	offset=0-NODE_POINTER_OFFSET;
#endif

	for (argument_number=1; argument_number<n_arguments; ++argument_number){
		INSTRUCTION_GRAPH a_graph;

		a_graph=graph->instruction_parameters[argument_number].p;
		
		if (a_graph!=NULL){
			if (ad_a[argument_number].ad_mode>=100){
				ad_a[argument_number].ad_mode-=100;
#ifdef M68000
				move_float_ad_pi (&ad_a[argument_number],reg_1);
#else
				move_float_ad_id (&ad_a[argument_number],offset,reg_1);
				offset+=8;
#endif
				++argument_number;
			} else {
#ifdef M68000
				if (argument_number!=n_arguments-1)
					instruction_ad_pi (IMOVE,&ad_a[argument_number],reg_1);
				else
					instruction_ad_id (IMOVE,&ad_a[argument_number],0,reg_1);
#else
				instruction_ad_id (IMOVE,&ad_a[argument_number],offset,reg_1);
				offset+=4;
#endif
			}
		} else
#ifdef M68000
			i_add_i_r (4,reg_1);
#else
			offset+=4;
#endif
	}
	
	if (--*ad_a[0].ad_count_p==0)
		free_aregister (reg_1);
	
	memory_free (ad_a);
}

static int movem_registers (int a_reg,register int n_registers,int registers[],int offset,int update_a_reg)
{
#ifdef M68000
	if (n_registers>=5){
		i_movem_id (offset,a_reg,n_registers,registers);
		return offset+(n_registers<<2);
	} else {
		register int i;

		for (i=0; i<n_registers; ++i)
			if (i==n_registers-1 && !update_a_reg)
				i_move_id_r (offset,a_reg,registers[i]);
			else {
				if (offset!=0){
					i_add_i_r (offset,a_reg);
					offset=0;
				}
				i_move_pi_r (a_reg,registers[i]);
			}
		return offset;
	}
#else
	int i;

	for (i=0; i<n_registers; ++i){
		i_move_id_r (offset,a_reg,registers[i]);
		offset+=4;
	}
	return offset;
#endif
}

static void linearize_movem_operator (INSTRUCTION_GRAPH graph)
{
	ADDRESS ad_1;
	int registers[16],offset,n_registers,a_reg,arity,address_register_used;
	register int argument_number;
	
	linearize_graph (graph->instruction_parameters[1].p,&ad_1);

#ifdef M68000
	in_alterable_address_register (&ad_1);
#else
	in_address_register (&ad_1);
#endif
	a_reg=ad_1.ad_register;
	
	arity=graph->inode_arity;
	
	offset=graph->instruction_parameters[0].i;
	address_register_used=0;
	n_registers=0;
	
	for (argument_number=0; argument_number<arity; ++argument_number){
		register INSTRUCTION_GRAPH a_graph;
		
		a_graph=graph->instruction_parameters[2+argument_number].p;
		if (a_graph==NULL || a_graph->node_count<=0){
			if (n_registers>0){
				offset=movem_registers (a_reg,n_registers,registers,offset,1);
				address_register_used=0;
				n_registers=0;
			}
			offset+=4;
		} else {
			if (a_graph->instruction_code==GFMOVEMI){
				int l_f_reg;
				
				l_f_reg=get_fregister();
				float_register_node (a_graph,l_f_reg);

				if (n_registers>0){
					offset=movem_registers (a_reg,n_registers,registers,offset,1);
					address_register_used=0;
					n_registers=0;
				}
			
				if (argument_number<arity-2){
#ifdef M68000
					if (offset){
						i_add_i_r (offset,a_reg);
						offset=0;
					}
					i_fmove_pi_fr (a_reg,l_f_reg);
#else
					i_fmove_id_fr (offset,a_reg,l_f_reg);
					offset+=8;
#endif
				} else
					i_fmove_id_fr (offset,a_reg,l_f_reg);
				
				++argument_number;
			} else {
			if (a_graph->instruction_parameters[1].i!=0){
				int reg_n;
				
				reg_n=a_graph->instruction_parameters[1].i>>1;
				if (try_allocate_register_number (reg_n)){
					register_node (a_graph,reg_n);
					
					offset=movem_registers (a_reg,n_registers,registers,offset,1);
					address_register_used=0;
					n_registers=0;		
					
					if (argument_number<arity-1){
#ifdef M68000
						if (offset){
							i_add_i_r (offset,a_reg);
							offset=0;
						}
						i_move_pi_r (a_reg,reg_n);
#else
						i_move_id_r (offset,a_reg,reg_n);
						offset+=4;
#endif
					} else
						i_move_id_r (offset,a_reg,reg_n);
					continue;
				} else
					if (reg_n==ad_1.ad_register && *ad_1.ad_count_p==1 
						&& argument_number==arity-1)
					{
						register_node (a_graph,reg_n);
						
						offset=movem_registers (a_reg,n_registers,registers,offset,1);
						address_register_used=0;
						n_registers=0;		
						
						if (argument_number<arity-1){
#ifdef M68000
							if (offset){
								i_add_i_r (offset,a_reg);
								offset=0;
							}
							i_move_pi_r (a_reg,reg_n);
#else
							i_move_id_r (offset,a_reg,reg_n);
							offset+=4;
#endif
						} else
							i_move_id_r (offset,a_reg,reg_n);
						
						return;
					}
			}
			
			if (a_graph->instruction_d_min_a_cost<=0){
				int l_d_reg;
				
				l_d_reg=get_dregister();
				register_node (a_graph,l_d_reg);
			
				if (d_reg_num (l_d_reg)>=N_DATA_PARAMETER_REGISTERS){
					offset=movem_registers (a_reg,n_registers,registers,offset,1);
					n_registers=0;
					address_register_used=0;
					if (argument_number<arity-1){
#ifdef M68000
						if (offset){
							i_add_i_r (offset,a_reg);
							offset=0;
						}
						i_move_pi_r (a_reg,l_d_reg);
#else
						i_move_id_r (offset,a_reg,l_d_reg);
						offset+=4;
#endif
					} else
						i_move_id_r (offset,a_reg,l_d_reg);
				} else {
					if (address_register_used){
						offset=movem_registers (a_reg,n_registers,registers,offset,1);
						n_registers=0;
						address_register_used=0;
					}
					registers[n_registers++]=l_d_reg;
				}
			} else {
				int l_a_reg;
				
				l_a_reg=get_aregister();
				register_node (a_graph,l_a_reg);
				
				if (a_reg_num (l_a_reg)>=N_ADDRESS_PARAMETER_REGISTERS){
					offset=movem_registers (a_reg,n_registers,registers,offset,1);
					n_registers=0;
					address_register_used=0;
					if (argument_number<arity-1){
#ifdef M68000
						if (offset){
							i_add_i_r (offset,a_reg);
							offset=0;
						}
						i_move_pi_r (a_reg,l_a_reg);
#else
						i_move_id_r (offset,a_reg,l_a_reg);
						offset+=4;
#endif
					} else
						i_move_id_r (offset,a_reg,l_a_reg);
				} else {
					registers[n_registers++]=l_a_reg;
					address_register_used=1;
				}
			}
			}
		}
	}
	
	offset=movem_registers (a_reg,n_registers,registers,offset,0);

	if (--*ad_1.ad_count_p==0)
		free_aregister (ad_1.ad_register);
}

static void linearize_movemi_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	linearize_movem_operator (graph->instruction_parameters[0].p);

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=graph->instruction_parameters[0].i;
	ad_p->ad_count_p=&graph->node_count;
}

static void linearize_fmovemi_operator (INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	linearize_movem_operator (graph->instruction_parameters[0].p);

	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=graph->instruction_parameters[0].i;
	ad_p->ad_count_p=&graph->node_count;
}

static void linearize_copy_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	ADDRESS ad;
	int areg_1,areg_2;

	linearize_graph (graph->instruction_parameters[0].p,&ad);
	linearize_graph (graph->instruction_parameters[1].p,ad_p);

#ifdef M68000
	in_alterable_address_register (&ad);
#else
	in_address_register (&ad);
#endif
	in_alterable_address_register (ad_p);

	areg_1=ad.ad_register;
	areg_2=ad_p->ad_register;

#ifdef M68000
	i_move_pi_id (areg_1,0,areg_2);
	i_move_pi_id (areg_1,4,areg_2);
	i_move_pi_id (areg_1,8,areg_2);
#else
	i_move_id_id (0,areg_1,0,areg_2);
	i_move_id_id (4,areg_1,4,areg_2);
	i_move_id_id (8,areg_1,8,areg_2);
#endif

	if (--*ad.ad_count_p==0)
		free_aregister (ad.ad_register);

	if (*ad_p->ad_count_p>1)
		*ad_p->ad_count_p += graph->node_count-1;
	else {
		ad_p->ad_count_p=&graph->node_count;
		register_node (graph,areg_2);
	}
}

#define FLOAT_SIZE 8

static void in_alterable_float_register (ADDRESS *ad_p)
{
	int freg;
	
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
			if (*ad_p->ad_count_p==1)
				return;
			freg=get_fregister();
			i_fmove_fr_fr (ad_p->ad_register,freg);
			--*ad_p->ad_count_p;
			break;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			freg=get_fregister();
			i_fmove_id_fr (ad_p->ad_offset,ad_p->ad_register,freg);
			break;
		case P_F_IMMEDIATE:
			freg=get_fregister();
			i_fmove_if_fr (ad_p->ad_real,freg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
					freg=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
					i_fmove_x_fr (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,freg,load_x_graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
					i_fmove_x_fr (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,freg);
#endif
				} else {
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
					freg=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
					i_fmove_id_fr_f (i_ad_p->ad_offset,i_ad_p->ad_register,freg,load_x_graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
					i_fmove_id_fr (i_ad_p->ad_offset,i_ad_p->ad_register,freg);
#endif
				}
				i_ad_p->ad_register=freg;
			} else {
				freg=i_ad_p->ad_register;
			}
			break;
		}
		default:
			internal_error_in_function ("in_alterable_float_register");
			return;
	}
	
	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=freg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void in_float_register (ADDRESS *ad_p)
{
	int freg;
	
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
			return;
		case P_INDIRECT:
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			freg=get_fregister();
			i_fmove_id_fr (ad_p->ad_offset,ad_p->ad_register,freg);
			break;
		case P_F_IMMEDIATE:
			freg=get_fregister();
			i_fmove_if_fr (ad_p->ad_real,freg);
			break;
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

				if (i_ad_p->ad_mode==P_INDEXED){
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
					freg=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
					i_fmove_x_fr (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,freg,load_x_graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
					i_fmove_x_fr (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,freg);
#endif
				} else {
					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
					freg=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
					i_fmove_id_fr_f (i_ad_p->ad_offset,i_ad_p->ad_register,freg,load_x_graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
					i_fmove_id_fr (i_ad_p->ad_offset,i_ad_p->ad_register,freg);
#endif
				}
				i_ad_p->ad_register=freg;
			} else {
				freg=i_ad_p->ad_register;
			}
			break;
		}
		default:
			internal_error_in_function ("in_float_register");
			return;
	}
	
	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=freg;
	ad_p->ad_count_p=&ad_p->ad_count;
	ad_p->ad_count=1;
}

static void instruction_ad_fr (int instruction_code,ADDRESS *ad_p,int register_1)
{
	struct instruction *instruction;
	struct parameter *parameter_p;
	
	instruction=i_new_instruction2 (instruction_code);
	
	parameter_p=&instruction->instruction_parameters[0];
	
	switch (ad_p->ad_mode){
		case P_F_REGISTER:
			set_float_register_parameter (*parameter_p,ad_p->ad_register);
#if defined (FMADD)
			parameter_p->parameter_flags=0;
#endif
			if (--*ad_p->ad_count_p==0){
#if defined (FP_STACK_OPTIMIZATIONS) || defined (FMADD)
				parameter_p->parameter_flags |= FP_REG_LAST_USE;
#endif
				free_fregister (ad_p->ad_register);
			}
			break;
		case P_INDIRECT:
			parameter_p->parameter_type=P_INDIRECT;
			parameter_p->parameter_offset=ad_p->ad_offset;
			parameter_p->parameter_data.i=ad_p->ad_register;
#ifdef ALIGN_REAL_ARRAYS
			parameter_p->parameter_flags=0;
#endif
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			break;
		case P_F_IMMEDIATE:
		{
			DOUBLE *rp;
			
			parameter_p->parameter_type=P_F_IMMEDIATE;
			rp=(DOUBLE*)fast_memory_allocate (sizeof (DOUBLE));
			parameter_p->parameter_data.r=rp;
			*rp=ad_p->ad_real;
			break;
		}
		case P_INDEXED:
		{
			struct index_registers *index_registers;
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;
			
			if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

				if (i_ad_p->ad_mode==P_INDEXED){
					index_registers=(struct index_registers*)fast_memory_allocate (sizeof (struct index_registers));
			
					parameter_p->parameter_type=P_INDEXED;
					parameter_p->parameter_offset=i_ad_p->ad_offset;
					parameter_p->parameter_data.ir=index_registers;

					index_registers->a_reg.r=i_ad_p->ad_areg;
					index_registers->d_reg.r=i_ad_p->ad_dreg;

					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_areg);
					if (--*i_ad_p->ad_count_p2==0)
						free_dregister (i_ad_p->ad_dreg);
				} else {
					parameter_p->parameter_type=P_INDIRECT;
					parameter_p->parameter_offset=i_ad_p->ad_offset;
					parameter_p->parameter_data.i=i_ad_p->ad_register;

					if (--*i_ad_p->ad_count_p==0)
						free_aregister (i_ad_p->ad_register);
				}
#ifdef ALIGN_REAL_ARRAYS
				parameter_p->parameter_flags = load_x_graph->inode_arity & LOAD_STORE_ALIGNED_REAL;
#endif
			} else {
				set_float_register_parameter (*parameter_p,i_ad_p->ad_register);
				if (--*i_ad_p->ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
					parameter_p->parameter_flags |= FP_REG_LAST_USE;
#endif
					free_fregister (i_ad_p->ad_register);
				}
			}
			break;
		}
		default:
			internal_error_in_function ("instruction_ad_fr");
	}

	set_float_register_parameter (instruction->instruction_parameters[1],register_1);
}

static void instruction_p_fr (int instruction_code,struct parameter *parameter1_p,int register_1)
{
	struct instruction *instruction;
	
	instruction=i_new_instruction (instruction_code,2,2*sizeof (struct parameter));
	
	instruction->instruction_parameters[0]=*parameter1_p;
	
	set_float_register_parameter (instruction->instruction_parameters[1],register_1);
}

static void instruction_lad_fr (int instruction_code,ADDRESS *ad_p,int f_register_1)
{
	register struct instruction *instruction;
	
	instruction=i_new_instruction2 (instruction_code);
	
	ad_to_parameter (ad_p,&instruction->instruction_parameters[0]);
	
	set_float_register_parameter (instruction->instruction_parameters[1],f_register_1);
}

static void linearize_fhigh_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	linearize_float_graph (graph->instruction_parameters[0].p,ad_p);
	
	switch (ad_p->ad_mode){
		case P_INDIRECT:
			if (graph->node_count>1){
				int reg;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg=get_dregister();
				else
					reg=get_aregister();
				i_move_id_r (ad_p->ad_offset,ad_p->ad_register,reg);
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
					
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				graph->instruction_code=P_REGISTER;
				graph->instruction_parameters[0].i=reg;
			}
			return;
		case P_F_REGISTER:
			local_data_offset-=FLOAT_SIZE;

			i_fmove_fr_id (ad_p->ad_register,local_data_offset,B_STACK_POINTER);
			if (--*ad_p->ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
				last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
				free_fregister (ad_p->ad_register);
			}
	
			if (graph->node_count>1){
				int reg;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg=get_dregister();
				else
					reg=get_aregister();
				i_move_id_r (local_data_offset,B_STACK_POINTER,reg);
					
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg);
			} else {
				ad_p->ad_mode=P_INDIRECT;
				ad_p->ad_offset=local_data_offset;
				ad_p->ad_register=B_STACK_POINTER;
				ad_p->ad_count_p=&ad_p->ad_count;
				ad_p->ad_count=graph->node_count+1;
			}
#ifdef g_fhighlow
			{
				INSTRUCTION_GRAPH low_graph;
				
				low_graph=graph->instruction_parameters[1].p;
				
				if (low_graph!=NULL){
					low_graph->instruction_code=GLOAD;
					low_graph->instruction_parameters[0].i=local_data_offset+4;
					low_graph->instruction_parameters[1].i=B_STACK_POINTER;
				}
			}
#endif
			return;
		case P_F_IMMEDIATE:
		{
			DOUBLE r;
			
			r=ad_p->ad_real;
			
			if (graph->node_count>1){	
				int reg;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg=get_dregister();
				else
					reg=get_aregister();
					
				i_move_i_r (((LONG*)&r)[0],reg);
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg);
			} else {
				ad_p->ad_mode=P_IMMEDIATE;
				ad_p->ad_offset=((LONG*)&r)[0];
			}
			return;
		}
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;

			if (graph->node_count>1){
				int reg;

				if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
					load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

					if (i_ad_p->ad_mode==P_INDEXED){
						if (--*i_ad_p->ad_count_p==0)
							free_aregister (i_ad_p->ad_areg);
						if (--*i_ad_p->ad_count_p2==0)
							free_dregister (i_ad_p->ad_dreg);
						if (graph->instruction_d_min_a_cost<=0)
							reg=get_dregister();
						else
							reg=get_aregister();
						i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,reg);
					} else {
						if (--*i_ad_p->ad_count_p==0)
							free_aregister (i_ad_p->ad_register);
						if (graph->instruction_d_min_a_cost<=0)
							reg=get_dregister();
						else
							reg=get_aregister();
						i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,reg);
					}
					i_ad_p->ad_register=reg;
				} else {
					int reg;
		
					local_data_offset-=FLOAT_SIZE;

					i_fmove_fr_id (i_ad_p->ad_register,local_data_offset,B_STACK_POINTER);
#ifdef FP_STACK_OPTIMIZATIONS
					last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
					free_fregister (i_ad_p->ad_register);
				
					if (graph->instruction_d_min_a_cost<=0)
						reg=get_dregister();
					else
						reg=get_aregister();
					i_move_id_r (local_data_offset,B_STACK_POINTER,reg);
				}
				
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				graph->instruction_code=P_REGISTER;
				graph->instruction_parameters[0].i=reg;

				register_node (graph,reg);
			} else {
				if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
				} else {
					local_data_offset-=FLOAT_SIZE;

					i_fmove_fr_id (i_ad_p->ad_register,local_data_offset,B_STACK_POINTER);
#ifdef FP_STACK_OPTIMIZATIONS
					last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
					free_fregister (i_ad_p->ad_register);

					ad_p->ad_mode=P_INDIRECT;
					ad_p->ad_offset=local_data_offset;
					ad_p->ad_register=B_STACK_POINTER;
					ad_p->ad_count_p=&ad_p->ad_count;
					ad_p->ad_count=graph->node_count+1;
				}
			}
			return;
		}

		default:
			internal_error_in_function ("linearize_fhigh_operator");
	}
}

static void linearize_flow_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	linearize_float_graph (graph->instruction_parameters[0].p,ad_p);
	
	switch (ad_p->ad_mode){
		case P_INDIRECT:
			if (graph->node_count>1){
				register int reg;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg=get_dregister();
				else
					reg=get_aregister();
				i_move_id_r (ad_p->ad_offset+4,ad_p->ad_register,reg);
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
					
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg);
			} else
				ad_p->ad_offset+=4;
			return;
		case P_F_REGISTER:
			local_data_offset-=FLOAT_SIZE;
				
			i_fmove_fr_id (ad_p->ad_register,local_data_offset,B_STACK_POINTER);
			if (--*ad_p->ad_count_p==0)
#ifdef FP_STACK_OPTIMIZATIONS
				last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
				free_fregister (ad_p->ad_register);

				if (graph->node_count>1){
				int reg;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg=get_dregister();
				else
					reg=get_aregister();
				i_move_id_r (local_data_offset+4,B_STACK_POINTER,reg);
					
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				graph->instruction_code=P_REGISTER;
				graph->instruction_parameters[0].i=reg;
			} else {
				ad_p->ad_mode=P_INDIRECT;
				ad_p->ad_offset=local_data_offset+4;
				ad_p->ad_register=B_STACK_POINTER;
				ad_p->ad_count_p=&ad_p->ad_count;
				ad_p->ad_count=graph->node_count+1;
			}
#ifdef g_fhighlow
			{
				INSTRUCTION_GRAPH high_graph;
				
				high_graph=graph->instruction_parameters[1].p;
				
				if (high_graph!=NULL){
					high_graph->instruction_code=GLOAD;
					high_graph->instruction_parameters[0].i=local_data_offset;
					high_graph->instruction_parameters[1].i=B_STACK_POINTER;
				}
			}
#endif
			return;
		case P_F_IMMEDIATE:
		{
			DOUBLE r;
			
			r=ad_p->ad_real;
			
			if (graph->node_count>1){	
				int reg;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg=get_dregister();
				else
					reg=get_aregister();
					
				i_move_i_r (((LONG*)&r)[1],reg);
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg);
			} else {
				ad_p->ad_mode=P_IMMEDIATE;
				ad_p->ad_offset=((LONG*)&r)[1];
			}
			return;
		}
		case P_INDEXED:
		{
			INSTRUCTION_GRAPH load_x_graph;
			ADDRESS *i_ad_p;

			load_x_graph=(INSTRUCTION_GRAPH)ad_p->ad_offset;
			i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;

			if (graph->node_count>1){
				int reg;

				if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
					load_x_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

					if (i_ad_p->ad_mode==P_INDEXED){
						if (--*i_ad_p->ad_count_p==0)
							free_aregister (i_ad_p->ad_areg);
						if (--*i_ad_p->ad_count_p2==0)
							free_dregister (i_ad_p->ad_dreg);
						if (graph->instruction_d_min_a_cost<=0)
							reg=get_dregister();
						else
							reg=get_aregister();
						i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,reg);
					} else {
						if (--*i_ad_p->ad_count_p==0)
							free_aregister (i_ad_p->ad_register);
						if (graph->instruction_d_min_a_cost<=0)
							reg=get_dregister();
						else
							reg=get_aregister();
						i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,reg);
					}
					i_ad_p->ad_register=reg;
				} else
					reg=i_ad_p->ad_register;
				
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg;
				ad_p->ad_count_p=&graph->node_count;
				
				graph->instruction_code=P_REGISTER;
				graph->instruction_parameters[0].i=reg;
			} else {
				if (load_x_graph->inode_arity & LOAD_X_TO_ADDRESS){
					if (i_ad_p->ad_mode==P_INDEXED)
						i_ad_p->ad_offset+=4<<2;
					else
						i_ad_p->ad_offset+=4;
				} else {
					local_data_offset-=FLOAT_SIZE;

					i_fmove_fr_id (i_ad_p->ad_register,local_data_offset,B_STACK_POINTER);
#ifdef FP_STACK_OPTIMIZATIONS
					last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
					free_fregister (i_ad_p->ad_register);

					ad_p->ad_mode=P_INDIRECT;
					ad_p->ad_offset=local_data_offset+4;
					ad_p->ad_register=B_STACK_POINTER;
					ad_p->ad_count_p=&ad_p->ad_count;
					ad_p->ad_count=graph->node_count+1;				
				}
			}
			return;
		}
		default:
			internal_error_in_function ("linearize_flow_operator");
	}
}

static void linearize_fjoin_operator (INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH high_graph,low_graph;
	ADDRESS ad_1,ad_2;
	
	high_graph=graph->instruction_parameters[0].p;
	low_graph=graph->instruction_parameters[1].p;

#ifdef g_fhighlow
	linearize_graph (low_graph,&ad_2);
	linearize_graph (high_graph,&ad_1);
#else	
	linearize_graph (high_graph,&ad_1);
	linearize_graph (low_graph,&ad_2);
#endif

#ifdef sparc
	if (ad_1.ad_mode==P_INDIRECT && ad_2.ad_mode==P_INDIRECT){
		int freg;
	
		freg=get_fregister();
		instruction_ad_fr (IFMOVEHI,&ad_1,freg);
		instruction_ad_fr (IFMOVELO,&ad_2,freg);
			
		ad_p->ad_mode=P_F_REGISTER;
		ad_p->ad_register=freg;
		ad_p->ad_count_p=&graph->node_count;
			
		if (graph->node_count>1)
			float_register_node (graph,freg);
	
		return;
	}
#endif

	local_data_offset-=FLOAT_SIZE;

#ifdef g_fhighlow
	instruction_ad_id (IMOVE,&ad_2,local_data_offset+4,B_STACK_POINTER);
	instruction_ad_id (IMOVE,&ad_1,local_data_offset,B_STACK_POINTER);
#else	
	instruction_ad_id (IMOVE,&ad_1,local_data_offset,B_STACK_POINTER);
	instruction_ad_id (IMOVE,&ad_2,local_data_offset+4,B_STACK_POINTER);
#endif

	if (graph->node_count>1){
		int freg;
		
		freg=get_fregister();
		i_fmove_id_fr (local_data_offset,B_STACK_POINTER,freg);
		
		ad_p->ad_mode=P_F_REGISTER;
		ad_p->ad_register=freg;
		ad_p->ad_count_p=&graph->node_count;
		
		float_register_node (graph,freg);
	} else {
		ad_p->ad_mode=P_INDIRECT;
		ad_p->ad_offset=local_data_offset;
		ad_p->ad_register=B_STACK_POINTER;
		ad_p->ad_count_p=&ad_p->ad_count;
		ad_p->ad_count=1+1;
	}
}

static int float_compare_node (INSTRUCTION_GRAPH graph,int i_test_1,int i_test_2)
{
	ADDRESS ad_1,ad_2;
	
	linearize_float_graph (graph->instruction_parameters[0].p,&ad_1);
	linearize_float_graph (graph->instruction_parameters[1].p,&ad_2);

	if (ad_1.ad_mode==P_F_REGISTER){
		instruction_ad_fr (IFCMP,&ad_2,ad_1.ad_register);
		if (--*ad_1.ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
			last_instruction->instruction_parameters[1].parameter_flags |= FP_REG_LAST_USE;
#endif
			free_fregister (ad_1.ad_register);
		}
		return i_test_2;
	} else {
		in_float_register (&ad_2);
		instruction_ad_fr (IFCMP,&ad_1,ad_2.ad_register);
		if (--*ad_2.ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
			last_instruction->instruction_parameters[1].parameter_flags |= FP_REG_LAST_USE;
#endif
			free_fregister (ad_2.ad_register);
		}
		return i_test_1;
	}
}

static void linearize_float_compare_operator (int i_test_1,int i_test_2,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	int condition;
	
	condition=float_compare_node (graph,i_test_1,i_test_2);
	condition_to_register (graph,condition,ad_p);
}

static void linearize_dyadic_commutative_float_operator (int instruction_code,register INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	if (graph->order_left){
		linearize_float_graph (graph->instruction_parameters[0].p,&ad_1);
		linearize_float_graph (graph->instruction_parameters[1].p,&ad_2);
	} else {
		linearize_float_graph (graph->instruction_parameters[1].p,&ad_2);
		linearize_float_graph (graph->instruction_parameters[0].p,&ad_1);
	}
	
	if (ad_1.ad_mode==P_F_REGISTER && *ad_1.ad_count_p==1
		/* added 17-3-1999: prefer result in lowest register number */
		&& !(ad_2.ad_mode==P_F_REGISTER && *ad_2.ad_count_p==1 && ad_2.ad_register<ad_1.ad_register)
		/* */
	){
		reg_1=ad_1.ad_register;
		instruction_ad_fr (instruction_code,&ad_2,reg_1);

		*ad_1.ad_count_p=0;
	} else {
#if 1
		struct parameter parameter1;
		int free_f_register;

#if defined (FMADD)
		parameter1.parameter_flags=0;
#endif
		free_f_register=fad_to_parameter (&ad_1,&parameter1);
		in_alterable_float_register (&ad_2);
		if (free_f_register){
#if defined (FP_STACK_OPTIMIZATIONS) || defined (FMADD)
			parameter1.parameter_flags |= FP_REG_LAST_USE;
#endif
			free_fregister (ad_1.ad_register);
		}
		reg_1=ad_2.ad_register;
		instruction_p_fr (instruction_code,&parameter1,reg_1);
#else
		in_alterable_float_register (&ad_2);
		reg_1=ad_2.ad_register;
		instruction_ad_fr (instruction_code,&ad_1,reg_1);
#endif
		--*ad_2.ad_count_p;
	}
	
	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=reg_1;
	ad_p->ad_count_p=&graph->node_count;
	
	if (*ad_p->ad_count_p>1)
		float_register_node (graph,reg_1);
}

static void linearize_dyadic_non_commutative_float_operator (int instruction_code,INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	if (graph->order_left){
		linearize_float_graph (graph->instruction_parameters[0].p,&ad_1);
		linearize_float_graph (graph->instruction_parameters[1].p,&ad_2);
	} else {
		linearize_float_graph (graph->instruction_parameters[1].p,&ad_2);
		linearize_float_graph (graph->instruction_parameters[0].p,&ad_1);
	}

#if 1
# if defined (I486) || defined (G_POWER)
	/* added 15-12-1998 */
	if (ad_1.ad_mode==P_F_REGISTER && *ad_1.ad_count_p==1
		&& !(ad_2.ad_mode==P_F_REGISTER && ad_2.ad_register<ad_1.ad_register && *ad_2.ad_count_p==1)
	){
		reg_1=ad_1.ad_register;
		instruction_ad_fr (instruction_code,&ad_2,reg_1);
		last_instruction->instruction_parameters[1].parameter_flags=1;

		*ad_1.ad_count_p=0;
	} else
# endif
	{
		struct parameter parameter1;
		int free_f_register;

# if defined (FMADD)
		parameter1.parameter_flags=0;
# endif

		free_f_register=fad_to_parameter (&ad_1,&parameter1);
		in_alterable_float_register (&ad_2);
		if (free_f_register){	
#if defined (FP_STACK_OPTIMIZATIONS) || defined (FMADD)
			parameter1.parameter_flags |= FP_REG_LAST_USE;
#endif
			free_fregister (ad_1.ad_register);
		}
		reg_1=ad_2.ad_register;
		instruction_p_fr (instruction_code,&parameter1,reg_1);
# if defined (I486) || defined (G_POWER)
		last_instruction->instruction_parameters[1].parameter_flags=0;
# endif
	}
#else
	in_alterable_float_register (&ad_2);
	reg_1=ad_2.ad_register;
	instruction_ad_fr (instruction_code,&ad_1,reg_1);
# if defined (I486) || defined (G_POWER)
	last_instruction->instruction_parameters[1].parameter_flags=0;
# endif
#endif

	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=reg_1;
	ad_p->ad_count_p=&graph->node_count;
	
	if (*ad_p->ad_count_p>1)
		float_register_node (graph,reg_1);
}

static void linearize_itor_operator (INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	int reg_1;
	
	linearize_graph (graph->instruction_parameters[0].p,ad_p);
	to_data_addressing_mode (ad_p);
	
	reg_1=get_fregister();
	instruction_lad_fr (IFMOVEL,ad_p,reg_1);
	
	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=reg_1;
	ad_p->ad_count_p=&graph->node_count;
	
	if (*ad_p->ad_count_p>1)
		float_register_node (graph,reg_1);
}

static void linearize_monadic_float_operator (INSTRUCTION_GRAPH graph,register ADDRESS *ad_p,int i_code)
{
	int freg;
	
	linearize_float_graph (graph->instruction_parameters[0].p,ad_p);
	
	if (ad_p->ad_mode==P_F_REGISTER && *ad_p->ad_count_p==1){
		free_fregister (ad_p->ad_register);
		freg=get_fregister();
		instruction_fr_fr (i_code,ad_p->ad_register,freg);
#ifdef FP_STACK_OPTIMIZATIONS
		last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
	} else {
		freg=get_fregister();
		instruction_ad_fr (i_code,ad_p,freg);
	}
	
	ad_p->ad_mode=P_F_REGISTER;
	ad_p->ad_register=freg;
	ad_p->ad_count_p=&graph->node_count;
	
	if (*ad_p->ad_count_p>1)
		float_register_node (graph,freg);
}

static void linearize_float_keep_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	ADDRESS ad_1;
			
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;

	linearize_graph (graph_1,&ad_1);
			
	switch (ad_1.ad_mode){
		case P_REGISTER:
		case P_INDIRECT:
			if (--*ad_1.ad_count_p==0)
				free_register (ad_1.ad_register);
	}
			
	linearize_float_graph (graph_2,ad_p);

	if (graph->node_count>1){
		in_float_register (ad_p);
/* changed 10-12-97
		float_register_node (graph,ad_p->ad_register);
		ad_p->ad_count_p=&graph->node_count;
*/
		float_register_node (graph_2,ad_p->ad_register);
		graph_2->node_count += graph->node_count;
		ad_p->ad_count_p=&graph_2->node_count;

		graph->instruction_parameters[0].p=graph_2;
		graph->instruction_code=GINDIRECTION;
/* */
	}			
}

static void linearize_fload_x_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	int offset;
	
	graph_1=graph->instruction_parameters[0].p;
	offset=graph->instruction_parameters[1].i;
	graph_2=graph->instruction_parameters[2].p;

	if (graph_2!=NULL){
		ADDRESS ad_1;

		if (graph->order_left){
			linearize_graph (graph_1,ad_p);
			linearize_graph (graph_2,&ad_1);
		} else {
			linearize_graph (graph_2,&ad_1);
			linearize_graph (graph_1,ad_p);
		}

		in_data_register (&ad_1);
		in_address_register (ad_p);

		if (graph->node_count<=1){
			ADDRESS *i_ad_p;
						
			ad_p->ad_mode=P_INDEXED;
			ad_p->ad_offset=(LONG)graph;
			
			i_ad_p=(ADDRESS*)fast_memory_allocate (sizeof (ADDRESS));
			graph->instruction_parameters[1].p=(INSTRUCTION_GRAPH)i_ad_p;
			
			i_ad_p->ad_mode=P_INDEXED;
			i_ad_p->ad_offset=offset;
			i_ad_p->ad_areg=ad_p->ad_register;
			i_ad_p->ad_dreg=ad_1.ad_register;
			
			i_ad_p->ad_count_p=ad_p->ad_count_p;
			if (ad_1.ad_count_p!=&ad_1.ad_count)
				/* it points to a counter in a graph node */
				i_ad_p->ad_count_p2=ad_1.ad_count_p;
			else {
				/* prevent a scope problem */
				i_ad_p->ad_count_p2=&ad_p->ad_count2;
				ad_p->ad_count2=ad_1.ad_count;
			}
			graph->node_count=0;
			graph->inode_arity|=LOAD_X_TO_ADDRESS;
		} else {
			int reg_1;
			
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
			if (--*ad_1.ad_count_p==0)
				free_dregister (ad_1.ad_register);
						
			reg_1=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
			i_fmove_x_fr (offset,ad_p->ad_register,ad_1.ad_register,reg_1,graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
			i_fmove_x_fr (offset,ad_p->ad_register,ad_1.ad_register,reg_1);
#endif
			ad_p->ad_mode=P_F_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
						
			float_register_node (graph,reg_1);
		}
	} else {
		linearize_graph (graph_1,ad_p);
		in_address_register (ad_p);

		if (graph->node_count<=1){
			ADDRESS *i_ad_p;

			ad_p->ad_mode=P_INDEXED;
			ad_p->ad_offset=(LONG)graph;
			
			i_ad_p=(ADDRESS*)fast_memory_allocate (sizeof (ADDRESS));
			graph->instruction_parameters[1].p=(INSTRUCTION_GRAPH)i_ad_p;
			
			i_ad_p->ad_mode=P_INDIRECT;
			i_ad_p->ad_offset=offset>>2;
			i_ad_p->ad_register=ad_p->ad_register;
			i_ad_p->ad_count_p=ad_p->ad_count_p;
			
			graph->node_count=0;
			graph->inode_arity|=LOAD_X_TO_ADDRESS;
		} else {
			int reg_1;
	
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
					
			reg_1=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
			i_fmove_id_fr_f (offset>>2,ad_p->ad_register,reg_1,graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
			i_fmove_id_fr (offset>>2,ad_p->ad_register,reg_1);
#endif				
			ad_p->ad_mode=P_F_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;

			float_register_node (graph,reg_1);
		}
	}
}

static void linearize_float_graph (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	switch (graph->instruction_code){
		case GFREGISTER:
			ad_p->ad_mode=P_F_REGISTER;
			ad_p->ad_register=graph->instruction_parameters[0].i;
			ad_p->ad_count_p=&graph->node_count;
			break;
		case GGFREGISTER:
			ad_p->ad_mode=P_F_REGISTER;
			ad_p->ad_register=graph->instruction_parameters[0].i;
			ad_p->ad_count_p=&ad_p->ad_count;
			ad_p->ad_count=1+1;
			break;
		case GFLOAD:
		{
			if (graph->node_count<=1){
				ad_p->ad_mode=P_INDIRECT;
				ad_p->ad_count_p=&ad_p->ad_count;
				ad_p->ad_count=1+1;
				ad_p->ad_offset=graph->instruction_parameters[0].i;
				ad_p->ad_register=graph->instruction_parameters[1].i;
				graph->node_count=0;
			} else {
				int reg_1;
				
				reg_1=get_fregister();
					
				i_fmove_id_fr (graph->instruction_parameters[0].i,
							   graph->instruction_parameters[1].i,reg_1);
		
				ad_p->ad_mode=P_F_REGISTER;
				ad_p->ad_register=reg_1;
				ad_p->ad_count_p=&graph->node_count;
				
				float_register_node (graph,reg_1);
			}
			return;
		}
		case GFLOAD_I:
			if (graph->node_count<=1){
				ad_p->ad_mode=P_F_IMMEDIATE;
				ad_p->ad_real=*(DOUBLE*)&graph->instruction_parameters[0];
			} else {
				int reg_1;
				
				reg_1=get_fregister();
				i_fmove_if_fr (*(DOUBLE*)&graph->instruction_parameters[0],reg_1);
				
				ad_p->ad_mode=P_F_REGISTER;
				ad_p->ad_register=reg_1;
				ad_p->ad_count_p=&graph->node_count;
				
				float_register_node (graph,reg_1);
			}
			return;
		case GFLOAD_ID:
		{
			INSTRUCTION_GRAPH graph_1;
			
			graph_1=graph->instruction_parameters[1].p;
			linearize_graph (graph_1,ad_p);
			in_address_register (ad_p);
			
			if (graph->node_count<=1){
				ad_p->ad_mode=P_INDIRECT;
				ad_p->ad_offset=graph->instruction_parameters[0].i;
				/* added 26-7-97 */
				--graph->node_count;
				/* */
			} else {
				int reg_1;
				
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
				
				reg_1=get_fregister();
				i_fmove_id_fr (graph->instruction_parameters[0].i,ad_p->ad_register,reg_1);
				
				ad_p->ad_mode=P_F_REGISTER;
				ad_p->ad_register=reg_1;
				ad_p->ad_count_p=&graph->node_count;
				
				float_register_node (graph,reg_1);
			}
			return;
		}
		case GFMOVEMI:
			linearize_fmovemi_operator (graph,ad_p);
			return;
		case GFLOAD_X:
			linearize_fload_x_operator (graph,ad_p);
			return;
		case GFADD:
			linearize_dyadic_commutative_float_operator (IFADD,graph,ad_p);
			return;
		case GFDIV:
			linearize_dyadic_non_commutative_float_operator (IFDIV,graph,ad_p);
			return;
		case GFMUL:
			linearize_dyadic_commutative_float_operator (IFMUL,graph,ad_p);
			return;
		case GFREM:
			linearize_dyadic_non_commutative_float_operator (IFREM,graph,ad_p);
			return;
		case GFSUB:
			linearize_dyadic_non_commutative_float_operator (IFSUB,graph,ad_p);
			return;
		case GFJOIN:
			linearize_fjoin_operator (graph,ad_p);
			return;
		case GFITOR:
			linearize_itor_operator (graph,ad_p);
			return;
		case GFCOS:
			linearize_monadic_float_operator (graph,ad_p,IFCOS);
			break;
		case GFNEG:
			linearize_monadic_float_operator (graph,ad_p,IFNEG);
			break;
		case GFABS:
			linearize_monadic_float_operator (graph,ad_p,IFABS);
			break;
		case GFSIN:
			linearize_monadic_float_operator (graph,ad_p,IFSIN);
			break;
		case GFTAN:
			linearize_monadic_float_operator (graph,ad_p,IFTAN);
			break;
#ifdef M68000
		case GFASIN:
			linearize_monadic_float_operator (graph,ad_p,IFASIN);
			break;
		case GFACOS:
			linearize_monadic_float_operator (graph,ad_p,IFACOS);
			break;
		case GFATAN:
			linearize_monadic_float_operator (graph,ad_p,IFATAN);
			break;
		case GFLN:
			linearize_monadic_float_operator (graph,ad_p,IFLN);
			break;
		case GFLOG10:
			linearize_monadic_float_operator (graph,ad_p,IFLOG10);
			break;
		case GFEXP:
			linearize_monadic_float_operator (graph,ad_p,IFEXP);
			break;
#endif
		case GFKEEP:
			linearize_float_keep_operator (graph,ad_p);
			break;
/* added 10-12-97 */
		case GINDIRECTION:
			linearize_float_graph (graph->instruction_parameters[0].p,ad_p);
			break;
/* */
#if !defined (G_POWER)
		case GFSQRT:
			linearize_monadic_float_operator (graph,ad_p,IFSQRT);
			break;
#endif
		default:
			/* printf ("%d %d\n",(int)graph,graph->instruction_code); */
			internal_error_in_function ("linearize_float_graph");
	}
}

static void linearize_fstore_operator (register INSTRUCTION_GRAPH graph)
{
	int offset,stack_pointer;
	ADDRESS ad_1;
	
	linearize_float_graph (graph->instruction_parameters[2].p,&ad_1);
	
	offset=graph->instruction_parameters[0].i;
	stack_pointer=graph->instruction_parameters[1].i;
	
	linearize_load_graph (graph->instruction_parameters[3].p);
	linearize_load_graph (graph->instruction_parameters[4].p);
	
	move_float_ad_id (&ad_1,offset,stack_pointer);
}

static void linearize_fstore_x_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p);

static void linearize_fstore_r_operator (register INSTRUCTION_GRAPH graph)
{
	INSTRUCTION_GRAPH *register_graph_1_p,*register_graph_2_p;
	register int reg_1;
	ADDRESS ad_1;
	
	linearize_float_graph (graph->instruction_parameters[1].p,&ad_1);
	
	reg_1=graph->instruction_parameters[0].i;
	
	if ((unsigned)reg_1<(unsigned)N_FLOAT_PARAMETER_REGISTERS)
		register_graph_1_p=&global_block.block_graph_f_register_parameter_node[reg_1];
	else
		register_graph_1_p=NULL;
	
	if (ad_1.ad_mode==P_F_REGISTER && ad_1.ad_register==reg_1){
		if (register_graph_1_p!=NULL && *register_graph_1_p!=NULL)
			(*register_graph_1_p)->instruction_code=GGFREGISTER;
		return;
	}
	
	if (ad_1.ad_mode==P_F_REGISTER){
		register int reg_2;
		
		reg_2=ad_1.ad_register;
		
		if ((unsigned)reg_2<(unsigned)N_FLOAT_PARAMETER_REGISTERS)
			register_graph_2_p=&global_block.block_graph_f_register_parameter_node[reg_2];
		else
			register_graph_2_p=NULL;
		
		if (register_graph_1_p!=NULL && *register_graph_1_p!=NULL && (*register_graph_1_p)->node_count>0){
#ifdef FP_STACK_OPTIMIZATIONS
			if (register_graph_2_p!=NULL && (*register_graph_2_p==NULL || (*register_graph_2_p)->instruction_code!=GGFREGISTER)){
				INSTRUCTION_GRAPH graph;
				
				i_fexg_fr_fr (reg_2,reg_1);
			
				(*register_graph_1_p)->instruction_parameters[0].i=reg_2;
					
				if (*register_graph_2_p!=NULL){
					(*register_graph_2_p)->instruction_code=GGFREGISTER;
					(*register_graph_2_p)->instruction_parameters[0].i=reg_1;
				}
				
				graph=*register_graph_1_p;
				*register_graph_1_p=*register_graph_2_p;
				*register_graph_2_p=graph;
			} else {
#endif
			register int reg_2;
						
			reg_2=get_fregister();
			if ((unsigned)reg_2<(unsigned)N_FLOAT_PARAMETER_REGISTERS)
				register_graph_2_p=&global_block.block_graph_f_register_parameter_node [reg_2];
			else
				register_graph_2_p=NULL;
			
			i_fmove_fr_fr (reg_1,reg_2);
			instruction_ad_fr (IFMOVE,&ad_1,reg_1);
				
			(*register_graph_1_p)->instruction_parameters[0].i=reg_2;
				
			if (register_graph_2_p!=NULL)
				*register_graph_2_p=*register_graph_1_p;
		
			*register_graph_1_p=NULL;
#ifdef FP_STACK_OPTIMIZATIONS
			}
#endif
		} else {
			i_fmove_fr_fr (reg_2,reg_1);
			if (--*ad_1.ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
				last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
				free_fregister (reg_2);
			}
		}
	} else {
		if (ad_1.ad_mode==P_INDIRECT)
			--*ad_1.ad_count_p;
		if (register_graph_1_p!=NULL && *register_graph_1_p!=NULL 
			&& (*register_graph_1_p)->node_count>0)
		{
			register int reg_2;
		
			reg_2=get_fregister();
			if ((unsigned)reg_2<(unsigned)N_FLOAT_PARAMETER_REGISTERS)
				register_graph_2_p=
					&global_block.block_graph_f_register_parameter_node [reg_2];
			else
				register_graph_2_p=NULL;
			
			i_fmove_fr_fr (reg_1,reg_2);
			
			(*register_graph_1_p)->instruction_parameters[0].i=reg_2;
			
			if (register_graph_2_p!=NULL)
				*register_graph_2_p=*register_graph_1_p;
			
			*register_graph_1_p=NULL;
		}
		if (ad_1.ad_mode==P_INDIRECT)
			++*ad_1.ad_count_p;
		instruction_ad_fr (IFMOVE,&ad_1,reg_1);
	}
	
	allocate_fregister (reg_1);
}

static void linearize_create_r_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p,int result_register)
{
	ADDRESS ad_1,ad_2;
	int reg_1;
	
	last_block->block_n_new_heap_cells+=3;

#ifndef M68000
# ifdef I486
	if (offset_from_heap_register+(3<<2) >= 127){
# else
#	ifdef G_POWER
	if (offset_from_heap_register+(3<<2) >= 32767){
		heap_pointer_offset_in_basic_block+=offset_from_heap_register;
#	else
	if (offset_from_heap_register+(3<<2) >= 4096){
#	endif
# endif
		i_add_i_r (offset_from_heap_register,HEAP_POINTER);
		IF_G_POWER (last_heap_pointer_update=last_instruction;) 
		offset_from_heap_register=0;
	}
#endif

	linearize_float_graph (graph->instruction_parameters[1].p,&ad_2);
	linearize_graph (graph->instruction_parameters[0].p,&ad_1);
	
	if (result_register!=0 && try_allocate_register_number (result_register>>1))
		reg_1=result_register>>1;
	else
		if (graph->instruction_d_min_a_cost<=0)
			reg_1=get_dregister();
		else
			reg_1=get_aregister();

#ifdef G_POWER
	i_lea_id_r (offset_from_heap_register+4+NODE_POINTER_OFFSET,HEAP_POINTER,reg_1);
#else
#	ifndef M68000
	if (offset_from_heap_register!=0)
		i_lea_id_r (offset_from_heap_register,HEAP_POINTER,reg_1);
	else
#	endif
	i_move_r_r (HEAP_POINTER,reg_1);
#endif

#ifdef M68000
	instruction_ad_pi (IMOVE,&ad_1,HEAP_POINTER);
	move_float_ad_pi (&ad_2,HEAP_POINTER);
#else
#	ifdef G_POWER
	instruction_ad_id (IMOVE,&ad_1,offset_from_heap_register+4,HEAP_POINTER);
	move_float_ad_id (&ad_2,offset_from_heap_register+8,HEAP_POINTER);
#	else
	instruction_ad_id (IMOVE,&ad_1,offset_from_heap_register,HEAP_POINTER);
	move_float_ad_id (&ad_2,offset_from_heap_register+4,HEAP_POINTER);
#	endif
	offset_from_heap_register+=12;
#endif

	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg_1;
	ad_p->ad_count_p=&graph->node_count;

	if (*ad_p->ad_count_p>1)
		register_node (graph,reg_1);
}

static void linearize_rtoi_operator (INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	int reg;
	
	linearize_float_graph (graph->instruction_parameters[0].p,ad_p);
	in_float_register (ad_p);	
	
	reg=get_dregister();
	i_fmovel_fr_r (ad_p->ad_register,reg);
	if (--*ad_p->ad_count_p==0){
#ifdef FP_STACK_OPTIMIZATIONS
		last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
#endif
		free_fregister (ad_p->ad_register);
	}
	
	if (graph->instruction_d_min_a_cost>0){
		int areg;
		
		areg=get_aregister();
		i_move_r_r (reg,areg);
		free_dregister (reg);
		reg=areg;
	}
	
	ad_p->ad_mode=P_REGISTER;
	ad_p->ad_register=reg;
	ad_p->ad_count_p=&graph->node_count;
	
	if (*ad_p->ad_count_p>1)
		register_node (graph,reg);
}

static void linearize_load_x_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p
#ifdef LOAD_X_PARAMETER_REGISTER
							,int result_register
#endif
							)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	int offset;
	
	graph_1=graph->instruction_parameters[0].p;
	offset=graph->instruction_parameters[1].i;
	graph_2=graph->instruction_parameters[2].p;

	if (graph_2!=NULL){
		ADDRESS ad_1;

		if (graph->order_left){
			linearize_graph (graph_1,ad_p);
			linearize_graph (graph_2,&ad_1);
		} else {
			linearize_graph (graph_2,&ad_1);
			linearize_graph (graph_1,ad_p);
		}

		in_data_register (&ad_1);
		in_address_register (ad_p);

		if (graph->node_count<=1){
			ADDRESS *i_ad_p;
			
 			ad_p->ad_mode=P_INDEXED;
			ad_p->ad_offset=(LONG)graph;

			i_ad_p=(ADDRESS*)fast_memory_allocate (sizeof (ADDRESS));
			graph->instruction_parameters[1].p=(INSTRUCTION_GRAPH)i_ad_p;
			
			i_ad_p->ad_mode=P_INDEXED;
			i_ad_p->ad_offset=offset;
			i_ad_p->ad_areg=ad_p->ad_register;
			i_ad_p->ad_dreg=ad_1.ad_register;

			i_ad_p->ad_count_p=ad_p->ad_count_p;
			if (ad_1.ad_count_p!=&ad_1.ad_count)
				/* it points to a counter in a graph node */
				i_ad_p->ad_count_p2=ad_1.ad_count_p;
			else {
				/* prevent a scope problem */
				i_ad_p->ad_count_p2=&ad_p->ad_count2;
				ad_p->ad_count2=ad_1.ad_count;
			}

			graph->node_count=0;
			graph->inode_arity|=LOAD_X_TO_ADDRESS;
		} else {
			int reg_1;
	
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);

			if (--*ad_1.ad_count_p==0)
				free_dregister (ad_1.ad_register);

#ifdef LOAD_X_PARAMETER_REGISTER
			if (result_register!=0 && try_allocate_register_number (result_register>>1))
				reg_1=result_register>>1;
			else
#endif
			if (graph->instruction_d_min_a_cost<=0)
				reg_1=get_dregister();
			else
				reg_1=get_aregister();
			
			i_move_x_r (offset,ad_p->ad_register,ad_1.ad_register,reg_1);
		
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
		
			register_node (graph,reg_1);
		}
	} else {
		linearize_graph (graph_1,ad_p);
		in_address_register (ad_p);

		if (graph->node_count<=1){
			ADDRESS *i_ad_p;

 			ad_p->ad_mode=P_INDEXED;
			ad_p->ad_offset=(LONG)graph;

			i_ad_p=(ADDRESS*)fast_memory_allocate (sizeof (ADDRESS));
			graph->instruction_parameters[1].p=(INSTRUCTION_GRAPH)i_ad_p;
			
			i_ad_p->ad_mode=P_INDIRECT;
			i_ad_p->ad_offset=offset>>2;
			i_ad_p->ad_register=ad_p->ad_register;
			i_ad_p->ad_count_p=ad_p->ad_count_p;

			graph->node_count=0;
			graph->inode_arity|=LOAD_X_TO_ADDRESS;
		} else {
			int reg_1;
	
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);
	
#ifdef LOAD_X_PARAMETER_REGISTER
			if (result_register!=0 && try_allocate_register_number (result_register>>1))
				reg_1=result_register>>1;
			else
#endif
			if (graph->instruction_d_min_a_cost<=0)
				reg_1=get_dregister();
			else
				reg_1=get_aregister();
			i_move_id_r (offset>>2,ad_p->ad_register,reg_1);
			
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
			
			register_node (graph,reg_1);
		}
	}
}

static void linearize_store_r_node (INSTRUCTION_GRAPH graph)
{
	INSTRUCTION_GRAPH *register_graph_1_p,*register_graph_2_p;
	int reg_1;
	ADDRESS ad_1;
	
	switch (graph->instruction_parameters[1].p->instruction_code){
		case GCREATE:
			linearize_create_operator (graph->instruction_parameters[1].p,&ad_1,(graph->instruction_parameters[0].i<<1)+1);
			break;
		case GCREATE_R:
			linearize_create_r_operator (graph->instruction_parameters[1].p,&ad_1,(graph->instruction_parameters[0].i<<1)+1);
			break;
#ifdef LOAD_X_PARAMETER_REGISTER
		case GLOAD_X:
			linearize_load_x_operator (graph->instruction_parameters[1].p,&ad_1,(graph->instruction_parameters[0].i<<1)+1);
			break;
#endif
		default:
			linearize_graph (graph->instruction_parameters[1].p,&ad_1);
	}
	
	reg_1=graph->instruction_parameters[0].i;
	
	register_graph_1_p=NULL;
	
	if (is_d_register (reg_1)){
		if ((unsigned)d_reg_num (reg_1)<(unsigned)N_DATA_PARAMETER_REGISTERS
#ifdef MORE_PARAMETER_REGISTERS
									+ N_ADDRESS_PARAMETER_REGISTERS
#endif
		)
			register_graph_1_p=&global_block.block_graph_d_register_parameter_node[d_reg_num (reg_1)];
	} else
		if ((unsigned)a_reg_num (reg_1)<(unsigned)N_ADDRESS_PARAMETER_REGISTERS)
			register_graph_1_p=&global_block.block_graph_a_register_parameter_node[a_reg_num (reg_1)];
	
	if (ad_1.ad_mode==P_REGISTER && ad_1.ad_register==reg_1){
		if (register_graph_1_p!=NULL && *register_graph_1_p!=NULL)
			(*register_graph_1_p)->instruction_code=GGREGISTER;
		return;
	}
	
	if (ad_1.ad_mode==P_REGISTER){
		int reg_2;
		
		reg_2=ad_1.ad_register;
		
		register_graph_2_p=NULL;
		
		if ((unsigned)d_reg_num (reg_2)<(unsigned)N_DATA_PARAMETER_REGISTERS
#ifdef MORE_PARAMETER_REGISTERS
											+ N_ADDRESS_PARAMETER_REGISTERS
#endif
		)
			register_graph_2_p=&global_block.block_graph_d_register_parameter_node[d_reg_num (reg_2)];
			
		if ((unsigned)a_reg_num (reg_2)<(unsigned)N_ADDRESS_PARAMETER_REGISTERS)
			register_graph_2_p=&global_block.block_graph_a_register_parameter_node[a_reg_num (reg_2)];
		
		if (register_graph_1_p!=NULL && *register_graph_1_p!=NULL && (*register_graph_1_p)->node_count>0){
#if defined (M68000) || defined (I486)
			if (register_graph_2_p!=NULL && (*register_graph_2_p==NULL || (*register_graph_2_p)->instruction_code!=GGREGISTER)){
				INSTRUCTION_GRAPH graph;
				
				i_exg_r_r (reg_2,reg_1);
			
				(*register_graph_1_p)->instruction_parameters[0].i=reg_2;
					
				if (*register_graph_2_p!=NULL){
					(*register_graph_2_p)->instruction_code=GGREGISTER;
					(*register_graph_2_p)->instruction_parameters[0].i=reg_1;
				}
				
				graph=*register_graph_1_p;
				*register_graph_1_p=*register_graph_2_p;
				*register_graph_2_p=graph;
			} else {
#endif
				register int reg_2;
			
				register_graph_2_p=NULL;
			
				if ((*register_graph_1_p)->instruction_d_min_a_cost<=0){
					reg_2=get_dregister();
					if ((unsigned)d_reg_num (reg_2)<(unsigned)N_DATA_PARAMETER_REGISTERS
#ifdef MORE_PARAMETER_REGISTERS
														+ N_ADDRESS_PARAMETER_REGISTERS
#endif
					)
						register_graph_2_p=&global_block.block_graph_d_register_parameter_node[d_reg_num (reg_2)];
				} else {
					reg_2=get_aregister();
					if ((unsigned)a_reg_num (reg_2)<(unsigned)N_ADDRESS_PARAMETER_REGISTERS)
						register_graph_2_p=&global_block.block_graph_a_register_parameter_node[a_reg_num (reg_2)];
				}
			
				i_move_r_r (reg_1,reg_2);
				instruction_ad_r (IMOVE,&ad_1,reg_1);
				
				(*register_graph_1_p)->instruction_parameters[0].i=reg_2;
				
				if (register_graph_2_p!=NULL)
					*register_graph_2_p=*register_graph_1_p;
			
				*register_graph_1_p=NULL;
#if defined (M68000) || defined (I486)
			}
#endif
		} else {
			i_move_r_r (reg_2,reg_1);
			if (--*ad_1.ad_count_p==0)
				free_register (reg_2);
		}
	} else {
		switch (ad_1.ad_mode){
			case P_INDIRECT:
				--*ad_1.ad_count_p;
				break;
			case P_INDEXED:
			{
				INSTRUCTION_GRAPH load_x_graph;
				ADDRESS *i_ad_p;
	
				load_x_graph=(INSTRUCTION_GRAPH)ad_1.ad_offset;
				i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;			
#ifdef M68000
				if ((load_x_graph->inode_arity & LOAD_X_TO_ADDRESS) && i_ad_p->ad_mode==P_INDEXED)
					--*i_ad_p->ad_count_p2;
#endif
				--*i_ad_p->ad_count_p;
				break;
			}
		}
		
		if (register_graph_1_p!=NULL && *register_graph_1_p!=NULL && (*register_graph_1_p)->node_count>0){
			int reg_2;
			
			register_graph_2_p=NULL;
			
			if ((*register_graph_1_p)->instruction_d_min_a_cost<=0){
				reg_2=get_dregister();
				if ((unsigned)d_reg_num (reg_2)<(unsigned)N_DATA_PARAMETER_REGISTERS
#ifdef MORE_PARAMETER_REGISTERS
													+ N_ADDRESS_PARAMETER_REGISTERS
#endif
				)
					register_graph_2_p=&global_block.block_graph_d_register_parameter_node[d_reg_num (reg_2)];
			} else {
				reg_2=get_aregister();
				if ((unsigned)a_reg_num (reg_2)<(unsigned)N_ADDRESS_PARAMETER_REGISTERS)
					register_graph_2_p=&global_block.block_graph_a_register_parameter_node[a_reg_num (reg_2)];
			}
						
			i_move_r_r (reg_1,reg_2);
			
			(*register_graph_1_p)->instruction_parameters[0].i=reg_2;
			
			if (register_graph_2_p!=NULL)
				*register_graph_2_p=*register_graph_1_p;
			
			*register_graph_1_p=NULL;
		}
		
		switch (ad_1.ad_mode){
			case P_INDIRECT:
				++*ad_1.ad_count_p;
				break;
			case P_INDEXED:
			{
				INSTRUCTION_GRAPH load_x_graph;
				ADDRESS *i_ad_p;
	
				load_x_graph=(INSTRUCTION_GRAPH)ad_1.ad_offset;
				i_ad_p=(ADDRESS *)load_x_graph->instruction_parameters[1].p;			
#ifdef M68000
				if ((load_x_graph->inode_arity & LOAD_X_TO_ADDRESS) && i_ad_p->ad_mode==P_INDEXED)
					++*i_ad_p->ad_count_p2;
#endif
				++*i_ad_p->ad_count_p;
				break;
			}
		}
		
		instruction_ad_r (IMOVE,&ad_1,reg_1);
	}
	
	if (is_a_register (reg_1))
		allocate_aregister (reg_1);
	else
		allocate_dregister (reg_1);
}

static void linearize_load_b_x_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	register INSTRUCTION_GRAPH graph_1,graph_2;
	register int offset;
	
	graph_1=graph->instruction_parameters[0].p;
	offset=graph->instruction_parameters[1].i;
	graph_2=graph->instruction_parameters[2].p;

	if (graph_2!=NULL){
		ADDRESS ad_1;

		if (graph->order_left){
			linearize_graph (graph_1,ad_p);
			linearize_graph (graph_2,&ad_1);
		} else {
			linearize_graph (graph_2,&ad_1);
			linearize_graph (graph_1,ad_p);
		}

#if defined (sparc) || defined (G_POWER)
		{
			int reg_1,reg_2;

			in_register (&ad_1);
			in_register (ad_p);
			
			if (--*ad_1.ad_count_p==0)
				free_register (ad_1.ad_register);
			if (--*ad_p->ad_count_p==0)
				free_register (ad_p->ad_register);

			if (offset>>2==0){				
				if (graph->instruction_d_min_a_cost<=0)
					reg_2=get_dregister();
				else
					reg_2=get_aregister();
				i_moveb_x_r (ad_p->ad_register,ad_1.ad_register,reg_2);
			} else {
				reg_1=get_aregister();
				
				i_lea_x_r (ad_p->ad_register,ad_1.ad_register,reg_1);

				free_aregister (reg_1);
				
				if (graph->instruction_d_min_a_cost<=0)
					reg_2=get_dregister();
				else
					reg_2=get_aregister();
				i_moveb_id_r (offset>>2,reg_1,reg_2);
			}

# ifdef G_POWER
			if (offset & 1)
				i_extb_r (reg_2);
# endif
	
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_2;
			ad_p->ad_count_p=&graph->node_count;

			if (graph->node_count<=1)
				graph->inode_arity=(graph->inode_arity & ~LOAD_X_TO_ADDRESS) | LOAD_X_TO_REGISTER;
			else
				register_node (graph,reg_2);
		}
#else
		{
			int reg_1;
	
			in_data_register (&ad_1);
			in_address_register (ad_p);
		
			if (--*ad_p->ad_count_p==0)
				free_aregister (ad_p->ad_register);

			if (--*ad_1.ad_count_p==0)
				free_dregister (ad_1.ad_register);

			reg_1=get_dregister();
# ifdef I486
			i_moveb_x_r (offset & ~3,ad_p->ad_register,ad_1.ad_register,reg_1);
# else
			if (offset & 1){
				i_moveb_x_r (offset & ~3,ad_p->ad_register,ad_1.ad_register,reg_1);
				i_extb_r (reg_1);
			} else {
				if (ad_1.ad_register==reg_1){
					i_moveb_x_r (offset,ad_p->ad_register,ad_1.ad_register,reg_1);
					i_and_i_r (255,reg_1);
				} else {
					i_move_i_r (0,reg_1);
					i_moveb_x_r (offset,ad_p->ad_register,ad_1.ad_register,reg_1);
				}
			}
# endif	
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
		
			if (graph->node_count<=1)
				graph->inode_arity=(graph->inode_arity & ~LOAD_X_TO_ADDRESS) | LOAD_X_TO_REGISTER;
			else
				register_node (graph,reg_1);
		}
#endif
	} else {
		int reg_1;

		linearize_graph (graph_1,ad_p);
		in_address_register (ad_p);
	
		if (--*ad_p->ad_count_p==0)
			free_aregister (ad_p->ad_register);

		if (graph->instruction_d_min_a_cost<=0)
			reg_1=get_dregister();
		else
			reg_1=get_aregister();

#ifdef M68000
		if (!(offset & 1))
			i_move_i_r (0,reg_1);
#endif
		i_moveb_id_r (offset>>2,ad_p->ad_register,reg_1);
#if defined (M68000) || defined (G_POWER)
		if (offset & 1)
			i_extb_r (reg_1);			
#endif
		
		ad_p->ad_mode=P_REGISTER;
		ad_p->ad_register=reg_1;
		ad_p->ad_count_p=&graph->node_count;
		
		if (graph->node_count<=1)
			graph->inode_arity=(graph->inode_arity & ~LOAD_X_TO_ADDRESS) | LOAD_X_TO_REGISTER;
		else
			register_node (graph,reg_1);
	}
}

#if 0
static int selects_from_array (INSTRUCTION_GRAPH select_graph,INSTRUCTION_GRAPH graph)
{
	if (select_graph==graph)
		return 1;
	
	switch (select_graph->instruction_code){
		case GLOAD_X:
		case GFLOAD_X:
		case GLOAD_B_X:
			return selects_from_array (select_graph->instruction_parameters[0].p,graph);
		case GLOAD_ID:
			return selects_from_array (select_graph->instruction_parameters[1].p,graph);
		default:
			return 0;
	}
}
#endif

static void do_array_selects_before_update (INSTRUCTION_GRAPH select_graph,INSTRUCTION_GRAPH graph_1,INSTRUCTION_GRAPH graph_2)
{
	for (; select_graph!=NULL; select_graph=select_graph->instruction_parameters[3].p){
		switch (select_graph->instruction_code){
			case GLOAD_X:
#if 1
				if (graph_2==select_graph->instruction_parameters[0].p){
#else
				if (selects_from_array (select_graph->instruction_parameters[0].p,graph_2)){
#endif
					if (select_graph->node_count>0){
						ADDRESS s_ad;
										
						++select_graph->node_count;
						linearize_load_x_operator (select_graph,&s_ad
#ifdef LOAD_X_PARAMETER_REGISTER
											,0
#endif
											);
						--select_graph->node_count;
					} else if ((select_graph->inode_arity & LOAD_X_TO_ADDRESS) && select_graph!=graph_1){
						ADDRESS *i_ad_p;
						int dreg;
						
						i_ad_p=(ADDRESS *)select_graph->instruction_parameters[1].p;
			
						select_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);

						if (i_ad_p->ad_mode==P_INDEXED){
							if (--*i_ad_p->ad_count_p==0)
								free_aregister (i_ad_p->ad_areg);
							if (--*i_ad_p->ad_count_p2==0)
								free_dregister (i_ad_p->ad_dreg);
							dreg=get_dregister();
							i_move_x_r (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,dreg);
						} else {
							if (--*i_ad_p->ad_count_p==0)
								free_aregister (i_ad_p->ad_register);
							dreg=get_dregister();
							i_move_id_r (i_ad_p->ad_offset,i_ad_p->ad_register,dreg);
						}
						i_ad_p->ad_register=dreg;
					}
				}		
				break;
			case GLOAD_B_X:
#if 1
				if (graph_2==select_graph->instruction_parameters[0].p){
#else
				if (selects_from_array (select_graph->instruction_parameters[0].p,graph_2)){
#endif
					if (select_graph->node_count>0 /* added 29-2-2000: */ && !(select_graph->inode_arity & LOAD_X_TO_REGISTER) /**/){
						ADDRESS s_ad;
										
						++select_graph->node_count;
						linearize_load_b_x_operator (select_graph,&s_ad);
						--select_graph->node_count;
					}
				}		
				break;
			case GFLOAD_X:
#if 1
				if (graph_2==select_graph->instruction_parameters[0].p){
#else
				if (selects_from_array (select_graph->instruction_parameters[0].p,graph_2)){
#endif
					if (select_graph->node_count>0){
						ADDRESS s_ad;
						
						++select_graph->node_count;
						linearize_fload_x_operator (select_graph,&s_ad);
						--select_graph->node_count;
					} else if ((select_graph->inode_arity & LOAD_X_TO_ADDRESS) && select_graph!=graph_1){
						ADDRESS *i_ad_p;
						int freg;
							
						i_ad_p=(ADDRESS *)select_graph->instruction_parameters[1].p;
				
						select_graph->inode_arity ^= (LOAD_X_TO_ADDRESS | LOAD_X_TO_REGISTER);;

						if (i_ad_p->ad_mode==P_INDEXED){
							if (--*i_ad_p->ad_count_p==0)
								free_aregister (i_ad_p->ad_areg);
							if (--*i_ad_p->ad_count_p2==0)
								free_dregister (i_ad_p->ad_dreg);
							freg=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
							i_fmove_x_fr (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,freg,select_graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
							i_fmove_x_fr (i_ad_p->ad_offset,i_ad_p->ad_areg,i_ad_p->ad_dreg,freg);
#endif
						} else {
							if (--*i_ad_p->ad_count_p==0)
								free_aregister (i_ad_p->ad_register);
							freg=get_fregister();
#ifdef ALIGN_REAL_ARRAYS
							i_fmove_id_fr_f (i_ad_p->ad_offset,i_ad_p->ad_register,freg,select_graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
							i_fmove_id_fr (i_ad_p->ad_offset,i_ad_p->ad_register,freg);
#endif
						}
						i_ad_p->ad_register=freg;
					}
				}
				break;
			case GREGISTER:
			case GFREGISTER:
			case GFHIGH:
			case GFLOW:
			case GGREGISTER:
				break;
			default:
				internal_error_in_function ("do_array_selects_before_update");
		}
	}
}

static void linearize_store_x_operator (int i_instruction_code,register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2,graph_3;
	int offset;
	ADDRESS ad_1,ad_3;
	
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	offset=graph->instruction_parameters[2].i;
	graph_3=graph->instruction_parameters[3].p;

	if (graph_3==NULL){
		if (graph->order_left){
			linearize_graph (graph_1,&ad_1);
			linearize_graph (graph_2,ad_p);
		} else {
			linearize_graph (graph_1,&ad_1);
			linearize_graph (graph_2,ad_p);
		}
	} else {
		switch (graph->order_left){
			case 0:
				linearize_graph (graph_1,&ad_1);
				linearize_graph (graph_2,ad_p);
				linearize_graph (graph_3,&ad_3);
				break;
			case 1:
				linearize_graph (graph_1,&ad_1);
				linearize_graph (graph_3,&ad_3);
				linearize_graph (graph_2,ad_p);
				break;
			case 2:
				linearize_graph (graph_2,ad_p);
				linearize_graph (graph_1,&ad_1);
				linearize_graph (graph_3,&ad_3);
				break;
			case 3:
				linearize_graph (graph_2,ad_p);
				linearize_graph (graph_3,&ad_3);
				linearize_graph (graph_1,&ad_1);
				break;
			case 4:
				linearize_graph (graph_3,&ad_3);
				linearize_graph (graph_2,ad_p);
				linearize_graph (graph_1,&ad_1);
				break;
			case 5:
				linearize_graph (graph_3,&ad_3);
				linearize_graph (graph_1,&ad_1);
				linearize_graph (graph_2,ad_p);
				break;
			default:
				internal_error_in_function ("linearize_store_x_operator");
		}
	}
	
	do_array_selects_before_update (graph->instruction_parameters[4].p,graph_1,graph_2);

	in_address_register (ad_p);

	if (graph_3==NULL){
		if (i_instruction_code==IMOVEB && ad_1.ad_mode!=P_IMMEDIATE)
			in_data_register (&ad_1);

		instruction_ad_id (i_instruction_code,&ad_1,offset>>2,ad_p->ad_register);
	} else {
		in_data_register (&ad_3);
		
#ifndef M68000
		if (ad_1.ad_mode==P_INDEXED)
			in_register (&ad_1);
#endif

		if (i_instruction_code==IMOVEB && ad_1.ad_mode!=P_IMMEDIATE
#ifdef M68000
			&& ad_1.ad_mode!=P_INDEXED
#endif
			)
			in_data_register (&ad_1);

		if (--*ad_3.ad_count_p==0)
			free_dregister (ad_3.ad_register);

		instruction_ad_x (i_instruction_code,&ad_1,offset,ad_p->ad_register,ad_3.ad_register);
	}
		
	if (graph->node_count>1){
/* added 10-12-97 */
	/* changed 27-6-2001 */
	
		while (graph_2->instruction_code==GINDIRECTION)
			graph_2=graph_2->instruction_parameters[0].p;
		
 		if (graph_2->instruction_code==GREGISTER){
 	/*	if (graph_2->instruction_code==GREGISTER || graph_2->instruction_code==GINDIRECTION){ */

			register_node (graph_2,ad_p->ad_register);
			graph_2->node_count += graph->node_count;
			ad_p->ad_count_p=&graph_2->node_count;

			graph->instruction_parameters[0].p=graph_2;
			graph->instruction_code=GINDIRECTION;
		} else
/* */
		{
			register_node (graph,ad_p->ad_register);
			ad_p->ad_count_p=&graph->node_count;
		}
	}
/* added 27-6-2001 */
	else {
		if (graph_2->instruction_code==GREGISTER){
			graph->instruction_parameters[0].p=graph_2;
			graph->instruction_code=GINDIRECTION;
		} else if (graph_2->instruction_code==GINDIRECTION){
			graph->instruction_parameters[0].p=graph_2->instruction_parameters[0].p;
			graph->instruction_code=GINDIRECTION;
		}
 	}
/**/
}

static void linearize_bounds_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	ADDRESS ad_1;

	if (graph->order_left){
		linearize_graph (graph->instruction_parameters[0].p,&ad_1);
		linearize_graph (graph->instruction_parameters[1].p,ad_p);
	} else {
		linearize_graph (graph->instruction_parameters[1].p,ad_p);
		linearize_graph (graph->instruction_parameters[0].p,&ad_1);
	}

	in_address_register (&ad_1);

	if (graph->node_count>1)
		in_alterable_data_register (ad_p);
	else
		in_data_register (ad_p);

	i_cmp_id_r (4,ad_1.ad_register,ad_p->ad_register);
	instruction_l (IBHS,index_error_label);

	if (--*ad_1.ad_count_p==0)
		free_aregister (ad_1.ad_register);

	if (graph->node_count>1){
		register_node (graph,ad_p->ad_register);
		ad_p->ad_count_p=&graph->node_count;
	}
}

static void linearize_exit_if_operator (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	INSTRUCTION_GRAPH graph_1,graph_2;
	int condition;
	
	graph_1=graph->instruction_parameters[1].p;
	graph_2=graph->instruction_parameters[2].p;

	condition=linearize_condition (graph_1);
	
	instruction_l (condition_to_branch_false_instruction[condition],graph->instruction_parameters[0].l);
		
	linearize_graph (graph_2,ad_p);

	if (graph->node_count>1){
		in_address_register (ad_p);
		register_node (graph_2,ad_p->ad_register);
		graph_2->node_count += graph->node_count;
		ad_p->ad_count_p=&graph_2->node_count;

		graph->instruction_parameters[0].p=graph_2;
		graph->instruction_code=GINDIRECTION;
	}			
}

static void linearize_graph (INSTRUCTION_GRAPH graph,ADDRESS *ad_p)
{
	switch (graph->instruction_code){
		case GLOAD:
		{
			if (graph->node_count<=1){
				ad_p->ad_mode=P_INDIRECT;
				ad_p->ad_count_p=&ad_p->ad_count;
				ad_p->ad_count=1+1;
				ad_p->ad_offset=graph->instruction_parameters[0].i;
				ad_p->ad_register=graph->instruction_parameters[1].i;
				graph->node_count=0;
			} else {
				int reg_1;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg_1=get_dregister();
				else
					reg_1=get_aregister();
					
				i_move_id_r (graph->instruction_parameters[0].i,graph->instruction_parameters[1].i,reg_1);
		
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg_1;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg_1);
			}
			return;
		}
		case GREGISTER:
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=graph->instruction_parameters[0].i;
			ad_p->ad_count_p=&graph->node_count;
			return;
		case GGREGISTER:
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=graph->instruction_parameters[0].i;
			ad_p->ad_count_p=&ad_p->ad_count;
			ad_p->ad_count=1+1;
			return;
		case GLOAD_I:
			if (graph->node_count<=1){
				ad_p->ad_mode=P_IMMEDIATE;
				ad_p->ad_offset=graph->instruction_parameters[0].i;
			} else {
				register int reg_1;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg_1=get_dregister();
				else
					reg_1=get_aregister();
				i_move_i_r (graph->instruction_parameters[0].i,reg_1);
				
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg_1;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg_1);
			}
			return;
		case GLOAD_DES_I:
			if (graph->node_count<=1){
				ad_p->ad_mode=P_DESCRIPTOR_NUMBER;
				ad_p->ad_label=graph->instruction_parameters[0].l;
				ad_p->ad_offset=graph->instruction_parameters[1].i;
			} else {
				int reg_1;
				
				if (graph->instruction_d_min_a_cost<=0)
					reg_1=get_dregister();
				else
					reg_1=get_aregister();
					
				i_move_d_r (graph->instruction_parameters[0].l,graph->instruction_parameters[1].i,reg_1);
				
				ad_p->ad_mode=P_REGISTER;
				ad_p->ad_register=reg_1;
				ad_p->ad_count_p=&graph->node_count;
				
				register_node (graph,reg_1);
			}
			return;
		case GLEA:
		{
			register int reg_1;

#ifdef I486
			if (graph->node_count==1){
				ad_p->ad_mode=P_DESCRIPTOR_NUMBER;
				if (graph->inode_arity==1)
					ad_p->ad_offset=graph->instruction_parameters[1].i;
				else
					ad_p->ad_offset=0;
				ad_p->ad_label=graph->instruction_parameters[0].l;
				ad_p->ad_count_p=&graph->node_count;
				return;
			} else {
#endif

#ifndef M68000
			if (graph->instruction_d_min_a_cost<0)
				reg_1=get_dregister();
			else
#endif
			reg_1=get_aregister();

			if (graph->inode_arity==1)
				i_lea_l_i_r (graph->instruction_parameters[0].l,graph->instruction_parameters[1].i,reg_1);
			else
				i_lea_l_i_r (graph->instruction_parameters[0].l,0,reg_1);
#ifdef M68000			
			if (graph->instruction_d_min_a_cost<0){
				int reg_2;
				
				reg_2=get_dregister();
				i_move_r_r (reg_1,reg_2);
				free_aregister (reg_1);
				reg_1=reg_2;
			}
#endif
			if (graph->node_count>1)
				register_node (graph,reg_1);
			
			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
			
			return;
#ifdef I486
			}
#endif
		}
		case GLOAD_ID:
		{
			INSTRUCTION_GRAPH graph_1;
			
			graph_1=graph->instruction_parameters[1].p;
			linearize_graph (graph_1,ad_p);
			in_address_register (ad_p);
			
			if (graph->node_count<=1){
				ad_p->ad_mode=P_INDIRECT;
				ad_p->ad_offset=graph->instruction_parameters[0].i;
				--graph->node_count;
			} else {
				int reg_1;
				
				if (--*ad_p->ad_count_p==0)
					free_aregister (ad_p->ad_register);
				
				if (graph->instruction_d_min_a_cost<=0)
					reg_1=get_dregister();
				else
					reg_1=get_aregister();
				i_move_id_r (graph->instruction_parameters[0].i,ad_p->ad_register,reg_1);
				
				U3( ad_p, ad_mode=P_REGISTER, ad_register=reg_1, ad_count_p=&graph->node_count);
				
				register_node (graph,reg_1);
			}
			return;
		}
		case GLOAD_B_ID:
		{
			register INSTRUCTION_GRAPH graph_1;
			ADDRESS ad_1;
			register int reg_1;
			
			graph_1=graph->instruction_parameters[1].p;
			linearize_graph (graph_1,&ad_1);
			in_address_register (&ad_1);
			
			if (--*ad_1.ad_count_p==0)
				free_aregister (ad_1.ad_register);
				
			reg_1=get_dregister();
#ifdef M68000
			i_move_i_r (0,reg_1);
#endif
			i_moveb_id_r (graph->instruction_parameters[0].i,ad_1.ad_register,reg_1);
			
			if (graph->instruction_d_min_a_cost>0){
				int reg_2;
				
				reg_2=get_aregister();
				i_move_r_r (reg_1,reg_2);
				free_dregister (reg_1);
				reg_1=reg_2;
			}
			
			if (graph->node_count>1)
				register_node (graph,reg_1);

			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
	
			return;
		}
		case GLOAD_DES_ID:
		{
			register INSTRUCTION_GRAPH graph_1;
			ADDRESS ad_1;
			register int reg_1;
			
			graph_1=graph->instruction_parameters[1].p;
			linearize_graph (graph_1,&ad_1);
			in_address_register (&ad_1);
			
			if (--*ad_1.ad_count_p==0)
				free_aregister (ad_1.ad_register);
			
			if (graph->instruction_d_min_a_cost<0){
				reg_1=get_dregister();
				i_movew_id_r (graph->instruction_parameters[0].i,ad_1.ad_register,reg_1);
#ifdef M68000
				i_ext_r (reg_1);
#endif
			} else {
				reg_1=get_aregister();
				i_movew_id_r (graph->instruction_parameters[0].i,ad_1.ad_register,reg_1);
			}
			
			if (graph->node_count>1)
				register_node (graph,reg_1);

			ad_p->ad_mode=P_REGISTER;
			ad_p->ad_register=reg_1;
			ad_p->ad_count_p=&graph->node_count;
	
			return;
		}
		case GSTORE:
		{
			INSTRUCTION_GRAPH a_graph;
			int offset,stack_pointer;
			
			a_graph=graph->instruction_parameters[2].p;
			
			if (a_graph->node_count==1)
				switch (a_graph->instruction_code){
					case GFILL:
						linearize_store_fill_operator (graph,a_graph);
						return;
					case GCREATE:
						if (a_graph->inode_arity>3 && a_graph->instruction_parameters[0].p!=NULL){
							int arity;
												
							arity=a_graph->inode_arity;
							while (--arity>0 && a_graph->instruction_parameters[arity].p!=NULL)
								;

							if (arity>0)
								break;
						}					
	
						offset=graph->instruction_parameters[0].i;
						stack_pointer=graph->instruction_parameters[1].i;

						linearize_load_graph (graph->instruction_parameters[3].p);

						linearize_store_create_operator (offset,stack_pointer,a_graph);
						return;
				}
		
			linearize_graph (a_graph,ad_p);
			
			offset=graph->instruction_parameters[0].i;
			stack_pointer=graph->instruction_parameters[1].i;
			
			linearize_load_graph (graph->instruction_parameters[3].p);
						
			instruction_ad_id (IMOVE,ad_p,offset,stack_pointer);
			return;
		}
		case GBEFORE0:
		case GBEFORE:
		{
			INSTRUCTION_GRAPH graph_1;
			int i;
			
			graph_1=graph->instruction_parameters[0].p;
			linearize_graph (graph_1,ad_p);

			if (graph->instruction_code==GBEFORE){
				/* added 16-5-2000 */				
				for (i=1; i<graph->inode_arity; ++i){
					INSTRUCTION_GRAPH graph_2;

					graph_2=graph->instruction_parameters[i].p;
					if (graph_2->node_count==1 && graph_2->instruction_code==GMOVEMI)
						graph_2->node_count=0;
				}
				/**/
				for (i=1; i<graph->inode_arity; ++i){
					INSTRUCTION_GRAPH graph_2;
					ADDRESS ad_1;

					graph_2=graph->instruction_parameters[i].p;
					if (graph_2->node_count>1){
						linearize_graph (graph_2,&ad_1);					
						in_register (&ad_1);
						register_node (graph_2,ad_1.ad_register);
					}
				}
			}
			
			if (graph->node_count>1){
				in_address_register (ad_p);
/* changed 27-12-2000
				register_node (graph,ad_p->ad_register);
				ad_p->ad_count_p=&graph->node_count;
*/
				register_node (graph_1,ad_p->ad_register);
				graph_1->node_count += graph->node_count;
				ad_p->ad_count_p=&graph_1->node_count;

				/* graph->instruction_parameters[0].p=graph_1; */
				graph->instruction_code=GINDIRECTION;
/* */
			}			

			return;
		}
		case GKEEP:
		{
			INSTRUCTION_GRAPH graph_1,graph_2;
			ADDRESS ad_1;
			
			graph_1=graph->instruction_parameters[0].p;
			graph_2=graph->instruction_parameters[1].p;

			linearize_graph (graph_1,&ad_1);
			
			switch (ad_1.ad_mode){
				case P_REGISTER:
				case P_INDIRECT:
					if (--*ad_1.ad_count_p==0)
						free_register (ad_1.ad_register);
			}
			
			linearize_graph (graph_2,ad_p);
						
			if (graph->node_count>1){
				in_address_register (ad_p);
/* changed 10-12-97
				register_node (graph,ad_p->ad_register);
				ad_p->ad_count_p=&graph->node_count;
*/
				register_node (graph_2,ad_p->ad_register);
				graph_2->node_count += graph->node_count;
				ad_p->ad_count_p=&graph_2->node_count;

				graph->instruction_parameters[0].p=graph_2;
				graph->instruction_code=GINDIRECTION;
/**/
			}			

			return;
		}
/* added 10-12-97 */
		case GINDIRECTION:
			linearize_graph (graph->instruction_parameters[0].p,ad_p);
			return;
/* */		
		case GLOAD_X:
			linearize_load_x_operator (graph,ad_p
#ifdef LOAD_X_PARAMETER_REGISTER
								,0
#endif
								);
			return;
		case GLOAD_B_X:
			linearize_load_b_x_operator (graph,ad_p);
			return;			
		case GSTORE_X:
			linearize_store_x_operator (IMOVE,graph,ad_p);
			return;
		case GSTORE_B_X:
			linearize_store_x_operator (IMOVEB,graph,ad_p);
			return;
		case GSTORE_R:
			linearize_store_r_node (graph);
			break;
		case GFSTORE:
			linearize_fstore_operator (graph);
			return;
		case GFSTORE_X:
			linearize_fstore_x_operator (graph,ad_p);
			return;
		case GFSTORE_R:
			linearize_fstore_r_operator (graph);
			return;
		case GADD:
			linearize_dyadic_commutative_operator (IADD,graph,ad_p);
			return;
		case GAND:
			linearize_dyadic_commutative_data_operator (IAND,graph,ad_p);
			return;
		case GMUL:
			linearize_dyadic_commutative_data_operator (IMUL,graph,ad_p);
			return;
		case GOR:
			linearize_dyadic_commutative_data_operator (IOR,graph,ad_p);
			return;
		case GEOR:
#ifdef M68000
			linearize_eor_operator (graph,ad_p);
#else
			linearize_dyadic_commutative_data_operator (IEOR,graph,ad_p);
#endif
			return;
		case GSUB:
			linearize_dyadic_non_commutative_operator (ISUB,graph,ad_p);
			return;
#if defined (I486) || defined (G_POWER)
		case GDIV:
			linearize_div_rem_operator (IDIV,graph,ad_p);
			return;
		case GMOD:
			linearize_div_rem_operator (IMOD,graph,ad_p);
			return;
# ifdef I486
		case GDIVU:
			linearize_div_rem_operator (IDIVU,graph,ad_p);
			return;
		case GREMU:
			linearize_div_rem_operator (IREMU,graph,ad_p);
			return;
# endif
#else
		case GDIV:
			linearize_dyadic_non_commutative_data_operator (IDIV,graph,ad_p);
			return;
		case GMOD:
			linearize_mod_operator (IMOD,graph,ad_p);
			return;
#endif
		case GCMP_EQ:
			linearize_compare_operator (CEQ,CEQ,graph,ad_p);
			return;
		case GCMP_LT:
			linearize_compare_operator (CLT,CGT,graph,ad_p);
			return;
		case GCMP_LTU:
			linearize_compare_operator (CLTU,CGTU,graph,ad_p);
			return;
		case GCMP_GT:
			linearize_compare_operator (CGT,CLT,graph,ad_p);
			return;
		case GCMP_GTU:
			linearize_compare_operator (CGTU,CLTU,graph,ad_p);
			return;
		case GCNOT:
			linearize_conditional_not_operator (graph,ad_p);
			return;
		case GLSR:
			linearize_shift_operator (ILSR,graph,ad_p);
			return;
		case GLSL:
			linearize_shift_operator (ILSL,graph,ad_p);
			return;
		case GASR:
			linearize_shift_operator (IASR,graph,ad_p);
			return;
		case GCREATE:
			linearize_create_operator (graph,ad_p,0);
			return;
#ifdef G_POWER
		case GCREATE_S:
			ad_p->ad_mode=P_STORE_HP_INSTRUCTION;
			ad_p->ad_offset=(LONG) linearize_store_create_operator (0,HEAP_POINTER,graph);
			return;
#endif
		case GCREATE_R:
			linearize_create_r_operator (graph,ad_p,0);
			return;
		case GFILL:
			linearize_fill_operator (graph,ad_p);
			return;
		case GFILL_R:
			linearize_fill_r_operator (graph,ad_p);
			return;
		case GNEG:
			linearize_monadic_data_operator (INEG,graph,ad_p);
			return;
#if defined (I486) || defined (G_POWER)
		case GNOT:
			linearize_monadic_data_operator (INOT,graph,ad_p);
			return;
#endif
		case GMOVEMI:
			linearize_movemi_operator (graph,ad_p);
			return;
		case GCOPY:
			linearize_copy_operator (graph,ad_p);
			return;
		case GFHIGH:
			linearize_fhigh_operator (graph,ad_p);
			return;
		case GFLOW:
			linearize_flow_operator (graph,ad_p);
			return;
		case GFCMP_EQ:
			linearize_float_compare_operator (CFEQ,CFEQ,graph,ad_p);
			return;
		case GFCMP_LT:
			linearize_float_compare_operator (CFLT,CFGT,graph,ad_p);
			return;
		case GFCMP_GT:
			linearize_float_compare_operator (CFGT,CFLT,graph,ad_p);
			return;
		case GFRTOI:
			linearize_rtoi_operator (graph,ad_p);
			return;
		case GBOUNDS:
			linearize_bounds_operator (graph,ad_p);
			return;
		case GEXIT_IF:
			linearize_exit_if_operator (graph,ad_p);
			return;
		case GADD_O:
			linearize_add_o_operator (graph,ad_p,NULL);
			return;
		case GMUL_O:
			linearize_mul_o_operator (graph,ad_p,NULL);
			break;
		case GSUB_O:
			linearize_sub_o_operator (graph,ad_p,NULL);
			return;
		case GTEST_O:
			linearize_test_o_operator (graph,ad_p);
			return;
#ifdef G_POWER
		case GUMULH:
			linearize_dyadic_commutative_data_operator (IUMULH,graph,ad_p);
			return;
#endif
		default:
			/* printf ("%d %d\n",(int)graph,graph->instruction_code); */
			internal_error_in_function ("linearize_graph");
	}
}

#pragma segment Code30

static void linearize_fstore_x_operator (register INSTRUCTION_GRAPH graph,register ADDRESS *ad_p)
{
	register INSTRUCTION_GRAPH graph_1,graph_2,graph_3;
	register int offset;
	ADDRESS ad_1,ad_3;
			
	graph_1=graph->instruction_parameters[0].p;
	graph_2=graph->instruction_parameters[1].p;
	offset=graph->instruction_parameters[2].i;
	graph_3=graph->instruction_parameters[3].p;

	if (graph_3==NULL){
		if (graph->order_left){
			linearize_float_graph (graph_1,&ad_1);
			linearize_graph (graph_2,ad_p);
		} else {
			linearize_graph (graph_2,ad_p);
			linearize_float_graph (graph_1,&ad_1);
		}
	} else {
		switch (graph->order_left){
			case 0:
				linearize_float_graph (graph_1,&ad_1);
				linearize_graph (graph_2,ad_p);
				linearize_graph (graph_3,&ad_3);
				break;
			case 1:
				linearize_float_graph (graph_1,&ad_1);
				linearize_graph (graph_3,&ad_3);
				linearize_graph (graph_2,ad_p);
				break;
			case 2:
				linearize_graph (graph_2,ad_p);
				linearize_float_graph (graph_1,&ad_1);
				linearize_graph (graph_3,&ad_3);
				break;
			case 3:
				linearize_graph (graph_2,ad_p);
				linearize_graph (graph_3,&ad_3);
				linearize_float_graph (graph_1,&ad_1);
				break;
			case 4:
				linearize_graph (graph_3,&ad_3);
				linearize_graph (graph_2,ad_p);
				linearize_float_graph (graph_1,&ad_1);
				break;
			case 5:
				linearize_graph (graph_3,&ad_3);
				linearize_float_graph (graph_1,&ad_1);
				linearize_graph (graph_2,ad_p);
				break;
			default:
				internal_error_in_function ("linearize_fstore_x_operator");
		}
	}
	
	do_array_selects_before_update (graph->instruction_parameters[4].p,graph_1,graph_2);

	in_address_register (ad_p);

	if (graph_3==NULL){
#ifdef ALIGN_REAL_ARRAYS
		if (ad_1.ad_mode==P_F_REGISTER){
			i_fmove_fr_id_f (ad_1.ad_register,offset>>2,ad_p->ad_register,graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
			if (--*ad_1.ad_count_p==0){
# ifdef FP_STACK_OPTIMIZATIONS
				last_instruction->instruction_parameters[0].parameter_flags |= FP_REG_LAST_USE;
# endif
				free_fregister (ad_1.ad_register);
			}
		} else
#endif
		move_float_ad_id (&ad_1,offset>>2,ad_p->ad_register);
	} else {
		in_data_register (&ad_3);

		if (--*ad_3.ad_count_p==0)
			free_dregister (ad_3.ad_register);

#ifndef M68000
# ifdef I486
		if (ad_1.ad_mode==P_INDEXED)
# else
		if (ad_1.ad_mode!=P_F_REGISTER)
# endif
			in_float_register (&ad_1);
#endif
#ifdef ALIGN_REAL_ARRAYS
		move_float_ad_x (&ad_1,offset,ad_p->ad_register,ad_3.ad_register,graph->inode_arity & LOAD_STORE_ALIGNED_REAL);
#else
		move_float_ad_x (&ad_1,offset,ad_p->ad_register,ad_3.ad_register);
#endif
	}
	
	if (graph->node_count>1){
		register_node (graph,ad_p->ad_register);
		ad_p->ad_count_p=&graph->node_count;
	}
}

void calculate_and_linearize_branch_false (LABEL *label,INSTRUCTION_GRAPH graph)
{
	int condition,condition_on_stack;
	
	calculate_graph_register_uses (graph);
	condition=linearize_condition (graph);
	
	condition_on_stack=adjust_stack_pointers_without_altering_condition_codes (is_float_condition (condition),condition);

#ifdef M68000
	if (condition_on_stack){
		i_tstb_pi (REGISTER_A7);
		instruction_l (IBEQ,label);
	} else
#endif
		instruction_l (condition_to_branch_false_instruction[condition],label);	
}

void calculate_and_linearize_branch_true (LABEL *label,INSTRUCTION_GRAPH graph)
{
	int condition,condition_on_stack;
	
	calculate_graph_register_uses (graph);
	condition=linearize_condition (graph);

	condition_on_stack=adjust_stack_pointers_without_altering_condition_codes (is_float_condition (condition),condition);

#ifdef M68000
	if (condition_on_stack){
		i_tstb_pi (REGISTER_A7);
		instruction_l (IBNE,label);
	} else
#endif
		instruction_l (condition_to_branch_true_instruction[condition],label);
}

void calculate_and_linearize_graph (INSTRUCTION_GRAPH graph)
{
	ADDRESS ad;

	calculate_graph_register_uses (graph);
	linearize_graph (graph,&ad);
}

void initialize_linearization (VOID)
{
	free_dregisters.r_s_list.r_l_next=NULL;
	free_aregisters.r_s_list.r_l_next=NULL;
	free_fregisters.r_s_list.r_l_next=NULL;
	
	free_all_aregisters();
	free_all_dregisters();
	free_all_fregisters();

	IF_G_POWER (last_heap_pointer_update=NULL;)
}

#if 0
static char *i_instruction_names[] = {
	"ADD",		"AND",		"ASR",		"BEQ",		"BGE",		"BGT",		"BHS",
	"BLE",		"BLT",		"BMI",		"BMOVE",	"BNE",		"CMP",		"CMPW",
	"DIV",		"EOR",		"EXG",		"EXT",		"FACOS",	"FADD",		"FASIN",
	"FATAN",	"FBEQ",		"FBGE",		"FBGT",		"FBLE",		"FBLT",		"FBNE",
	"FCMP",		"FCOS",		"FDIV",		"FEXP",		"FLN",		"FLOG10",	"FMUL",
	"FREM",		"FSEQ",		"FSGE",		"FSGT",		"FSIN",		"FSLE",		"FSLT",
	"FSNE",		"FSQRT",	"FSUB",		"FTAN",		"FTST",		"FMOVE",	"FMOVEL",
	"JMP",		"JSR",		"LEA",		"LSL",		"LSR",		"MOD",		"MOVE",
	"MOVEB",	"MOVEM",	"MOVEW",	"MUL",		"OR",		"RTS",		"SEQ",
	"SGE",		"SGT",		"SLE",		"SLT",		"SNE",		"SUB",		"TST",
	"TSTB",		"WORD"
};

static void show_parameter (register struct parameter *parameter)
{
	switch (parameter->parameter_type){
		case P_REGISTER:
		{
			int reg=parameter->parameter_data.reg.r;
			if (is_d_register (reg))
				printf ("D%d",d_reg_num (reg));
			else
				printf ("A%d",a_reg_num (reg));
			break;
		}
		case P_LABEL:
			if (parameter->parameter_data.l->label_number!=0)
				printf ("L%d",parameter->parameter_data.l->label_number);
			else
				printf ("%s",parameter->parameter_data.l->label_name);
			break;
		case P_DESCRIPTOR_NUMBER:
			printf ("#!%s+%d",parameter->parameter_data.l->label_name,parameter->parameter_offset);
			break;
		case P_INDIRECT:
			if (parameter->parameter_offset==0)
				printf ("(A%d)",a_reg_num (parameter->parameter_data.reg.r));
			else
				printf ("%d(A%d)",parameter->parameter_offset,
						a_reg_num (parameter->parameter_data.reg.r));
			break;
		case P_IMMEDIATE:
			printf ("#%ld",parameter->parameter_data.i);
			break;
#ifdef M68000
		case P_POST_INCREMENT:
			printf ("(A%d)+",a_reg_num (parameter->parameter_data.reg.r));	
			break;
		case P_PRE_DECREMENT:
			printf ("-(A%d)",a_reg_num (parameter->parameter_data.reg.r));
			break;
#endif
		case P_F_REGISTER:
			printf ("FP%d",parameter->parameter_data.reg.r);
			break;
		case P_F_IMMEDIATE:
			printf ("#%g",*parameter->parameter_data.r);
			break;
		case P_INDEXED:
		{
			struct index_registers *index_registers;
			int offset;
			
			index_registers=parameter->parameter_data.ir;
			offset=parameter->parameter_offset;
			
			printf ("%d(a%d,d%d*%d)",offset>>2,a_reg_num (index_registers->a_reg.r),
				d_reg_num (index_registers->d_reg.r),1<<(offset & 3));
		}
	}
}

void show_instructions (register struct instruction *instructions)
{
	while (instructions!=NULL){
		register struct parameter *parameter;
		register int n_parameters;
		
		printf ("%s ",i_instruction_names[instructions->instruction_icode]);
		
		n_parameters=instructions->instruction_arity;
		parameter=instructions->instruction_parameters;
		while (n_parameters>1){
			show_parameter (parameter);
			printf (",");
			++parameter;
			--n_parameters;
		}
		if (n_parameters==1)
			show_parameter (parameter);
		printf ("\n");
		
		instructions=instructions->instruction_next;
	}
}
#endif
