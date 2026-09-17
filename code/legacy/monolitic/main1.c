#include "P:\stdwea\stdwea.h"
#include "P:\stdwea\dynbuffwea.h"

#define SOCKOPS \
X(nop)\
X(psh)\
X(dup)\
X(drop)\
X(leap)\
X(swap)\
X(rtf)\
X(rtb)\
X(add)\
X(sub)\
X(lt)\
X(dmp)\
X(and)\
X(hcf)\
X(jpz)\
X(jmp)\
X(call)\
X(ret)\
X(if)\
X(elif)\
X(else)\
X(while)\
X(colon)\
X(lpar)\
X(rpar)\
X(lbra)\
X(rbra)\
X(u1)\
X(u2)\
X(u4)\
X(u8)\
X(count)

#define X(x) skop_##x,
typedef enum SOCKOP SOCKOP;
enum SOCKOP { SOCKOPS };
#undef X
#define X(x) #x,
u1* skops2str[] = { SOCKOPS };
#undef X
#undef SOCKOPS

#define TKTYPES \
X(eof)\
X(num)\
X(sym)\
X(op)\
X(delim)\
X(flow)\
X(loopop)\
X(branchop)\
X(block)\
X(type)\
X(symdef)\
X(sign)\
X(ops)\

#define X(x) tktp_##x,
typedef enum TKTYPE TKTYPE;
enum TKTYPE { TKTYPES };
#undef X
#define X(x) #x,
u1* tktp2str[] = { TKTYPES };
#undef X

typedef struct TK TK;
struct TK {
	u4 col;
	u4 row;

	u4 type;
	
	u4 padd0; //
	union {
		u8  val; // literal int value, symbol index, random bs
		DYNBUFF* ops;
		DYNBUFF* argsrets;
	};
};


typedef enum SYMTYPE SYMTYPE;
enum SYMTYPE {
	symtp_undef,
	symtp_seen,
	symtp_proc,
	symtp_rsym, // reserved symbol
	symtp_dmem, // arrays
	symtp_smem, // static arrays
};


typedef struct SYMTABLEENTRY SYMTABLEENTRY;
struct SYMTABLEENTRY {
	u4 off;    // symtable's symbol name pool
	u4 size;   // size of symbol
	u4 type;   // proc mem etc
	u4 objid;  // proc index, mem index, etc index
};


typedef struct SYMTABLE SYMTABLE;
struct SYMTABLE {
	DYNBUFF* sympool;
	DYNBUFF* entries;
};


typedef struct SKPROC SKPROC;
struct SKPROC {
	u8       sym;
	union {
		struct { u4 padd0; u2 padd1; u1 padd2; u1 defined; };
		u8   icraddr;
	};
	DYNBUFF* ops;
	DYNBUFF* sign;
};

typedef struct COMPUNIT COMPUNIT;
struct COMPUNIT {
	DYNBUFF* procs;
	SYMTABLE symtable;
	u1* entrypoint;
	u8  epindx; // entrypoint index proc
	u1* sourcefile;
	u1* sourcecode;
	u1* scoff;// source code offset
	u8 errorcount;
	u8 tks;
	TK tk;
};

// TODO:: implement hashes and dictionaries, this is slow as hell but good enough for a basic implementation
// TODO:: change it so it uses a compile unit instead
u8 ST_Index(SYMTABLE* symtable, TK* tk, u1* sym, u4 size) {
	u8 i = 0;
	u8 symtableentrycount = DB_ElementCount(symtable->entries);
	while (i < symtableentrycount) {
		SYMTABLEENTRY* entry = (SYMTABLEENTRY*)DB_Index(symtable->entries, i); // returns null if there's not entry was found
		if (CstrCmpS(symtable->sympool->data + entry->off, sym, size)) { return i; }
		i++;
	}
	// not found so let's insert the new symbol
	SYMTABLEENTRY tmp = (SYMTABLEENTRY){ .type = symtp_undef, .off = DB_ElementCount(symtable->sympool), .size = size, .objid = i };
	DB_Push(symtable->entries, &tmp);
	DB_Append(symtable->sympool, sym, size);
	return i;
}

