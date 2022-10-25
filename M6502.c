/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                         M6502.c                         **/
/**                                                         **/
/** This file contains implementation for 6502 CPU. Don't   **/
/** forget to provide Rd6502(), Wr6502(), Loop6502(), and   **/
/** possibly Op6502() functions to accomodate the emulated  **/
/** machine's architecture.                                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**               Alex Krasivsky  1996                      **/
/** Modyfied      BERO            1998                      **/
/** Modyfied      hmmx            1998                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/   
/**     changes to this file.                               **/
/*************************************************************/


#include "stdinc.h"
#include "M6502.h"
//#include "debug.h"
//#include "pcedebug.h"
#ifdef HuC6280
#include "HuTables.h"
#define	BANK_SET(P,V)	bank_set(P,V)
extern void bank_set(byte P,byte V);
//Page[P]=ROMMap[V]-(P)*0x2000/*;printf("bank%x,%02x ",P,V)*/
extern byte *ROMMap[];
extern void  IO_write(word A,byte V);
#else
//#include "Tables.h"
#endif
//#include <stdio.h>


extern	M6502 M;


/** INLINE ***************************************************/
/** Different compilers inline C functions differently.     **/
/*************************************************************/

/*
#ifdef __GNUC__
#define INLINE static inline
#else
#ifdef _WIN32
#define INLINE static inline
#else
#define INLINE static
#endif
#endif
*/

/** System-Dependent Stuff ***********************************/
/** This is system-dependent code put here to speed things  **/
/** up. It has to stay inlined to be fast.                  **/
/*************************************************************/
#if 1 //def INES
#define FAST_RDOP
extern byte *Page[];
extern byte RAM[];
#if 1
//inline byte Op6502(register unsigned A) { return (Page[A>>13][A]); }
//inline unsigned Op6502w(register unsigned A) { return(Page[A>>13][A])|(Page[(A+1)>>13][A+1]<<8); }
//inline unsigned RdRAMw(register unsigned A) { return RAM[A]|(RAM[A+1]<<8); }
//inline byte Op6502(register  unsigned A) { return (Page[A>>13][A]); }
//inline unsigned Op6502w(register  unsigned A) { return(Page[A>>13][A])|(Page[(A+1)>>13][A+1]<<8); }
//inline unsigned RdRAMw(register  unsigned A) { return RAM[A]|(RAM[A+1]<<8); }

/*
INLINE void Wr6502(unsigned A,byte V)
{
	if (A<0x800) RAM[A]=V; else _Wr6502(A,V);
}

INLINE byte Rd6502(unsigned A)
{
	if (A<0x800 || A>=0x8000) return Page[A>>13][A]; else return _Rd6502(A);
}
*/
/*
#ifdef HuC6280
extern byte IOAREA[];
extern void  IO_write(word A,byte V);
extern byte  IO_read(word A);

#define _Rd6502(A) \
	((Page[(A)>>13]!=IOAREA) ? Page[(A)>>13][A] : IO_read(A))

#define _Wr6502(A,V) \
do { \
	word a=A; \
	byte v=V; \
	(Page[a>>13]!=IOAREA) ? Page[a>>13][a]=v : IO_write(a,v); \
} while (0)

/*
inline byte _Rd6502(word A)
{
	if (Page[A>>13]!=IOAREA) return Page[A>>13][A];
	else return IO_read(A);
}

inline void _Wr6502(word A,byte V)
{
	if (Page[A>>13]!=IOAREA) Page[A>>13][A]=V;
	else IO_write(A,V);
}
*/
/*#endif
*/
#define	RdRAM(A)	RAM[A]
#define	WrRAM(A,V)	RAM[A]=V
#define	AdrRAM(A)	&RAM[A]
#else
#define	Op6502(A)	Rd6502(A)
#define	Op6502w(A)	(Op6502(A)|(Op6502(A+1)<<8))
#define	RdRAM(A)	Rd6502(A)
#define	WrRAM(A,V)	Wr6502(A,V)
#endif

#endif

/** FAST_RDOP ************************************************/
/** With this #define not present, Rd6502() should perform  **/
/** the functions of Rd6502().                              **/
/*************************************************************/
#ifndef FAST_RDOP
#define Op6502(A) Rd6502(A)
#endif

#define	C_SFT	0
#define	Z_SFT	1
#define	I_SFT	2
#define	D_SFT	3
#define	B_SFT	4
#define	R_SFT	5
#define	V_SFT	6
#define	N_SFT	7

#define	_A	M.A
#define	_P	M.P
#define	_X	M.X
#define	_Y	M.Y
#define	_S	M.S
#define	_PC	M.PC
#define	_PC_	M.PC.W
#define _ZF	M.ZF
#define _VF	M.VF
#define _NF	M.NF
#define	_IPeriod	M.IPeriod
#define	_ICount		M.ICount
#define	_IRequest	M.IRequest
#define	_IPeriod	M.IPeriod
#define	_IBackup	M.IBackup
#define	_TrapBadOps	M.TrapBadOps
#define	_Trace	M.Trace
#define	_Trap	M.Trap
#define	_AfterCLI	M.AfterCLI
#define	_User	M.User
#define _CycleCount	(*(int*)&_User)

#define	ZP	0
#define	SP	0x100

/** Addressing Methods ***************************************/
/** These macros calculate and return effective addresses.  **/
/*************************************************************/
#define MCZp()	(Op6502(_PC_++))+ZP
#define MCZx()	(byte)(Op6502(_PC_++)+_X)+ZP
#define MCZy()	(byte)(Op6502(_PC_++)+_Y)+ZP
//#define MCZx()	((Op6502(_PC_++)+_X)&0xff)
//#define MCZy()	((Op6502(_PC_++)+_Y)&0xff)
#define	MCIx()	(RdRAMw(MCZx()))
#define	MCIy()	(RdRAMw(MCZp())+_Y)
#define	MCAb()	(Op6502w(_PC_))
#define MCAx()	(Op6502w(_PC_)+_X)
#define MCAy()	(Op6502w(_PC_)+_Y)

#define MC_Ab(Rg)	M_LDWORD(Rg)
#define MC_Zp(Rg)	Rg.B.l=Op6502(_PC_++);Rg.B.h=ZP>>8
#define MC_Zx(Rg)	Rg.B.l=Op6502(_PC_++)+R->X;Rg.B.h=ZP>>8
#define MC_Zy(Rg)	Rg.B.l=Op6502(_PC_++)+R->Y;Rg.B.h=ZP>>8
#define MC_Ax(Rg)	M_LDWORD(Rg);Rg.W+=_X
#define MC_Ay(Rg)	M_LDWORD(Rg);Rg.W+=_Y
#if 1
#define MC_Ix(Rg)	K.W=MCZx(); \
			Rg.B.l=RdRAM(K.W);Rg.B.h=RdRAM(K.W+1)
