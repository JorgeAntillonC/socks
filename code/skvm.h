#ifndef SKVM_INCLUDE
#define SKVM_INCLUDE

#include "P:\stdwea\gdefwea.h"
#include "P:\stdwea\memwea.h"
#include "P:\stdwea\iowea.h"
#include "P:\stdwea\dynbuffwea.h"
//         ac  rc bcc crname
#define SKVM_OPS \
X(nop,      0, 0, 0, "nop")\
X(push,     0, 1, 1, "push")\
X(dup,      1, 2, 0, "*>")\
X(drop,     1, 0, 0, "//")\
X(leap,     2, 3, 0, "'>")\
X(swap,     2, 2, 0, "><")\
X(rtf,      3, 3, 0, "@>")\
X(rtb,      3, 3, 0, "<@")\
X(mw1,      2, 0, 0, "@1")\
X(mw2,      2, 0, 0, "@2")\
X(mw4,      2, 0, 0, "@4")\
X(mw8,      2, 0, 0, "@8")\
X(mr1,      1, 1, 0, "!1")\
X(mr2,      1, 1, 0, "!2")\
X(mr4,      1, 1, 0, "!4")\
X(mr8,      1, 1, 0, "!8")\
X(add,      2, 1, 0, "+")\
X(sub,      2, 1, 0, "-")\
X(mlt,      2, 1, 0, "*")\
X(div,      2, 1, 0, "/")\
X(mod,      2, 1, 0, "%")\
X(lt,       2, 1, 0, "<")\
X(gt,       2, 1, 0, ">")\
X(eq,       2, 1, 0, "=")\
X(dmp,      1, 0, 0, "dmp")\
X(and,      2, 1, 0, "&")\
X(xor,      2, 1, 0, "^")\
X(ior,      2, 1, 0, "|")\
X(not,      1, 1, 0, "~")\
X(shl,      2, 1, 0, "<<")\
X(shr,      2, 1, 0, ">>")\
X(hcf,      0, 0, 0, "hcf")\
X(jpz,      1, 0, 1, "jpz")\
X(jmp,      0, 0, 1, "jmp")\
X(call,     1, 0, 0, "#")\
X(ecall,    1, 0, 1, "#")\
X(ret,      0, 0, 0, "ret")\
X(res,      0, 0, 1, "res")\
X(rel,      0, 0, 1, "rel")\
X(idx,      0, 1, 1, "idx")\
X(lea,      0, 1, 1, "lea")\
X(oogabuga, 0, 0, 0, "oogabuga")\

#define X(x, ...) skvm_op_##x,
typedef enum SKVM_OP SKVM_OP;
enum SKVM_OP { SKVM_OPS skvm_op_count };
#undef X

typedef struct SKVM_OPDESC SKVM_OPDESC;
struct SKVM_OPDESC {
	u4 argc;
	u4 retc;
	u4 bcargc;
	u1* name;
	u1* crname; // code representation name
};

#define X(x, _argc, _retc, _bcargc, _crname) [skvm_op_##x] = { .argc = _argc, .retc = _retc, .bcargc = _bcargc, .name = #x, .crname = _crname },
SKVM_OPDESC skvm_opdesc[skvm_op_count] = { SKVM_OPS };
#undef X
#undef SKVM_OPS

#define SKVM_SYMTYPES \
X(proc) \
X(smlay) \
X(str) \
X(inv) \

#define X(x) skvm_symtp_##x,
typedef enum SKVM_SYMTYPE SKVM_SYMTYPE;
enum SKVM_SYMTYPE { SKVM_SYMTYPES skvm_symtp_count };
#undef X

#define X(x) #x,
u1* skvm_symtp2str[] = { SKVM_SYMTYPES };
#undef X

typedef struct SKVM_SYM SKVM_SYM;
struct SKVM_SYM {
	u1* bc;
	u1* off;
	u1* bcend;
	u4  len;
	u4  type; // string, proc, array
};

typedef struct SKVM_PROG SKVM_PROG;
struct SKVM_PROG {
	DYNBUFF* bytecode;
	DYNBUFF* loadlibs;
	DYNBUFF* symbols;
};

#define SKVM_VMREGS \
X(pp)\
X(sp)\
X(rp)\
X(lvp)

#define X(x) skvm_vmreg_##x,
typedef enum SKVM_VMREG SKVM_VMREG;
enum SKVM_VMREG { SKVM_VMREGS skvm_vmreg_count };
#undef X

#define X(x) #x,
u1* skvm_vmreg2str[] = { SKVM_VMREGS };
#undef X

typedef struct SKVM SKVM;
struct SKVM {
	u1* ram;
	u1* ramtop;
	u1* rom;
	u1* romtop;
	
	union {
		struct {
#define X(x) u1* x;
			SKVM_VMREGS
#undef X
		};
		u1* ptrs[skvm_vmreg_count];
	};
	u4 state;
};
#undef SKVM_VMREGS

#define SKVM_STATES \
X(halt)\
X(running)\
X(waiting)\

#define X(x) skvm_stt_##x,
typedef enum SKVM_STT SKVM_STT;
enum SKVM_STT { SKVM_STATES skvm_stt_count };
#undef X

#define X(x) #x,
u1* skvm_stt2str[] = { SKVM_STATES };
#undef X
#undef SKVM_STATES

// debuger defs

typedef struct SKVM_DBCOMMAND SKVM_DBCOMMAND;
struct SKVM_DBCOMMAND {
	u8 type;
	u8 val;
};

#define SKVM_DBCOMMANDS \
X(exit)\
X(rst)\
X(run)\
X(runto)\
X(nxt)\
X(ovr)\
X(ret)\
X(goto)\
X(psh)\
X(pop)\
X(log)\
X(mem)\
X(brk)\
X(wtch)\
X(sym)\
X(cstck)\
X(help)\
X(back)\
X(eoc)

#define X(x) skvm_dbcmd_##x,
typedef enum SKVM_DBCOMMANDTYPE SKVM_DBCOMMANDTYPE;
enum SKVM_DBCOMMANDTYPE { SKVM_DBCOMMANDS skvm_dbcmd_count, skvm_dbcmd_inv, skvm_dbcmd_int };
#undef X

typedef struct SKVM_BDCOMMANDDESC SKVM_BDCOMMANDDESC;
struct SKVM_BDCOMMANDDESC {
	u1* name;
	u4 namelen;
};

#define X(x) {.name = #x, .namelen= sizeof(#x)-1 },
SKVM_BDCOMMANDDESC skvm_dbcmddesc[] = { SKVM_DBCOMMANDS };
#undef X

typedef struct SKVM_RECT SKVM_RECT;
struct SKVM_RECT { u4 x; u4 y; u4 w; u4 h; };

typedef enum SKVM_ADDRTETYPE SKVM_ADDRTETYPE;
enum SKVM_ADDRTETYPE {
	skvm_addrtetp_empty,
	skvm_addrtetp_occupied
};

typedef enum SKVM_ADDRTETAG SKVM_ADDRTETAG;
enum SKVM_ADDRTETAG {
	skvm_addrtetag_notag = 0x00,
	skvm_addrtetag_breakpoint = 0x01,
	skvm_addrtetag_temporal = 0x02,
};

typedef struct SKVM_ADDRTE SKVM_ADDRTE;
struct SKVM_ADDRTE {
	u8 addr;
	u4 type;
	u4 tag;
};

typedef struct SKVM_DEBUGGER SKVM_DEBUGGER;
struct SKVM_DEBUGGER {
	u4 screenw;
	u4 screenh;
	u4 currentlog;
	u4 workingmemlog;
	u4 inbuffsize;
	u4 cmdlen;

	u1* inbuff;
	DYNBUFF* symbols;
	DYNBUFF* addrtable;

	DYNBUFF* memlogs;
	SKVM_SYM* currentproc;

	SKVM_RECT bytecoderect;
	SKVM_RECT regsrect;
	SKVM_RECT logrect;
	SKVM_RECT commandrect;
	SKVM_RECT stackrect;
	SKVM_RECT logtabsrect;
	SKVM_DBCOMMAND cmd;
};