void CU_NxtTK(COMPUNIT* cu) {
	TK* tk = &cu->tk;
	SYMTABLE* symtable = &cu->symtable;
	tk->col += cu->tks;
	cu->scoff += cu->tks;
	do{
		while (*cu->scoff && CharIsSpace(*cu->scoff)) {
			cu->scoff++;
			tk->col++;
			if (CharIsNL(*cu->scoff)) {
				tk->col = 0;
				while (CharIsNL(*cu->scoff)) { if (*cu->scoff == '\n') { tk->row++; } cu->scoff++; }
			}
		}

		if (*cu->scoff == ';') {
			cu->scoff++;
			tk->col++;
			if (*cu->scoff == ';') {
				cu->scoff++;
				tk->col++;
				while (*cu->scoff && !(*cu->scoff == ';' && *(cu->scoff+1) == ';')) { 
					cu->scoff++;
					tk->col++;
					if (CharIsNL(*cu->scoff)) {
						tk->col = 0;
						while (CharIsNL(*cu->scoff)) { if (*cu->scoff == '\n') { tk->row++; } cu->scoff++; }
					}
				}
				if (*cu->scoff) {
					cu->scoff += 2;
					tk->col += 2;
				}
			}
			else {
				while (*cu->scoff && !CharIsNL(*cu->scoff)) { cu->scoff++; }
				tk->col = 0;
				while (*cu->scoff && CharIsNL(*cu->scoff)) { if (*cu->scoff == '\n') { tk->row++; } cu->scoff++; }
			}
		}
    } while (*cu->scoff && (CharIsSpace(*cu->scoff) || *cu->scoff == ';'));
	
	u1* base = cu->scoff;
	if (CharIsAlp(*cu->scoff)) {
		// TODO:: use a dictionary and hashes

		while (CharIsAlpNum(*base) || *base == '_') { base++; }
		cu->tks = (u8)base - (u8)cu->scoff;
		
		if (CstrCmpS("u1", cu->scoff, cu->tks)) {
			tk->type = tktp_type;
			tk->val = skop_u1;
			return;
		}
		if (CstrCmpS("u2", cu->scoff, cu->tks)) {
			tk->type = tktp_type;
			tk->val = skop_u2;
			return;
		}
		if (CstrCmpS("u4", cu->scoff, cu->tks)) {
			tk->type = tktp_type;
			tk->val = skop_u4;
			return;
		}
		if (CstrCmpS("u8", cu->scoff, cu->tks)) {
			tk->type = tktp_type;
			tk->val = skop_u8;
			return;
		}
		if (CstrCmpS("if", cu->scoff, cu->tks)) {
			tk->type = tktp_flow;
			tk->val  = skop_if;
			return;
		}
		if (CstrCmpS("elif", cu->scoff, cu->tks)) {
			tk->type = tktp_flow;
			tk->val = skop_elif;
			return;
		}
		if (CstrCmpS("else", cu->scoff, cu->tks)) {
			tk->type = tktp_flow;
			tk->val = skop_else;
			return;
		}
		if (CstrCmpS("while", cu->scoff, cu->tks)) {
			tk->type = tktp_flow;
			tk->val = skop_while;
			return;
		}
		tk->type = tktp_sym;
		tk->val = ST_Index(symtable, tk, cu->scoff, cu->tks);
		return;
	}
	if (CharIsNum(*cu->scoff)) {
		tk->type = tktp_num;
		u8 v = 0;
		
		while ((u1)(*base - '0') < 10) { v = v * 10 + *base++ - '0'; }
		tk->val = v;
		cu->tks = (u8)(base - cu->scoff);
		return;
	}
	
	if (CstrCmpS("*>", cu->scoff, 2)) {
		tk->type = tktp_op;
		tk->val = skop_dup;
		cu->tks = 2;
		return;
	}
	if (CstrCmpS("@>", cu->scoff, 2)) {
		tk->type = tktp_op;
		tk->val = skop_rtf;
		cu->tks = 2;
		return;
	}
	if (CstrCmpS("<@", cu->scoff, 2)) {
		tk->type = tktp_op;
		tk->val = skop_rtb;
		cu->tks = 2;
		return;
	}
	if (CstrCmpS("'>", cu->scoff, 2)) {
		tk->type = tktp_op;
		tk->val = skop_leap;
		cu->tks = 2;
		return;
	}
	if (CstrCmpS("><", cu->scoff, 2)) {
		tk->type = tktp_op;
		tk->val = skop_swap;
		cu->tks = 2;
		return;
	}
	if (CstrCmpS("//", cu->scoff, 2)) {
		tk->type = tktp_op;
		tk->val = skop_drop;
		cu->tks = 2;
		return;
	}
	
	switch (*cu->scoff) {
		case '(':  { cu->tks = 1; tk->type = tktp_delim; tk->val = skop_lpar;  } break;
		case ')':  { cu->tks = 1; tk->type = tktp_delim; tk->val = skop_rpar;  } break;
		case '{':  { cu->tks = 1; tk->type = tktp_delim; tk->val = skop_lbra;  } break;
		case '}':  { cu->tks = 1; tk->type = tktp_delim; tk->val = skop_rbra;  } break;
		case ':':  { cu->tks = 1; tk->type = tktp_delim; tk->val = skop_colon; } break;
		case '+':  { cu->tks = 1; tk->type = tktp_op;    tk->val = skop_add;   } break;
		case '&':  { cu->tks = 1; tk->type = tktp_op;    tk->val = skop_and;   } break;
		case '-':  { cu->tks = 1; tk->type = tktp_op;    tk->val = skop_sub;   } break;
		case '^':  { cu->tks = 1; tk->type = tktp_op;    tk->val = skop_dmp;   } break;
		case '<':  { cu->tks = 1; tk->type = tktp_op;    tk->val = skop_lt;    } break;
		case '#':  { cu->tks = 1; tk->type = tktp_op;    tk->val = skop_call;  } break;
		case '\0': { cu->tks = 0; tk->type = tktp_eof;   tk->val = skop_hcf;   } break;
		default: { FatalError(0, "TK_Nxt:: unhandled character { %c } at %s:%4u:%4u\n", *cu->scoff, cu->sourcefile, tk->row+1, tk->col+1); }
	}
	return;
}