#define MC_Iy(Rg)	K.W=MCZp(); \
			Rg.B.l=RdRAM(K.W);Rg.B.h=RdRAM(K.W+1); \
			Rg.W+=_Y
#else
#define	MC_Ix(Rg)	Rg.W=(RdRAMw(MCZx()))
#define	MC_Iy(Rg)	Rg.W=(RdRAMw(MCZp())+_Y)
#endif
/*
#define MC_Ix(Rg)	{byte *p=AdrRAM(MCZx()); \
			Rg.B.l=p[0];Rg.B.h=p[1]; }
#define MC_Iy(Rg)	{byte *p=AdrRAM(MCZp()); \
			Rg.B.l=p[0];Rg.B.h=p[1]; } \
			Rg.W+=_Y;
*/

/** Reading From Memory **************************************/
/** These macros calculate address and read from it.        **/
/*************************************************************/
#define MR_Ab(Rg)	MC_Ab(J);Rg=Rd6502(J.W)
//#define MR_Ab(Rg)	Rg=Rd6502(MCAb());_PC_+=2
#define MR_Im(Rg)	Rg=Op6502(_PC_++)
#define	MR_Zp(Rg)	Rg=RdRAM(MCZp())
#define MR_Zx(Rg)	Rg=RdRAM(MCZx())
#define MR_Zy(Rg)	Rg=RdRAM(MCZy())
#define	MR_Ax(Rg)	MC_Ax(J);Rg=Rd6502(J.W)
#define MR_Ay(Rg)	MC_Ay(J);Rg=Rd6502(J.W)
//#define MR_Ax(Rg)	Rg=Rd6502(MCAx());_PC_+=2;
//#define MR_Ay(Rg)	Rg=Rd6502(MCAy());_PC_+=2;
//#define MR_Ix(Rg)	Rg=Rd6502(MCIx())
//#define MR_Iy(Rg)	Rg=Rd6502(MCIy())
#define MR_Ix(Rg)	MC_Ix(J);Rg=Rd6502(J.W)
#define MR_Iy(Rg)	MC_Iy(J);Rg=Rd6502(J.W)

/** Writing To Memory ****************************************/
/** These macros calculate address and write to it.         **/
/*************************************************************/
//#define MW_Ab(Rg)	Wr6502(MCAb(),Rg);_PC_+=2
#define MW_Ab(Rg)	MC_Ab(J);Wr6502(J.W,Rg)
#define MW_Zp(Rg)	WrRAM(MCZp(),Rg)
#define MW_Zx(Rg)	WrRAM(MCZx(),Rg)
#define MW_Zy(Rg)	WrRAM(MCZy(),Rg)
#define MW_Ax(Rg)	MC_Ax(J);Wr6502(J.W,Rg)
#define MW_Ay(Rg)	MC_Ay(J);Wr6502(J.W,Rg)
//#define MW_Ax(Rg)	Wr6502(MCAx(),Rg);_PC_+=2
//#define MW_Ay(Rg)	Wr6502(MCAy(),Rg);_PC_+=2
//#define MW_Ix(Rg)	Wr6502(MCIx(),Rg)
//#define MW_Iy(Rg)	Wr6502(MCIy(),Rg)
#define MW_Ix(Rg)	MC_Ix(J);Wr6502(J.W,Rg)
#define MW_Iy(Rg)	MC_Iy(J);Wr6502(J.W,Rg)

/** Modifying Memory *****************************************/
/** These macros calculate address and modify it.           **/
/*************************************************************/
#define MM_Ab(Cmd)	MC_Ab(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Zp(Cmd)	J.W=MCZp();I=RdRAM(J.W);Cmd(I);WrRAM(J.W,I)
#define MM_Zx(Cmd)	J.W=MCZx();I=RdRAM(J.W);Cmd(I);WrRAM(J.W,I)
//#define MM_Zp(Cmd)	{unsigned A=MCZp();I=RdRAM(A);Cmd(I);WrRAM(A,I); }
//#define MM_Zx(Cmd)	{unsigned A=MCZx();I=RdRAM(A);Cmd(I);WrRAM(A,I); }
//#define MM_Zp(Cmd)	{byte *p=AdrRAM(MCZp());I=*p;Cmd(I);*p=I;}
//#define MM_Zx(Cmd)	{byte *p=AdrRAM(MCZx());I=*p;Cmd(I);*p=I;}
#define MM_Ax(Cmd)	MC_Ax(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)

/** Other Macros *********************************************/
/** Calculating flags, stack, jumps, arithmetics, etc.      **/
/*************************************************************/
#define M_FL(Rg)	_ZF=_NF=Rg
#define M_LDWORD(Rg)	Rg.B.l=Op6502(_PC_);Rg.B.h=Op6502(_PC_+1);_PC_+=2

#define M_PUSH(Rg)	WrRAM(SP+_S,Rg);_S--
#define M_POP(Rg)	_S++;Rg=RdRAM(SP+_S)
#define M_PUSH_P(Rg)	M_PUSH(((Rg)&~(N_FLAG|V_FLAG|Z_FLAG))|(_NF&N_FLAG)|(_VF&V_FLAG)|(_ZF? 0:Z_FLAG))
#define M_POP_P(Rg)		M_POP(Rg);_NF=_VF=Rg;_ZF=(Rg&Z_FLAG? 0:1)
#ifdef HuC6280
#define M_JR		_PC_+=(offset)Op6502(_PC_)+1;cycle+=2
#else
#define M_JR		_PC_+=(offset)Op6502(_PC_)+1;cycle++
#endif

#define M_ADC(Rg) \
  if(_P&D_FLAG) \
  { \
    K.B.l=(_A&0x0F)+(Rg&0x0F)+(_P&C_FLAG); \
    K.B.h=(_A>>4)+(Rg>>4);/*+(K.B.l>15? 1:0);*/ \
    if(K.B.l>9) { K.B.l+=6;K.B.h++; } \
    if(K.B.h>9) K.B.h+=6; \
    _A=(K.B.l&0x0F)|(K.B.h<<4); \
    _P=(_P&~C_FLAG)|(K.B.h>15? C_FLAG:0); \
	_ZF=_NF=_A; \
	cycle++; \
  } \
  else \
  { \
    K.W=_A+Rg+(_P&C_FLAG); \
    _P&=~C_FLAG; \
    _P|=(K.B.h? C_FLAG:0); \
    _VF=(~(_A^Rg)&(_A^K.B.l))>>1; \
	_ZF=_NF=K.B.l; \
    _A=K.B.l; \
  }