u8 SKVM_CallNative(u8 func, u8* args, u8 argcsrets);

// TODO:: as for now the instructions take 8 byte each as well as the arguments
// i should reconsider variable lenght instructions, since the current model it's verwaste full
// the whole instruction set at the momment fits in a byte
inline static void SKVM_ExecuteOp(SKVM* vm);

void SKVM_Exe(SKVM_PROG* program);

SKVM_ADDRTE* SKVM_AddrTableIndex(DYNBUFF* addrtable, u1* ptr);

void SKVM_AddrTableInsert(DYNBUFF* addrtable, u1* ptr, u4 tag);

void SKVM_AddrTableRemove(DYNBUFF* addrtable, u1* ptr);

// TODO:: hash this up
void SKVM_NxtCommand(SKVM_DEBUGGER* debugger);

SKVM_SYM* SKVM_FindSymbolFromAddr(u1* ptr, DYNBUFF* symbols);

void SKVM_PrintInst(u8* ptr, u4 instnum);

void SKVM_PrintByteCodeRaw(u1* ptr, u8 bclen);

// TOCONSIDER:if it hits a str or a smlay it prints the whole thing, should i bother and make it so it only prints bclen many bytes?
// maybe idk
void SKVM_PrintByteCodeWithSymbols(u1* ptr, u8 bclen, DYNBUFF* symbols);

// TODO:: add better printing cababilities like PrintLnRect, etc so every rect is generic and could hold whatever information
void SKVM_ClearRect(SKVM_RECT* rect, u1* c);

void SKVM_PrintRect(SKVM_RECT* rect, u1* msg, ...);

inline static void SKVM_Getui(SKVM_DEBUGGER* debugger, u1* msg);

void SKVM_PrintByteCode(u1* ptr, u8 bclen, DYNBUFF* symbols, SKVM_RECT* rect);

void SKVM_PrintRegs(SKVM* vm, SKVM_RECT* rect);

void SKVM_PrintStack(SKVM* vm, SKVM_RECT* rect);

void SKVM_UpdateDebuggerInterFace(SKVM* vm, SKVM_DEBUGGER* debugger);

// TODO:: change this into a properlog
void SKVM_TextLog(SKVM_DEBUGGER* debugger, u1* msg, ...);

inline static u1 SKVM_ExecuteOpDebug(SKVM* vm, SKVM_DEBUGGER* debugger);

void SKVM_ExeDebug(SKVM_PROG* program);

#endif // SKVM_INCLUDE

#ifndef SKVM_DEF
#define SKVM_DEF

#define SKVM_MOVCUR(x, y) FS_WriteFmtb(STDOUT, "\x1b[%4u;%4uH", y + 1, x + 1)

u8 SKVM_CallNative(u8 func, u8* args, u8 argcsrets) {
	u8 argc = argcsrets & 0xFFFFFFFF;
	u8 hasret = argcsrets >> 32;
	switch (argc) {
	case 0: {
		if (hasret) return ((u8(*)(void))func)();
		((void (*)(void))func)();
	} return 0;
	case 1: {
		if (hasret) return ((u8(*)(u8))func)(args[0]);
		((void (*)(u8))func)(args[0]);
	} return 0;
	case 2: {
		if (hasret) return ((u8(*)(u8, u8))func)(args[1], args[0]);
		((void (*)(u8, u8))func)(args[1], args[0]);
	} return 0;
	case 3: {
		if (hasret) return ((u8(*)(u8, u8, u8))func)(args[2], args[1], args[0]);
		((void (*)(u8, u8, u8))func)(args[2], args[1], args[0]);
	} return 0;
	case 4: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8))func)(args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8))func)(args[3], args[2], args[1], args[0]);
	} return 0;
	case 5: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8))func)(args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8))func)(args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 6: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8))func)(args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8))func)(args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 7: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8))func)(args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8))func)(args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 8: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8))func)(args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
			((void (*)(u8, u8, u8, u8, u8, u8, u8, u8))func)(args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		} return 0;
	case 9: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 10: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 11: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 12: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 13: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[12], args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[12], args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 14: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[13], args[12], args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[13], args[12], args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	case 15: {
		if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[14], args[13], args[12], args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
		((void (*)(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8))func)(args[14], args[13], args[12], args[11], args[10], args[9], args[8], args[7], args[6], args[5], args[4], args[3], args[2], args[1], args[0]);
	} return 0;
	default: { FatalError(0, "too many arguments"); }return 0;
	}
}

// TODO:: as for now the instructions take 8 byte each as well as the arguments
// i should reconsider variable lenght instructions, since the current model it's verwaste full
// the whole instruction set at the momment fits in a byte
inline static void SKVM_ExecuteOp(SKVM* vm) {
#define Push(xx) do {*(u8*)(vm->sp) = (u8)(xx); vm->sp -= 8; } while(0)
#define Pop(xx) do { vm->sp += 8; (xx) = *(u8*)(vm->sp); } while(0)
	switch (*(u8*)vm->pp) {
		case skvm_op_nop: {} break;
		case skvm_op_oogabuga: {
			Print("\n\nsp:%u\n", (u8)(vm->ramtop - (vm->sp)) >> 3);
			for (u1* ptr = vm->ramtop; ptr >= vm->sp + 8; ptr -= 8) { Print("(%u)", *(u8*)ptr); }
			Print("\n\n");
		}break;
		case skvm_op_push: {
			vm->pp += 8;
			Push(*(u8*)vm->pp);
		} break;
		case skvm_op_idx: {
			vm->pp += 8;
			Push(vm->lvp + *(u8*)vm->pp);
		} break;
		case skvm_op_lea: {
			vm->pp += 8;
			Push(vm->rom + (*(u8*)vm->pp << 3));
		} break;
		case skvm_op_mw1: {
			u1* dest; Pop((u8)dest);
			u8 val; Pop(val);
			*dest = (u1)val;
		} break;
		case skvm_op_mw2: {
			u2* dest; Pop((u8)dest);
			u8 val; Pop(val);
			*dest = (u2)val;
		} break;
		case skvm_op_mw4: {
			u4* dest; Pop((u8)dest);
			u8 val; Pop(val);
			*dest = (u4)val;
		} break;
		case skvm_op_mw8: {
			u8* dest; Pop((u8)dest);
			u8 val; Pop(val);
			*dest = val;
		} break;
		case skvm_op_mr1: {
			u1* src; Pop((u8)src);
			Push(*src);
		} break;
		case skvm_op_mr2: {
			u2* src; Pop((u8)src);
			Push(*src);
		} break;
		case skvm_op_mr4: {
			u4* src; Pop((u8)src);
			Push(*src);
		} break;
		case skvm_op_mr8: {
			u8* src; Pop((u8)src);
			Push(*src);
		} break;
		case skvm_op_drop: {
			vm->sp += 8;
		} break;
		case skvm_op_dup: {
			u8 a;
			Pop(a);
			Push(a);
			Push(a);
		} break;
		case skvm_op_add: {
			u8 a, b;
			Pop(b);
			Pop(a);
			u8 r = a + b;
			Push(r);
		} break;
		case skvm_op_sub: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a - b);
		} break;
		case skvm_op_mlt: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a * b);
		} break;
		case skvm_op_div: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a / b);
		} break;
		case skvm_op_mod: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a % b);
		} break;
		case skvm_op_and: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a & b);
		} break;
		case skvm_op_xor: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a ^ b);
		} break;
		case skvm_op_ior: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a | b);
		} break;
		case skvm_op_not: {
			u8 a;
			Pop(a);
			Push(~a);
		} break;
		case skvm_op_shl: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a << b);
		} break;
		case skvm_op_shr: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a >> b);
		} break;
		case skvm_op_lt: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a < b);
		} break;
		case skvm_op_gt: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a > b);
		} break;
		case skvm_op_eq: {
			u8 a, b;
			Pop(b);
			Pop(a);
			Push(a == b);
		} break;
		case skvm_op_dmp: {
			u8 a;
			Pop(a);
			Print("%u\n", a);
		} break;
		case skvm_op_jpz: {
			u8 a;
			Pop(a);
			vm->pp += 8;
			if (a == 0) { vm->pp = vm->rom + (*(u8*)vm->pp << 3) - 8; }
		} break;
		case skvm_op_jmp: {
			vm->pp += 8;
			vm->pp = vm->rom + (*(u8*)vm->pp << 3) - 8;
		} break;
		case skvm_op_hcf: {
			Print("the program has halted!!!\n");
			vm->state = skvm_stt_halt;
		} break;
		case skvm_op_rtb: {
			u8 a, b, c;
			Pop(c);  Pop(b);  Pop(a);
			Push(b); Push(c); Push(a);
		} break;
		case skvm_op_rtf: {
			u8 a, b, c;
			Pop(c);  Pop(b);  Pop(a);
			Push(c); Push(a); Push(b);
		} break;
		case skvm_op_leap: {
			u8 a, b;
			Pop(b); Pop(a);
			Push(a); Push(b); Push(a);
		} break;
		case skvm_op_swap: {
			u8 a, b;
			Pop(b); Pop(a);
			Push(b); Push(a);
		} break;
		case skvm_op_call: {
			u8 procaddr;
			Pop(procaddr);
			*(u8*)(vm->rp) = (u8)vm->pp;
			*(u8*)(vm->rp + 8) = (u8)vm->lvp;
			vm->rp += 16;
			vm->lvp = vm->rp;
			vm->pp = vm->rom + (procaddr << 3) - 8;
		} break;
		case skvm_op_ecall: {
			u8 procaddr;
			Pop(procaddr);
			vm->pp += 8;
			u8 argsrets = *(u8*)vm->pp;
			u8 ret = SKVM_CallNative(procaddr, (u8*)vm->sp + 1, argsrets);
			vm->sp += (argsrets & 0xFFFFFFFF) << 3;
			if (argsrets >> 32) { Push(ret); }
		} break;
		case skvm_op_ret: {
			vm->lvp = (u1*)(*(u8*)(vm->rp - 8));
			vm->pp = (u1*)(*(u8*)(vm->rp - 16));
			vm->rp -= 16;
		} break;
		case skvm_op_res: {
			vm->pp += 8;
			u8 amount2resever = *(u8*)vm->pp;
			vm->rp += amount2resever;
		} break;
		case skvm_op_rel: {
			vm->pp += 8;
			u8 amount2release = *vm->pp;
			vm->rp -= amount2release;
		} break;
		default: {
			if (*(u8*)vm->pp < skvm_op_count) {
				FatalError(0, "SK_Exe::the opcode %s at 0x%x is not implement yet\n", skvm_opdesc[*(u8*)vm->pp].name, vm->pp);
			}
			else {
				FatalError(0, "SK_Exe:: 0x%x at 0x%x is not a valid opcode\n", *(u8*)vm->pp, vm->pp);
			}
		} break;
	}