inline static void TK_PrintTKInfo(TK* tk) {
	Print("(%s) at %u:%u\n", tktp2str[tk->type], (u8)tk->row + 1, (u8)tk->col + 1);
}

inline static u8 TK_2Str(TK* tk, u1* buff) {
	if (tk->type == tktp_num || tk->type == tktp_sym) {
		return CstrFmt(buff, "(%s, %u)", tktp2str[tk->type], tk->val);
	}
	else if (tk->val < skop_count){
		return CstrFmt(buff, "(%s, %s)", tktp2str[tk->type], skops2str[tk->val]);
	}
	else {
		return CstrFmt(buff, "(%s)", tktp2str[tk->type]);
	}
}

inline static void SK_ExeArgumentCheck(u8 pp, u8 sp, u8 arguments, u1* opcode) {
	if (sp > (255 - arguments)) {
		FatalError(0, "SK_Exe:: not enough arguments for %s, execution stop at 0x%x\n", opcode, pp);
	}
}

//TOCONSIDER:: maybe i should make the operations into functions and instead of a switch use an array
// i think it would be nicer long term idk

void SK_Exe(DYNBUFF* bytecode) {
	Print("\n\n===========execution=======\n\n");
	u8* stack = (u8*)Malloc(256 * sizeof(u8));
	u8* retaddr = (u8*)Malloc(256 * sizeof(u8));
	u8* mem = (u8*)bytecode->data;
	
	u8 pp = 0;
	u8 rp = 0;
	u8 sp = 255;
	u1 run = 1;
	while (run) {
		/*
			Print("0x%xr0:%s", pp, (u8)4, skops2str[mem[pp]]);
			if (mem[pp] == skop_psh || mem[pp] == skop_jpz || mem[pp] == skop_jmp) {
				Print(", 0x%xr0", mem[pp+1], 16);
			}
			Print("\n");
		*/
		switch (mem[pp]) {
			case skop_nop: { }break;
			case skop_psh: {
				if (sp == (u8)(-1)) FatalError(0, "SK_Exe:: stack overflow at 0x%x!!!!\n", pp);
				pp += 1;
				stack[sp--] = mem[pp];
			} break;
			case skop_dup: {
				SK_ExeArgumentCheck(pp, sp, 1, skops2str[mem[pp]]);
				u8 a = stack[++sp];
				stack[sp--] = a;
				stack[sp--] = a;
			} break;
			case skop_drop: {
				SK_ExeArgumentCheck(pp, sp, 1, skops2str[mem[pp]]);
				++sp;
			} break;
			case skop_add: {
				SK_ExeArgumentCheck(pp, sp, 2, skops2str[mem[pp]]);
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = a + b;
			} break;
			case skop_sub: {
				SK_ExeArgumentCheck(pp, sp, 2, skops2str[mem[pp]]);
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = a - b;
			} break;
			case skop_and: {
				SK_ExeArgumentCheck(pp, sp, 2, skops2str[mem[pp]]);
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = a & b;
			} break;
			case skop_lt: {
				SK_ExeArgumentCheck(pp, sp, 2, skops2str[mem[pp]]);
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = a < b;
			} break;
			case skop_dmp: {
				SK_ExeArgumentCheck(pp, sp, 1, skops2str[mem[pp]]);
				u8 a = stack[++sp];
				Print("%u\n", a);
			} break;
			case skop_jpz: {
				SK_ExeArgumentCheck(pp, sp, 1, skops2str[mem[pp]]);
				u8 a = stack[++sp];
				++pp;
				if (a == 0) { pp = mem[pp]-1; }
			} break;
			case skop_jmp: {
				pp = mem[++pp] - 1;
			} break;
			case skop_hcf: {
				Print("the program has halted!!!\n");
				run = 0;
			} break;
			case skop_rtb:{
				SK_ExeArgumentCheck(pp, sp, 3, skops2str[mem[pp]]);
				u8 c = stack[++sp];
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = b;
				stack[sp--] = c;
				stack[sp--] = a;
			} break;
			case skop_rtf: {
				SK_ExeArgumentCheck(pp, sp, 3, skops2str[mem[pp]]);
				u8 c = stack[++sp];
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = c;
				stack[sp--] = a;
				stack[sp--] = b;
			} break;
			case skop_leap: {
				SK_ExeArgumentCheck(pp, sp, 2, skops2str[mem[pp]]);
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = a;
				stack[sp--] = b;
				stack[sp--] = a;
			} break;
			case skop_swap: {
				SK_ExeArgumentCheck(pp, sp, 2, skops2str[mem[pp]]);
				u8 b = stack[++sp];
				u8 a = stack[++sp];
				stack[sp--] = b;
				stack[sp--] = a;
			} break;
			case skop_call:{
				SK_ExeArgumentCheck(pp, sp, 1, skops2str[mem[pp]]);
				u8 procaddr = stack[++sp];
				retaddr[rp++] = pp;
				pp = procaddr-1;
			} break;
			case skop_ret: {
				pp = retaddr[--rp];
			} break;
			default: {
				if (pp < skop_count) {
					FatalError(0, "SK_Exe::the opcode %s at 0x%x is not implement yet\n", skops2str[mem[pp]], pp);
				}
				else {
					FatalError(0, "SK_Exe:: 0x%x at 0x%x is not a valid opcode\n", mem[pp], pp);
				}
			} break;
		}
		pp++;
		/*
			Print("sp:%u\n", 255-(u8)sp);
			for (u8 i = 0; i < 255-(u8)sp; i++) {
				Print("(%u)", stack[255-i]);
			}
			Print("\n\n");
		*/
	}
	Print("\n\n===========================\n\n");
	Free(stack);
	Free(retaddr);
}