/* Warning! C_FLAG is inverted before SBC and after it */
#define M_SBC(Rg) \
  if(_P&D_FLAG) \
  { \
    K.B.l=(_A&0x0F)-(Rg&0x0F)-(~_P&C_FLAG); \
    if(K.B.l&0x10) K.B.l-=6; \
    K.B.h=(_A>>4)-(Rg>>4)-((K.B.l&0x10)==0x10); \
    if(K.B.h&0x10) K.B.h-=6; \
    _A=(K.B.l&0x0F)|(K.B.h<<4); \
    _P=(_P&~C_FLAG)|((K.B.h&0x10)? 0:C_FLAG); \
	_ZF=_NF=_A; \
	cycle++; \
  } \
  else \
  { \
    K.W=_A-Rg-(~_P&C_FLAG); \
    _P&=~C_FLAG; \
    _P|=(K.B.h? 0:C_FLAG); \
    _VF=((_A^Rg)&(_A^K.B.l))>>1; \
    _ZF=_NF=K.B.l; \
    _A=K.B.l; \
  }

#define M_CMP(Rg1,Rg2) \
  K.W=Rg1-Rg2; \
  _P&=~C_FLAG; \
  _P|=(K.B.h? 0:C_FLAG); \
  _ZF=_NF=K.B.l
#define M_BIT(Rg) \
  _NF=_VF=Rg;_ZF=Rg&_A

#define M_AND(Rg)	_A&=Rg;M_FL(_A)
#define M_ORA(Rg)	_A|=Rg;M_FL(_A)
#define M_EOR(Rg)	_A^=Rg;M_FL(_A)
#define M_INC(Rg)	Rg++;M_FL(Rg)
#define M_DEC(Rg)	Rg--;M_FL(Rg)

#define M_ASL(Rg)	_P&=~C_FLAG;_P|=Rg>>7;Rg<<=1;M_FL(Rg)
#define M_LSR(Rg)	_P&=~C_FLAG;_P|=Rg&C_FLAG;Rg>>=1;M_FL(Rg)
#define M_ROL(Rg)	K.B.l=(Rg<<1)|(_P&C_FLAG); \
			_P&=~C_FLAG;_P|=Rg>>7;Rg=K.B.l; \
			M_FL(Rg)
#define M_ROR(Rg)	K.B.l=(Rg>>1)|(_P<<7); \
			_P&=~C_FLAG;_P|=Rg&C_FLAG;Rg=K.B.l; \
			M_FL(Rg)

/** Reset6502() **********************************************/
/** This function can be used to reset the registers before **/
/** starting execution with Run6502(). It sets registers to **/
/** their initial values.                                   **/
/*************************************************************/
void Reset6502(M6502 *R)
{
#ifdef HuC6280
  R->MPR[7]=0x00; BANK_SET(7,0x00);
  R->MPR[6]=0x05; BANK_SET(6,0x05);
  R->MPR[5]=0x04; BANK_SET(5,0x04);
  R->MPR[4]=0x03; BANK_SET(4,0x03);
  R->MPR[3]=0x02; BANK_SET(3,0x02);
  R->MPR[2]=0x01; BANK_SET(2,0x01);
  R->MPR[1]=0xF8; BANK_SET(1,0xF8);
  R->MPR[0]=0xFF; BANK_SET(0,0xFF);
#endif
  _A=_X=_Y=0x00;
  _P=I_FLAG;
  _NF=_VF=0;
  _ZF=0xFF;
  _S=0xFF;
  _PC.B.l=Op6502(VEC_RESET);
  _PC.B.h=Op6502(VEC_RESET+1);   
  _ICount=_IPeriod;
  _IRequest=INT_NONE;
  _AfterCLI=0;
#ifdef HuC6280
  _CycleCount=0;
#endif
}


/** Int6502() ************************************************/
/** This function will generate interrupt of a given type.  **/
/** INT_NMI will cause a non-maskable interrupt. INT_IRQ    **/
/** will cause a normal interrupt, unless I_FLAG set in R.  **/
/*************************************************************/
	 byte I;