#undef Push
#undef Pop
}

void SKVM_Exe(SKVM_PROG* program) {
	const u8 stackmemsizebytes = 512 * sizeof(u8);

	SKVM vm = { 0 };
	vm.ram = (u1*)Malloc(stackmemsizebytes);
	vm.rom = program->bytecode->data;
	vm.romtop = vm.rom + program->bytecode->occ;
	vm.pp = vm.rom;
	vm.rp = (u1*)vm.ram;
	vm.lvp = vm.rp;
	vm.ramtop = vm.ram + stackmemsizebytes - 8;
	vm.sp = vm.ramtop;

	vm.state = skvm_stt_running;
	while (vm.state) {
		SKVM_ExecuteOp(&vm);
		vm.pp += 8;
	}
	Free(vm.ram);
	DB_Free(program->loadlibs);
	DB_Free(program->bytecode);
	DB_Free(program->symbols);
}

SKVM_ADDRTE* SKVM_AddrTableIndex(DYNBUFF* addrtable, u1* ptr) {
	u4 addrtablesize = DB_Size(addrtable);
	u4 idx = (u8)ptr % addrtablesize;
	SKVM_ADDRTE* start = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
	SKVM_ADDRTE* addrte = start;
	while (addrte->type != skvm_addrtetp_empty) {
		if (addrte->addr == (u8)ptr) { return addrte; }
		idx = (idx + 1) % addrtablesize;
		addrte = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
		if (addrte == start) { return 0; }
	}
	return 0;
}

void SKVM_AddrTableInsert(DYNBUFF* addrtable, u1* ptr, u4 tag) {
	u4 addrtablesize = DB_Size(addrtable);
	u4 idx = (u8)ptr % addrtablesize;

	SKVM_ADDRTE* start = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
	SKVM_ADDRTE* addrte = start;
	while (addrte->type != skvm_addrtetp_empty) {
		if (addrte->addr == (u8)ptr) {
			addrte->tag = tag;
			return;
		}
		idx = (idx + 1) % addrtablesize;
		addrte = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
		if (addrte == start) {
			DYNBUFF* tmp = DB_Create(addrtablesize << 1, sizeof(SKVM_ADDRTE));
			SKVM_ADDRTE* entry = (SKVM_ADDRTE*)DB_Index(addrtable, 0);
			for (u4 i = 0; i < addrtablesize; i++) {
				if (entry->type != skvm_addrtetp_empty)
					SKVM_AddrTableInsert(tmp, (u1*)entry->addr, entry->tag);
				entry++;
			}
			Free(addrtable->data);
			memcpy(addrtable, tmp, sizeof(DYNBUFF));
			Free(tmp);
			addrtablesize <<= 1;
			idx = (u8)ptr % addrtablesize;
			start = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
			addrte = start;
		}
	}
	addrte->type = skvm_addrtetp_occupied;
	addrte->addr = (u8)ptr;
	addrte->tag = tag;
	return;
}

void SKVM_AddrTableRemove(DYNBUFF* addrtable, u1* ptr) {
	u4 addrtablesize = DB_Size(addrtable);
	u4 idx = (u8)ptr % addrtablesize;

	SKVM_ADDRTE* start = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
	SKVM_ADDRTE* addrte = start;
	while (addrte->type != skvm_addrtetp_empty) {
		if (addrte->addr == (u8)ptr) {
			u4 holeidx = idx;
			idx = (idx + 1) % addrtablesize;
			start = addrte;
			SKVM_ADDRTE* nxtaddrte = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
			while (nxtaddrte->type != skvm_addrtetp_empty) {
				u4 targetidx = nxtaddrte->addr % addrtablesize;
				u4 holeidxd = (holeidx - targetidx + addrtablesize) % addrtablesize;
				u4 currentidx = (idx - targetidx + addrtablesize) % addrtablesize;
				if (holeidxd < currentidx) {
					memcpy(addrte, nxtaddrte, sizeof(SKVM_ADDRTE));
					addrte = nxtaddrte;
					holeidx = idx;
				}
				idx = (idx + 1) % addrtablesize;
				nxtaddrte = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
			}
			addrte->type = skvm_addrtetp_empty;
			return;
		}
		idx = (idx + 1) % addrtablesize;
		addrte = (SKVM_ADDRTE*)DB_Index(addrtable, idx);
		if (addrte == start) { return; }
	}
}

