
#ifdef G_POWER
# define FMADD
#endif

#ifdef I486
# define FP_STACK_OPTIMIZATIONS
#endif

#define IBHS IBGEU

enum {
	IADD,		IAND,		IASR,		IBEQ,		IBGE,		IBGEU,		IBGT,
	IBGTU,		IBLE,		IBLEU,		IBLT,		IBLTU,		IBNE,		IBNO,
	IBO,		ICMP,		ICMPW,		IDIV,		IEOR,		IEXG,		IEXT,
	IFADD,		IFBEQ,		IFBGE,		IFBGT,		IFBLE,		IFBLT,		IFBNE,
	IFABS,		IFCMP,		IFCOS,		IFDIV,		IFMUL,		IFNEG,		IFREM,
	IFSEQ, 		IFSGE,		IFSGT,		IFSIN,		IFSLE,		IFSLT,		IFSNE,
	IFSUB, 		IFTAN,		IFTST,		IFMOVE,		IFMOVEL,	IJMP,		IJSR,
	ILEA,		ILSL,		ILSR,		IMOD,		IMOVE,		IMOVEB,		IMOVEW,
	IMUL,		INEG,		IOR,		IRTS,		ISCHEDULE,	ISEQ,		ISGE,
	ISGEU,		ISGT,		ISGTU,		ISLE,		ISLEU,		ISLT,		ISLTU,
	ISNE,		ISNO,		ISO,		ISUB,		ITST,		IWORD
#if !defined (G_POWER)
	,IFSQRT
#endif
#ifdef M68000
	,IFACOS,	IFASIN,		IFATAN,		IFEXP,		IFLN,		IFLOG10,
	IBMI,		IBMOVE,		IMOVEM,		ITSTB
#endif
#if defined (M68000) || defined (G_POWER)
	,IEXTB
#endif
#ifndef M68000
	,IBTST
#endif
#ifdef sparc
	,IFMOVEHI,	IFMOVELO
#endif
#ifdef G_POWER
	,IBNEP,IMTCTR
#endif
#if defined (G_POWER) || defined (sparc)
	,IADDI,	ILSLI
	,IADDO,	ISUBO
#endif
#ifdef G_POWER
	,ICMPLW
	,IMULO
#endif
#if defined (G_POWER) || defined (I486)
	,IJMPP	,IRTSP, INOT
#endif
#if defined (I486) && defined (FP_STACK_OPTIMIZATIONS)
	,IFEXG
#endif
#if defined (I486)
	,IRTSI, IDIVI, IREMI, IREMU
#endif
#if defined (I486) || defined (G_POWER)
	,IDIVU
#endif
#ifdef G_POWER
	,IUMULH
#endif
};

enum {
	P_REGISTER,			P_LABEL,			P_DESCRIPTOR_NUMBER,	P_INDIRECT,	
	P_IMMEDIATE,		P_F_IMMEDIATE,		P_F_REGISTER,			P_INDEXED
#if defined (M68000) || defined (I486)
	,P_POST_INCREMENT,	P_PRE_DECREMENT
#endif
#if defined (G_POWER)
	,P_INDIRECT_WITH_UPDATE,	P_INDIRECT_HP,	P_STORE_HP_INSTRUCTION
#endif
};
