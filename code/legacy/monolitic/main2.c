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
X(mw1)\
X(mr1)\
X(mw2)\
X(mr2)\
X(mw4)\
X(mr4)\
X(mw8)\
X(mr8)\
X(add)\
X(sub)\
X(lt)\
X(dmp)\
X(and)\
X(hcf)\
X(jpz)\
X(jmp)\
X(call)\
X(ecall)\
X(ret)\
X(res)\
X(rel)\
X(idx)\
X(lea)\
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
X(ptr)\
X(extrn)\
X(count)

#define X(x) skop_##x,
typedef enum SKOP SKOP;
enum SKOP { SOCKOPS };
#undef X
#define X(x) #x,
u1* skops2str[] = { SOCKOPS };
#undef X
#undef SOCKOPS

#define NODETYPES \
X(eof)\
X(num)\
X(sym)\
X(str)\
X(op)\
X(delim)\
X(flow)\
X(cmd)\
X(cmdstrs)\
X(loopop)\
X(branchop)\
X(type)\
X(symdef)\
X(procdef)\
X(call)\
X(sign)\
X(ops)\
X(types)\
X(mlay)\
X(mlaymem)\
X(count)
// meml -> memlayout

#define X(x) ndtp_##x,
typedef enum NODETYPE NODETYPE;
enum NODETYPE { NODETYPES };
#undef X
#define X(x) #x,
u1* ndtp2str[] = { NODETYPES };
#undef X

typedef struct NODE NODE;
struct NODE {
	u2 col;
	u2 row;

	u2 type;
	union {
		u2 ext;
		u2 argsretsf;
		u2 mlaymemf;
		u2 mlayf;
		u2 loopf;
		struct {
			u1 branchtype;
			u1 branchf;
		};
	};
	union {
		u8 val;         // lite ral int value, symbol index, random bs
		u8 stroff;
		struct {
			union {
				u4 opsc;
				u4 cmdstrc;
				u4 mlaymemc;   // for mem layout
				u4 argsc;
				u4 primsize;    // type real size in bytes
				u4 mlaymemsize; // for mem layout arrays
				u4 branchc;
				u4 op;
			};
			union {
				u4 mlaysym;
				u4 mlaymemsym; // for mem layout member
				u4 retsc;
				u4 prim; // the type of types lol
				u4 cmdtp;
				u4 callsym;
			};
		};
		u4 argsrets[2];
	};
};

#define SYMTYPES \
X(undef)\
X(seen)\
X(proc)\
X(eproc)\
X(rsym)\
X(mlaymem)\
X(dmlay)\
X(str)\
X(count)

#define X(x) symtp_##x,
typedef enum SYMTYPE SYMTYPE;
enum SYMTYPE { SYMTYPES };
#undef X
#define X(x) #x,
u1* symtp2str[] = { SYMTYPES };
#undef X
#undef SYMTYPES


typedef enum SYMSTT SYMSTT;
enum SYMSTT {
	symstt_unpatched,
	symstt_seen,
	symstt_patched,
};

typedef struct SYMTE SYMTE;
struct SYMTE { // symbol table entry
	u1 type;   // proc mem etc
	u1 status; // undef, seen (but still undef), def, used at the codegen stage
	u2 size;   // size of symbol
	u4 off;    // symtable's symbol name pool
	u4 objid;  // proc index, mem index, etc index nodepool
	union {
		u4 icr;    // proc index, mem index, etc index "final icr addr
		u4 haslocals;
	};
};

typedef struct TABLE TABLE;
struct TABLE {
	DYNBUFF* pool;
	DYNBUFF* entries;
};

typedef struct COMPUNIT COMPUNIT;
struct COMPUNIT {
	DYNBUFF* nodes;
	TABLE syms;
	
	u1* entrypoint;
	u1* sourcefile;
	u1* sourcecode;
	u1* scoff;// source code offset
	
	u8  epindx; // entrypoint index proc
	u4 errorcount;
	u4 nodelen; // len of substring in the sourcecode
	NODE cnode; // current node
};

typedef struct SK_VMPROG SK_VMPROG;
struct SK_VMPROG {
	DYNBUFF* mem;
	DYNBUFF* loadlibs;
};

// TODO:: implement hashes and dictionaries, this is slow as hell but good enough for a basic implementation
u8 SYMT_Index(COMPUNIT* cu, u1* sym, u4 size) {
	u8 i = 0;
	u8 symtableentrycount = DB_ElementCount(cu->syms.entries);
	while (i < symtableentrycount) {
		SYMTE* entry = (SYMTE*)DB_Index(cu->syms.entries, i); // returns null if there's not entry was found
		if (CstrCmpS(cu->syms.pool->data + entry->off, sym, size)) { return i; }
		i++;
	}
	// not found so let's insert the new symbol
	SYMTE tmp = (SYMTE){ .type = symtp_undef, .off = DB_ElementCount(cu->syms.pool), .size = size, .objid = i };
	DB_Push(cu->syms.entries, &tmp);
	DB_Append(cu->syms.pool, sym, size);
	return i;
}