// TODO:: hash this up
void SKVM_NxtCommand(SKVM_DEBUGGER* debugger) {
	if (debugger->cmdlen >= debugger->inbuffsize) {
		debugger->cmd.type = skvm_dbcmd_inv;
		return;
	}
	u1* start = debugger->inbuff + debugger->cmdlen;
	while (*start && CharIsSpace(*start)) { start++; }
	if (*start == '?' && *(start + 1) == '\0') {
		debugger->cmd.type = skvm_dbcmd_help;
		debugger->cmdlen += 1;
		return;
	}
	if (*start == 'o' && *(start + 1) == '\0') {
		debugger->cmd.type = skvm_dbcmd_ovr;
		debugger->cmdlen += 1;
		return;
	}
	if (*start == 'n' && *(start + 1) == '\0') {
		debugger->cmd.type = skvm_dbcmd_nxt;
		debugger->cmdlen += 1;
		return;
	}
	if (*start == 'r' && *(start + 1) == '\0') {
		debugger->cmd.type = skvm_dbcmd_ret;
		debugger->cmdlen += 1;
		return;
	}
	if (*start == 'm' && *(start + 1) == '\0') {
		debugger->cmd.type = skvm_dbcmd_run;
		debugger->cmdlen += 1;
		return;
	}
	if (CharIsNum(*start)) {
		debugger->cmd.type = skvm_dbcmd_int;
		u4 len = 0;
		debugger->cmd.val = CstrParseNum(start, &len);
		debugger->cmdlen += len;
		return;
	}
	if (CharIsAlp(*start)) {
		u1* end = start;
		while (*end && !CharIsSpace(*end)) { end++; }
		u4 len = end - start;
		for (u4 i = 0; i < skvm_dbcmd_count; i++) {
			if (len == skvm_dbcmddesc[i].namelen && CstrCmpS(skvm_dbcmddesc[i].name, start, len)) {
				debugger->cmd.type = i;
				debugger->cmdlen += len;
				return;
			}
		}
	}
	if (*start == '\0') {
		debugger->cmd.type = skvm_dbcmd_eoc;
		return;
	}
	debugger->cmd.type = skvm_dbcmd_inv;
	return;
}

SKVM_SYM* SKVM_FindSymbolFromAddr(u1* ptr, DYNBUFF* symbols) {
	SKVM_SYM* sym = DB_Index(symbols, 0);
	u4 symcount = DB_ElementCount(symbols);
	for (u4 i = 0; i < symcount; i++) {
		if (sym->bc <= ptr && ptr < sym->bcend) return sym;
		sym++;
	}
	return DB_Peek(symbols, 1);
}

void SKVM_PrintInst(u8* ptr, u4 instnum) {
	for (u4 i = 0; i < instnum; i++) {
		if (*ptr < skvm_op_count) {
			Print("0x%xr0:\x1B[34m%s\x1B[37m", ptr, (u8)16, skvm_opdesc[*ptr].name);
			if (skvm_opdesc[*ptr].bcargc) { Print(", 0x%xr0", *++ptr, (u8)16); }
		}
		else {
			Print("0x%xr0:0x%xr0", ptr, (u8)16, *ptr, (u8)16);
		}
		ptr++;
		Print("\n");
	}
}

void SKVM_PrintByteCodeRaw(u1* ptr, u8 bclen) {
	Print("\n\n=================bytecode===============\n\n");
	Print("bclen:%u %u\n", bclen, bclen >> 3);
	u8* ops = (u8*)ptr;
	for (u8 i = 0; i < bclen; i += 8) {
		if (*ops < skvm_op_count) {
			Print("0x%xr0:\x1B[34m%s\x1B[37m", ops, (u8)16, skvm_opdesc[*ops].name);
			if (skvm_opdesc[*ops].bcargc) {
				Print(", 0x%xr0", *++ops, (u8)16);
				i += 8;
			}
		}
		else {
			Print("0x%xr0:0x%xr0", ops, (u8)16, *ops, (u8)16);
		}
		ops++;
		Print("\n");
	}
	Print("\n\n=================bytecode===============\n\n");
}

// TOCONSIDER:if it hits a str or a smlay it prints the whole thing, should i bother and make it so it only prints bclen many bytes?
// maybe idk
void SKVM_PrintByteCodeWithSymbols(u1* ptr, u8 bclen, DYNBUFF* symbols) {
	Print("\n\n=================bytecode===============\n\n");
	Print("bclen:%u %u\n", bclen, bclen >> 3);

	DYNBUFF* readdressedsymbols = DB_Create(256, sizeof(SKVM_SYM));
	{
		DB_Extend(readdressedsymbols, symbols);
		u4 skvmsymcount = DB_ElementCount(readdressedsymbols);
		SKVM_SYM* sym = DB_Index(readdressedsymbols, 0);
		for (u8 i = 0; i < skvmsymcount; i++) {
			sym->bc = (u8)sym->bc + ptr;
			sym->bcend = (u8)sym->bcend + ptr;
			sym++;
		}
		DB_Push(readdressedsymbols, &(SKVM_SYM){
			.bc = 0, .bcend = 0,
			.len = sizeof(skvm_symtp2str[skvm_symtp_inv]), .off = skvm_symtp2str[skvm_symtp_inv],
			.type = skvm_symtp_inv
		});
	}


	u8* addr = (u8*)ptr;
	u8* targetaddr = (u8*)(ptr + bclen);
	do {
		SKVM_SYM* sym = SKVM_FindSymbolFromAddr((u1*)addr, readdressedsymbols);
		switch (sym->type) {
			case skvm_symtp_proc: {
				Print("\n\x1B[32m%$:\x1B[37m\n", sym->off, sym->len);
				while ((u1*)addr < sym->bcend) {
					if (*addr < skvm_op_count) {
						Print("0x%xr0:\x1B[34m%s\x1B[37m", addr, (u8)16, skvm_opdesc[*addr].name);
						if (skvm_opdesc[*addr].bcargc) {
							Print(", 0x%xr0", *++addr, (u8)16);
						}
					}
					else {
						Print("0x%xr0:0x%xr0", addr, (u8)16, *addr, (u8)16);
					}
					addr++;
					Print("\n");
				}
			} break;
			case skvm_symtp_str: {
				u1 buff[256];
				CstrFromRawBytes((u1*)addr, (u4)(sym->bcend - (u1*)addr), buff, 256);
				Print("\n\x1B[32mstr:\x1B[37m\n");
				Print("0x%xr0 %s\n", addr, (u8)16, buff);
				Print("0x%xr0\n", sym->bcend - 1, (u8)16);
				addr = (u8*)sym->bcend;
			} break;
			case skvm_symtp_smlay: {
				Print("\n\x1B[32m%$:\x1B[37m\n", sym->off, sym->len);
				u8 bytes2print = sym->bcend - (u1*)addr;
				for (u8 i = 0; i < bytes2print; i++) {
					if ((i & 31) == 0) {
						Print("\n0x%xr0:%xr0", (i + (u1*)addr), (u8)16, *(i + (u1*)addr), (u8)2);
					}
					else {
						Print(" %xr0", *(u1*)(i + (u1*)addr), (u8)2);
					}
				}
				Print("\n");
				addr = (u8*)sym->bcend;
			} break;
			case skvm_symtp_inv: {
				Print("\n\x1B[32munmap:\x1B[37m\n");
				u8 bytes2print = bclen - (u8)((u1*)addr - ptr);
				for (u8 i = 0; i < bytes2print; i++) {
					if ((i & 31) == 0) {
						Print("\n%xr0:%xr0", (i + addr), (u8)16, *(u1*)(i + addr), (u8)2);
					}
					else {
						Print(" %xr0", *(u1*)(i + addr), (u8)2);
					}
				}
				Print("\n");
				addr = targetaddr;
			} break;
			default: {
				if (sym->type < skvm_symtp_count) {
					FatalError(0, "SKVM_PrintByteCodeWithSymbols:: handling for { %s } is not implemented yet\n", skvm_symtp2str[sym->type]);
				}
				else {
					FatalError(0, "SKVM_PrintByteCodeWithSymbols::unknown symbol type\n");
				}
			} break;
		}
	} while (addr < targetaddr);
	Print("\n\n=================bytecode===============\n\n");
	DB_Free(readdressedsymbols);
}

#define SKVM_MOVCUR(x, y) FS_WriteFmtb(STDOUT, "\x1b[%4u;%4uH", y + 1, x + 1)