void SK_Error(COMPUNIT* cu, u1* errmsg, ...) {
	TK* tk = &cu->tk;
	u1 errbuff[1024];
	VAR_ARG(errmsg, stack);
	CstrFmtS(errbuff, errmsg, stack);
	Print("ERROR::%s at %s:%u:%u\n", errbuff, cu->sourcefile, tk->col+1, tk->row+1);
	cu->errorcount++;
}

/*

**: empty
, : 'or could be'
* : '0 or more of '
\x: literal x
(): enclosure for 'counters'
? : 1 or 0 of 
counter: *, ?

program:
	procs
procs:
	procs proc, proc, **
proc:
	symdcl \( args rets \) block
symdcl:
	sym :
args: 
	args type, type, **
rets:
    : type, rets type, **
type:
	u1, u2, u4, u8
block:
	{ ops }	
ops:
	ops op, op, **
op:
	stackop, binop, uniop, branchop, loopop
loopop: 
	while op block
branchop:
	if op  block *(elif ops block) ?(else block)
stackop:
	psh, drop, swp, dup, leap, rtf, rtb, ...(many)
binop:
	+,<,-, ...(many)
uniop:
	~,^, ...(many)
*/

// TODO::
// create an 'static' stack that checks the stack remains valanced to generate a valid program
// add while loops, static mem, dyn mem and funcs
// only then  start actual code gen aka outputing x86_64 insts
void SK_BuildBlocks(COMPUNIT* cu, u1* sourcefile, u1* entrypoint) {
	memset(cu, 0, sizeof(COMPUNIT));
	cu->entrypoint = entrypoint;
	cu->epindx = (u8)(-1);
	cu->sourcefile = sourcefile;
	cu->sourcecode = LoadFile(sourcefile);
	cu->scoff = cu->sourcecode;
	
	Print("\n\n===========source==========\n\n");
	Print(cu->sourcecode);
	Print("\n\n===========================\n\n");

	cu->symtable = (SYMTABLE){ .entries = DB_Create(256, sizeof(SYMTABLEENTRY)), .sympool = DB_Create(256, sizeof(u1)) };
	cu->procs = DB_Create(256, sizeof(SKPROC));
	cu->errorcount = 0;
	
	DYNBUFF* syntaxstack = DB_Create(256, sizeof(TK));

	TK* tk;
	u1 buff[256];
	
	do {
		CU_NxtTK(cu);
		tk = &cu->tk;
		
	skp:Print("next token:\n");
		TK_2Str(tk, buff);
		Print("%s\n", buff);

		switch (tk->type) {
			case tktp_delim: {
				switch (tk->val) {
					case skop_colon: {
						TK* toptk = (TK*)DB_Peek(syntaxstack, 1);
						if (toptk && toptk->type == tktp_sym) {
							SYMTABLEENTRY* symentry = DB_Index(cu->symtable.entries, toptk->val);
							if (symentry->type == symtp_undef) {
								symentry->type = symtp_seen;
								toptk->type = tktp_symdef;
							}
							else {
								SK_Error(cu, "redefinition of %$", cu->symtable.sympool->data + symentry->off, symentry->size);
							}
						}
						else if (toptk && (toptk->type == tktp_type || toptk->val == skop_lpar)){
							DB_Push(syntaxstack, tk);
						}
						else {
							SK_Error(cu, "misplaced \":\"");
						}
					} break;
					case skop_lpar: {
						DB_Push(syntaxstack, tk);
					} break;
					case skop_rpar: {
						TK* toptk = (TK*)DB_Peek(syntaxstack, 1);
						if (toptk) {
							TK* topanchor = toptk;
							while (toptk->val != skop_lpar && toptk != (TK*)syntaxstack->data) { toptk--; }
							if (toptk->val == skop_lpar) {
								u8 opcount = ((u8)topanchor - (u8)toptk) / sizeof(TK);
								toptk->type = tktp_sign;
								toptk->argsrets = DB_Create(opcount, sizeof(TK));
								toptk->argsrets->occ = (u8)topanchor - (u8)toptk;
								memcpy(toptk->argsrets->data, toptk + 1, (u8)topanchor - (u8)toptk);
								Print("signature:\n");
								for (u8 i = 0; i < DB_ElementCount(toptk->argsrets); ++i) {
									TK_2Str((TK*)DB_Index(toptk->argsrets, i), buff);
									Print("%s", buff);
								}
								syntaxstack->occ -= (u8)topanchor - (u8)toptk;
							}
							else {
								SK_Error(cu, "misplaced \")\"");
							}								
						}
						else {
							SK_Error(cu, "misplaced \")\"");
						}
					} break;
					case skop_lbra: {
						DB_Push(syntaxstack, tk);
					} break;
					case skop_rbra: {
						TK* toptk = (TK*)DB_Peek(syntaxstack, 1);
						if (toptk) {
							TK* topanchor = toptk;
							while (toptk->val != skop_lbra && toptk != (TK*)syntaxstack->data) { toptk--; }
							if (toptk->val == skop_lbra) {
								u8 opcount = ((u8)topanchor - (u8)toptk) / sizeof(TK);
								toptk->type = tktp_block;
								toptk->ops = DB_Create(opcount, sizeof(TK));
								toptk->ops->occ = (u8)topanchor - (u8)toptk;
								memcpy(toptk->ops->data, toptk + 1, (u8)topanchor - (u8)toptk);
								Print("ops:\n");
								for (u8 i = 0; i < DB_ElementCount(toptk->ops); ++i) { 
									TK_2Str((TK*)DB_Index(toptk->ops, i), buff);
									Print("%s", buff);
								}
								syntaxstack->occ -= (u8)topanchor - (u8)toptk;
								
								// checking if the block can get reduce
								TK* block = toptk;
								TK* node2 = DB_Peek(syntaxstack, 2);
								
								if (node2 && node2->type == tktp_sign) {
									TK* node3 = DB_Peek(syntaxstack, 3);
									if (node3 && node3->type == tktp_symdef) {
										DB_Push(cu->procs, &(SKPROC){.ops = block->ops, .sign = node2->argsrets, .sym = node3->val, .icraddr = (u8)(-1)});
										syntaxstack->occ -= 3 * sizeof(TK);
										
										SYMTABLEENTRY* symentry = DB_Index(cu->symtable.entries, node3->val);
										symentry->type = symtp_proc;
										symentry->objid = DB_ElementCount(cu->procs)-1;
										if (CstrCmpS(cu->entrypoint, cu->symtable.sympool->data + symentry->off, symentry->size)) {
											cu->epindx = symentry->objid;
										}
									}
									// TODO:: unnamed proc? let's not support this for now
									// it could be good if i need to create arrays of procs or pass it as an argument, etc
									else {
										SK_Error(cu, "unnamed procedure noticed ");
									}
								}
								else if (node2 && node2->type == tktp_ops) {
									TK* node3 = DB_Peek(syntaxstack, 3);
									if (node3 && node3->type == tktp_flow) {
										syntaxstack->occ -= 2 * sizeof(TK);
										switch (node3->val) {
											case skop_while:{
												DYNBUFF* loopbody = DB_Create(2, sizeof(TK));
												DB_Push(loopbody, node2);
												DB_Push(loopbody, block);
												node3->type = tktp_loopop;
												node3->ops = loopbody;
											} break;
											case skop_if: {
												DYNBUFF* branchbody = DB_Create(2, sizeof(TK));
												DB_Push(branchbody, node2);
												DB_Push(branchbody, block);
												node3->type = tktp_branchop;
												node3->ops = branchbody;
											} break;
											case skop_elif:{ // branchop elif ops block
												TK* branchop = DB_Peek(syntaxstack, 2);
												if (branchop->type == tktp_branchop) {
													DB_Push(branchop->ops, node2);
													DB_Push(branchop->ops, block);
													syntaxstack->occ -= 1 * sizeof(TK);
												}
												else {
													SK_Error(cu, "rogue \"elif\" missing \"if\" pair");
												}
											} break;
											default: {
												FatalError(0, "SK_BuildBlocks:: unreachable, block gen");
											} break;
										}
									}
									else { 
										//TODO:: this means i got at least ... ops block on the stack, let's not support it for now
										// it could be used for temporal objets in a separeted scope   
										SK_Error(cu, "rogue block not attached to an operation or definiton");
									}
								}
								else if (node2 && node2->type == tktp_flow && node2->val == skop_else) {
									TK* node3 = DB_Peek(syntaxstack, 3);
									if (node3 && node3->type == tktp_branchop) {
										// branchop else block
										syntaxstack->occ -= 2 * sizeof(TK);
										DB_Push(node3->ops, block);
									}
									else {
										//TODO:: this means i got at least ... ops block on the stack, let's not support it for now
										// it could be used for temporal objets in a separeted scope   
										SK_Error(cu, "rogue \"else\" missing \"if\" pair");
									}
								}
								else {
									//TODO:: this means i got at least ... ops block on the stack, let's not support it for now
									// it could be used for temporal objets in a separeted scope   
									SK_Error(cu, "rogue block not attached to an operation or definiton");
								}
							}
							else {
								SK_Error(cu, "misplaced \"}\"");
							}
						}
						else {
							SK_Error(cu, "misplaced \"}\"");
						}
					} break;
					default: {
						FatalError(0, "SK_BuildBlocks::unreachable { %s:\"%$\" }", tktp2str[tk->type], cu->scoff, (u8)cu->tks);
					} break;
				}
			} break;
			case tktp_sym:{
				DB_Push(syntaxstack, tk);
			} break;
			case tktp_num:
			case tktp_op: {
				TK* toptk = (TK*)DB_Peek(syntaxstack, 1);
				if (toptk) {
					switch (toptk->type) {
						case tktp_ops: {
							DB_Push(toptk->ops, tk);
						} break;
						case tktp_sym: {
							DYNBUFF* ops = DB_Create(256, sizeof(TK));
							DB_Push(ops, toptk); // this makes copies
							DB_Push(ops, tk);
							toptk->type = tktp_ops;
							toptk->ops = ops;
						} break;
						default: {
							DYNBUFF* ops = DB_Create(256, sizeof(TK));
							DB_Push(ops, tk);
							tk->type = tktp_ops;
							tk->ops = ops;
							DB_Push(syntaxstack, tk);
						} break;
					}
				}
				else {
					if (tk->type == tktp_num) {
						SK_Error(cu, "misplaced \"num:%u\"", tk->val);
					}
					else {
						SK_Error(cu, "misplaced \"op:%s\"", skops2str[tk->val]);
					}
				}
			} break;
			case tktp_type:{
				TK* toptk = (TK*)DB_Peek(syntaxstack, 1);
				if (toptk && (
						toptk->val  == skop_lpar ||
						toptk->type == tktp_type ||
						toptk->val  == skop_colon)
								) {
					DB_Push(syntaxstack, tk);
				}
				else {
					SK_Error(cu, "misplaced type \"%$\"", cu->scoff, (u8)cu->tks);
				}
			} break;
			case tktp_flow:{ 
				DB_Push(syntaxstack, tk);
			} break;
			case tktp_eof:{
				if (DB_ElementCount(syntaxstack) > 0) {
					SK_Error(cu, "premature eof; %u tokens left on stack", DB_ElementCount(syntaxstack));
				}
			} break;
			default: {
				FatalError(0, "SK_BuildBlocks:: unhandle symbol { %s:\"%$\" }", tktp2str[tk->type], cu->scoff, (u8)cu->tks);
			} break;
		}
		Print("\n============syntaxstack=========\n");
		u8 syntaxstacksize = DB_ElementCount(syntaxstack);
		for (u8 i = 0; i < syntaxstacksize; ++i) {
			TK* tki = (TK*)DB_Index(syntaxstack, i);
			TK_2Str(tki, buff);
			Print("%s", buff);
		}
		Print("\n===============================\n\n");
	} while (tk->type != tktp_eof);
	
	Print("%u procs where defined\n", DB_ElementCount(cu->procs));
	for (u8 i = 0; i < DB_ElementCount(cu->procs); i++) {
		SKPROC* proc = (SKPROC*)DB_Index(cu->procs, i);
		SYMTABLEENTRY* symentry = DB_Index(cu->symtable.entries, proc->sym);
		Print("%u:%$\n", proc->sym, cu->symtable.sympool->data + symentry->off, symentry->size);
	}
	
	if (cu->epindx == (u8)(-1)) {
		SK_Error(cu, "entry point \"%s\" was not found", cu->entrypoint);
	}
	else {
		Print("main proc is located at: %u\n", cu->epindx);
	}
	if (cu->errorcount) {
		FatalError(0, "you made %u %s, you suck!\n", cu->errorcount, cu->errorcount > 1? "errors" : "error");
	}
	Free(cu->sourcecode);
	cu->sourcecode = 0;
	DB_Free(syntaxstack);
}

