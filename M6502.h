/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                         M6502.h                         **/
/**                                                         **/
/** This file contains declarations relevant to emulation   **/
/** of 6502 CPU.                                            **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**               Alex Krasivsky  1996                      **/
/** Modyfied      BERO            1998                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/   
/**     changes to this file.                               **/
/*************************************************************/

#define HuC6280

#ifndef M6502_H
#define M6502_H

                               /* Compilation options:       */
/* #define FAST_RDOP */        /* Separate Op6502()/Rd6502() */
/* #define DEBUG */            /* Compile debugging version  */
/* #define LSB_FIRST */        /* Compile for low-endian CPU */

                               /* Loop6502() returns:        */
#define INT_NONE  0            /* No interrupt required      */
#define INT_IRQ	  1            /* Standard IRQ interrupt     */
#define INT_NMI	  2            /* Non-maskable interrupt     */
#define INT_QUIT  3            /* Exit the emulation         */

#define	INT_TIMER 4
#define	INT_IRQ2	8
#define	VEC_RESET	0xFFFE
#define	VEC_NMI		0xFFFC
#define	VEC_TIMER	0xFFFA
#define	VEC_IRQ		0xFFF8
#define	VEC_IRQ2	0xFFF6
#define	VEC_BRK		0xFFF6

                               /* 6502 status flags:         */
#define	C_FLAG	  0x01         /* 1: Carry occured           */
#define	Z_FLAG	  0x02         /* 1: Result is zero          */
#define	I_FLAG	  0x04         /* 1: Interrupts disabled     */
#define	D_FLAG	  0x08         /* 1: Decimal mode            */
#define	B_FLAG	  0x10         /* Break [0 on stk after int] */
#define	T_FLAG	  0x20         /* Memory operation flag      */
#define	V_FLAG	  0x40         /* 1: Overflow occured        */
#define	N_FLAG	  0x80         /* 1: Result is negative      */

/** Simple Datatypes *****************************************/
/** NOTICE: sizeof(byte)=1 and sizeof(word)=2               **/
/*************************************************************/
//typedef unsigned char byte;
//typedef unsigned short word;
//typedef signed char offset;

/** Structured Datatypes *************************************/
/** NOTICE: #define LSB_FIRST for machines where least      **/
/**         signifcant byte goes first.                     **/
/*************************************************************/
typedef union
{
//#ifdef LSB_FIRST
  struct { byte l,h; } B;
//#else
//  struct { byte h,l; } B;
//#endif
  word W;
} pair;

typedef struct
{
  pair PC;
  byte A,P,X,Y,S;     // CPU registers and program counter   //
  byte VF;            // VF,ZF,NF is flag cache              //
                      // parted because of pentium's paring optimization //
  int IPeriod,ICount; // Set IPeriod to number of CPU cycles //
                      // between calls to Loop6502()         //
  byte IRequest;      // Set to the INT_IRQ when pending IRQ //
  byte AfterCLI;      // Private, don't touch                //
  byte ZF;
  int IBackup;        // Private, don't touch                //
  byte NF;
  void *User;         // Arbitrary user data (ID,RAM*,etc.)  //
  byte TrapBadOps;    // Set to 1 to warn of illegal opcodes //
  word Trap;          // Set Trap to address to trace from   //
  byte Trace;         // Set Trace=1 to start tracing        //
  byte MPR[8];
} M6502;

/** Reset6502() **********************************************/
/** This function can be used to reset the registers before **/
/** starting execution with Run6502(). It sets registers to **/
/** their initial values.                                   **/
/*************************************************************/
void Reset6502(register M6502 *R);

/** Exec6502() ***********************************************/
/** This function will execute a single 6502 opcode. It     **/
/** will then return next PC, and current register values   **/
/** in R.                                                   **/
/*************************************************************/
word Exec6502(register M6502 *R);

/** Int6502() ************************************************/
/** This function will generate interrupt of a given type.  **/
/** INT_NMI will cause a non-maskable interrupt. INT_IRQ    **/
/** will cause a normal interrupt, unless I_FLAG set in R.  **/
/*************************************************************/
void Int6502();

/** Run6502() ************************************************/
/** This function will run 6502 code until Loop6502() call  **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word Run6502(register M6502 *R);
word _Run6502(register M6502 *R);

/** Rd6502()/Wr6502/Op6502() *********************************/
/** These functions are called when access to RAM occurs.   **/
/** They allow to control memory access. Op6502 is the same **/
/** as Rd6502, but used to read *opcodes* only, when many   **/
/** checks can be skipped to make it fast. It is only       **/
/** required if there is a #define FAST_RDOP.               **/
/************************************ TO BE WRITTEN BY USER **/
void _Wr6502(register word Addr,register byte Value);
byte _Rd6502(register word Addr);
//byte Op6502(register word Addr);
#define	Wr6502	_Wr6502
#define	Rd6502	_Rd6502

/** Debug6502() **********************************************/
/** This function should exist if DEBUG is #defined. When   **/
/** Trace!=0, it is called after each command executed by   **/
/** the CPU, and given the 6502 registers. Emulation exits  **/
/** if Debug6502() returns 0.                               **/
/*************************************************************/
byte Debug6502(register M6502 *R);

/** Loop6502() ***********************************************/
/** 6502 emulation calls this function periodically to      **/
/** check if the system hardware requires any interrupts.   **/
/** This function must return one of following values:      **/
/** INT_NONE, INT_IRQ, INT_NMI, or INT_QUIT to exit the     **/
/** emulation loop.                                         **/
/************************************ TO BE WRITTEN BY USER **/
byte Loop6502(register M6502 *R);


#ifdef HuC6280
byte TimerInt(M6502 *R);
#endif
extern byte *Page[];
extern byte RAM[0x8000];
static inline byte Op6502(register unsigned A) { return (Page[A>>13][A]); }
static inline unsigned Op6502w(register unsigned A) { return(Page[A>>13][A])|(Page[(A+1)>>13][A+1]<<8); }
static inline unsigned RdRAMw(register unsigned A) { return RAM[A]|(RAM[A+1]<<8); }



//#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

//inline int min(int x1, int x2){ return (x1<x2)? x1: x2; }

#endif /* M6502_H */