// TODO:: add better printing cababilities like PrintLnRect, etc so every rect is generic and could hold whatever information
void SKVM_ClearRect(SKVM_RECT* rect, u1* c) {
	for (u4 y = rect->y; y < rect->y + rect->h; y++) {
		SKVM_MOVCUR(rect->x, y);
		for (u4 x = rect->x; x < rect->x + rect->w; x++) {
			FS_Writeb(STDOUT, c, 1);
		}
	}
}

void SKVM_PrintRect(SKVM_RECT* rect, u1* msg, ...) {
	SKVM_ClearRect(rect, " ");
	u1 buff[1024];
	VAR_ARG(msg, stack);
	u4 bufflen = CstrFmtS(buff, msg, stack);

	u4 idx = 0;
	for (u4 y = rect->y; y < rect->y + rect->h && idx < bufflen; y++) {
		SKVM_MOVCUR(rect->x, y);
		for (u4 x = 0; x < rect->w && idx < bufflen;) {
			if (CharIsPrintable(buff[idx])) {
				FS_Writeb(STDOUT, &buff[idx++], 1);
			}
			else {
				while (idx < bufflen && !CharIsPrintable(buff[idx])) {
					switch (buff[idx]) {
					case '\n': {
						SKVM_MOVCUR(rect->x, ++y);
						x = -1;
					} break;
					case '\x1B': {
						switch (buff[++idx]) {
						case 'r': { FS_Writeb(STDOUT, "\x1b[31m", 5); } break;// change colors red
						case 'g': { FS_Writeb(STDOUT, "\x1b[32m", 5); } break;// green
						case 'y': { FS_Writeb(STDOUT, "\x1b[33m", 5); } break;// yellow
						case 'b': { FS_Writeb(STDOUT, "\x1b[34m", 5); } break;// blue
						case 'm': { FS_Writeb(STDOUT, "\x1b[35m", 5); } break;// magenta
						case 'c': { FS_Writeb(STDOUT, "\x1b[36m", 5); } break;// cyan
						case 'w': { FS_Writeb(STDOUT, "\x1b[37m", 5); } break;// white
						case '?': { FS_Writeb(STDOUT, "\x1b[?25h", 7); } break;//hide the cursor
						case '!': { FS_Writeb(STDOUT, "\x1B[?25l", 7); } break;//show the cursor
						default: {
							FS_WriteFmtb(STDOUT, "SKVM_PrintRect::{ i don't understand the sequence \\x1B%c}", buff[idx - 1]);
						} break;
						}
					} break;
					default: { FS_Writeb(STDOUT, &buff[idx], 1); } break;
					}
					idx++;
				}
			}
			x++;
		}
	}
}

inline static void SKVM_Getui(SKVM_DEBUGGER* debugger, u1* msg) {
	debugger->cmdlen = 0;
	SKVM_ClearRect(&debugger->commandrect, " ");
	SKVM_PrintRect(&debugger->commandrect, "%s\x1b?", msg);
	FS_Flush(STDOUT);
	u4 pos = 0, read = 0, c = 0;
	while (pos < debugger->inbuffsize - 1) {
		if (!ReadFile(STDIN->h, &c, 1, &read, NULL) || !read) break;
		if (c == '\r' || c == '\n') break;
		else if (c == '\b' && pos > 0) {
			--pos;
			Print("\b \b");
		}
		else if (c >= 32 && c < 127) {
			debugger->inbuff[pos++] = c;
			Print("%c", c);
		}
	}
	debugger->inbuff[pos] = '\0';
	FS_WriteFmtb(STDOUT, "\x1B[?25l"); // hide cursor
	SKVM_NxtCommand(debugger);
}

void SKVM_PrintByteCode(u1* ptr, u8 bclen, DYNBUFF* symbols, SKVM_RECT* rect) {
	SKVM_ClearRect(rect, " ");

	u8* addr = (u8*)ptr;
	u8* targetaddr = (u8*)(ptr + bclen);
	u4 linecount = rect->y;
#define NewLineBreak SKVM_MOVCUR(rect->x, linecount++); if (linecount > rect->y + rect->h) break;
	do {
		SKVM_SYM* sym = SKVM_FindSymbolFromAddr((u1*)addr, symbols);
		switch (sym->type) {
		case skvm_symtp_proc: {
			NewLineBreak;
			FS_WriteFmtb(STDOUT, "\x1B[32m%$:\x1B[37m", sym->off, sym->len);
			NewLineBreak;

			while ((u1*)addr < sym->bcend) {
				if (*addr < skvm_op_count) {
					FS_WriteFmtb(STDOUT, "  0x%xr0:\x1B[34m%s\x1B[37m", addr, (u8)16, skvm_opdesc[*addr].name);
					if (skvm_opdesc[*addr].bcargc) { FS_WriteFmtb(STDOUT, ", 0x%xr0", *++addr, (u8)16); }
				}
				else {
					FS_WriteFmtb(STDOUT, "  0x%xr0:0x%xr0", addr, (u8)16, *addr, (u8)16);
				}
				addr++;
				NewLineBreak;
			}
		} break;
		case skvm_symtp_str: {
			u1 buff[256];
			CstrFromRawBytes((u1*)addr, (u4)(sym->bcend - (u1*)addr), buff, 256);
			NewLineBreak;
			FS_WriteFmtb(STDOUT, "\x1B[32mstr:\x1B[37m");
			NewLineBreak;
			FS_WriteFmtb(STDOUT, "  0x%xr0 %s", addr, (u8)16, buff);
			NewLineBreak;
			FS_WriteFmtb(STDOUT, "  0x%xr0", sym->bcend - 1, (u8)16);
			addr = (u8*)sym->bcend;
		} break;
		case skvm_symtp_smlay: {
			NewLineBreak;
			FS_WriteFmtb(STDOUT, "\x1B[32m%$:\x1B[37m", sym->off, sym->len);
			u8 bytes2print = sym->bcend - (u1*)addr;
			for (u8 i = 0; i < bytes2print; i++) {
				if ((i & 31) == 0) {
					NewLineBreak;
					FS_WriteFmtb(STDOUT, "  0x%xr0:%xr0", (i + (u1*)addr), (u8)16, *(i + (u1*)addr), (u8)2);
				}
				else {
					FS_WriteFmtb(STDOUT, " %xr0", *(u1*)(i + (u1*)addr), (u8)2);
				}
			}
			NewLineBreak;
			addr = (u8*)sym->bcend;
		} break;
		case skvm_symtp_inv: {
			NewLineBreak;
			FS_WriteFmtb(STDOUT, "\x1B[32munmap:\x1B[37m");
			u8 bytes2print = bclen - (u8)((u1*)addr - ptr);
			NewLineBreak;
			for (u8 i = 0; i < bytes2print; i++) {
				if ((i & 31) == 0) {
					NewLineBreak;
					FS_WriteFmtb(STDOUT, "  %xr0:%xr0", (i + addr), (u8)16, *(u1*)(i + addr), (u8)2);
				}
				else {
					FS_WriteFmtb(STDOUT, " %xr0", *(u1*)(i + addr), (u8)2);
				}
			}
			NewLineBreak;
			addr = targetaddr;
		} break;
		default: {
			if (sym->type < skvm_symtp_count) {
				FatalError(0, "SKVM_PrintByteCodeWithSymbols:: handling for { %s } is not implemented yet\n", skvm_symtp2str[sym->type]);
			}
			else {
				FatalError(0, "SKVM_PrintByteCodeWithSymbols::unknown symbol type\n");
			}
		} break;
		}
	} while (addr < targetaddr && linecount < rect->h + rect->y);
	SKVM_MOVCUR(rect->x + 1, rect->y + 1);
	FS_WriteFmtb(STDOUT, "\x1B[32m>\x1B[37m");
#undef NewLineBreak
}