void CU_NxtTK(COMPUNIT* cu) {
	NODE* cnode = &cu->cnode;
	cnode->col += cu->nodelen;
	cu->scoff += cu->nodelen;
	do{
		while (*cu->scoff && CharIsSpace(*cu->scoff)) {
			cu->scoff++;
			cnode->col++;
			if (CharIsNL(*cu->scoff)) {
				cnode->col = 0;
				while (CharIsNL(*cu->scoff)) { if (*cu->scoff == '\n') { cnode->row++; } cu->scoff++; }
			}
		}

		if (*cu->scoff == ';') {
			cu->scoff++;
			cnode->col++;
			if (*cu->scoff == ';') {
				cu->scoff++;
				cnode->col++;
				while (*cu->scoff && !(*cu->scoff == ';' && *(cu->scoff+1) == ';')) { 
					cu->scoff++;
					cnode->col++;
					if (CharIsNL(*cu->scoff)) {
						cnode->col = 0;
						while (CharIsNL(*cu->scoff)) { if (*cu->scoff == '\n') { cnode->row++; } cu->scoff++; }
					}
				}
				if (*cu->scoff) {
					cu->scoff += 2;
					cnode->col += 2;
				}
			}
			else {
				while (*cu->scoff && !CharIsNL(*cu->scoff)) { cu->scoff++; }
				cnode->col = 0;
				while (*cu->scoff && CharIsNL(*cu->scoff)) { if (*cu->scoff == '\n') { cnode->row++; } cu->scoff++; }
			}
		}
    } while (*cu->scoff && (CharIsSpace(*cu->scoff) || *cu->scoff == ';'));
	
	u1* nodeend = cu->scoff;
	if (CharIsAlp(*cu->scoff)) {
		while (CharIsAlpNum(*nodeend) || *nodeend == '_') { nodeend++; }
		cu->nodelen = (u8)nodeend - (u8)cu->scoff;
		
		if (CstrCmpS("u1", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_type;
			cnode->prim = skop_u1;
			cnode->primsize = 1;
			return;
		}
		if (CstrCmpS("u2", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_type;
			cnode->prim = skop_u2;
			cnode->primsize = 2;
			return;
		}
		if (CstrCmpS("u4", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_type;
			cnode->prim = skop_u4;
			cnode->primsize = 4;
			return;
		}
		if (CstrCmpS("ptr", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_type;
			cnode->prim = skop_ptr;
			cnode->primsize = 8;
			return;
		}
		if (CstrCmpS("u8", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_type;
			cnode->prim = skop_u8;
			cnode->primsize = 8;
			return;
		}
		if (CstrCmpS("if", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val  = skop_if;
			return;
		}
		if (CstrCmpS("elif", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skop_elif;
			return;
		}
		if (CstrCmpS("else", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skop_else;
			return;
		}
		if (CstrCmpS("while", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skop_while;
			return;
		}
		if (CstrCmpS("extrn", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_cmd;
			cnode->cmdtp  = skop_extrn;
			return;
		}
		cnode->type = ndtp_sym;
		cnode->val = SYMT_Index(cu, cu->scoff, cu->nodelen);
		return;
	}
	if (CharIsNum(*cu->scoff)) {
		cnode->type = ndtp_num;
		u8 v = 0;
		
		while ((u1)(*nodeend - '0') < 10) { v = v * 10 + *nodeend++ - '0'; }
		cnode->val = v;
		cu->nodelen = (u8)(nodeend - cu->scoff);
		return;
	}
	
	if (CstrCmpS("*>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_dup;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_rtf;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("<@", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_rtb;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("'>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_leap;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("><", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_swap;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("//", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_drop;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@1", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mw1;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@2", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mw2;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@4", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mw4;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@8", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mw8;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!1", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mr1;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!2", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mr2;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!4", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mr4;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!8", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skop_mr8;
		cu->nodelen = 2;
		return;
	}
		
	switch (*cu->scoff) {
		case '"': {
			nodeend++;
			u1 buff[512];
			u1* bh = buff;
			while (*nodeend && *nodeend != '"') {
				if (*nodeend == '\\') {
					switch (*++nodeend) {
						case 'n': {
							*bh++ = '\n';
						} break;
						case '\\': {
							*bh++ = '\\';
						} break;
						default: {
							FatalError(0, "scapecode \\\"%c\" is not implementeyet", *nodeend);
						} break;
					}
					nodeend++;
				}
				else {
					*bh++ = *nodeend++;
				}
			}
			if (*nodeend == '"') {
				nodeend++;
				cu->nodelen = (u8)(nodeend - cu->scoff);
				cnode->type = ndtp_str;
				cnode->stroff = SYMT_Index(cu, buff, (u8)(bh - buff));
				SYMTE* strte = (SYMTE*)DB_Index(cu->syms.entries, cnode->stroff);
				strte->type = symtp_str;
			}
			else {
				FatalError(0, "TK_Nxt:: premature oef while defining string literal at %s:%4u:%4u\n", *cu->scoff, cu->sourcefile, cnode->row + 1, cnode->col + 1);
			}
		} break;
		case '(':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val = skop_lpar;  } break;
		case ')':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val = skop_rpar;  } break;
		case '{':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val = skop_lbra;  } break;
		case '}':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val = skop_rbra;  } break;
		case ':':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val = skop_colon; } break;
		case '+':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op  = skop_add;   } break;
		case '&':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op  = skop_and;   } break;
		case '-':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op  = skop_sub;   } break;
		case '^':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op  = skop_dmp;   } break;
		case '<':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op  = skop_lt;    } break;
		case '#':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op  = skop_call;  } break;
		case '\0': { cu->nodelen = 0; cnode->type = ndtp_eof;   cnode->val = skop_hcf;   } break;
		default: { FatalError(0, "TK_Nxt:: unhandled character { %c } at %s:%4u:%4u\n", *cu->scoff, cu->sourcefile, cnode->row+1, cnode->col+1); }
	}
	return;
}

inline static u8 NODE_2Str(COMPUNIT* cu, NODE* node, u1* buff) {
	switch (node->type) {
		case ndtp_symdef:
		case ndtp_procdef:
		case ndtp_str:
		case ndtp_sym: {
			SYMTE* symentry = (SYMTE*)DB_Index(cu->syms.entries, node->val);
			return CstrFmt(buff, "(%s:%$)", ndtp2str[node->type], cu->syms.pool->data + symentry->off, (u8)symentry->size);
		} break;
		case ndtp_call: {
			SYMTE* symentry = (SYMTE*)DB_Index(cu->syms.entries, node->callsym);
			return CstrFmt(buff, "(%s:%$)", ndtp2str[node->type], cu->syms.pool->data + symentry->off, (u8)symentry->size);
		} break;
		case ndtp_num:{
			return CstrFmt(buff, "(%s:%u)", ndtp2str[node->type], node->val);
		}
		case ndtp_sign: {
			return CstrFmt(buff, "(%s:%4u %4u)", ndtp2str[node->type], node->argsc, node->retsc);
		}
		case ndtp_mlay: {
			SYMTE* symentry = (SYMTE*)DB_Index(cu->syms.entries, node->mlaysym);
			return CstrFmt(buff, "(%s:%$ %u)", ndtp2str[node->type], cu->syms.pool->data + symentry->off, (u8)symentry->size, (u8)node->mlaymemc);
		}
		case ndtp_mlaymem: {
			SYMTE* symentry = (SYMTE*)DB_Index(cu->syms.entries, node->mlaymemsym);
			return CstrFmt(buff, "(%s:%u %$ %1br0)", ndtp2str[node->type], node->mlaymemsize, cu->syms.pool->data + symentry->off, (u8)symentry->size, node->mlaymemf, (u8)3);
		}
		case ndtp_ops: {
			return CstrFmt(buff, "(%s:%u)", ndtp2str[node->type], node->opsc);
		}
		case ndtp_types: {
			return CstrFmt(buff, "(%s:%4u %4u)", ndtp2str[node->type], node->argsc, node->retsc);
		}
		case ndtp_type: {
			return CstrFmt(buff, "(%s:%s)", ndtp2str[node->type], skops2str[node->prim]);
		}
		case ndtp_cmd: {
			return CstrFmt(buff, "(%s:%s %u)", ndtp2str[node->type], skops2str[node->cmdtp], node->cmdstrc);
		}
		case ndtp_cmdstrs: {
			return CstrFmt(buff, "(%s:%s %u)", ndtp2str[node->type], skops2str[node->cmdtp], node->cmdstrc);
		}
		case ndtp_flow:
		case ndtp_op:
		case ndtp_delim: {
			return CstrFmt(buff, "(%s:%s)", ndtp2str[node->type], skops2str[node->val]);
		}
		default: {
			if (node->type < ndtp_count) {
				return CstrFmt(buff, "(%s)", ndtp2str[node->type]);
			}
			return CstrFmt(buff, "(?? %u)", node->type);
		}
	}
}

inline static void NODE_PrintInfo(COMPUNIT* cu, NODE* node) {
	u1 buff[256];
	NODE_2Str(cu, node, buff);
	Print("%s at %u:%u\n", buff, (u8)node->row + 1, (u8)node->col + 1);
}

inline static void SK_ExeArgumentCheck(u8 pp, u1* sp, u8 stacksize, u1* stackbase, u8 argumentsneeded, u1* opcode) {
	u8 elementsonthestack = ((u8)stacksize - (u8)sp + (u8)stackbase) >> 3;
	if (elementsonthestack < argumentsneeded) {
		FatalError(0, "SK_Exe:: not enough arguments for %s, execution stop at 0x%x\n", opcode, pp);
	}
}