void SK_PrintByteCode(DYNBUFF* bytecode) {
	u8 pp = 0;
	Print("\n\n=================bytecode===============\n\n");
	u8 icrlen = DB_ElementCount(bytecode);
	//Print("icrlen:%u\n", icrlen);
	u8* ops = (u8*)bytecode->data;
	while (pp < icrlen) {
		u8 op = ops[pp];
		if (op < skop_count) {
			Print("0x%xr0:%s", pp, 2, skops2str[op]);
			if (op == skop_psh || op == skop_jpz || op == skop_jmp) {
				Print(", 0x%xr0", ops[++pp], 16);
			}
		}
		else {
			Print("0x%xr0:0x%xr0", pp, 2, op, 16);
		}
		pp++;
		Print("\n");
	}
	Print("\n\n=================bytecode===============\n\n");
}

// TODO:: rename the func to something more applicable
// TODO:: check stack balance
void SK_GenOpsByteCode(COMPUNIT* cu, TK* tk, DYNBUFF* icr, DYNBUFF* unresolvedprocs) {
	switch (tk->type) {
		case tktp_ops: {
			u8 opcount = DB_ElementCount(tk->ops);
			for (u8 k = 0; k < opcount; k++) {
				TK* op = (TK*)DB_Index(tk->ops, k);
				switch (op->type) {
					case tktp_num: {
						DB_Push(icr, &(u8){ skop_psh });
						DB_Push(icr, &(u8){ op->val });
					} break;
					case tktp_op: {
						DB_Push(icr, &(u8){ op->val });
					} break;
					case tktp_sym: {
						DB_Push(icr, &(u8){ skop_psh });
						SYMTABLEENTRY* symentry = (SYMTABLEENTRY*)DB_Index(cu->symtable.entries, op->val);
						if (symentry->type == symtp_proc) {
							SKPROC* proc = (SKPROC*)DB_Index(cu->procs, symentry->objid);
							if (proc->defined & 0x80) {
								if (proc->icraddr == (u8)(-1)) {
									DB_Push(unresolvedprocs, &(u8){ symentry->objid });
									proc->icraddr = DB_ElementCount(icr);
									DB_Push(icr, &(u8){ proc->icraddr });
								}
								else {
									proc->defined &= 0x7F;
									DB_Push(icr, &(u8){ proc->icraddr });
									proc->icraddr = DB_ElementCount(icr) - 1;
								}
								proc->defined |= 0x80;
							}
							else {
								DB_Push(icr, &(u8){ proc->icraddr });
							}
						}
						else {
							FatalError(0, "idk\n");
						}
					} break;
				}
			}
		} break;
		case tktp_loopop: {
			TK* conditionops = (TK*)DB_Index(tk->ops, 0);
			TK* block = (TK*)DB_Index(tk->ops, 1);

			u8 loopingaddr = DB_ElementCount(icr);
			SK_GenOpsByteCode(cu, conditionops, icr, unresolvedprocs);
			
			DB_Push(icr, &(u8){ skop_jpz });
			u8 jpzargaddr = DB_ElementCount(icr);
			DB_Push(icr, &(u8){ 0 });
			
			u8 blockopscount = DB_ElementCount(block->ops);
			for (u8 k = 0; k < blockopscount; k++) {
				TK* blockop = (TK*)DB_Index(block->ops, k);
				SK_GenOpsByteCode(cu, blockop, icr, unresolvedprocs);
			}
			DB_Push(icr, &(u8){ skop_jmp });
			DB_Push(icr, &(u8){ loopingaddr });
			u8 blockendaddr = DB_ElementCount(icr);
			*(u8*)DB_Index(icr, jpzargaddr) = blockendaddr;
		} break;
		case tktp_branchop:{
			u8 branchescount = DB_ElementCount(tk->ops);
			u8 i = 0;
			u8 jmpargaddr = (u8)(-1);

			while (i < branchescount) {
				TK* node = DB_Index(tk->ops, i);
				if (node->type == tktp_ops) {
					SK_GenOpsByteCode(cu, node, icr, unresolvedprocs);
					DB_Push(icr, &(u8){ skop_jpz });
					u8 jpzargaddr = DB_ElementCount(icr);
					
					DB_Push(icr, &(u8){ 0 });
					node = DB_Index(tk->ops, ++i);
					u8 blockopscount = DB_ElementCount(node->ops);
					for (u8 k = 0; k < blockopscount; k++) {
						TK* blockop = (TK*)DB_Index(node->ops, k);
						SK_GenOpsByteCode(cu, blockop, icr, unresolvedprocs);
					}
					if (++i != branchescount) {
						DB_Push(icr, &(u8){ skop_jmp });
						if (jmpargaddr == (u8)(-1)) {
							jmpargaddr = DB_ElementCount(icr);
							DB_Push(icr, &(u8){ jmpargaddr });
						}
						else {
							DB_Push(icr, &(u8){ jmpargaddr });
							jmpargaddr = DB_ElementCount(icr)-1;
						}
					}
					*(u8*)DB_Index(icr, jpzargaddr) = DB_ElementCount(icr);
				}
				else if (node->type == tktp_block) {
					u8 blockopscount = DB_ElementCount(node->ops);
					for (u8 k = 0; k < blockopscount; k++) {
						TK* blockop = (TK*)DB_Index(node->ops, k);
						SK_GenOpsByteCode(cu, blockop, icr, unresolvedprocs);
					}
					i++;
				}
			}
			if (jmpargaddr != (u8)(-1)) {
				u8 jmpargaddrtarget = DB_ElementCount(icr);
				while (*(u8*)DB_Index(icr, jmpargaddr) != jmpargaddr) {
					u8 tmp = *(u8*)DB_Index(icr, jmpargaddr);
					*(u8*)DB_Index(icr, jmpargaddr) = jmpargaddrtarget;
					jmpargaddr = tmp;
				}
				*(u8*)DB_Index(icr, jmpargaddr) = jmpargaddrtarget;
			}
		} break;
		default: {
			FatalError(0, "SK_GenBytecode:: \"%s\" not supported yet\n", tktp2str[tk->type]);
		} break;
	}
}