void SKVM_PrintRegs(SKVM* vm, SKVM_RECT* rect) {
	SKVM_ClearRect(rect, " ");

	SKVM_MOVCUR(rect->x, rect->y);
	FS_WriteFmtb(STDOUT, "state:\x1B[33m%s\x1B[37m", skvm_stt2str[vm->state]);
	for (u4 i = 0; i < skvm_vmreg_count; i++) {
		SKVM_MOVCUR(rect->x, rect->y + i + 1);
		if (i != skvm_vmreg_lvp) {
			FS_WriteFmtb(STDOUT, "\x1B[31m%s: \x1B[37m0x%xr0", skvm_vmreg2str[i], vm->ptrs[i], (u8)16);

		}
		else {
			FS_WriteFmtb(STDOUT, "\x1B[31m%s:\x1B[37m0x%xr0", skvm_vmreg2str[i], vm->ptrs[i], (u8)16);
		}
	}
}

void SKVM_PrintStack(SKVM* vm, SKVM_RECT* rect) {
	SKVM_ClearRect(rect, " ");
	SKVM_MOVCUR(rect->x, rect->y);
	u4 stacksize = (vm->ramtop - vm->sp) >> 3;
	FS_WriteFmtb(STDOUT, "stack:\x1B[31m%u\x1B[37m", stacksize);
	for (u4 i = 0; i < stacksize; i++) {
		SKVM_MOVCUR(rect->x, rect->y + rect->h - (i + 1));
		FS_WriteFmtb(STDOUT, "0x%xr0", *(u8*)(vm->ramtop - (i << 3)), (u8)16);
	}
}

void SKVM_UpdateDebuggerInterFace(SKVM* vm, SKVM_DEBUGGER* debugger) {
	SKVM_PrintByteCode(vm->pp, debugger->currentproc->bcend - vm->pp, debugger->symbols, &debugger->bytecoderect);
	SKVM_PrintRegs(vm, &debugger->regsrect);
	SKVM_PrintStack(vm, &debugger->stackrect);
	FS_Flush(STDOUT);
}

// TODO:: change this into a properlog
void SKVM_TextLog(SKVM_DEBUGGER* debugger, u1* msg, ...) {
#define buffsize 1024
	u1 buff[buffsize];
	s4 read = 0;
	VAR_ARG(msg, stack);
	CstrFmtS(buff + (buffsize >> 1), msg, stack);
	CstrFmt(buff, "%s\npress \x1byany key\x1bw to \x1bgcontinue\x1bw", buff + (buffsize >> 1));
	SKVM_PrintRect(&debugger->logrect, buff);
	FS_Flush(STDOUT);
	ReadFile(STDIN->h, &buff, 1, &read, NULL);
#undef buffsize
}

inline static u1 SKVM_ExecuteOpDebug(SKVM* vm, SKVM_DEBUGGER* debugger) {
	u1 hitbreak = 0;
	if (vm->state != skvm_stt_waiting) {
		SKVM_ADDRTE* addrte = SKVM_AddrTableIndex(debugger->addrtable, vm->pp);
		if (addrte) {
			vm->state = skvm_stt_waiting;
			hitbreak = 1;
			SKVM_TextLog(debugger, "\x1Brbreakpoint\x1Bw \x1Byhit!\x1Bw at \x1Bg0x%xr0\x1Bw", vm->pp, (u8)16);
			if (addrte->tag & skvm_addrtetag_temporal) {
				addrte->tag &= ~skvm_addrtetag_temporal;
				if (!addrte->tag) SKVM_AddrTableRemove(debugger->addrtable, vm->pp);
			}
		}
	}
	if (!hitbreak) {
		u8 stacksize = (u8)(vm->ramtop - (vm->sp)) >> 3;
		if (stacksize < skvm_opdesc[*(u8*)vm->pp].argc) {
			vm->state = skvm_stt_waiting;
			SKVM_TextLog(debugger, "\x1brError\x1bw:not enough arguments for \x1by%s\x1bw:, execution stop at 0x%x\n", skvm_opdesc[*(u8*)vm->pp].name, vm->pp);
			return 1;
		}
		s4 stackdelta = (s4)skvm_opdesc[*(u8*)vm->pp].retc - (s4)skvm_opdesc[*(u8*)vm->pp].argc;

		if (vm->sp - stackdelta > vm->ramtop) {
			vm->state = skvm_stt_waiting;
			SKVM_TextLog(debugger, "\x1brError\x1bw:stack-underflow detected at 0x%x\n", vm->pp);
			return 1;
		}
		if (vm->sp - stackdelta <= vm->rp) {
			vm->state = skvm_stt_waiting;
			SKVM_TextLog(debugger, "\x1brError\x1bw:stack-overrflow detected at 0x%x\n", vm->pp);
			return 1;
		}
		if (*(u8*)vm->pp == skvm_op_mw1 || *(u8*)vm->pp == skvm_op_mw2 || *(u8*)vm->pp == skvm_op_mw4 || *(u8*)vm->pp == skvm_op_mw8) {
			u1* addr = (u1*)(*(u8*)(vm->sp + 8));
			if (!PtrRangeIsValid(addr, 8)) {
				vm->state = skvm_stt_waiting;
				SKVM_TextLog(debugger, "\x1brError\x1bw:tying to write to an invalid address 0x%xr0 at 0x%x\n", addr, (u8)16, vm->pp);
				return 1;
			}
		}

		SKVM_ExecuteOp(vm);
		if (vm->state != skvm_stt_halt) vm->pp += 8;
		if (debugger->currentproc->bc > vm->pp || debugger->currentproc->bcend <= vm->pp) {
			SKVM_SYM* newworkingaddr = (SKVM_SYM*)DB_Index(debugger->symbols, 0);
			u4 symbolcount = DB_ElementCount(debugger->symbols);

			u1 invalidexecutionaddr = 1;
			for (u4 i = 0; i < symbolcount; i++) {
				if (newworkingaddr->type == skvm_symtp_proc && newworkingaddr->bc <= vm->pp && vm->pp < newworkingaddr->bcend) {
					invalidexecutionaddr = 0;
					debugger->currentproc = newworkingaddr;
					break;
				}
				newworkingaddr++;
			}
			if (invalidexecutionaddr) {
				vm->state = skvm_stt_waiting;
				debugger->currentproc = DB_Peek(debugger->symbols, 1);
				SKVM_TextLog(debugger, "\x1brError\x1bw:pp is pointing to an invalid address!");
				return 1;
			}
		}
		return 0;
	}
	return 0;
}