u8 SK_CallNative(u8 func, u8* args, u8 argcsrets){
	u8 argc    = argcsrets & 0xFFFFFFFF;
	u8 hasret  = argcsrets >> 32;
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
			if (hasret) return ((u8(*)(u8, u8))func)(args[0], args[1]);
			((void (*)(u8, u8))func)(args[0], args[1]);
		} return 0;
		case 3: {
			if (hasret) return ((u8(*)(u8, u8, u8))func)(args[0], args[1], args[2]);
			((void (*)(u8, u8, u8))func)(args[0], args[1], args[2]);
		} return 0;
		case 4: {
			if (hasret) return ((u8(*)(u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3]);
			((void (*)(u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3]);
		} return 0;
		case 5: {
			if (hasret) return ((u8(*)(u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4]);
			((void (*)(u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4]);
		} return 0;
		case 6: {
			if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4], args[5]);
			((void (*)(u8, u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4], args[5]);
		} return 0;
		case 7: {
			if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3],args[4], args[5], args[6]);
			((void (*)(u8, u8, u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
		} return 0;
		case 8: {
			if (hasret) return ((u8(*)(u8, u8, u8, u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			((void (*)(u8, u8, u8, u8, u8, u8, u8, u8))func)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
		} return 0;
		default: { FatalError(0, "too many arguments"); }return 0;
	}
}
// TOCONSIDER:: maybe i should make the operations into functions and instead of a switch use an array
// i think it would be nicer long term idk
// TODO:: move this into it's own file, make a proper vm, with a nice debug mode
// it'll come handy to trace bugs, or creating new headaches when it becomes out off sync with the real asm code
void SK_Exe(SK_VMPROG* program) {
	Print("\n\n===========execution=======\n\n");
	const u8 stackmemsizebytes = 512 * sizeof(u8);
	u1* stackmem = (u1*)Malloc(stackmemsizebytes);
	u8* mem = (u8*)program->mem->data;
	
	u8 pp = 0;
	
	u1* rp = (u1*)stackmem;
	u1* sp = (u1*)stackmem + stackmemsizebytes-8;
	
	u1 run = 1;
	while (run) {
		/*
			Print("0x%xr0:%s", pp, (u8)4, skops2str[mem[pp]]);
			if (mem[pp] == skop_psh || mem[pp] == skop_jpz || mem[pp] == skop_jmp) {
				Print(", 0x%xr0", mem[pp+1], 16);
			}
			Print("\n");
		*/
#define Push(xx) do {*(u8*)sp = (xx); sp -= 8; } while(0)
#define Pop(xx) do { sp += 8; (xx) = *(u8*)sp; } while(0)
		switch (mem[pp]) {
			case skop_nop: {}break;
			case skop_psh: {
				if (sp <= rp) FatalError(0, "SK_Exe:: stack overflow at 0x%x!!!!\n", pp);
				pp += 1;
				Push(mem[pp]);
			} break;
			case skop_idx: {
				if (sp <= rp) FatalError(0, "SK_Exe:: stack overflow at 0x%x!!!!\n", pp);
				pp += 1;
				Push((u8)(rp - mem[pp]));
			} break;
			case skop_lea: {
				if (sp <= rp) FatalError(0, "SK_Exe:: stack overflow at 0x%x!!!!\n", pp);
				pp += 1;
				Push((u8)(mem + mem[pp]));
			} break;
			case skop_mw1: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u1* dest; Pop((u8)dest);
				u8 val; Pop(val);
				*dest = (u1)val;
			} break;
			case skop_mr1: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u1* src; Pop((u8)src);
				Push(*src);
			} break;
			case skop_mw2: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u2* dest; Pop((u8)dest);
				u8 val; Pop(val);
				*dest = (u2)val;
			} break;
			case skop_mr2: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u2* src; Pop((u8)src);
				Push(*src);
			} break;
			case skop_mw4: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u4* dest; Pop((u8)dest);
				u8 val; Pop(val);
				*dest = (u4)val;
			} break;
			case skop_mr4: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u4* src; Pop((u8)src);
				Push(*src);
			} break;
			case skop_mw8: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8* dest; Pop((u8)dest);
				u8 val; Pop(val);
				*dest = val;
			} break;
			case skop_mr8: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u8* src; Pop((u8)src);
				Push(*src);
			} break;
			case skop_dup: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u8 a;
				Pop(a);
				Push(a);
				Push(a);
			} break;
			case skop_drop: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				sp += 8;
			} break;
			case skop_add: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8 a, b;
				Pop(b);
				Pop(a);
				Push(a + b);
			} break;
			case skop_sub: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8 a, b;
				Pop(b);
				Pop(a);
				Push(a - b);
			} break;
			case skop_and: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8 a, b;
				Pop(b);
				Pop(a);
				Push(a & b);
			} break;
			case skop_lt: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8 a, b;
				Pop(b);
				Pop(a);
				Push(a < b);
			} break;
			case skop_dmp: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u8 a;
				Pop(a);
				Print("%u\n", a);
			} break;
			case skop_jpz: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u8 a;
				Pop(a);
				++pp;
				if (a == 0) { pp = mem[pp] - 1; }
			} break;
			case skop_jmp: {
				pp = mem[++pp] - 1;
			} break;
			case skop_hcf: {
				Print("the program has halted!!!\n");
				run = 0;
			} break;
			case skop_rtb: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 3, skops2str[mem[pp]]);
				u8 a, b, c;
				Pop(c);  Pop(b);  Pop(a);
				Push(b); Push(c); Push(a);
			} break;
			case skop_rtf: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 3, skops2str[mem[pp]]);
				u8 a, b, c;
				Pop(c);  Pop(b);  Pop(a);
				Push(c); Push(a); Push(b);
			} break;
			case skop_leap: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8 a, b;
				Pop(b); Pop(a);
				Push(a); Push(b); Push(a);
			} break;
			case skop_swap: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 2, skops2str[mem[pp]]);
				u8 a, b;
				Pop(b); Pop(a);
				Push(b); Push(a);
			} break;
			case skop_call: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u8 procaddr;
				Pop(procaddr);
				*(u8*)rp = pp;
				rp += 8;
				pp = procaddr - 1;
			} break;
			case skop_ecall: {
				SK_ExeArgumentCheck(pp, sp, stackmemsizebytes, stackmem, 1, skops2str[mem[pp]]);
				u8 procaddr;
				Pop(procaddr);
				u8 argsrets = mem[++pp];
				u8 ret = SK_CallNative(procaddr, (u8*)sp + 1, argsrets);
				sp += (argsrets & 0xFFFFFFFF) << 3;
				if (argsrets >> 32) { Push(ret); }
			} break;
			case skop_ret: {
				if ((u8)(rp - 8) < (u8)stackmem) FatalError(0, "SK_Exe:: return pointer underflow!!! at 0x%x\n", pp);
				rp -= 8;
				pp = *(u8*)rp;
			} break;
			case skop_res: {
				u8 amount2reseverd = mem[++pp];
				rp += amount2reseverd;
				if ((u8)rp > (u8)sp) FatalError(0, "SK_Exe:: return pointer overflow!!! at 0x%x\n", pp);
			} break;
			case skop_rel: {
				u8 amount2release = mem[++pp];
				rp -= amount2release;
				if ((u8)(rp) < (u8)stackmem) FatalError(0, "SK_Exe:: return pointer underflow!!! at 0x%x\n", pp);
			} break;
			default: {
				if (mem[pp] < skop_count) {
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
#undef Push
#undef Pop
	Print("\n\n===========================\n\n");
	Free(stackmem);
	for (u8 i = 0; i < DB_ElementCount(program->loadlibs); i++) {
		//FreeLibrary(*(HMODULE*)DB_Index(program->loadlibs, i));
		HMODULE lib = *(HMODULE*)DB_Index(program->loadlibs, i);

		Print("lib[%u] = %x\n", i, lib);

		BOOL result = FreeLibrary(lib);

		Print("FreeLibrary = %u\n", result);
	}
	DB_Free(program->loadlibs);
	DB_Free(program->mem);
}

// TODO:: write another proc that takes a node as reference to print the possition only
void SK_Error(COMPUNIT* cu, u1* errmsg, ...) {
	NODE* cnode = &cu->cnode;
	u1 errbuff[1024];
	VAR_ARG(errmsg, stack);
	CstrFmtS(errbuff, errmsg, stack);
	Print("ERROR::%s at %s:%u:%u\n", errbuff, cu->sourcefile, cnode->col+1, cnode->row+1);
	cu->errorcount++;
}

/*

**: empty
x, : 'or could be' x
x* : '0 or more of x'
x+ : '1 or more of x'
\x : literal x
(x): enclosure for 'counters'
x? : 1 or 0 of x
counter: *, ?, +

program:
	procs
procs:
	procs proc, proc, **
proc:
	procdef block
procdef:
	symdcl types
types:
	\( args rets \)
symdcl:
	sym :
args: 
	args type, type, **
rets:
    : type, rets type, **
type:
	u1, u2, u4, u8
block:
	\{ ops \}	
ops:
	ops op, op, **
op:
	stackop, binop, uniop, branchop, loopop, dmemlayout
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
dmemlayout:
	symdef \{ (num sym type)+ \}
*/

// TODO::
// add static mem, stack balance checker, type check, load external procs 
// only then  start actual code gen aka outputing x86_64 insts
// do proper copling for the scenario ops x -> ops

inline static void SK_OpenNewContext(COMPUNIT* cu, DYNBUFF* syntaxstack, DYNBUFF* contxtstack, NODE* cnode, u8 contxttype) {
	cnode->type = contxttype;
	cnode->val = 0;
	cnode->ext = 0;
	DB_Push(syntaxstack, cnode);
	DB_Push(contxtstack, &(u8){ DB_ElementCount(cu->nodes) });
	DB_Push(cu->nodes, cnode);
}

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

	cu->syms = (TABLE){ .entries = DB_Create(256, sizeof(SYMTE)), .pool = DB_Create(256, sizeof(u1)) };
	cu->nodes = DB_Create(256, sizeof(NODE));
	cu->errorcount = 0;
	SYMT_Index(cu, "null", 4);

	DYNBUFF* syntaxstack = DB_Create(256, sizeof(NODE));
	DYNBUFF* contxtstack = DB_Create(256, sizeof(u8));

	NODE* cnode;
	u1 buff[256];
	
	u1 haslocals = 0;

	do {
		CU_NxtTK(cu);
		cnode = &cu->cnode;
		
		Print("next token:\n");
		NODE_PrintInfo(cu,cnode);

		switch (cnode->type) {
			case ndtp_cmd:{
				DB_Push(syntaxstack, cnode);
			} break;
			case ndtp_sym:{
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_ops && topnode->type == ndtp_sym) {
					DB_Push(cu->nodes, topnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc += 1;
					syntaxstack->occ -= sizeof(NODE);
					DB_Push(syntaxstack, cnode);
				}
				else {
					DB_Push(syntaxstack, cnode);
				}
			} break;
			case ndtp_delim: {
				switch (cnode->val) {
					case skop_colon: {
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						if (topnode && topnode->type == ndtp_sym) {
							SYMTE* symentry = DB_Index(cu->syms.entries, topnode->val);
							if (symentry->type == symtp_undef) {
								symentry->type = symtp_seen;
								topnode->type = ndtp_symdef;
							}
							else {
								SK_Error(cu, "redefinition of %$", cu->syms.pool->data + symentry->off, symentry->size);
							}
						}
						else if (topnode && topnode->type == ndtp_types) {
							NODE* ndtypes = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndtypes->argsretsf = 1;
						}
						else {
							SK_Error(cu, "rogue \":\"");
						}
					} break;
					case skop_lpar: {
						// command(, symdef(
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						if (topnode){
							switch (topnode->type) {
								case ndtp_symdef:{
									SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_types);
								} break;
								case ndtp_cmd:{
									u8 tmp = topnode->cmdtp;
									syntaxstack->occ -= sizeof(NODE);
									SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_cmdstrs);
									NODE* ndcmdstrs = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
									ndcmdstrs->cmdtp = tmp;
								} break;
								default: {
									DB_Push(syntaxstack, cnode);
									SK_Error(cu, "rogue \"(\"");
								} break;
							}
						}
						else {
							SK_Error(cu, "rogue \"(\"");
						}
					} break;
					case skop_rpar: {
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						u8 syntaxstacksize = DB_ElementCount(syntaxstack);
						if (syntaxstacksize >= 1 && topnode->type == ndtp_types) {
							Print("signature:\n");
							NODE* ndtypes = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							Print("args %4u:\n", ndtypes->argsc);
							for (u8 i = 0; i < ndtypes->argsc; ++i) {
								NODE_2Str(cu, ndtypes + 1 + i, buff);
								Print("%s", buff);
							}
							Print("\nrets %4u:\n", ndtypes->retsc);
							for (u8 i = ndtypes->argsc; i < ndtypes->argsc + ndtypes->retsc; ++i) {
								NODE_2Str(cu, ndtypes + 1 + i, buff);
								Print("%s", buff);
							}
							// symdef types -> procdef
							syntaxstack->occ -= sizeof(NODE);
							(topnode-1)->type = ndtp_procdef;
						}
						else if (syntaxstacksize >= 1 && topnode->type == ndtp_cmdstrs) {
							NODE* ndcmdstrs = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							Print("cmds %4u:\n", ndcmdstrs->cmdstrc);
							for (u8 i = 0; i < ndcmdstrs->cmdstrc; ++i) {
								NODE_2Str(cu, ndcmdstrs + 1 + i, buff);
								Print("%s", buff);
							}
							SYMTE* symentry = DB_Index(cu->syms.entries, (topnode - 1)->val);
							symentry->type = symtp_eproc;
							symentry->status = symstt_unpatched;
							symentry->objid = *(u8*)DB_Peek(contxtstack, 2);
							symentry->icr = 0;
							
							if (CstrCmpS(cu->entrypoint, cu->syms.pool->data + symentry->off, symentry->size)) {
								cu->epindx = (topnode - 1)->val;
							}
							syntaxstack->occ -= 2 * sizeof(NODE);
							contxtstack->occ -= 2 * sizeof(u8);
						}
						else {
							SK_Error(cu, "rogue \")\"");
						}
					} break;
					case skop_lbra: {
						// procdef {, symdef {, flow ops {, flow {
						// TOCONSIDER add support for blocks only for the sake of temporal scopes
						u8 syntaxstacksize = DB_ElementCount(syntaxstack);
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						if (syntaxstacksize >= 1 && topnode->type == ndtp_procdef) {
							haslocals = 0;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
						}
						else if (syntaxstacksize >= 2 && topnode->type == ndtp_ops && (topnode - 1)->type == ndtp_loopop) {
							NODE* ndloopop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
							if (ndloopop->loopf) {
								// TODO:: for now this an error
								FatalError(0, "blocks are not implemented inside of a loopop yet");
							}
							else {
								ndloopop->loopf = 1;
								contxtstack->occ -= 1 * sizeof(u8); // removes the condition
								syntaxstack->occ -= 1 * sizeof(NODE);
								SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops); // creates the block
							}
						}
						else if (syntaxstacksize >= 2 && topnode->type == ndtp_ops && (topnode - 1)->type == ndtp_branchop) {
							NODE* ndbranchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
							if (ndbranchop->branchf) {
								// TODO:: for now this an error
								FatalError(0, "blocks are not implemented inside of a branchop yet");
							}
							else {
								ndbranchop->branchf = 1;
								contxtstack->occ -= 1 * sizeof(u8); // removes the condition
								syntaxstack->occ -= 1 * sizeof(NODE);
								SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops); // creates the block
							}
						}
						else if (syntaxstacksize >= 1 && topnode->type == ndtp_branchop) {
							NODE* ndbranchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndbranchop->branchf = 1;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops); // creates the block
						}
						else if (syntaxstacksize && topnode->type == ndtp_symdef && (topnode-1)->type == ndtp_ops) {
							syntaxstack->occ -= 1 * sizeof(NODE);
							u4 symv = topnode->val;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_mlay);
							NODE* ndmemlay = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndmemlay->mlaysym = symv;
							topnode->mlaysym = symv;
							NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
							ndops->opsc++;
						}
						else {
							SK_Error(cu, "rogue \"{\"");
						}
					} break;
					case skop_rbra: {
						// symdef types ops, flow ops ops, flow ops, symdef mlay
						u8 syntaxstacksize = DB_ElementCount(syntaxstack);
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						// TODO:: validate topnode->type in this reduction (could be branchop/loopop)
						// TODO:: unnamed proc? let's not support this for now
						// it could be good if i need to create arrays of procs or pass it as an argument, etc
						
						if (!topnode) FatalError(0, "what the fuck?!?");
						switch (topnode->type) {
							case ndtp_ops: {
								NODE* ndops   = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
								Print("ops %4u:\n", ndops->opsc);
								for (u8 i = 0; i < ndops->opsc; ++i) {
									NODE_2Str(cu, ndops + 1 + i, buff);
									Print("%s", buff);
								}
								
								if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_procdef) {
									SYMTE* symentry  = DB_Index(cu->syms.entries, (topnode-1)->val);
									symentry->type   = symtp_proc;
									symentry->status = symstt_unpatched;
									symentry->objid  = *(u8*)DB_Peek(contxtstack, 2);
									symentry->icr    = 0;
									symentry->haslocals = haslocals;
									haslocals = 0;
									
									if (CstrCmpS(cu->entrypoint, cu->syms.pool->data + symentry->off, symentry->size)) {
										cu->epindx = (topnode - 1)->val;
									}
									syntaxstack->occ -= 2 * sizeof(NODE);
									contxtstack->occ -= 2 * sizeof(u8);
								}
								else if (syntaxstacksize >= 3 && (topnode - 1)->type == ndtp_loopop) {
									NODE* ndloop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
									if (!ndloop->loopf) {
										// TODO:: check SK_Error's todo
										SK_Error(cu, "missing \"{\" after while condition");
									}
									syntaxstack->occ -= 2 * sizeof(NODE);
									contxtstack->occ -= 2 * sizeof(u8);
								}
								else if (syntaxstacksize >= 3 && (topnode - 1)->type == ndtp_branchop) {
									NODE* ndbranchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
									if (!ndbranchop->branchf) {
										// TODO:: check SK_Error's todo
										SK_Error(cu, "missing \"{\" after %s condition", skops2str[ndbranchop->val]);
									}
									// ops branchop
									switch (ndbranchop->branchtype) {
										case skop_if: {
											ndbranchop->branchf = 2;
											syntaxstack->occ -= 1 * sizeof(NODE);
											contxtstack->occ -= 1 * sizeof(u8);
										} break;
										case skop_elif: {
											ndbranchop->branchf = 2;
											syntaxstack->occ -= 1 * sizeof(NODE);
											contxtstack->occ -= 1 * sizeof(u8);
										} break;
										case skop_else: {
											// merge ops branchop ops
											ndbranchop->branchf = 2;
											syntaxstack->occ -= 2 * sizeof(NODE);
											contxtstack->occ -= 2 * sizeof(u8);
										} break;
										default: {
											FatalError(0, "unknow branch type %s:%4u", skops2str[ndbranchop->val], ndbranchop->branchtype);
										} break;
									}								
								}
								else {
									FatalError(0, "unreachable \"}\"");
								}
							} break;
							case ndtp_mlay: {
								if (syntaxstacksize && topnode->type == ndtp_mlay) {
									NODE* mlaymemdesc = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
									Print("mlaymem %4u:\n", mlaymemdesc->mlaymemc);
									for (u8 i = 0; i < mlaymemdesc->mlaymemc << 1; i += 2) {
										NODE_2Str(cu, mlaymemdesc + i + 1, buff);
										Print("%s", buff);
										NODE_2Str(cu, mlaymemdesc + i + 2, buff);
										Print("%s", buff);
									}

									SYMTE* symentry = DB_Index(cu->syms.entries, mlaymemdesc->mlaysym);
									symentry->type = symtp_dmlay;
									symentry->status = symstt_unpatched;
									symentry->objid = *(u8*)DB_Peek(contxtstack, 1);
									symentry->icr   = 0;

									syntaxstack->occ -= sizeof(NODE);
									contxtstack->occ -= sizeof(u8);
									haslocals = 1;
								}
							} break;
							default: {
								SK_Error(cu, "rogue \"}\"");
							} break;
						}
					} break;
					default: {
						FatalError(0, "SK_BuildBlocks::unreachable { %s:\"%$\" }", ndtp2str[cnode->type], cu->scoff, (u8)cu->nodelen);
					} break;
				}
			}break;
			case ndtp_type: {
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				if (topnode && topnode->type == ndtp_types) {
					DB_Push(cu->nodes, cnode);
					NODE* ndtypes = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndtypes->argsrets[ndtypes->argsretsf]++;
				}
				else if (topnode && topnode->type == ndtp_mlaymem) {
					NODE* ndmlaymem = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndmlaymem->mlaymemf |= 0b001;
					DB_Push(cu->nodes, cnode);
					syntaxstack->occ -= 1 * sizeof(NODE);
					contxtstack->occ -= 1 * sizeof(u8);
				}
				else if (topnode && topnode->type == ndtp_mlay) {
					NODE* ndmlaydesc = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndmlaydesc->mlaymemc++;
					DB_Push(cu->nodes, &(NODE){.type = ndtp_mlaymem, .mlaymemsize = 1, .mlaymemsym = 0, .mlaymemf = 0b001 });
					DB_Push(cu->nodes, cnode);
				}
				else {
					SK_Error(cu, "rogue type \"%$\"", cu->scoff, (u8)cu->nodelen);
				}
			} break;
			case ndtp_num: {
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				if (syntaxstacksize && topnode->type == ndtp_ops) {
					DB_Push(cu->nodes, cnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc++;
				}
				else if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_ops && topnode->type == ndtp_sym) {
					DB_Push(cu->nodes, topnode);
					DB_Push(cu->nodes, cnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc += 2;
					syntaxstack->occ -= sizeof(NODE);
				}
				else if (syntaxstacksize && topnode->type == ndtp_mlay) {
					u8 val = cnode->val;
					SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_mlaymem);
					NODE* nmlaymem = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					nmlaymem->mlaymemsize = val;
					// TODO:: do some error handling here
					nmlaymem->mlaymemf = 0b100;
					NODE* nmlaydesc = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
					nmlaydesc->mlaymemc++;
				}
				else {
					SK_Error(cu, "rogue \"num:%u\"", cnode->val);
				}
			} break;
			case ndtp_op: {
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				if (syntaxstacksize && topnode->type == ndtp_ops) {
					DB_Push(cu->nodes, cnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc++;
				}
				else if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_ops && topnode->type == ndtp_sym) {
					if (cnode->op == skop_call) {
						cnode->type    = ndtp_call;
						cnode->callsym = topnode->val;
						DB_Push(cu->nodes, cnode);
						NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
						ndops->opsc += 1;
						syntaxstack->occ -= sizeof(NODE);
					}
					else {
						DB_Push(cu->nodes, topnode);
						DB_Push(cu->nodes, cnode);
						NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
						ndops->opsc += 2;
						syntaxstack->occ -= sizeof(NODE);
					}
				}
				else {
					SK_Error(cu, "rogue \"op:%s\"", skops2str[cnode->val]);
				}
			} break;
			case ndtp_str: {
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				if (syntaxstacksize && topnode->type == ndtp_ops) {
					DB_Push(cu->nodes, cnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc++;
				}
				else if (syntaxstacksize && topnode->type == ndtp_cmdstrs) {
					DB_Push(cu->nodes, cnode);
					NODE* ndcmdstrs = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndcmdstrs->cmdstrc++;
				}
				else if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_ops && topnode->type == ndtp_sym) {
					DB_Push(cu->nodes, topnode);
					DB_Push(cu->nodes, cnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc += 2;
					syntaxstack->occ -= sizeof(NODE);
				}
				else {
					SYMTE* strte = (SYMTE*)DB_Index(cu->syms.entries, cnode->stroff);
					SK_Error(cu, "rogue \"str:%$\"", cu->syms.pool->data + strte->off, (u8)strte->size);
				}
			} break;
			case ndtp_flow: {
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				switch (cnode->val) {
					case skop_while: {
						if (syntaxstacksize && topnode->type == ndtp_ops) {
							NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndops->opsc += 1;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_loopop);
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
							NODE* ndloopop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndloopop->loopf = 0;
						}
						else if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_ops && topnode->type == ndtp_sym) {
							DB_Push(cu->nodes, topnode);
							NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndops->opsc += 1;
							syntaxstack->occ -= sizeof(NODE);
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_loopop);
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
							NODE* ndloopop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndloopop->loopf = 0;
						}
						else {
							SK_Error(cu, "rogue \"%s\"", skops2str[cnode->val]);
						}
					} break;
					case skop_if: {
						if (syntaxstacksize && topnode->type == ndtp_ops) {
							NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndops->opsc += 1;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_branchop);
							NODE* branchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							branchop->branchtype = skop_if;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
						}
						else if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_ops && topnode->type == ndtp_sym) {
							DB_Push(cu->nodes, topnode);
							NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							ndops->opsc += 1;
							syntaxstack->occ -= sizeof(NODE);;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_branchop);
							NODE* branchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							branchop->branchtype = skop_if;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
							
						}
						else {
							SK_Error(cu, "rogue \"%s\"", skops2str[cnode->val]);
						}
					} break;
					case skop_elif: {
						if (syntaxstacksize && topnode->type == ndtp_branchop) {
							NODE* branchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							if (branchop->branchf == 2 && (branchop->branchtype == skop_if || branchop->branchtype == skop_elif)) {
								branchop->branchc++;
								branchop->branchf = 0;
								branchop->branchtype = skop_elif;
								SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
							}
							else {
								FatalError(0, "idk man"); // TODO:: change this, from branchf you could guess what's missing either { or } anyway
							}
						}
						else {
							SK_Error(cu, "rogue \"%s\"", skops2str[cnode->val]);
						}
					} break;
					case skop_else: {
						if (syntaxstacksize && topnode->type == ndtp_branchop) {
							NODE* branchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							if (branchop->branchf == 2 && (branchop->branchtype == skop_if || branchop->branchtype == skop_elif)) {
								branchop->branchc++;
								branchop->branchf = 0;
								branchop->branchtype = skop_else;
							}
							else {
								FatalError(0, "idk man"); // TODO:: change this, from branchf you could guess what's missing either { or } anyway
							}
						}
						else {
							SK_Error(cu, "rogue \"%s\"", skops2str[cnode->val]);
						}
					} break;
					default: {
						FatalError(0, "control flow \"%s\" not impelmented yet", skops2str[cnode->val]);
					} break;
				}
			} break;
			case ndtp_eof: {
				if (DB_ElementCount(syntaxstack) > 0) {
					SK_Error(cu, "premature eof; %u nodes left on the syntax stack", DB_ElementCount(syntaxstack));
				}
				if (DB_ElementCount(contxtstack) > 0) {
					SK_Error(cu, "premature eof; %u nodes left on the conxtext stack", DB_ElementCount(contxtstack));
				}
			} break;
			default: {
				FatalError(0, "SK_BuildBlocks:: unhandle symbol { %s:\"%$\" }", ndtp2str[cnode->type], cu->scoff, (u8)cu->nodelen);
			} break;
		}
		Print("\n%4u %4u", DB_ElementCount(syntaxstack), DB_ElementCount(contxtstack));
		Print("\n============syntaxstack=========\n");
		u8 syntaxstacksize = DB_ElementCount(syntaxstack);
		for (u8 i = 0; i < syntaxstacksize; ++i) {
			NODE* nd = (NODE*)DB_Index(syntaxstack, i);
			NODE_2Str(cu, nd, buff);
			Print("%s", buff);
		}
		Print("\n============nodepool============\n");
		u8 nodepoolsize = DB_ElementCount(cu->nodes);
		for (u8 i = 0; i < nodepoolsize; ++i) {
			NODE* nd = (NODE*)DB_Index(cu->nodes, i);
			NODE_2Str(cu, nd, buff);
			Print("%s", buff);
		}
		Print("\n================================\n\n");
	} while (cnode->type != ndtp_eof);
	
	if (cu->epindx == (u8)(-1)) {
		SK_Error(cu, "entry point \"%s\" was not found", cu->entrypoint);
	}
	else {
		Print("main proc is located at: %u\n", cu->epindx);
	}
	if (cu->errorcount) {
		FatalError(0, "you made %u %s, you suck!\n", cu->errorcount, cu->errorcount > 1 ? "errors" : "error");
	}

	Free(cu->sourcecode);
	cu->sourcecode = 0;
	DB_Free(syntaxstack);
	DB_Free(contxtstack);
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
			if (op == skop_psh || op == skop_jpz || op == skop_jmp || op == skop_idx ||
				op == skop_res || op == skop_rel || op == skop_lea || op == skop_ecall) { Print(", 0x%xr0", ops[++pp], 16); }
		}
		else {
			Print("0x%xr0:0x%xr0", pp, 2, op, 16);
		}
		pp++;
		Print("\n");
	}
	Print("\n\n=================bytecode===============\n\n");
}