DYNBUFF* SK_GenBytecode(COMPUNIT* cu) {
	u1 buff[256];
	DYNBUFF* icr = DB_Create(256, sizeof(u8));
	
	DYNBUFF* unresolvedprocs = DB_Create(256, sizeof(u8));
	DB_Push(unresolvedprocs, &(u8){ cu->epindx });
	
	u1 mainprocf = 1;
	while (unresolvedprocs->occ){
		SKPROC* wproc = (SKPROC*)DB_Index(cu->procs, *(u8*)DB_SoftPop(unresolvedprocs));
		u8 procaddr = DB_ElementCount(icr);
		u8 opscount = DB_ElementCount(wproc->ops);
		for (u8 i = 0; i < opscount; i++) {
			TK* tk = (TK*)DB_Index(wproc->ops, i);
			TK_2Str(tk, buff);
			Print("%s\n", buff);
			SK_GenOpsByteCode(cu, tk, icr, unresolvedprocs);
		}
		if (mainprocf) {
			DB_Push(icr, &(u8){ skop_hcf });
			mainprocf = 0;
		}
		else {
			DB_Push(icr, &(u8){ skop_ret });
		}
		// proc patching
		if (wproc->icraddr != (u8)(-1)) {
			wproc->defined = wproc->defined & 0x7F;
			while (*(u8*)DB_Index(icr, wproc->icraddr) != wproc->icraddr) {
				u8 tmp = *(u8*)DB_Index(icr, wproc->icraddr);
				*(u8*)DB_Index(icr, wproc->icraddr) = procaddr;
				wproc->icraddr = tmp;
			}
			*(u8*)DB_Index(icr, wproc->icraddr) = procaddr;
		}
		wproc->icraddr = procaddr;
	}
	DB_Free(unresolvedprocs);
	return icr;
}


// TODO:: you're leaking mem like creazy, i should've done a tkpool from the beg, change the current think to that
u4 main(u4 args, u1** vargs) {
	u1* tests[] = { 
		"procs.sk",
		// "conditionals.sk",
		// "while.sk",
		// "main.sk",
	};
	for (u8 i = 0; i < sizeof(tests)/sizeof(tests[0]);i++) {
		COMPUNIT cu;
		SK_BuildBlocks(&cu, tests[i], "main");
		DYNBUFF* bytecode = SK_GenBytecode(&cu);
		SK_PrintByteCode(bytecode);
		SK_Exe(bytecode);

		// SK_PrintByteCode(bytecode);
		// Free(bytecode);
	}
	return 0x45;
}