void SKVM_ExeDebug(SKVM_PROG* program) {
	const u8 stackmemsizebytes = 512 * sizeof(u8);

	SKVM vm = { 0 };
	vm.ram = (u1*)Malloc(stackmemsizebytes);
	vm.ramtop = vm.ram + stackmemsizebytes - 8;
	vm.rom = Malloc(program->bytecode->occ);
	memcpy(vm.rom, program->bytecode->data, program->bytecode->occ);
	vm.romtop = vm.rom + program->bytecode->occ;

	vm.pp = vm.rom;
	vm.rp = vm.ram;
	vm.lvp = vm.rp;
	vm.sp = vm.ramtop;
	vm.state = skvm_stt_waiting;

	s4 oldmode;
	GetConsoleMode(STDIN->h, &oldmode);
	SetConsoleMode(STDIN->h, oldmode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

	
	{
		u4 skvmsymcount = DB_ElementCount(program->symbols);
		SKVM_SYM* sym = DB_Index(program->symbols, 0);
		for (u8 i = 0; i < skvmsymcount; i++) {
			sym->bc = (u8)sym->bc + vm.rom;
			sym->bcend = (u8)sym->bcend + vm.rom;
			sym++;
		}
		DB_Push(program->symbols, &(SKVM_SYM){
			.bc = 0, .bcend = 0,
				.len = sizeof(skvm_symtp2str[skvm_symtp_inv]), .off = skvm_symtp2str[skvm_symtp_inv],
				.type = skvm_symtp_inv
		});
	}

	SKVM_DEBUGGER debugger = { 0 };

#define INBUFF_MAXCAP 512
	debugger.inbuffsize = INBUFF_MAXCAP;
	debugger.inbuff = Malloc(INBUFF_MAXCAP);
	debugger.symbols = program->symbols;
	debugger.currentproc = DB_Index(program->symbols, 0);
	debugger.addrtable = DB_Create(256, sizeof(SKVM_ADDRTE));
	debugger.memlogs = DB_Create(256, sizeof(u8));
	debugger.screenw = 120;
	debugger.screenh = 30;
	debugger.bytecoderect = (SKVM_RECT){ .x = 0, .y = 0, .w = 50, .h = 19 };
	debugger.regsrect = (SKVM_RECT){ .x = debugger.screenw - 30, .y = 0, .w = 30, .h = 5 };
	debugger.logrect = (SKVM_RECT){ .x = 0, .y = 20, .w = debugger.screenw, .h = 9 };
	debugger.commandrect = (SKVM_RECT){ .x = 0, .y = debugger.screenh - 1, .w = debugger.screenw, .h = 1 };
	debugger.stackrect = (SKVM_RECT){ .x = debugger.regsrect.x, .y = debugger.regsrect.y + debugger.regsrect.h , .w = debugger.regsrect.w, .h = debugger.logrect.y - (debugger.regsrect.y + debugger.regsrect.h) };
	debugger.logtabsrect = (SKVM_RECT){ .x = 0, .y = debugger.logrect.y - 1, .w = debugger.screenw - 30, .h = 1 };
	debugger.currentlog = skvm_dbcmd_log;
	debugger.workingmemlog = 0;
	debugger.cmd = (SKVM_DBCOMMAND){ 0 };

	SKVM_ClearRect(&(SKVM_RECT) { .x = 0, .y = 0, .w = debugger.screenw, .h = debugger.screenh }, " ");
	SKVM_UpdateDebuggerInterFace(&vm, &debugger);
	SKVM_PrintRect(&debugger.logtabsrect, "\x1Bylog\x1Bw|mem:%4u/%4u|brk|wtch|sym", debugger.workingmemlog, DB_ElementCount(debugger.memlogs));

	while (vm.state) {
		switch (vm.state) {
		case skvm_stt_waiting: {
		waiting_for_input:
			SKVM_Getui(&debugger, "(?/help)>");
			switch (debugger.cmd.type) {
			case skvm_dbcmd_exit: {
				vm.state = skvm_stt_halt;
			} break;
			case skvm_dbcmd_rst: {
				Free(vm.ram);
				vm.ram = (u1*)Malloc(stackmemsizebytes);

				vm.ramtop = vm.ram + stackmemsizebytes - 8;
				memcpy(vm.rom, program->bytecode->data, program->bytecode->occ);
				vm.romtop = vm.rom + program->bytecode->occ;

				vm.pp = vm.rom;
				vm.rp = vm.ram;
				vm.lvp = vm.rp;
				vm.sp = vm.ramtop;
				vm.state = skvm_stt_waiting;
				SKVM_UpdateDebuggerInterFace(&vm, &debugger);
				SKVM_TextLog(&debugger, "the \x1byvm\x1bw has \x1bgreset\x1bw!");
			} break;
			case skvm_dbcmd_run: {
				SKVM_ExecuteOpDebug(&vm, &debugger);
				vm.state = skvm_stt_running;
			} break;
			case skvm_dbcmd_nxt: {
				SKVM_ExecuteOpDebug(&vm, &debugger);
				SKVM_UpdateDebuggerInterFace(&vm, &debugger);
			} break;
			case skvm_dbcmd_goto: {
				SKVM_NxtCommand(&debugger);
				while (debugger.cmd.type != skvm_dbcmd_int) {
					SKVM_Getui(&debugger, "(type a valid \x1Bghex address\x1Bw or type \x1Byback\x1Bw)>");
					if (debugger.cmd.type == skvm_dbcmd_back) {
						goto waiting_for_input;
					}
				}
				if (!PtrRangeIsValid((u1*)debugger.cmd.val, 8)) {
					SKVM_TextLog(&debugger, "you don't have access to the address \x1br0x%xr0\x1bw", debugger.cmd.val, (u8)(16));
					goto waiting_for_input;
				}
				vm.pp = (u1*)debugger.cmd.val;
				SKVM_UpdateDebuggerInterFace(&vm, &debugger);
			} break;
			case skvm_dbcmd_ovr: {
				u1* destaddr = vm.pp + (8 << skvm_opdesc[*(u8*)(vm.pp)].bcargc);
				SKVM_ADDRTE* addrte = SKVM_AddrTableIndex(debugger.addrtable, destaddr);
				SKVM_AddrTableInsert(debugger.addrtable, destaddr, addrte ? addrte->tag | skvm_addrtetag_temporal : skvm_addrtetag_temporal);
				vm.state = skvm_stt_running;
			} break;
			case skvm_dbcmd_brk: {
				SKVM_NxtCommand(&debugger);
				while (debugger.cmd.type != skvm_dbcmd_int) {
					SKVM_Getui(&debugger, "(type a valid \x1Bghex address\x1Bw or type \x1byback\x1bw)>");
					if (debugger.cmd.type == skvm_dbcmd_back) {
						goto waiting_for_input;
					}
				}
				if (!PtrRangeIsValid((u1*)debugger.cmd.val, 8)) {
					SKVM_TextLog(&debugger, "you don't have access to the address \x1bg0%xr0\x1bw", debugger.cmd.val, (u8)(16));
					goto waiting_for_input;
				}

				SKVM_ADDRTE* addrte = SKVM_AddrTableIndex(debugger.addrtable, (u1*)debugger.cmd.val);
				if (addrte) {
					SKVM_AddrTableRemove(debugger.addrtable, (u1*)debugger.cmd.val);
					SKVM_TextLog(&debugger, "the breakpoint \x1bg0x%xr0\x1bw was \x1brremoved\x1bw", debugger.cmd.val, (u8)16);
				}
				else {
					SKVM_AddrTableInsert(debugger.addrtable, (u1*)debugger.cmd.val, skvm_addrtetag_breakpoint);
					SKVM_TextLog(&debugger, "the breakpoint \x1bg0x%xr0\x1bw was \x1bgadded\x1bw", debugger.cmd.val, (u8)16);
				}
			} break;
			case skvm_dbcmd_mem: {
				SKVM_NxtCommand(&debugger);
				while (debugger.cmd.type != skvm_dbcmd_int) {
					SKVM_Getui(&debugger, "(type a valid \x1Bghex address\x1Bw or type \x1byback\x1bw)>");
					if (debugger.cmd.type == skvm_dbcmd_back) {
						goto waiting_for_input;
					}
				}
				if (!PtrRangeIsValid((u1*)debugger.cmd.val, 8)) {
					SKVM_TextLog(&debugger, "you don't have access to the address \x1bg0%xr0\x1bw", debugger.cmd.val, (u8)(16));
					goto waiting_for_input;
				}
				u4 memlogcount = DB_ElementCount(debugger.memlogs);

				u1 activememlog = 0;
				if (memlogcount) {
					u8* memlogadrr = (u8*)DB_Index(debugger.memlogs, 0);
					for (u4 i = 0; i < memlogcount; i++) {
						if (*memlogadrr == debugger.cmd.val) {
							memcpy(memlogadrr, memlogadrr + 1, debugger.memlogs->occ - i * sizeof(u8));
							debugger.memlogs->occ -= sizeof(u8);
							activememlog = 1;
							SKVM_TextLog(&debugger, "the memlog to \x1bg0x%xr0\x1bw was \x1Brremoved\x1Bw", debugger.cmd.val, (u8)16);
							break;
						}
						memlogadrr++;
					}
				}
				if (!activememlog) {
					DB_Push(debugger.memlogs, &(u8){ debugger.cmd.val });
					SKVM_TextLog(&debugger, "a memlog to \x1bg0x%xr0\x1bw was added", debugger.cmd.val, (u8)16);
				}
			} break;
			case skvm_dbcmd_log: {
				SKVM_NxtCommand(&debugger);
				while (debugger.cmd.type < skvm_dbcmd_log || debugger.cmd.type > skvm_dbcmd_sym) {
					SKVM_PrintRect(&debugger.logrect,
						"type either (\x1bylog\x1bw, \x1bymem\x1bw, \x1bybrk\x1bw, \x1bywtch\x1bw, \x1bysym\x1bw) to show the associated log\n"
						"or \x1byback\x1bw");
					SKVM_Getui(&debugger, "()>");
					if (debugger.cmd.type == skvm_dbcmd_back) {
						goto waiting_for_input;
					}
				}
				switch (debugger.cmd.type) {
				case skvm_dbcmd_mem: {
					u4 activememlogs = DB_ElementCount(debugger.memlogs);
					if (activememlogs) {
						SKVM_NxtCommand(&debugger);
						while (debugger.cmd.type != skvm_dbcmd_int || (!debugger.cmd.val || debugger.cmd.val > activememlogs)) {
							SKVM_PrintRect(&debugger.logrect, "type a valid memlog index \x1Bg1 to %4u\x1Bw or type \x1Byback\x1Bw", activememlogs);
							SKVM_Getui(&debugger, "()>");
							if (debugger.cmd.type == skvm_dbcmd_back) {
								goto waiting_for_input;
							}
						}
					}
					else {
						SKVM_PrintRect(&debugger.logrect, "there's not active memlog, trying using the \x1Bymem\x1Bw command");
					}
				} break;
				default: {
					SKVM_TextLog(&debugger, "logging { %s } is not implemented yet", skvm_dbcmddesc[debugger.cmd.type].name);
				} break;
				}
			} break;
			case skvm_dbcmd_psh: {
				SKVM_NxtCommand(&debugger);
				while (debugger.cmd.type != skvm_dbcmd_int) {
					SKVM_Getui(&debugger, "(type a valid \x1Bghex value\x1Bw or type \x1Byback\x1Bw)>");
					if (debugger.cmd.type == skvm_dbcmd_back) {
						goto waiting_for_input;
					}
				}
				if (vm.sp - 8 < vm.rp) SKVM_TextLog(&debugger, "\x1brError\x1bw: stack overflow!");
				else {
					*(u8*)vm.sp = debugger.cmd.val;
					vm.sp -= 8;
					SKVM_UpdateDebuggerInterFace(&vm, &debugger);
				}
			} break;
			case skvm_dbcmd_pop: {
				if (vm.sp + 8 > vm.ramtop) SKVM_TextLog(&debugger, "\x1brError\x1bw: stack underflow!");
				else {
					vm.sp += 8;
					SKVM_UpdateDebuggerInterFace(&vm, &debugger);
				}
			} break;
			case skvm_dbcmd_help: {
				u1* helplogmsg[] = {
					"\x1b""cexit\x1bw  : Quit the debugger.\n"
					"\x1b""crst\x1bw   : Reset the VM to its initial state.\n"
					"\x1b""crun\x1bw   : Continue execution until a breakpoint or halt is encountered.\n"
					"\x1b""crunto\x1bw : Temporarily break at the specified address and run until it is hit.\n"
					"      | usage: \x1b""crunto\x1bw \x1bg<address>\x1bw\n"
					"\x1b""cnxt\x1bw   : Execute the next instruction, then return to waiting mode.\n"
					"\x1b""covr\x1bw   : Step over the next instruction; if it is a \x1bycall\x1bw, run until the matching \x1byret\x1bw returns.\n"
					"\x1b""cret\x1bw   : Continue execution until the current subroutine returns, then enter waiting mode.",

					"\x1b""cgoto\x1bw    : Move the program pointer to a specified address.\n"
					"        | usage: \x1b""cgoto\x1bw \x1bg<address>\x1bw\n"
					"\x1b""cpeek\x1bw    : Dump raw memory values from a specified address and range.\n"
					"        | usage: \x1b""cpeek\x1bw \x1bg<address>\x1bw \x1bg<range>\x1bw\n"
					"        |      | \x1b""cpeek\x1bw \x1bg<address>\x1bw \x1bg<address>\x1bw\n"
					"        |      | \x1b""cpeek\x1bw \x1bg<address>\x1bw\n"
					"\x1b""cpoke\x1bw    : Write raw hex bytes to a specified memory address.\n"
					"        | usage: \x1b""cpoke\x1bw \x1bg<address>\x1bw \x1bg<hex values>\x1bw",

					"\x1b""cbrk\x1bw     : Toggle a breakpoint at a specified address.\n"
					"        | usage: \x1b""cbrk\x1bw \x1bg<address>\x1bw\n"
					"        | If a breakpoint already exists at that address, it is removed.\n"
					"        | When a breakpoint is hit, the VM enters waiting mode.\n"
					"\x1b""cbrklst\x1bw  : List all active breakpoints.\n"
					"\x1b""cclrbrk\x1bw  : Remove all breakpoints.",

					"\x1b""cwatch\x1bw   : Toggle a watchpoint at a specified address.\n"
					"        | usage: \x1b""cwatch\x1bw \x1bg<address>\x1bw\n"
					"        | If a watchpoint already exists at that address, it is removed.\n"
					"        | When the value at a watchpoint address changes, the VM enters waiting mode.\n"
					"\x1b""cchlst\x1bw : List all active watchpoints.\n"
					"\x1b""crwtch\x1bw : Remove all watchpoints.\n",

					"\x1b""cdis\x1bw     : Disassemble instructions starting at a specified address.\n"
					"        | usage: \x1b""cdis\x1bw \x1bg<address>\x1bw \x1bg<range>\x1bw\n"
					"        |      | \x1b""cdis\x1bw \x1bg<address>\x1bw \x1bg<address>\x1bw\n"
					"        |      | \x1b""cdis\x1bw \x1bg<address>\x1bw"
					"\x1b""ccstck\x1bw   : Display the whole call stack.\n"
					"\x1b""csym\x1bw     : Display the symbol table (function names and addresses).\n"
					"\x1b""chelp\x1bw    : Display this list of available commands.\n"
					"\x1b""c?\x1bw       : Alias for \x1b""chelp\x1bw.",

					"\x1b""cn\x1bw       : Alias for \x1b""cnxt\x1bw.\n"
					"\x1b""co\x1bw       : Alias for \x1b""covr\x1bw.\n"
					"\x1b""cr\x1bw       : Alias for \x1b""cret\x1bw."
				};

				for (u4 i = 0; i < sizeof(helplogmsg) / sizeof(u1*); i++) {
					SKVM_PrintRect(&debugger.logrect, helplogmsg[i]);
					SKVM_Getui(&debugger, "(press \x1Bgenter\x1Bw to continue or type \x1Byback\x1Bw)>");
					if (debugger.cmd.type == skvm_dbcmd_back) { break; }
				}
			} break;
			default: {
				SKVM_PrintRect(&debugger.logrect, "\x1brError\x1bw::Unkown command { \x1by%s\x1bw }", debugger.inbuff);
			} break;
			}
		} break;
		case skvm_stt_running: {
			if (SKVM_ExecuteOpDebug(&vm, &debugger) || vm.state == skvm_stt_waiting) {
				SKVM_UpdateDebuggerInterFace(&vm, &debugger);
			}
		} break;
		default: {
			if (vm.state < skvm_stt_count) {
				FatalError(0, "vm state { %s } is not implemented yet", skvm_stt2str[vm.state]);
			}
			else {
				FatalError(0, "vm state { %1u } is not implemented yet", vm.state);
			}
		} break;
		}
	}

	SetConsoleMode(STDIN->h, oldmode);

	Free(vm.ram);
	Free(vm.rom);
	Free(debugger.inbuff);
	DB_Free(debugger.memlogs);
	DB_Free(debugger.addrtable);

	DB_Free(program->loadlibs);
	DB_Free(program->bytecode);
	DB_Free(program->symbols);
}
#undef SKVM_MOVCUR
#endif // SKVM_DEF