inline static void SK_TagSym(SYMTE* symte, DYNBUFF* icr, DYNBUFF* unresolvedsymbols, u8 symidx) {
	switch (symte->status) {
		case symstt_unpatched: {
			DB_Push(unresolvedsymbols, &(u8){ symidx });
			symte->icr = DB_ElementCount(icr);
			DB_Push(icr, &(u8){ symte->icr });
			symte->status = symstt_seen;
		} break;
		case symstt_seen: {
			DB_Push(icr, &(u8){ symte->icr });
			symte->icr = DB_ElementCount(icr) - 1;
		} break;
		case symstt_patched: {
			DB_Push(icr, &(u8){ symte->icr });
		} break;
		default: {
			FatalError(0, "unreachable symbol status patchunresolved");
		} break;
	}
}

void SK_PatchUnresolvedSym(COMPUNIT* cu, DYNBUFF* icr, SYMTE* symte, u8 patch) {
	if (symte->status == symstt_seen) {
		while (*(u8*)DB_Index(icr, symte->icr) != symte->icr) {
			u8 tmp = *(u8*)DB_Index(icr, symte->icr);
			*(u8*)DB_Index(icr, symte->icr) = patch;
			symte->icr = tmp;
		}
		*(u8*)DB_Index(icr, symte->icr) = patch;
		symte->icr = patch;
		symte->status = symstt_patched;
	}
}