void Int6502()
{
   pair J;

  if((I==INT_NMI)||(/*(Type==INT_IRQ)&&*/!(_P&I_FLAG)))
  {
    _ICount-=7;
    M_PUSH(_PC.B.h);
    M_PUSH(_PC.B.l);
    M_PUSH_P(_P&~(B_FLAG|T_FLAG));
    _P&=~D_FLAG;
    if (I==INT_NMI){
		J.W=VEC_NMI;
    } else {
    _P|=I_FLAG;
    switch(I){
    case INT_IRQ:J.W=VEC_IRQ;break;
#ifdef HuC6280
    case INT_IRQ2:J.W=VEC_IRQ2;break;
    case INT_TIMER:J.W=VEC_TIMER;break;
#endif
    }
    }
    _PC.B.l=Op6502(J.W);
    _PC.B.h=Op6502(J.W+1);
  } else {
	  _IRequest|=I;
  }
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
	static int	cycle;
	static int CycleCountOld;
	 pair J,K;
	 byte I;
typedef void (*funcptr)(void);
#define MC_Id(Rg)	K.W=MCZp(); \
			Rg.B.l=RdRAM(K.W);Rg.B.h=RdRAM(K.W+1)
#define MR_Id(Rg)	MC_Id(J);Rg=Rd6502(J.W)
//#define	M_FL2(Rg)	_P&=~(Z_FLAG|N_FLAG|V_FLAG);_P|=(Rg&0xc0)|ZNTable[Rg]
#define	TSB(Rg)	\
	_NF=_VF=Rg; \
	Rg|=_A; _ZF=Rg;
#define	TRB(Rg) \
	_NF=_VF=Rg; \
	Rg&=~_A; _ZF=Rg;
#define T_INIT() \
	word src,dist,length; \
	src  = Op6502w(_PC_); _PC_+=2; \
	dist = Op6502w(_PC_); _PC_+=2; \
	length = Op6502w(_PC_); _PC_+=2; \
	cycle=length*6
#define M_ADCx(Rg) \
 { byte *Mx=AdrRAM(_X+ZP); \
  if(_P&D_FLAG) \
  { \
    K.B.l=(*Mx&0x0F)+(Rg&0x0F)+(_P&C_FLAG); \
    K.B.h=(*Mx>>4)+(Rg>>4);/*+(K.B.l>>4);*/ \
    if(K.B.l>9) { K.B.l+=6;K.B.h++; } \
    if(K.B.h>9) K.B.h+=6; \
    *Mx=(K.B.l&0x0F)|(K.B.h<<4); \
    _P=(_P&~C_FLAG)|(K.B.h>15? C_FLAG:0); \
	_ZF=_NF=*Mx; \
	cycle++; \
  } \
  else \
  { \
    K.W=*Mx+Rg+(_P&C_FLAG); \
    _P&=~C_FLAG; \
    _P|=(K.B.h? C_FLAG:0); \
	_VF=(~(*Mx^Rg)&(*Mx^K.B.l))>>1; \
	_ZF=_NF=K.B.l; \
    *Mx=K.B.l; \
  } \
 }

#define M_ANDx(Rg)	*AdrRAM(_X+ZP)&=Rg;M_FL(RdRAM(_X+ZP))
#define M_EORx(Rg)	*AdrRAM(_X+ZP)^=Rg;M_FL(RdRAM(_X+ZP))
#define M_ORAx(Rg)	*AdrRAM(_X+ZP)|=Rg;M_FL(RdRAM(_X+ZP))

void	tD4(void){}
void	t54(void){}
void t44(){
	 M_PUSH(_PC.B.h);M_PUSH(_PC.B.l); /* BSR * REL */
	_PC_+=(offset)Op6502(_PC_)+1;  /* BRA * REL */
}
void t80(){ _PC_+=(offset)Op6502(_PC_)+1; } /* BRA * REL */
/* JMP ($ssss,x) */
void t7C(){
	M_LDWORD(K);K.W+=_X;
	_PC.B.l = Op6502(K.W);
	_PC.B.h = Op6502(K.W+1);
  }

void tDA(){ M_PUSH(_X);}               /* PHX */
void t5A(){ M_PUSH(_Y);}               /* PHY */
void tFA(){ M_POP(_X);M_FL(_X);}       /* PLX */
void t7A(){ M_POP(_Y);M_FL(_Y);}       /* PLY */

void t62(){ _A=0; } /* CLA */
void t82(){ _X=0; } /* CLX */
void tC2(){ _Y=0; } /* CLY */

void t02(){ I=_X;_X=_Y;_Y=I;}  /* SXY */
void t22(){ I=_A;_A=_X;_X=I;}  /* SAX */
void t42(){ I=_A;_A=_Y;_Y=I;}  /* SAY */

void t3A(){ M_DEC(_A); } /* DEC A */
void t1A(){ M_INC(_A); } /* INC A */

void t72(){ MR_Id(I);M_ADC(I);}       /* ADC ($ss) INDIR */
void t32(){ MR_Id(I);M_AND(I);}       /* AND ($ss) INDIR */
void tD2(){ MR_Id(I);M_CMP(_A,I);}       /* CMP ($ss) INDIR */
void t52(){ MR_Id(I);M_EOR(I);}       /* EOR ($ss) INDIR */
void tB2(){ MR_Id(_A);M_FL(_A);}       /* LDA ($ss) INDIR */
void t12(){ MR_Id(I);M_ORA(I);}       /* ORA ($ss) INDIR */
void t92(){ MC_Id(J);Wr6502(J.W,_A);}  /* STA ($ss) INDIR */
void tF2(){ MR_Id(I);M_SBC(I);}      /* SBC ($ss) INDIR */

void t89(){ MR_Im(I);M_BIT(I);}       /* BIT #$ss IMM */
void t34(){ MR_Zx(I);M_BIT(I);}       /* BIT $ss,x ZP,x */
void t3C(){ MR_Ax(I);M_BIT(I);}       /* BIT $ssss,x ABS,x */

void t64(){ MW_Zp(0x00);}             /* STZ $ss ZP */
void t74(){ MW_Zx(0x00);}             /* STZ $ss,x ZP,x */
void t9C(){ MW_Ab(0x00);}             /* STZ $ssss ABS */
void t9E(){ MW_Ax(0x00);}             /* STZ $ssss,x ABS,x */

void tF4(){	 /* SET */
	I=Op6502(_PC_++);
	cycle+=Cycles[I]+3;
	switch(I){
	case 0x65: MR_Zp(I);M_ADCx(I);return; /* ADC $ss ZP */
	case 0x6D: MR_Ab(I);M_ADCx(I);return; /* ADC $ssss ABS */
	case 0x69: MR_Im(I);M_ADCx(I);return; /* ADC #$ss IMM */
	case 0x75: MR_Zx(I);M_ADCx(I);return; /* ADC $ss,x ZP,x */
	case 0x79: MR_Ay(I);M_ADCx(I);return; /* ADC $ssss,y ABS,y */
	case 0x7D: MR_Ax(I);M_ADCx(I);return; /* ADC $ssss,x ABS,x */
	case 0x61: MR_Ix(I);M_ADCx(I);return; /* ADC ($ss,x) INDEXINDIR */
	case 0x71: MR_Iy(I);M_ADCx(I);return; /* ADC ($ss),y INDIRINDEX */
	case 0x72: MR_Id(I);M_ADCx(I);return; /* ADC ($ss) INDIR */

	case 0x25: MR_Zp(I);M_ANDx(I);return; /* AND $ss ZP */
	case 0x2D: MR_Ab(I);M_ANDx(I);return; /* AND $ssss ABS */
	case 0x29: MR_Im(I);M_ANDx(I);return; /* AND #$ss IMM */
	case 0x35: MR_Zx(I);M_ANDx(I);return; /* AND $ss,x ZP,x */
	case 0x39: MR_Ay(I);M_ANDx(I);return; /* AND $ssss,y ABS,y */
	case 0x3D: MR_Ax(I);M_ANDx(I);return; /* AND $ssss,x ABS,x */
	case 0x21: MR_Ix(I);M_ANDx(I);return; /* AND ($ss,x) INDEXINDIR */
	case 0x31: MR_Iy(I);M_ANDx(I);return; /* AND ($ss),y INDIRINDEX */
	case 0x32: MR_Id(I);M_ANDx(I);return; /* AND ($ss) INDIR */

	case 0x45: MR_Zp(I);M_EORx(I);return; /* EOR $ss ZP */
	case 0x4D: MR_Ab(I);M_EORx(I);return; /* EOR $ssss ABS */
	case 0x49: MR_Im(I);M_EORx(I);return; /* EOR #$ss IMM */
	case 0x55: MR_Zx(I);M_EORx(I);return; /* EOR $ss,x ZP,x */
	case 0x59: MR_Ay(I);M_EORx(I);return; /* EOR $ssss,y ABS,y */
	case 0x5D: MR_Ax(I);M_EORx(I);return; /* EOR $ssss,x ABS,x */
	case 0x41: MR_Ix(I);M_EORx(I);return; /* EOR ($ss,x) INDEXINDIR */
	case 0x51: MR_Iy(I);M_EORx(I);return; /* EOR ($ss),y INDIRINDEX */
	case 0x52: MR_Id(I);M_EORx(I);return; /* EOR ($ss) INDIR */

	case 0x05: MR_Zp(I);M_ORAx(I);return; /* ORA $ss ZP */
	case 0x0D: MR_Ab(I);M_ORAx(I);return; /* ORA $ssss ABS */
	case 0x09: MR_Im(I);M_ORAx(I);return; /* ORA #$ss IMM */
	case 0x15: MR_Zx(I);M_ORAx(I);return; /* ORA $ss,x ZP,x */
	case 0x19: MR_Ay(I);M_ORAx(I);return; /* ORA $ssss,y ABS,y */
	case 0x1D: MR_Ax(I);M_ORAx(I);return; /* ORA $ssss,x ABS,x */
	case 0x01: MR_Ix(I);M_ORAx(I);return; /* ORA ($ss,x) INDEXINDIR */
	case 0x11: MR_Iy(I);M_ORAx(I);return; /* ORA ($ss),y INDIRINDEX */
	case 0x12: MR_Id(I);M_ORAx(I);return; /* ORA ($ss) INDIR */

	}
	cycle-=Cycles[I];
	_PC_--;
}

void t03(void){ IO_write(0,Op6502(_PC_++));} /* ST0 */
void t13(void){ IO_write(2,Op6502(_PC_++));} /* ST1 */
void t23(void){ IO_write(3,Op6502(_PC_++));} /* ST2 */

void t43(void){ /* TMAi */
	I=Op6502(_PC_++);
	{int i;
	  for(i=0;i<8;i++,I>>=1){
		if (I&1) break;
	  }
	  _A = M.MPR[i];
	}
	}

void t53(void){ /* TAMi */
	I=Op6502(_PC_++);
	{int i;
	 for(i=0;i<8;i++,I>>=1){
	 	if (I&1) {
	 		M.MPR[i]=_A;
	 		BANK_SET(i,_A);
	 	}
	 }
	}
	}

void tC3(void){ // TDD 
	{ T_INIT();
	do {
		Wr6502(dist--,Rd6502(src));
		src--;
	} while(--length);
	}
	}

void t73(void){ // TII 
	{ T_INIT();
	do {
		Wr6502(dist++,Rd6502(src));
		src++;
	} while(--length);
	}
	}

void tE3(void){ // TIA /
	{ T_INIT();
	do {
		Wr6502(dist  ,Rd6502(src));
		src++;
		if (!(--length)) break;
		Wr6502(dist+1,Rd6502(src));
		src++;
	} while(--length);
	}
	}

void tF3(void){ // TAI 
	{ T_INIT();
	do {
		Wr6502(dist++,Rd6502(src));
		if (!(--length)) break;
		Wr6502(dist++,Rd6502(src+1));
	} while(--length);
	}
	}

void tD3(void){ // TIN/ 
	{ T_INIT();
	do {
		Wr6502(dist,Rd6502(src));
		src++;
	} while(--length);
	}
	}

void t14(void){ MM_Zp(TRB);} /* TRB $ss ZP */
void t1C(void){ MM_Ab(TRB);} /* TRB $ssss ABS */

void t04(void){ MM_Zp(TSB);} /* TSB $ss ZP */
void t0C(void){ MM_Ab(TSB);} /* TSB $ssss ABS */

void t83(void){ /* TST #$ss,$ss IMM,ZP */
	I=Op6502(_PC_++); J.B.l=RdRAM(MCZp());
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	}
void tA3(void){ /* TST #$ss,$ss,x IMM,ZP,x */
	I=Op6502(_PC_++); J.B.l=RdRAM(MCZx());
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	}
void t93(void){ /* TST #$ss,$ssss IMM,ABS */
	I=Op6502(_PC_++); MR_Ab(J.B.l);
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	}
void tB3(void){ /* TST #$ss,$ssss,x IMM,ABS,x */
	I=Op6502(_PC_++);MR_Ax(J.B.l);
	_NF=_VF=J.B.l; _ZF=I&J.B.l;
	}

void t0F(void){ if (RdRAM(MCZp())&0x01) _PC_++; else { M_JR; } } /* BBRi */
void t1F(void){ if (RdRAM(MCZp())&0x02) _PC_++; else { M_JR; } }
void t2F(void){ if (RdRAM(MCZp())&0x04) _PC_++; else { M_JR; } }
void t3F(void){ if (RdRAM(MCZp())&0x08) _PC_++; else { M_JR; } }
void t4F(void){ if (RdRAM(MCZp())&0x10) _PC_++; else { M_JR; } }
void t5F(void){ if (RdRAM(MCZp())&0x20) _PC_++; else { M_JR; } }
void t6F(void){ if (RdRAM(MCZp())&0x40) _PC_++; else { M_JR; } }
void t7F(void){ if (RdRAM(MCZp())&0x80) _PC_++; else { M_JR; } }

void t8F(void){ if (RdRAM(MCZp())&0x01) { M_JR; } else _PC_++; } /* BBSi */
void t9F(void){ if (RdRAM(MCZp())&0x02) { M_JR; } else _PC_++; }
void tAF(void){ if (RdRAM(MCZp())&0x04) { M_JR; } else _PC_++; }
void tBF(void){ if (RdRAM(MCZp())&0x08) { M_JR; } else _PC_++; }
void tCF(void){ if (RdRAM(MCZp())&0x10) { M_JR; } else _PC_++; }
void tDF(void){ if (RdRAM(MCZp())&0x20) { M_JR; } else _PC_++; }
void tEF(void){ if (RdRAM(MCZp())&0x40) { M_JR; } else _PC_++; }
void tFF(void){ if (RdRAM(MCZp())&0x80) { M_JR; } else _PC_++; }

#define	M_RMB(n)	*AdrRAM(MCZp())&=~n
#define	M_SMB(n)	*AdrRAM(MCZp())|=n

void t07(void){ M_RMB(0x01); } /* RMBi */
void t17(void){ M_RMB(0x02); } /* RMBi */
void t27(void){ M_RMB(0x04); } /* RMBi */
void t37(void){ M_RMB(0x08); } /* RMBi */
void t47(void){ M_RMB(0x10); } /* RMBi */
void t57(void){ M_RMB(0x20); } /* RMBi */
void t67(void){ M_RMB(0x40); } /* RMBi */
void t77(void){ M_RMB(0x80); } /* RMBi */

void t87(void){ M_SMB(0x01); } /* SMBi */
void t97(void){ M_SMB(0x02); } /* SMBi */
void tA7(void){ M_SMB(0x04); } /* SMBi */
void tB7(void){ M_SMB(0x08); } /* SMBi */
void tC7(void){ M_SMB(0x10); } /* SMBi */
void tD7(void){ M_SMB(0x20); } /* SMBi */
void tE7(void){ M_SMB(0x40); } /* SMBi */
void tF7(void){ M_SMB(0x80); } /* SMBi */
///////////////////////////////////////////////////////////////////////////////
void t10(void){ if(_NF&N_FLAG) _PC_++; else { M_JR; } } /* BPL * REL */
void t30(void){ if(_NF&N_FLAG) { M_JR; } else _PC_++; } /* BMI * REL */
void tD0(void){ if(!_ZF)       _PC_++; else { M_JR; } } /* BNE * REL */
void tF0(void){ if(!_ZF)       { M_JR; } else _PC_++; } /* BEQ * REL */
void t90(void){ if(_P&C_FLAG)  _PC_++; else { M_JR; } } /* BCC * REL */
void tB0(void){ if(_P&C_FLAG)  { M_JR; } else _PC_++; } /* BCS * REL */
void t50(void){ if(_VF&V_FLAG) _PC_++; else { M_JR; } } /* BVC * REL */
void t70(void){ if(_VF&V_FLAG) { M_JR; } else _PC_++; } /* BVS * REL */

/* RTI */
void t40(void){
  I=_P;
  M_POP_P(_P);
  if((_IRequest!=INT_NONE)&&(I&I_FLAG)&&!(_P&I_FLAG))
  {
    _AfterCLI=1;
    _IBackup=_ICount;
    _ICount=0;
  }
  M_POP(_PC.B.l);M_POP(_PC.B.h);
  }

/* RTS */
void t60(void){
  M_POP(_PC.B.l);M_POP(_PC.B.h);_PC_++;}

/* JSR $ssss ABS */
void t20(void){
  K.B.l=Op6502(_PC_++);
  K.B.h=Op6502(_PC_);
  M_PUSH(_PC.B.h);
  M_PUSH(_PC.B.l);
  _PC=K;}

/* JMP $ssss ABS */
void t4C(void){ M_LDWORD(K);_PC=K;}

/* JMP ($ssss) ABDINDIR */
void t6C(void){
  M_LDWORD(K);
  _PC.B.l=Op6502(K.W++);
  _PC.B.h=Op6502(K.W);
  }

/* BRK */
void t00(void){
  _PC_++;
  M_PUSH(_PC.B.h);M_PUSH(_PC.B.l);
  M_PUSH_P(_P&~T_FLAG|B_FLAG);
  _P=(_P|I_FLAG)&~D_FLAG;
  _PC.B.l=Op6502(VEC_BRK);
  _PC.B.h=Op6502(VEC_BRK+1);
//  TRACE("BRK instruction\n");
  }

/* CLI */
void t58(void){
  if((_IRequest!=INT_NONE)&&(_P&I_FLAG))
  {
    _AfterCLI=1;
    _IBackup=_ICount;
    _ICount=0;
  }
  _P&=~I_FLAG;
  }

/* PLP */
void t28(void){
  M_POP_P(I);
  if((_IRequest!=INT_NONE)&&((I^_P)&~I&I_FLAG))
  {
    _AfterCLI=1;
    _IBackup=_ICount;
    _ICount=0;
  }
  _P=I;
  }

void t08(void){ M_PUSH_P(_P&~T_FLAG|B_FLAG);}               /* PHP */
void t18(void){ _P&=~C_FLAG;}              /* CLC */
void tB8(void){ _VF=0;}              /* CLV */
void tD8(void){ _P&=~D_FLAG;}              /* CLD */
void t38(void){ _P|=C_FLAG;}               /* SEC */
void tF8(void){ _P|=D_FLAG;}               /* SED */
void t78(void){ _P|=I_FLAG;}               /* SEI */
void t48(void){ M_PUSH(_A);}               /* PHA */
void t68(void){ M_POP(_A);M_FL(_A);}     /* PLA */
void t98(void){ _A=_Y;M_FL(_A);}       /* TYA */
void tA8(void){ _Y=_A;M_FL(_Y);}       /* TAY */
void tC8(void){ _Y++;M_FL(_Y);}          /* INY */
void t88(void){ _Y--;M_FL(_Y);}          /* DEY */
void t8A(void){ _A=_X;M_FL(_A);}       /* TXA */
void tAA(void){ _X=_A;M_FL(_X);}       /* TAX */
void tE8(void){ _X++;M_FL(_X);}          /* INX */
void tCA(void){ _X--;M_FL(_X);}          /* DEX */
void tEA(void){ }                            /* NOP */
void t9A(void){ _S=_X;}                  /* TXS */
void tBA(void){ _X=_S;M_FL(_X);}                  /* TSX */

void t24(void){ MR_Zp(I);M_BIT(I);}       /* BIT $ss ZP */
void t2C(void){ MR_Ab(I);M_BIT(I);}       /* BIT $ssss ABS */

void t05(void){ MR_Zp(I);M_ORA(I);}       /* ORA $ss ZP */
void t06(void){ MM_Zp(M_ASL);}            /* ASL $ss ZP */
void t25(void){ MR_Zp(I);M_AND(I);}       /* AND $ss ZP */
void t26(void){ MM_Zp(M_ROL);}            /* ROL $ss ZP */
void t45(void){ MR_Zp(I);M_EOR(I);}       /* EOR $ss ZP */
void t46(void){ MM_Zp(M_LSR);}            /* LSR $ss ZP */
void t65(void){ MR_Zp(I);M_ADC(I);}       /* ADC $ss ZP */
void t66(void){ MM_Zp(M_ROR);}            /* ROR $ss ZP */
void t84(void){ MW_Zp(_Y);}             /* STY $ss ZP */
void t85(void){ MW_Zp(_A);}             /* STA $ss ZP */
void t86(void){ MW_Zp(_X);}             /* STX $ss ZP */
void tA4(void){ MR_Zp(_Y);M_FL(_Y);}  /* LDY $ss ZP */
void tA5(void){ MR_Zp(_A);M_FL(_A);}  /* LDA $ss ZP */
void tA6(void){ MR_Zp(_X);M_FL(_X);}  /* LDX $ss ZP */
void tC4(void){ MR_Zp(I);M_CMP(_Y,I);}  /* CPY $ss ZP */
void tC5(void){ MR_Zp(I);M_CMP(_A,I);}  /* CMP $ss ZP */
void tC6(void){ MM_Zp(M_DEC);}            /* DEC $ss ZP */
void tE4(void){ MR_Zp(I);M_CMP(_X,I);}  /* CPX $ss ZP */
void tE5(void){ MR_Zp(I);M_SBC(I);}       /* SBC $ss ZP */
void tE6(void){ MM_Zp(M_INC);}            /* INC $ss ZP */

void t0D(void){ MR_Ab(I);M_ORA(I);}       /* ORA $ssss ABS */
void t0E(void){ MM_Ab(M_ASL);}            /* ASL $ssss ABS */
void t2D(void){ MR_Ab(I);M_AND(I);}       /* AND $ssss ABS */
void t2E(void){ MM_Ab(M_ROL);}            /* ROL $ssss ABS */
void t4D(void){ MR_Ab(I);M_EOR(I);}       /* EOR $ssss ABS */
void t4E(void){ MM_Ab(M_LSR);}            /* LSR $ssss ABS */
void t6D(void){ MR_Ab(I);M_ADC(I);}       /* ADC $ssss ABS */
void t6E(void){ MM_Ab(M_ROR);}            /* ROR $ssss ABS */
void t8C(void){ MW_Ab(_Y);}             /* STY $ssss ABS */
void t8D(void){ MW_Ab(_A);}             /* STA $ssss ABS */
void t8E(void){ MW_Ab(_X);}             /* STX $ssss ABS */
void tAC(void){ MR_Ab(_Y);M_FL(_Y);}  /* LDY $ssss ABS */
void tAD(void){ MR_Ab(_A);M_FL(_A);}  /* LDA $ssss ABS */
void tAE(void){ MR_Ab(_X);M_FL(_X);}  /* LDX $ssss ABS */
void tCC(void){ MR_Ab(I);M_CMP(_Y,I);}  /* CPY $ssss ABS */
void tCD(void){ MR_Ab(I);M_CMP(_A,I);}  /* CMP $ssss ABS */
void tCE(void){ MM_Ab(M_DEC);}            /* DEC $ssss ABS */
void tEC(void){ MR_Ab(I);M_CMP(_X,I);}  /* CPX $ssss ABS */
void tED(void){ MR_Ab(I);M_SBC(I);}       /* SBC $ssss ABS */
void tEE(void){ MM_Ab(M_INC);}            /* INC $ssss ABS */

void t09(void){ MR_Im(I);M_ORA(I);}       /* ORA #$ss IMM */
void t29(void){ MR_Im(I);M_AND(I);}       /* AND #$ss IMM */
void t49(void){ MR_Im(I);M_EOR(I);}       /* EOR #$ss IMM */
void t69(void){ MR_Im(I);M_ADC(I);}       /* ADC #$ss IMM */
void tA0(void){ MR_Im(_Y);M_FL(_Y);}  /* LDY #$ss IMM */
void tA2(void){ MR_Im(_X);M_FL(_X);}  /* LDX #$ss IMM */
void tA9(void){ MR_Im(_A);M_FL(_A);}  /* LDA #$ss IMM */
void tC0(void){ MR_Im(I);M_CMP(_Y,I);}  /* CPY #$ss IMM */
void tC9(void){ MR_Im(I);M_CMP(_A,I);}  /* CMP #$ss IMM */
void tE0(void){ MR_Im(I);M_CMP(_X,I);}  /* CPX #$ss IMM */
void tE9(void){ MR_Im(I);M_SBC(I);}       /* SBC #$ss IMM */

void t15(void){ MR_Zx(I);M_ORA(I);}       /* ORA $ss,x ZP,x */
void t16(void){ MM_Zx(M_ASL);}            /* ASL $ss,x ZP,x */
void t35(void){ MR_Zx(I);M_AND(I);}       /* AND $ss,x ZP,x */
void t36(void){ MM_Zx(M_ROL);}            /* ROL $ss,x ZP,x */
void t55(void){ MR_Zx(I);M_EOR(I);}       /* EOR $ss,x ZP,x */
void t56(void){ MM_Zx(M_LSR);}            /* LSR $ss,x ZP,x */
void t75(void){ MR_Zx(I);M_ADC(I);}       /* ADC $ss,x ZP,x */
void t76(void){ MM_Zx(M_ROR);}            /* ROR $ss,x ZP,x */
void t94(void){ MW_Zx(_Y);}             /* STY $ss,x ZP,x */
void t95(void){ MW_Zx(_A);}             /* STA $ss,x ZP,x */
void t96(void){ MW_Zy(_X);}             /* STX $ss,y ZP,y */
void tB4(void){ MR_Zx(_Y);M_FL(_Y);}  /* LDY $ss,x ZP,x */
void tB5(void){ MR_Zx(_A);M_FL(_A);}  /* LDA $ss,x ZP,x */
void tB6(void){ MR_Zy(_X);M_FL(_X);}  /* LDX $ss,y ZP,y */
void tD5(void){ MR_Zx(I);M_CMP(_A,I);}  /* CMP $ss,x ZP,x */
void tD6(void){ MM_Zx(M_DEC);}            /* DEC $ss,x ZP,x */
void tF5(void){ MR_Zx(I);M_SBC(I);}       /* SBC $ss,x ZP,x */
void tF6(void){ MM_Zx(M_INC);}            /* INC $ss,x ZP,x */

void t19(void){ MR_Ay(I);M_ORA(I);}       /* ORA $ssss,y ABS,y */
void t1D(void){ MR_Ax(I);M_ORA(I);}       /* ORA $ssss,x ABS,x */
void t1E(void){ MM_Ax(M_ASL);}            /* ASL $ssss,x ABS,x */
void t39(void){ MR_Ay(I);M_AND(I);}       /* AND $ssss,y ABS,y */
void t3D(void){ MR_Ax(I);M_AND(I);}       /* AND $ssss,x ABS,x */
void t3E(void){ MM_Ax(M_ROL);}            /* ROL $ssss,x ABS,x */
void t59(void){ MR_Ay(I);M_EOR(I);}       /* EOR $ssss,y ABS,y */
void t5D(void){ MR_Ax(I);M_EOR(I);}       /* EOR $ssss,x ABS,x */
void t5E(void){ MM_Ax(M_LSR);}            /* LSR $ssss,x ABS,x */
void t79(void){ MR_Ay(I);M_ADC(I);}       /* ADC $ssss,y ABS,y */
void t7D(void){ MR_Ax(I);M_ADC(I);}       /* ADC $ssss,x ABS,x */
void t7E(void){ MM_Ax(M_ROR);}            /* ROR $ssss,x ABS,x */
void t99(void){ MW_Ay(_A);}             /* STA $ssss,y ABS,y */
void t9D(void){ MW_Ax(_A);}             /* STA $ssss,x ABS,x */
void tB9(void){ MR_Ay(_A);M_FL(_A);}  /* LDA $ssss,y ABS,y */
void tBC(void){ MR_Ax(_Y);M_FL(_Y);}  /* LDY $ssss,x ABS,x */
void tBD(void){ MR_Ax(_A);M_FL(_A);}  /* LDA $ssss,x ABS,x */
void tBE(void){ MR_Ay(_X);M_FL(_X);}  /* LDX $ssss,y ABS,y */
void tD9(void){ MR_Ay(I);M_CMP(_A,I);}  /* CMP $ssss,y ABS,y */
void tDD(void){ MR_Ax(I);M_CMP(_A,I);}  /* CMP $ssss,x ABS,x */
void tDE(void){ MM_Ax(M_DEC);}            /* DEC $ssss,x ABS,x */
void tF9(void){ MR_Ay(I);M_SBC(I);}       /* SBC $ssss,y ABS,y */
void tFD(void){ MR_Ax(I);M_SBC(I);}       /* SBC $ssss,x ABS,x */
void tFE(void){ MM_Ax(M_INC);}            /* INC $ssss,x ABS,x */

void t01(void){ MR_Ix(I);M_ORA(I);}       /* ORA ($ss,x) INDEXINDIR */
void t11(void){ MR_Iy(I);M_ORA(I);}       /* ORA ($ss),y INDIRINDEX */
void t21(void){ MR_Ix(I);M_AND(I);}       /* AND ($ss,x) INDEXINDIR */
void t31(void){ MR_Iy(I);M_AND(I);}       /* AND ($ss),y INDIRINDEX */
void t41(void){ MR_Ix(I);M_EOR(I);}       /* EOR ($ss,x) INDEXINDIR */
void t51(void){ MR_Iy(I);M_EOR(I);}       /* EOR ($ss),y INDIRINDEX */
void t61(void){ MR_Ix(I);M_ADC(I);}       /* ADC ($ss,x) INDEXINDIR */
void t71(void){ MR_Iy(I);M_ADC(I);}       /* ADC ($ss),y INDIRINDEX */
void t81(void){ MW_Ix(_A);}             /* STA ($ss,x) INDEXINDIR */
void t91(void){ MW_Iy(_A);}             /* STA ($ss),y INDIRINDEX */
void tA1(void){ MR_Ix(_A);M_FL(_A);}  /* LDA ($ss,x) INDEXINDIR */
void tB1(void){ MR_Iy(_A);M_FL(_A);}  /* LDA ($ss),y INDIRINDEX */
void tC1(void){ MR_Ix(I);M_CMP(_A,I);}  /* CMP ($ss,x) INDEXINDIR */
void tD1(void){ MR_Iy(I);M_CMP(_A,I);}  /* CMP ($ss),y INDIRINDEX */
void tE1(void){ MR_Ix(I);M_SBC(I);}       /* SBC ($ss,x) INDEXINDIR */
void tF1(void){ MR_Iy(I);M_SBC(I);}       /* SBC ($ss),y INDIRINDEX */

void t0A(void){ M_ASL(_A);}             /* ASL a ACC */
void t2A(void){ M_ROL(_A);}             /* ROL a ACC */
void t4A(void){ M_LSR(_A);}             /* LSR a ACC */
void t6A(void){ M_ROR(_A);}             /* ROR a ACC */

///////////////////////////////////////////////////////////////////////////////
void	t0B(void){}
void	t1B(void){}
void	t2B(void){}
void	t33(void){}
void	t3B(void){}
void	t4B(void){}
void	t5B(void){}
void	t5C(void){}
void	t63(void){}
void	t6B(void){}
void	t7B(void){}
void	t8B(void){}
void	t9B(void){}
void	tAB(void){}
void	tBB(void){}
void	tCB(void){}
void	tDB(void){}
void	tDC(void){}
void	tE2(void){}
void	tEB(void){}
void	tFB(void){}
char	new_adpcm_play;
void	tFC(void)
{

#include "cdemu.h"



}
funcptr functbl[] = {
	t00,t01,t02,t03,t04,t05,t06,t07,t08,t09,t0A,t0B,t0C,t0D,t0E,t0F,
	t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t1A,t1B,t1C,t1D,t1E,t1F,
	t20,t21,t22,t23,t24,t25,t26,t27,t28,t29,t2A,t2B,t2C,t2D,t2E,t2F,
	t30,t31,t32,t33,t34,t35,t36,t37,t38,t39,t3A,t3B,t3C,t3D,t3E,t3F,
	t40,t41,t42,t43,t44,t45,t46,t47,t48,t49,t4A,t4B,t4C,t4D,t4E,t4F,
	t50,t51,t52,t53,t54,t55,t56,t57,t58,t59,t5A,t5B,t5C,t5D,t5E,t5F,
	t60,t61,t62,t63,t64,t65,t66,t67,t68,t69,t6A,t6B,t6C,t6D,t6E,t6F,
	t70,t71,t72,t73,t74,t75,t76,t77,t78,t79,t7A,t7B,t7C,t7D,t7E,t7F,
	t80,t81,t82,t83,t84,t85,t86,t87,t88,t89,t8A,t8B,t8C,t8D,t8E,t8F,
	t90,t91,t92,t93,t94,t95,t96,t97,t98,t99,t9A,t9B,t9C,t9D,t9E,t9F,
	tA0,tA1,tA2,tA3,tA4,tA5,tA6,tA7,tA8,tA9,tAA,tAB,tAC,tAD,tAE,tAF,
	tB0,tB1,tB2,tB3,tB4,tB5,tB6,tB7,tB8,tB9,tBA,tBB,tBC,tBD,tBE,tBF,
	tC0,tC1,tC2,tC3,tC4,tC5,tC6,tC7,tC8,tC9,tCA,tCB,tCC,tCD,tCE,tCF,
	tD0,tD1,tD2,tD3,tD4,tD5,tD6,tD7,tD8,tD9,tDA,tDB,tDC,tDD,tDE,tDF,
	tE0,tE1,tE2,tE3,tE4,tE5,tE6,tE7,tE8,tE9,tEA,tEB,tEC,tED,tEE,tEF,
	tF0,tF1,tF2,tF3,tF4,tF5,tF6,tF7,tF8,tF9,tFA,tFB,tFC,tFD,tFE,tFF,
};
///////////////////////////////////////////////////////////////////////////////

/** Run6502() ************************************************/
/** This function will run 6502 code until Loop6502() call  **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word	_Run6502(M6502 *R)
{
//	register pair J,K;
//	register byte I;
	extern int TimerPeriod;
	extern int exit_flag;

  for(;;){

    I=Op6502(_PC_++);
	cycle = Cycles[I];


	functbl[I]();

/*
    switch(I)
    {
		#include "Codes.h"
		#include "HuCodes.h"
    }
*/
	_ICount-=cycle;
	_CycleCount += cycle;

	if(_ICount<=0){
		if(_AfterCLI){
			if (_IRequest&INT_TIMER) {
				_IRequest&=~INT_TIMER;
				I=INT_TIMER;
			}else
			if (_IRequest&INT_IRQ) {
				_IRequest&=~INT_IRQ;
				I=INT_IRQ;
			}else 
			if (_IRequest&INT_IRQ2) {
				_IRequest&=~INT_IRQ2;
				I=INT_IRQ2;
			}
			_ICount = 0;
			if (_IRequest==0){
				_ICount=_IBackup;  /* Restore the ICount        */
				_AfterCLI=0;            /* Done with AfterCLI state  */
			}
		}else{
			I=Loop6502(R);            /* Call the periodic handler */
			_ICount+=_IPeriod;     /* Reset the cycle counter   */
			if(exit_flag){
				if(I) Int6502();              /* Interrupt if needed  */ 
				if ((DWORD)(_CycleCount-CycleCountOld) > (DWORD)TimerPeriod*2)
					CycleCountOld = _CycleCount;
				return 0;
			}
		}
		if(I) Int6502();              /* Interrupt if needed  */ 
		if ((DWORD)(_CycleCount-CycleCountOld) > (DWORD)TimerPeriod*2)
			CycleCountOld = _CycleCount;
	}else {
		if (_CycleCount-CycleCountOld >= TimerPeriod) {
			CycleCountOld += TimerPeriod;
			I=TimerInt(R);
			if(I) Int6502();
		}
	}

  }

  return(_PC_);
}