// TODO:: check stack balance/ probably do it as a different stage between building blocks and codegen
NODE* SK_GenOpsByteCode(COMPUNIT* cu, NODE* node, DYNBUFF* icr, DYNBUFF* unresolvedprocs, u8* reservedstackspace) {
	u1 buff[256];
	NODE_2Str(cu, node, buff);
	Print("%s\n", buff);
	switch (node->type) {
		case ndtp_ops: {
			NODE* op = node + 1;
			for (u8 k = 0; k < node->opsc; k++) {
				switch (op->type) {
					case ndtp_num: {
						DB_Push(icr, &(u8){ skop_psh });
						DB_Push(icr, &(u8){ op->val });
					} break;
					case ndtp_call: {
						SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, op->callsym);
						DB_Push(icr, &(u8){ skop_psh });
						SK_TagSym(symte, icr, unresolvedprocs, op->callsym);
						switch (symte->type) {
							case symtp_proc: {
								DB_Push(icr, &(u8){ skop_call });
							} break;
							case symtp_eproc: {
								NODE* ndsignature = (NODE*)DB_Index(cu->nodes, symte->objid);
								DB_Push(icr, &(u8){ skop_ecall });
								DB_Push(icr, &(u8){ ndsignature->val }); // both argsc and retsc
							} break;
							default: {
								if (symte->type < symtp_count) {
									NODE_PrintInfo(cu, op);
									FatalError(0, "unreachable %s::%$ not implemented yet, genops call %4u:%4u",
										symtp2str[symte->type],
										cu->syms.pool->data + symte->off, (u8)symte->size,
										op->row + 1, op->col + 1);
								}
								else {
									FatalError(0, "unreachable {%u} not implemented yet, genops call", symte->type);
								}
							} break;
						}
					} break;
					case ndtp_op: {
						DB_Push(icr, &(u8){ op->val });
					} break;
					case ndtp_sym: {
						SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, op->val);
						switch (symte->type) {
							case symtp_proc:
							case symtp_eproc: {
								DB_Push(icr, &(u8){ skop_psh });
								SK_TagSym(symte, icr, unresolvedprocs, op->val);
							} break;
							case symtp_dmlay: {
								DB_Push(icr, &(u8){ skop_idx });
								SK_TagSym(symte, icr, unresolvedprocs, op->val);
							} break;
							default: {
								if (symte->type < symtp_count){
									NODE_PrintInfo(cu, op);
									FatalError(0, "unreachable %s::%$ not implemented yet, genops %4u:%4u", 
										symtp2str[symte->type],
										cu->syms.pool->data + symte->off, (u8)symte->size,
										op->row + 1, op->col+1);
								}
								else {
									FatalError(0, "unreachable {%u} not implemented yet, genops", symte->type);
								}
							} break;
						}
					} break;
					case ndtp_str:{
						SYMTE* strte = (SYMTE*)DB_Index(cu->syms.entries, op->stroff);
						DB_Push(icr, &(u8){ skop_psh });
						DB_Push(icr, &(u8){ strte->size });
						DB_Push(icr, &(u8){ skop_lea });
						SK_TagSym(strte, icr, unresolvedprocs, op->stroff);
					} break;
					case ndtp_mlay: case ndtp_loopop: case ndtp_branchop: {
						op = SK_GenOpsByteCode(cu, op, icr, unresolvedprocs, reservedstackspace)-1;
					} break;
					case ndtp_ops: {
						// TODO?? is this even reachable? i think this is a block inside of a block no? just leave it as is for now
						SK_GenOpsByteCode(cu, op, icr, unresolvedprocs, reservedstackspace);
						op = op + op->opsc;
					} break;
					default: {
						NODE_PrintInfo(cu,op);
						FatalError(0, "SK_GenBytecode:: \"%s\" not immplented (inner)\n", ndtp2str[op->type]);
					} break;
				}
				op += 1;
			}
			return op;
		} break;
		case ndtp_loopop: {
			u8 loopingaddr = DB_ElementCount(icr);
			NODE* block = SK_GenOpsByteCode(cu, node + 1, icr, unresolvedprocs, reservedstackspace);

			DB_Push(icr, &(u8){ skop_jpz });
			u8 jpzargaddr = DB_ElementCount(icr);
			DB_Push(icr, &(u8){ 0 });

			NODE* endloopop = SK_GenOpsByteCode(cu, block, icr, unresolvedprocs, reservedstackspace);

			DB_Push(icr, &(u8){ skop_jmp });
			DB_Push(icr, &(u8){ loopingaddr });
			u8 blockendaddr = DB_ElementCount(icr);
			*(u8*)DB_Index(icr, jpzargaddr) = blockendaddr;
			return endloopop;
		} break;
		case ndtp_mlay: {
			u8 maxsize =0 ;
			NODE* nd = node + 1;
			SYMTE* laysymte = (SYMTE*)DB_Index(cu->syms.entries, node->mlaymemsym);
			SK_PatchUnresolvedSym(cu, icr, laysymte, *reservedstackspace);
			
			// need to register the symbol
			for (u8 i = 0; i < node->mlaymemc << 1; i += 2) {
				NODE* memdesc = nd;
				NODE* memtype = nd+1;
				
				// Print("mlaymem desc:\n0b%br0\n%4u\n%$\n", memdesc->mlaymemf, (u8)3, memdesc->mlaymemsize, cu->syms.pool->data + symte->off, (u8)symte->size);
				// Print("type:\n%s", skops2str[nd->prim]);
				if (memdesc->mlaymemf & 0b010) { //TODO:: if the memember has a symbol so stuff
					SYMTE* memsymte = (SYMTE*)DB_Index(cu->syms.entries, memdesc->mlaymemsym);
					FatalError(0, "memlayouts with named members are not implemented yet");
				}
				else {
					maxsize += memdesc->mlaymemsize * memtype->primsize;
				}
				nd += 2;
			}
			*reservedstackspace += maxsize;
			Print("\nmaxsize:%u\n\n", maxsize);
			return nd;
		} break;
		case ndtp_branchop: {
			NODE* nxtbranch_end = node + 1; 
			u1 haselsebranch = node->branchtype == skop_else;
			u8 branchcount = node->branchc - haselsebranch;
			u8 jmpargaddr = (u8)(-1);
			u8 jpzargaddr = 0;
			do {
				NODE* ndbody = SK_GenOpsByteCode(cu, nxtbranch_end, icr, unresolvedprocs, reservedstackspace);
				DB_Push(icr, &(u8){ skop_jpz });
				jpzargaddr = DB_ElementCount(icr);
				DB_Push(icr, &(u8){ 0 });
				// fetching the start of the next condtion if any
				nxtbranch_end = SK_GenOpsByteCode(cu, ndbody, icr, unresolvedprocs, reservedstackspace);
				
				if (branchcount) {
					DB_Push(icr, &(u8){ skop_jmp });
					if (jmpargaddr == (u8)(-1)) {
						jmpargaddr = DB_ElementCount(icr);
						DB_Push(icr, &(u8){ jmpargaddr });
					}
					else {
						DB_Push(icr, &(u8){ jmpargaddr });
						jmpargaddr = DB_ElementCount(icr) - 1;
					}
				}
				*(u8*)DB_Index(icr, jpzargaddr) = DB_ElementCount(icr);
			} while (branchcount--);
			if (haselsebranch) {
				DB_Push(icr, &(u8){ skop_jmp });
				if (jmpargaddr == (u8)(-1)) {
					jmpargaddr = DB_ElementCount(icr);
					DB_Push(icr, &(u8){ jmpargaddr });
				}
				else {
					DB_Push(icr, &(u8){ jmpargaddr });
					jmpargaddr = DB_ElementCount(icr) - 1;
				}
				*(u8*)DB_Index(icr, jpzargaddr) = DB_ElementCount(icr);
				nxtbranch_end = SK_GenOpsByteCode(cu, nxtbranch_end, icr, unresolvedprocs, reservedstackspace);
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
			return nxtbranch_end;
		} break;
		default: {
			FatalError(0, "SK_GenBytecode:: \"%s\" not immplented (outer)\n", ndtp2str[node->type]);
			return 0;
		} break;
	}
	return 0;
}


// TODO:: maybe i should at a stage where i reorder some nodes and precompute stuff?
// that could come handy for the allocation of temporal variables
// either way it's not priority
// the way i handle local vars it's dumb, i need a better way
void SK_GenBytecode(SK_VMPROG* program, COMPUNIT* cu) {
	u1 buff[256];
	DYNBUFF* icr = DB_Create(256, sizeof(u8));
	DYNBUFF* loadedlibs = DB_Create(256, sizeof(HMODULE));
	program->mem = icr;
	program->loadlibs = loadedlibs;


	DYNBUFF* unresolvedsymbols = DB_Create(256, sizeof(u8));
	DB_Push(unresolvedsymbols, &(u8){ cu->epindx });
	
	u1 mainprocf = 1;
	while (unresolvedsymbols->occ){
		u8 symidx = *(u8*)DB_SoftPop(unresolvedsymbols);
		SYMTE* wsym = (SYMTE*)DB_Index(cu->syms.entries, symidx);
		
		u8 procaddr = DB_ElementCount(icr);
		Print("\ncompiling: %$:%4u\n", cu->syms.pool->data + wsym->off, (u8)wsym->size, symidx);

		switch (wsym->type) {
			case symtp_eproc:{
				NODE* procsign  = (NODE*)DB_Index(cu->nodes, wsym->objid);
				NODE* ndstrcmds = procsign + 1 + procsign->argsc + procsign->retsc;
				NODE* ndlib = ndstrcmds + 1;
				NODE* ndproc = ndlib + 1;
				
				SYMTE* strlib = DB_Index(cu->syms.entries, ndlib->stroff);
				SYMTE* strproc = DB_Index(cu->syms.entries, ndproc->stroff);
				
				u1 buff[256];
				CstrFmt(buff, "%$\0", cu->syms.pool->data + strlib->off, (u8)strlib->size);
				HMODULE lib = LoadLibraryA(buff);
				CstrFmt(buff, "%$\0", cu->syms.pool->data + strproc->off, (u8)strproc->size);
				u8 procaddr = (u8)GetProcAddress(lib, buff);
				SK_PatchUnresolvedSym(cu, icr, wsym, procaddr);
				DB_Push(loadedlibs, &(HMODULE){ lib });
			} break;
			case symtp_proc:{
				u8 resargaddr = 0;
				if (wsym->haslocals) {
					DB_Push(icr, &(u8){ skop_res });
					resargaddr = DB_ElementCount(icr);
					DB_Push(icr, &(u8){ 0 });
				}
				NODE* procsign  = (NODE*)DB_Index(cu->nodes, wsym->objid);
				NODE* procblock = procsign + 1 + procsign->argsc + procsign->retsc;
				u8 reservedstackspace = 0;
				SK_GenOpsByteCode(cu, procblock, icr, unresolvedsymbols, &reservedstackspace);
		
				if (reservedstackspace) {
					DB_Push(icr, &(u8){ skop_rel });
					DB_Push(icr, &(u8){ reservedstackspace });
					*(u8*)DB_Index(icr, resargaddr) = reservedstackspace;
				}
				if (mainprocf) {
					DB_Push(icr, &(u8){ skop_hcf });
					mainprocf = 0;
				}
				else {
					DB_Push(icr, &(u8){ skop_ret });
				}
				SK_PatchUnresolvedSym(cu, icr, wsym, procaddr);
			} break;
			case symtp_str: {
				u8 straddr = DB_ElementCount(icr);
				Print("icr:%x\n", icr->occ);
				DB_Append(icr, cu->syms.pool->data + wsym->off, wsym->size);
				Print("icr:%x\n", icr->occ);
				u8 remaining2nxtmultof8 = ((icr->occ + (u8)7) & (~(u8)7)) - icr->occ;
				memset(buff, 0, remaining2nxtmultof8);
				DB_Append(icr, buff, remaining2nxtmultof8);
				Print("icr:%x\n", icr->occ);
				SK_PatchUnresolvedSym(cu, icr, wsym, straddr);
			} break;
			case symtp_dmlay: { } break;
			default: {
				FatalError(0, "unhandle symbol type %s at code gen", symtp2str[wsym->type]);
			}
		}

	}
	DB_Free(unresolvedsymbols);
}

u4 main(u4 args, u1** vargs) {
	u1* tests[] = { 
		"strs.sk",
		// "memlayout.sk",
		// "conditionals.sk",
		// "procs.sk",
		// "while.sk",
		// "main.sk",
	};
	
	for (u8 i = 0; i < sizeof(tests)/sizeof(tests[0]);i++) {
		COMPUNIT cu;
		SK_BuildBlocks(&cu, tests[i], "main");
		
		SK_VMPROG prog;
		SK_GenBytecode(&prog, &cu);
		//TODO:: free the cu
		SK_PrintByteCode(prog.mem);
		SK_Exe(&prog);
	}
	
	return 0x45;
}