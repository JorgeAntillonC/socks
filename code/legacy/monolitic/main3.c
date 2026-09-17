#include "P:\stdwea\stdwea.h"
#include "P:\stdwea\dynbuffwea.h"
#include ".\skvm.h"

#define SKKEYWORDS \
X(if)\
X(elif)\
X(else)\
X(while)\
X(mr)\
X(mw)\
X(colon)\
X(lpar)\
X(rpar)\
X(lbra)\
X(rbra)\
X(lsqr)\
X(rsqr)\
X(null)\
X(extrn)\
X(include)\
X(using)

#define X(x) skkw_##x,
typedef enum SKKEYWORD SKKEYWORD;
enum SKKEYWORD { SKKEYWORDS skkw_count };
#undef X

#define X(x) #x,
u1* skkw2str[] =  { SKKEYWORDS };
#undef X
#undef SKKEYWORDS

// name, size in bytes, indirectionlvl
#define SKTYPES \
X(u1,  1, 0)\
X(u2,  2, 0)\
X(u4,  4, 0)\
X(u8,  8, 0)\
X(s1,  1, 0)\
X(s2,  2, 0)\
X(s4,  4, 0)\
X(s8,  8, 0)\
X(ptr, 8, 1)

#define X(x,...) sktp_##x,
typedef enum SKTYPE SKTYPE;
enum SKTYPE { SKTYPES sktp_count };
#undef X

typedef struct SKTYPEDESC SKTYPEDESC;
struct SKTYPEDESC {
	u1* name;
	u4  size;
	u4  lvl;
};

#define X(x, _size, _lvl) { .name = #x, .size = _size, . lvl = _lvl },
SKTYPEDESC sktpdesc[sktp_count] = { SKTYPES };
#undef X
#undef SKTYPES

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
X(cast)\
X(symdef)\
X(procdef)\
X(call)\
X(sign)\
X(ops)\
X(types)\
X(mlay)\
X(mlaymem)\
X(bland)\
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
		struct {
			u1 vtp; // virtual type
			u1 rsz; // real size
		};
		struct {
			u1 primsize; // if zero it's a compound type aka structs, if -1 then it's a proc 
			u1 primlvl;
		};
	};
	union {
		u8 val;         // lite ral int value, symbol index, random bs
		u8 stroff;
		u8 prim; // the type of types lol
		struct {
			union {
				u4 opsc;
				u4 cmdstrc;
				u4 mlaymemc;   // for mem layout
				u4 argsc;
				u4 mlaymemsize; // for mem layout arrays
				u4 branchc;
				u4 op;
			};
			union {
				u4 mlaysym;
				u4 mlaymemsym; // for mem layout member
				u4 retsc;
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
X(smlay)\
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
	u4 haslocals;
	
	u4 scopeid;
	u4 objid;  // proc index, mem index, etc index nodepool
	
	u8 off;    // symtable's symbol name pool
	u8 icr;    // proc index, mem index, etc index "final icr addr
};

typedef struct TABLE TABLE;
struct TABLE {
	DYNBUFF* pool;
	DYNBUFF* entries;
};

typedef struct COMPUNIT COMPUNIT;
struct COMPUNIT {
	DYNBUFF* nodes;
	DYNBUFF* errpool;
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

#define DEBUGMODES \
X(nodebug,    0x0000) \
X(nodeinf,    0x0001) \
X(parser,     0x0010) \
X(checker,    0x0020) \
X(vmcodegen,  0x0040) \
X(x64codegen, 0x0080) \
X(runtests,   0x8000)

#define X(x,...) #x,
u1* dbmd2str[] = { DEBUGMODES };
#undef X

#define X(x, v) dbmd_##x = v,
typedef enum SKDEBUGMODE SKDEBUGMODE;
enum SKDEBUGMODE { DEBUGMODES dbmd_count = sizeof(dbmd2str) / sizeof(u1*) };
#undef X
#undef DEBUGMODES

#define __fmtsym(_cu, _symte) (_cu->syms.pool->data + _symte->off), (u8)_symte->size

u8 SYMT_Hash(u1* sym, u4 size) {
	return 0;
}

u8 SYMT_Index(COMPUNIT* cu, u1* sym, u4 size) {
	u8 i = 0;
	u8 symtableentrycount = DB_ElementCount(cu->syms.entries);
	while (i < symtableentrycount) {
		SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, i); // returns null if there's not entry was found
		if (symte->size == size && CstrCmpS(sym, __fmtsym(cu, symte))) { return i; }
		i++;
	}
	SYMTE tmp = (SYMTE){ .type = symtp_undef, .off = DB_ElementCount(cu->syms.pool), .size = size, .objid = i };
	DB_Push(cu->syms.entries, &tmp);
	DB_Append(cu->syms.pool, sym, size);
	return i;
}

// TODO:: fix string comparation use the length of both strings
void CU_NxtNode(COMPUNIT* cu) {
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
		
		if (CstrCmpS("oogabuga", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_op;
			cnode->op = skvm_op_oogabuga;
			return;
		}
		if (CstrCmpS("dmp", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_op;
			cnode->op = skvm_op_dmp;
			return;
		}
		if (CstrCmpS("null", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_num;
			cnode->vtp = sktp_ptr;
			cnode->rsz = 1;
			cnode->val = 0;
			return;
		}
		
		for (u4 i = 0; i < sktp_count; i++) {
			if (CstrCmpS(sktpdesc[i].name, cu->scoff, cu->nodelen)) {
				cnode->type = ndtp_type;
				cnode->prim = i;
				cnode->primsize = sktpdesc[i].size;
				cnode->primlvl  = sktpdesc[i].lvl;
				return;
			}
		}

		if (CstrCmpS("if", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skkw_if;
			return;
		}
		if (CstrCmpS("elif", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skkw_elif;
			return;
		}
		if (CstrCmpS("else", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skkw_else;
			return;
		}
		if (CstrCmpS("while", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_flow;
			cnode->val = skkw_while;
			return;
		}
		if (CstrCmpS("extrn", cu->scoff, cu->nodelen)) {
			cnode->type = ndtp_cmd;
			cnode->cmdtp  = skkw_extrn;
			return;
		}
		cnode->type = ndtp_sym;
		cnode->val = SYMT_Index(cu, cu->scoff, cu->nodelen);
		return;
	}
	if (CharIsNum(*cu->scoff)) {
		cnode->val = CstrParseNum(cu->scoff, &cu->nodelen);
		cnode->type = ndtp_num;
		cnode->vtp = sktp_s4;
		if (cnode->val & ~(0xFFFFFFFFULL)) { cnode->rsz = 8; cnode->vtp = sktp_s8; }
		else if (cnode->val & ~(0xFFFFULL)) { cnode->rsz = 4; }
		else if (cnode->val & ~(0xFFULL)) { cnode->rsz = 2; }
		else { cnode->rsz = 1; }
		return;
	}
	
	if (CstrCmpS(">>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_shr;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("<<", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_shl;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("*>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_dup;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_rtf;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("<@", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_rtb;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("'>", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_leap;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("><", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_swap;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("//", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_drop;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@1", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mw1;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@2", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mw2;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@4", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mw4;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("@8", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mw8;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!1", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mr1;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!2", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mr2;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!4", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mr4;
		cu->nodelen = 2;
		return;
	}
	if (CstrCmpS("!8", cu->scoff, 2)) {
		cnode->type = ndtp_op;
		cnode->val = skvm_op_mr8;
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
						case 'r': {
							*bh++ = '\r';
						} break;
						case '0': {
							// TODO:: make restrictions so it only parses at most 3 digits and those have a maximum value of 0o377
							nodeend++;
							if (CharIsNum(*nodeend)) {
								u1 v = 0;
								while (CharIsNum(*nodeend)) {
									u1 c = *nodeend++;
									v = (v << 3) + c - '0';
								}
								*bh++ = v;
								nodeend--;
							}
							else {
								nodeend--;
								*bh++ = '\0';
							}
						} break;
						case 'x': { // TODO:: make restrictions so it only parses maximum 2 digits
							nodeend++;
							u1 v = 0;
							while (CharIsNumX(*nodeend)) {
								u1 c = *nodeend++;
								if ((u1)((c & 0xDF) - 'A') < 6) { c = (c & 0xDF) - 7; }
								v = (v << 4) + c - '0';
							}
							*bh++ = v;
							nodeend--;
						} break;
						case '\\': {
							*bh++ = '\\';
						} break;
						default: {
							FatalError(0, "scapecode \"%c\" is not implementeyet", *nodeend);
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
				FatalError(0, "TK_Nxt:: premature eof while defining string literal at %s:%4u:%4u\n", *cu->scoff, cu->sourcefile, cnode->row + 1, cnode->col + 1);
			}
		} return;
		case '-':  {
			if (CharIsNum(*(cu->scoff + 1))) {
				cnode->type = ndtp_num;
				cnode->val = CstrParseNum(cu->scoff+1, &cu->nodelen);
				cnode->vtp = sktp_s4;
				if (cnode->val & ~(0xFFFFFFFFULL)) { cnode->rsz = 8; cnode->vtp = sktp_s8; }
				else if (cnode->val & ~(0xFFFFULL)) { cnode->rsz = 4; }
				else if (cnode->val & ~(0xFFULL)) { cnode->rsz = 2; }
				else { cnode->rsz = 1; }
				cnode->val = -cnode->val;
				cu->nodelen += 1;
			}
			else {
				cu->nodelen = 1; 
				cnode->type = ndtp_op;
				cnode->op  = skvm_op_sub;
			}
		} return;
		case '@':  { cu->nodelen = 1; cnode->type = ndtp_cmd;   cnode->cmdtp = skkw_mw;    } return;
		case '!':  { cu->nodelen = 1; cnode->type = ndtp_cmd;   cnode->cmdtp = skkw_mr;    } return;
		case '(':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_lpar;  } return;
		case ')':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_rpar;  } return;
		case '[':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_lsqr;  } return;
		case ']':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_rsqr;  } return;
		case '{':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_lbra;  } return;
		case '}':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_rbra;  } return;
		case ':':  { cu->nodelen = 1; cnode->type = ndtp_delim; cnode->val   = skkw_colon; } return;
		case '+':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_add;   } return;
		case '*':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_mlt;   } return;
		case '/':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_div;   } return;
		case '%':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_mod;   } return;
		case '&':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_and;   } return;
		case '^':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_xor;   } return;
		case '=':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_eq;    } return;
		case '<':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_lt;    } return;
		case '>':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_gt;    } return;
		case '#':  { cu->nodelen = 1; cnode->type = ndtp_op;    cnode->op    = skvm_op_call;  } return;
		case '\0': { cu->nodelen = 0; cnode->type = ndtp_eof;   cnode->val   = skvm_op_hcf;   } return;
		default: { FatalError(0, "TK_Nxt:: unhandled character { %c } at %s:%4u:%4u\n", *cu->scoff, cu->sourcefile, cnode->row+1, cnode->col+1); }
	}
	return;
}

inline static u8 NODE_2Str(COMPUNIT* cu, NODE* node, u1* buff) {
	switch (node->type) {
		case ndtp_str: {
			SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, node->val);
			u1 strbuff[512];
			CstrFromRawBytes(cu->syms.pool->data + symte->off, symte->size, strbuff, 512);
			return CstrFmt(buff, "(%s:%s)", ndtp2str[node->type], strbuff);
		} break;
		case ndtp_symdef:
		case ndtp_procdef:
		case ndtp_sym: {
			SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, node->val);
			return CstrFmt(buff, "(%s:%$)", ndtp2str[node->type], __fmtsym(cu, symte));
		} break;
		case ndtp_call: {
			SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, node->callsym);
			return CstrFmt(buff, "(%s:%$)", ndtp2str[node->type], __fmtsym(cu, symte));
		} break;
		case ndtp_num:{
			if (sktp_s1 <= node->vtp && node->vtp <= sktp_s8) {
				return CstrFmt(buff, "(%s:%i)", ndtp2str[node->type], node->val);
			}
			else {
				return CstrFmt(buff, "(%s:%u)", ndtp2str[node->type], node->val);
			}
		}
		case ndtp_sign: {
			return CstrFmt(buff, "(%s:%4u %4u)", ndtp2str[node->type], node->argsc, node->retsc);
		}
		case ndtp_mlay: {
			SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, node->mlaysym);
			return CstrFmt(buff, "(%s:%$ %u)", ndtp2str[node->type], __fmtsym(cu, symte), (u8)node->mlaymemc);
		}
		case ndtp_mlaymem: {
			SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, node->mlaymemsym);
			return CstrFmt(buff, "(%s:%u %$ %1br0)", ndtp2str[node->type], node->mlaymemsize, __fmtsym(cu, symte), node->mlaymemf, (u8)3);
		}
		case ndtp_ops: {
			return CstrFmt(buff, "(%s:%u)", ndtp2str[node->type], node->opsc);
		}
		case ndtp_types: {
			return CstrFmt(buff, "(%s:%4u %4u)", ndtp2str[node->type], node->argsc, node->retsc);
		}
		case ndtp_cast:
		case ndtp_type: {
			return CstrFmt(buff, "(%s:%s %1u)", ndtp2str[node->type], sktpdesc[node->prim].name, node->primlvl);
		}
		case ndtp_cmd:
		case ndtp_cmdstrs: {
			return CstrFmt(buff, "(%s:%s %u)", ndtp2str[node->type], skkw2str[node->cmdtp], node->cmdstrc);
		}
		case ndtp_op: {
			return CstrFmt(buff, "(%s:%s)", ndtp2str[node->type], skvm_opdesc[node->op].name);
		}
		case ndtp_flow:
		case ndtp_delim: {
			return CstrFmt(buff, "(%s:%s)", ndtp2str[node->type], skkw2str[node->val]);
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
	Print("%s at %s:%u:%u\n", buff, cu->sourcefile, (u8)node->row + 1, (u8)node->col + 1);
}

static void SK_ErrorAtNode(COMPUNIT* cu, NODE* node, u1* errmsg, ...) {
	NODE* cnode = node;
	u1 errbuff[1024];
	u4 errlen;
	if (errmsg) {
		VAR_ARG(errmsg, stack);
		CstrFmtS(errbuff + 512, errmsg, stack);
		errlen = CstrFmt(errbuff, "ERROR::%s at %s:%2u:%2u\n", errbuff + 512, cu->sourcefile, cnode->row + 1, cnode->col + 1);
	}
	else {
		NODE_2Str(cu, node, errbuff + 512);
		errlen = CstrFmt(errbuff, "ERROR::rogue %s at %s:%2u:%2u\n", errbuff + 512, cu->sourcefile, cnode->row + 1, cnode->col + 1);
	}
	DB_Append(cu->errpool, errbuff, errlen);
	cu->errorcount++;
}

/*

**: empty
x, : 'or could be' x
x? : 1 or 0 of x
x* : '0 or more of x'
x+ : '1 or more of x'
\x : literal x
(x): enclosure for 'counters'
counter: *, ?, +

program:    program proc, program smemlay, **
smemlay:    sym : [ num type ]
proc:       procdef block, procdef extrn
procdef:    sym : types
types:      \( args : rets \)
args:       type*
rets:       type*
type:       u1, u2, u4, u8, s1, s2, s4, s8, ptr
block:      { ops }
ops:        op*
op:         literal, symop, callop, stackop, memop, binop, intrinsic, branchop, loopop, dmemlay, cast
cast:       \( type \)
literal:    num, string
symop:      sym
callop:     sym \#
stackop:    \*>, ><, '>, @>, <@, //
memop:      !(1,2,4,8), @(1,2,4,8)
binop:      \+, -, <, >, &, <<, >>, ^
intrinsic:  dmp
branchop:   if ops block (elif ops block)* (else block)?
loopop:     while ops block
dmemlay:    sym : { num? type }
extrn:      extrn \( string string \)
*/

inline static void SK_OpenNewContext(COMPUNIT* cu, DYNBUFF* syntaxstack, DYNBUFF* contxtstack, NODE* cnode, u8 contxttype) {
	cnode->type = contxttype;
	cnode->val = 0;
	cnode->ext = 0;
	DB_Push(syntaxstack, cnode);
	DB_Push(contxtstack, &(u8){ DB_ElementCount(cu->nodes) });
	DB_Push(cu->nodes, cnode);
}

void SK_NodeWeber(COMPUNIT* cu, u1* sourcefile, u1* entrypoint, const u2 debugmode) {
	memset(cu, 0, sizeof(COMPUNIT));
	cu->entrypoint = entrypoint;
	cu->epindx = (u8)(-1);
	cu->sourcefile = sourcefile;
	cu->sourcecode = LoadFile(sourcefile);
	cu->scoff = cu->sourcecode;
	
	if (debugmode & dbmd_parser) {
		Print("\n\n===========source==========\n\n");
		u4 sourcedoesize = CstrLen(cu->sourcecode);
		FS_Write(STDOUT, cu->sourcecode, sourcedoesize);
		Print("\n\n===========================\n\n");
	}

	cu->syms = (TABLE){ .entries = DB_Create(256, sizeof(SYMTE)), .pool = DB_Create(256, sizeof(u1)) };
	cu->nodes = DB_Create(256, sizeof(NODE));
	cu->errpool = DB_Create(256, sizeof(u1));
	cu->errorcount = 0;
	SYMTE* nullsymte = (SYMTE*)DB_Index(cu->syms.entries, SYMT_Index(cu, "null", 4));
	nullsymte->type = symtp_str;

	DYNBUFF* syntaxstack = DB_Create(256, sizeof(NODE));
	DYNBUFF* contxtstack = DB_Create(256, sizeof(u8));
	DYNBUFF* seensymbols = DB_Create(256, sizeof(u8));

	NODE* cnode;
	u1 buff[256];
	u1 haslocals = 0;
	
	if (debugmode & dbmd_parser) Print("\n\n===========parsing===========\n\n");
	do {
		CU_NxtNode(cu);
		cnode = &cu->cnode;
		
	skp:if (debugmode & dbmd_parser) {
			Print("next node:\n");
			NODE_PrintInfo(cu,cnode);
		}

		switch (cnode->type) {
			case ndtp_sym:{
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				
				DB_Push(syntaxstack, cnode);
				NODE* sym = (NODE*)DB_Peek(syntaxstack, 1);
				SYMTE* symte = DB_Index(cu->syms.entries, sym->val);
				if (symte->type == symtp_undef || symte->type == symtp_seen) {
					symte->objid = DB_ElementCount(cu->nodes);
				}
				
				CU_NxtNode(cu);
				
				if (syntaxstacksize) {
					switch (topnode->type) {
						case ndtp_ops: {
							if (cnode->type == ndtp_delim && cnode->val == skkw_colon) {
								if (symte->type == symtp_undef) {
									symte->type = symtp_seen;
									sym->type = ndtp_symdef;
								}
								else {
									SK_ErrorAtNode(cu, cnode, "redefinition of %$", __fmtsym(cu, symte));
								}
							}
							else if (cnode->type == ndtp_op  && cnode->op == skvm_op_call) {
								sym->type = ndtp_call;
								sym->callsym = sym->val;
								DB_Push(cu->nodes, sym);
								NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
								ndops->opsc += 1;
								syntaxstack->occ -= sizeof(NODE);
							}
							else {
								DB_Push(cu->nodes, sym);
								NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
								ndops->opsc += 1;
								syntaxstack->occ -= sizeof(NODE);
								goto skp;
							}
						} break;
						// TODO:: implement these for named mem layout members
						default: {
							SYMTE* symte = DB_Index(cu->syms.entries,cnode->val);
							SK_ErrorAtNode(cu, cnode, 0);
							syntaxstack->occ -= sizeof(NODE);
						} goto skp;
					}
				}
				else if (cnode->type == ndtp_delim && cnode->val == skkw_colon) {
						SYMTE* symte = DB_Index(cu->syms.entries, sym->val);
						if (symte->type == symtp_undef) {
							symte->type = symtp_seen;
							sym->type = ndtp_symdef;
						}
						else {
							SK_ErrorAtNode(cu, cnode, "redefinition of %$", __fmtsym(cu, symte));
						}
					}
				else {
					SYMTE* symte = DB_Index(cu->syms.entries,cnode->val);
					SK_ErrorAtNode(cu, cnode, 0);
					syntaxstack->occ -= sizeof(NODE);
					goto skp;
				}
			} break;
			case ndtp_delim: {
				switch (cnode->val) {
					case skkw_colon: {
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						if (topnode && topnode->type == ndtp_types) {
							NODE* ndtypes = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							// TODO:: Error report duplicates ":" 
							ndtypes->argsretsf = 1;
						}
						else if (topnode && topnode->type == ndtp_ops) {
							NODE tmp;
							memcpy(&tmp, cnode, sizeof(NODE));
							CU_NxtNode(cu);	
							if (cnode->type == ndtp_type) {
								((NODE*)DB_Index(cu->nodes,*(u8*)DB_Peek(contxtstack, 1)))->opsc++;
								cnode->type = ndtp_cast;
								DB_Push(cu->nodes, cnode);
							}
							else {
								SK_ErrorAtNode(cu, &tmp, 0);
								goto skp;
							}
						}
						else {
							SK_ErrorAtNode(cu, cnode, 0);
						}
					} break;
					case skkw_lpar: {
						// command( -> command, symdef(->procdef
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
									SK_ErrorAtNode(cu, cnode, 0);
								} break;
							}
						}
						else {
							SK_ErrorAtNode(cu, cnode, 0);
						}
					} break;
					case skkw_lsqr: {
						// symdef [
						// TODO IMPLEMENT THIS PROPERLY
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						if (topnode && topnode->type == ndtp_symdef) {
							syntaxstack->occ -= sizeof(NODE);
							u4 symv = topnode->val;
							SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_mlay);
							NODE* ndmemlay = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, symv);
							if (debugmode & dbmd_parser) Print("%$\n", __fmtsym(cu, symte));
							ndmemlay->mlaysym = symv;
							topnode->mlaysym = symv;
						}
						else {
							SK_ErrorAtNode(cu, cnode, 0);
						}
					} break;
					case skkw_rsqr: {
						u8 syntaxstacksize = DB_ElementCount(syntaxstack);
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						if (topnode->type == ndtp_mlay) {
							NODE* mlaymemdesc = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));

							if (debugmode & dbmd_parser) {
								Print("mlaymem %4u:\n", mlaymemdesc->mlaymemc);
								for (u8 i = 0; i < mlaymemdesc->mlaymemc << 1; i += 2) {
									NODE_2Str(cu, mlaymemdesc + i + 1, buff);
									Print("%s", buff);
									NODE_2Str(cu, mlaymemdesc + i + 2, buff);
									Print("%s", buff);
								}
							}

							SYMTE* symentry = DB_Index(cu->syms.entries, mlaymemdesc->mlaysym);
							symentry->type = symtp_smlay;
							symentry->status = symstt_unpatched;
							symentry->objid = *(u8*)DB_Peek(contxtstack, 1);
							symentry->icr = 0;

							syntaxstack->occ -= sizeof(NODE);
							contxtstack->occ -= sizeof(u8);
						}
					} break;
					case skkw_rpar: {
						NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
						u8 syntaxstacksize = DB_ElementCount(syntaxstack);
						if (syntaxstacksize >= 1 && topnode->type == ndtp_types) {
							if (debugmode & dbmd_parser) {
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
							}
							// symdef types -> procdef
							syntaxstack->occ -= sizeof(NODE);
							(topnode-1)->type = ndtp_procdef;
						}
						else if (syntaxstacksize >= 1 && topnode->type == ndtp_cmdstrs) {
							NODE* ndcmdstrs = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
							if (debugmode & dbmd_parser) {
								Print("cmds %4u:\n", ndcmdstrs->cmdstrc);
								for (u8 i = 0; i < ndcmdstrs->cmdstrc; ++i) {
									NODE_2Str(cu, ndcmdstrs + 1 + i, buff);
									Print("%s", buff);
								}
							}
							SYMTE* symentry = DB_Index(cu->syms.entries, (topnode - 1)->val);
							symentry->type = symtp_eproc;
							symentry->status = symstt_unpatched;
							symentry->objid = *(u8*)DB_Peek(contxtstack, 2);
							symentry->icr = 0;
							
							if (CstrCmpS(cu->entrypoint, __fmtsym(cu, symentry))) {
								cu->epindx = (topnode - 1)->val;
							}
							syntaxstack->occ -= 2 * sizeof(NODE);
							contxtstack->occ -= 2 * sizeof(u8);
						}
						else {
							SK_ErrorAtNode(cu, cnode, 0);
						}
					} break;
					case skkw_lbra: {
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
							SK_ErrorAtNode(cu, cnode, 0);
						}
					} break;
					case skkw_rbra: {
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
								if (debugmode & dbmd_parser) {
									Print("ops %4u:\n", ndops->opsc);
									for (u8 i = 0; i < ndops->opsc; ++i) {
										NODE_2Str(cu, ndops + 1 + i, buff);
										Print("%s", buff);
									}
								}
								
								if (syntaxstacksize >= 2 && (topnode - 1)->type == ndtp_procdef) {
									SYMTE* symte = DB_Index(cu->syms.entries, (topnode-1)->val);
									symte->type = symtp_proc;
									symte->status = symstt_unpatched;
									symte->objid = *(u8*)DB_Peek(contxtstack, 2);
									symte->icr = 0;
									symte->haslocals = haslocals;
									haslocals = 0;
									
									if (CstrCmpS(cu->entrypoint, __fmtsym(cu, symte))) {
										cu->epindx = (topnode - 1)->val;
									}
									syntaxstack->occ -= 2 * sizeof(NODE);
									contxtstack->occ -= 2 * sizeof(u8);
								}
								else if (syntaxstacksize >= 3 && (topnode - 1)->type == ndtp_loopop) {
									NODE* ndloop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
									if (!ndloop->loopf) {
										// TODO:: check SK_ErrorAtNode's todo
										SK_ErrorAtNode(cu, ndloop, "missing \"{\" after while condition");
									}
									syntaxstack->occ -= 2 * sizeof(NODE);
									contxtstack->occ -= 2 * sizeof(u8);
								}
								else if (syntaxstacksize >= 3 && (topnode - 1)->type == ndtp_branchop) {
									NODE* ndbranchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 2));
									if (!ndbranchop->branchf) {
										// TODO:: check SK_ErrorAtNode's todo
										SK_ErrorAtNode(cu, ndbranchop, "missing \"{\" after %s condition", skkw2str[ndbranchop->branchtype]);
									}
									// ops branchop
									switch (ndbranchop->branchtype) {
										case skkw_if:
										case skkw_elif:{
											syntaxstack->occ -= 1 * sizeof(NODE);
											contxtstack->occ -= 1 * sizeof(u8);
											CU_NxtNode(cu);
											if (cnode->type == ndtp_flow && (cnode->val == skkw_else || cnode->val == skkw_elif)) {
												ndbranchop->branchc++;
												ndbranchop->branchf = cnode->val == skkw_else;
												ndbranchop->branchtype = cnode->val;
												if (cnode->val == skkw_elif) { SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops); }
												CU_NxtNode(cu);
											}
											else {
												ndbranchop->branchf = 2;
												syntaxstack->occ -= 1 * sizeof(NODE);
												contxtstack->occ -= 1 * sizeof(u8);
											}
											goto skp;
										} break;
										case skkw_else: {
											// merge ops branchop ops
											ndbranchop->branchf = 2;
											syntaxstack->occ -= 2 * sizeof(NODE);
											contxtstack->occ -= 2 * sizeof(u8);
										} break;
										default: {
											FatalError(0, "unknow branch type %4u", ndbranchop->branchtype);
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
									
									if (debugmode & dbmd_parser) {
										Print("mlaymem %4u:\n", mlaymemdesc->mlaymemc);
										for (u8 i = 0; i < mlaymemdesc->mlaymemc << 1; i += 2) {
											NODE_2Str(cu, mlaymemdesc + i + 1, buff);
											Print("%s", buff);
											NODE_2Str(cu, mlaymemdesc + i + 2, buff);
											Print("%s", buff);
										}
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
								SK_ErrorAtNode(cu, cnode, 0);
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
					SK_ErrorAtNode(cu, cnode, 0);
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
					SK_ErrorAtNode(cu, cnode,  0);
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
				else {
					SK_ErrorAtNode(cu, cnode, 0);
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
				else {
					SYMTE* strte = (SYMTE*)DB_Index(cu->syms.entries, cnode->stroff);
					SK_ErrorAtNode(cu, cnode, 0);
				}
			} break;
			case ndtp_flow: {
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);
				switch (cnode->val) {
				case skkw_while: {
					if (syntaxstacksize && topnode->type == ndtp_ops) {
						NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
						ndops->opsc += 1;
						SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_loopop);
						SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
						NODE* ndloopop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					}
					else {
						SK_ErrorAtNode(cu, cnode, 0);
					}
				} break;
				case skkw_if: {
					if (syntaxstacksize && topnode->type == ndtp_ops) {
						NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
						ndops->opsc += 1;
						SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_branchop);
						NODE* branchop = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
						branchop->branchtype = skkw_if;
						SK_OpenNewContext(cu, syntaxstack, contxtstack, cnode, ndtp_ops);
					}
					else {
						SK_ErrorAtNode(cu, cnode, 0);
					}
				} break;
				default: {
					SK_ErrorAtNode(cu, cnode, 0);
				} break;
				}
			} break;
			case ndtp_cmd: {
				NODE* topnode = (NODE*)DB_Peek(syntaxstack, 1);
				u8 syntaxstacksize = DB_ElementCount(syntaxstack);

				if (syntaxstacksize && topnode->type == ndtp_ops && (cnode->cmdtp == skkw_mw || cnode->cmdtp == skkw_mr)) {
					DB_Push(cu->nodes, cnode);
					NODE* ndops = (NODE*)DB_Index(cu->nodes, *(u8*)DB_Peek(contxtstack, 1));
					ndops->opsc++;
				}
				else if (cnode->cmdtp != skkw_mw && cnode->cmdtp != skkw_mr) {
					DB_Push(syntaxstack, cnode);
				}
				else {
					SK_ErrorAtNode(cu, cnode, 0);
				}
			} break;
			case ndtp_eof: {
				if (DB_ElementCount(syntaxstack) > 0) {
					SK_ErrorAtNode(cu, cnode, "premature eof; %u nodes left on the syntax stack", DB_ElementCount(syntaxstack));
				}
				if (DB_ElementCount(contxtstack) > 0) {
					SK_ErrorAtNode(cu, cnode, "premature eof; %u nodes left on the conxtext stack", DB_ElementCount(contxtstack));
				}
			} break;
			default: {
				FatalError(0, "SK_BuildBlocks:: unhandled symbol { %s:\"%$\" }", ndtp2str[cnode->type], cu->scoff, (u8)cu->nodelen);
			} break;
		}
		if (debugmode & dbmd_parser) {
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
		}
	} while (cnode->type != ndtp_eof);

	if (cu->epindx == (u8)(-1)) {
		SK_ErrorAtNode(cu, cnode, "entry point \"%s\" was not found", cu->entrypoint);
	}
	if (debugmode & dbmd_parser) {
		Print("main proc is located at: %u\n", cu->epindx);
		Print("found symbols list:\n");
	}
	SYMTE* symte = DB_Index(cu->syms.entries, 0);
	u4 registeredsymbols = DB_ElementCount(cu->syms.entries);
	for (u4 i = 0; i < registeredsymbols; i++) {
		if (debugmode & dbmd_parser) Print("{ %$:%s }\n", __fmtsym(cu, symte), symtp2str[symte->type]);
		if (symte->type == symtp_undef || symte->type == symtp_seen){
			SK_ErrorAtNode(cu, DB_Index(cu->nodes, symte->objid), "{ %$ } was used but never defined", __fmtsym(cu, symte));
		}
		symte++;
	}


	Free(cu->sourcecode);
	cu->sourcecode = 0;
	DB_Free(seensymbols);
	DB_Free(syntaxstack);
	DB_Free(contxtstack);
}

inline static u1 SK_IsTypeSigned(u8 t) { return sktp_s1 <= t && t <= sktp_s8;  }
inline static u1 SK_IsTypeUnsigned(u8 t) { return sktp_u1 <= t && t <= sktp_u8;  }

// TODO:: return different values to know where it failed
u1 SK_CompatibleTypes(NODE* scrtype, NODE* tgtype, u1 tglowerlvl) {
	if (scrtype->prim == tgtype->prim && scrtype->primlvl == tgtype->primlvl) return 1;
	if (tgtype->prim == sktp_ptr) return scrtype->primlvl > 0;
	else {
		if (scrtype->primlvl != tgtype->primlvl - tglowerlvl) return 0;
		if (scrtype->primsize > tgtype->primsize) return 0;
		if (SK_IsTypeSigned(scrtype->prim) != SK_IsTypeSigned(tgtype->prim)) return 0;
		return 1;
	}
}

inline static void SK_CheckTypeArithmetic(NODE* a, NODE* b) {
	if (SK_IsTypeSigned(a->prim) == SK_IsTypeSigned(b->prim)) { // share the same sign
		if (a->primsize < b->primsize) {
			a->prim = b->prim;
			a->primsize = b->primsize;
		}
	}
	else { // don't share the same sign
		u8 prim = a->prim;
		u1 primsize = a->primsize;
		if (primsize <= b->primsize) {
			prim = b->prim;
			primsize = b->primsize;
			if (a->primsize == b->primsize && SK_IsTypeSigned(b->prim)) {
				prim = a->prim;
			}
			a->prim = prim;
			a->primsize = primsize;
		}
	}
}

inline static void SK_CheckBody(COMPUNIT* cu, NODE* ndobj, DYNBUFF* vstack, DYNBUFF* stacksnapshot, u1* bodytype, const u2 debugmode) {
	u4 ogvstacksize = DB_ElementCount(stacksnapshot);
	u4 vstacksize = DB_ElementCount(vstack);
	if (vstacksize != ogvstacksize) {
		if (vstacksize < ogvstacksize) {
			SK_ErrorAtNode(cu, ndobj, "%s's body underflow: expected exactly %4u element%c on top, but got %4u fewer", bodytype, ogvstacksize, ogvstacksize - vstacksize > 1 ? 's' : '\0', ogvstacksize - vstacksize);
			vstack->occ = 0;
			DB_Extend(vstack, stacksnapshot);
		}
		else if (vstacksize > ogvstacksize) {
			SK_ErrorAtNode(cu, ndobj, "%s's body overflow: expected exactly %4u element%c on top, but got %4u more", bodytype, ogvstacksize, vstacksize - ogvstacksize > 1 ? 's' : '\0', vstacksize - ogvstacksize);
			vstack->occ = 0;
			DB_Extend(vstack, stacksnapshot);
		}
	}
	else {
		NODE* vstackbase = (NODE*)DB_Index(vstack, 0);
		NODE* snapshotbase = (NODE*)DB_Index(stacksnapshot, 0);
		u1 mismatchestack = 0;
		for (u4 i = 0; i < ogvstacksize; i++) {
			if (debugmode & dbmd_checker) {
				NODE_PrintInfo(cu, snapshotbase);
				NODE_PrintInfo(cu, vstackbase);
			}
			if (!SK_CompatibleTypes(snapshotbase, vstackbase, 0)) {
				//TODO:: better error report
				SK_ErrorAtNode(cu, vstackbase, "missmatched stack entry");
				mismatchestack = 1;
			}
			snapshotbase++;
			vstackbase++;
		}
		if (mismatchestack) {
			vstack->occ = 0;
			DB_Extend(vstack, stacksnapshot);
		}
	}
}

inline static void SK_CheckCondition(COMPUNIT* cu, NODE* ndobj, DYNBUFF* vstack, DYNBUFF* stacksnapshot, u1* conditiontype, const u2 debugmode) {
	u4 vstacksize = DB_ElementCount(vstack);
	u4 ogvstacksize = DB_ElementCount(stacksnapshot);
	if (vstacksize < ogvstacksize + 1) {
		SK_ErrorAtNode(cu, ndobj, "condition underflow: expected exactly one extra value on top, but got %u fewer", (ogvstacksize + 1) - vstacksize);
		vstack->occ = 0;
		DB_Extend(vstack, stacksnapshot);
		vstack->occ += sizeof(NODE);
	}
	else if (vstacksize > ogvstacksize + 1) {
		SK_ErrorAtNode(cu, ndobj, "condition overflow: expected exactly one extra value on top, but got %u more", vstacksize - (ogvstacksize + 1));
		vstack->occ = 0;
		DB_Extend(vstack, stacksnapshot);
		vstack->occ += sizeof(NODE);
	}
	vstack->occ -= sizeof(NODE);

	NODE* vstackbase = (NODE*)DB_Index(vstack, 0);
	NODE* snapshotbase = (NODE*)DB_Index(stacksnapshot, 0);
	u1 mismatchestack = 0;
	for (u4 i = 0; i < ogvstacksize; i++) {
		if (debugmode & dbmd_checker) {
			NODE_PrintInfo(cu, snapshotbase);
			NODE_PrintInfo(cu, vstackbase);
		}
		if (!SK_CompatibleTypes(snapshotbase, vstackbase, 0)) {
			//TODO:: better error report
			SK_ErrorAtNode(cu, vstackbase, "missmatched stack entry in %s's condition", conditiontype);
			mismatchestack = 1;
		}
		snapshotbase++;
		vstackbase++;
	}
	if (mismatchestack) {
		vstack->occ = 0;
		DB_Extend(vstack, stacksnapshot);
	}
}

inline static u4 SK_PrinSize2Off(u4 primsize) {
	u4 r = 0;
	switch (primsize) {
		case 1: return 1;
		case 2: return 2;
		case 4: return 3;
		case 8: return 4;
		default: FatalError(0, "invalid size for memory op");
	}
}

// TODO:: use permanent stacks, don't use this weird creating dynbuffs all the time, that's wasteful
NODE* SK_CheckOp(COMPUNIT* cu, NODE* cnode, DYNBUFF* vstack, u1* errbuff, const u2 debugmode) {
	if (debugmode & dbmd_checker) {
		Print("\nnext node:");
		NODE_PrintInfo(cu, cnode);
	}
	switch (cnode->type) {
		case ndtp_ops: {
			NODE* ndop = cnode + 1;
			for (u8 i = 0; i < cnode->opsc; i++) {
				if (debugmode & dbmd_checker) {
					Print("\nnext node:");
					NODE_PrintInfo(cu, ndop);
				}
				switch (ndop->type) {
					case ndtp_op: {
						if (DB_ElementCount(vstack) >= skvm_opdesc[ndop->op].argc) {
							// TODO:: at language level i bevieve i dont' need to make a diffecerence between !n and @n,
							// and i could just create syntactic sugar and only write ! and @, and i should infer the size from what's on the stack
							// maybe i could left !n and @n, for a more strict user
							switch (ndop->op) {
								case skvm_op_oogabuga: {
									u4 vstackec = DB_ElementCount(vstack);
									Print("oogabuga!!\n");
									for (u4 i = 0; i < vstackec; i++) {
										NODE_PrintInfo(cu, DB_Index(vstack, i));
									}
									Print("\n");
								} break;
								case skvm_op_add: {
									NODE* b = (NODE*)DB_SoftPop(vstack);
									NODE* a = (NODE*)DB_Peek(vstack, 1);
									if (a->primlvl && b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "you can't add 2 pointers");
										a->prim = sktp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else if (a->primlvl) { /* we are done */ }
									else if (b->primlvl) {
										memcpy(a, b, sizeof(NODE));
									}
									else {
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								case skvm_op_sub: {
									NODE* b = (NODE*)DB_SoftPop(vstack);
									NODE* a = (NODE*)DB_Peek(vstack, 1);
									if (a->primlvl && b->primlvl) {
										a->prim = sktp_u8;
										a->primlvl = 0;
										a->primsize = 8;
									}
									else if (a->primlvl) { /* we are done */ }
									else if (b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "subtracting a pointer is not a valid operation\n");
										a->prim = sktp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else {
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								case skvm_op_gt: case skvm_op_eq:
								case skvm_op_lt: {
									NODE* b = (NODE*)DB_SoftPop(vstack);
									NODE* a = (NODE*)DB_Peek(vstack, 1);
									if (a->primlvl && b->primlvl) {
										a->prim = sktp_u1;
										a->primlvl = 0;
										a->primsize = 1;
									}
									else if (a->primlvl || b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "comparing a pointer and an integer is not a valid operation\n");
										a->prim = sktp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else {
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								case skvm_op_mw1: case skvm_op_mw2:
								case skvm_op_mw4: case skvm_op_mw8:
								{
									NODE* dst = (NODE*)DB_SoftPop(vstack);
									if (dst->primlvl < 1) {
										if (debugmode & dbmd_checker) {
											Print("\n\n\n====================\n");
											NODE_PrintInfo(cu, ndop);
											Print("\n\n\n====================\n");
										}
										// see todo, for example here, idk what's on the stack exactly so this is the best i can do
										SK_ErrorAtNode(cu, dst, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
									}
									NODE* scr = (NODE*)DB_SoftPop(vstack);
									if (dst->prim != sktp_ptr && !SK_CompatibleTypes(scr, dst, 1)) {
										NODE_PrintInfo(cu, ndop);
										NODE_PrintInfo(cu, scr);
										NODE_PrintInfo(cu, dst);
										NODE_2Str(cu, scr, errbuff);
										NODE_2Str(cu, dst, errbuff + 256);
										SK_ErrorAtNode(cu, scr, "%s and %s are not compatible for a write to memory", sktpdesc[scr->prim].name, sktpdesc[dst->prim].name, skvm_opdesc[ndop->op].crname);
									}
								} break;
								case skvm_op_mr1: case skvm_op_mr2:
								case skvm_op_mr4: case skvm_op_mr8:
								{
									NODE* scr = (NODE*)DB_Peek(vstack, 1);
									if (scr->primlvl < 1) {
										// see todo, for example here, idk what's on the stack exactly so this is the best i can do
										if (debugmode & dbmd_checker) {
											Print("\n\n\n====================\n");
											NODE_PrintInfo(cu, ndop);
											Print("\n\n\n====================\n");
										}
										SK_ErrorAtNode(cu, scr, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
									}
									else {
										scr->primlvl -= 1;
									}
								} break;
								case skvm_op_dmp:
								case skvm_op_drop: {
									vstack->occ -= sizeof(NODE);
								} break;
								case skvm_op_rtb: {
									NODE tmp;
									NODE* ndbase = DB_Peek(vstack, 3);
									memcpy(&tmp, ndbase, sizeof(NODE));
									memcpy(ndbase, ndbase + 1, sizeof(NODE) * 2);
									memcpy(ndbase + 2, &tmp, sizeof(NODE));
								} break;
								case skvm_op_rtf: {
									NODE tmp;
									NODE* ndtop = DB_Peek(vstack, 1);
									memcpy(&tmp, ndtop, sizeof(NODE));
									memcpy(ndtop, ndtop-1, sizeof(NODE));
									memcpy(ndtop-1, ndtop-2, sizeof(NODE));
									memcpy(ndtop-2, &tmp, sizeof(NODE));
								} break;
								case skvm_op_swap: {
									NODE tmp;
									NODE* ndtop = DB_Peek(vstack, 1);
									memcpy(&tmp, ndtop, sizeof(NODE));
									memcpy(ndtop, ndtop - 1, sizeof(NODE));
									memcpy(ndtop - 1, &tmp, sizeof(NODE));
								} break;
								case skvm_op_leap: {
									DB_Push(vstack, DB_Peek(vstack, 2));
								} break;
								case skvm_op_dup: {
									DB_Push(vstack, DB_Peek(vstack, 1));
								} break;
								case skvm_op_shl: case skvm_op_shr: 
								case skvm_op_xor: case skvm_op_and:
								case skvm_op_mlt: case skvm_op_mod:
								case skvm_op_div:
								{
									NODE* b = (NODE*)DB_SoftPop(vstack);
									NODE* a = (NODE*)DB_Peek(vstack, 1);
									if (a->primlvl || b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "you can't %s pointers", skvm_opdesc[ndop->op].name);
										a->prim = sktp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else {
										// TODO:: think how to handle this better
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								default: {
									FatalError(0, "checking for the operation %s is not implemented yet\n", skvm_opdesc[ndop->op].name);
								} break;
							}
						}
						else {
							SK_ErrorAtNode(cu, ndop, "not enough operands for %s", skvm_opdesc[ndop->op].name);
						}
					} break;
					case ndtp_sym: {
						SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, ndop->val);
						switch (symte->type) {
							case symtp_smlay:
							case symtp_dmlay: {
								NODE* ndmlay = (NODE*)DB_Index(cu->nodes, symte->objid);
								if (ndmlay->mlaymemc == 1) {
									NODE* nddmlaymemdesc = ndmlay + 1;
									NODE* nddmlaymemtype = nddmlaymemdesc + 1;
									
									if (debugmode & dbmd_checker) {
										NODE_PrintInfo(cu, nddmlaymemdesc);
										NODE_PrintInfo(cu, nddmlaymemtype);
									}
									
									DB_Push(vstack, nddmlaymemtype);
									NODE* ndtype = (NODE*)DB_Peek(vstack, 1);
									ndtype->type = ndtp_type;
									ndtype->primlvl = 1 + nddmlaymemtype->primlvl;
									ndtype->primsize = 8;
								}
								else {
									FatalError(0, "structs are not implemented yet { %s }\n", symtp2str[symte->type]);
								}
							} break;
							case symtp_undef: {
								// TODO:: this is really out of place i should just check all the symbols at the parsing stage
								SK_ErrorAtNode(cu, ndop, "you used %$ with out define it first\n", __fmtsym(cu, symte));
								DB_Push(vstack, &(NODE){.type = ndtp_type, .prim = sktp_ptr, .primlvl = 1, .primsize = 8, .col = ndop->col, .row = ndop->row });
							} break;
							default: {
								FatalError(0, "checking for symbols of the type { %s } is not implemented yet\n", symtp2str[symte->type]);
							} break;
						}
					} break;
					case ndtp_str: {
						DB_Push(vstack, ndop);
						NODE* ndtype = (NODE*)DB_Peek(vstack, 1);
						ndtype->type = ndtp_type;
						ndtype->prim = sktp_u1;
						ndtype->primlvl = 1;
						ndtype->primsize = 8;

						DB_Push(vstack, ndop);
						ndtype = (NODE*)DB_Peek(vstack, 1);
						ndtype->type = ndtp_type;
						ndtype->prim = sktp_u4;
						ndtype->primlvl = 0;
						ndtype->primsize = sktpdesc[ndtype->prim].size;
					} break;
					case ndtp_num: {
						DB_Push(vstack, ndop);
						NODE* ndtype = (NODE*)DB_Peek(vstack, 1);
						ndtype->type = ndtp_type;
						ndtype->prim = ndop->vtp;
						ndtype->primlvl = ndop->vtp == sktp_ptr;
						ndtype->primsize = sktpdesc[ndop->vtp].size;
					} break;
					case ndtp_mlay: {
						// TODO:: i think this node should be just ignore atm
						// maybe later i should register it's members as a proper 'type' with a hash func or something
						// then again i should've done that at the previous stage
						SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, ndop->mlaysym);
						if (debugmode & dbmd_checker) Print("%$ %u\n", __fmtsym(cu, symte), (u8)ndop->mlaymemc);
						ndop += (ndop->mlaymemc << 1);
					} break;
					case ndtp_call: {
						SYMTE* calledproc = (SYMTE*)DB_Index(cu->syms.entries, ndop->callsym);
						if (calledproc->type != symtp_undef && calledproc->type != symtp_seen) {
							NODE* calledtypes = (NODE*)DB_Index(cu->nodes, calledproc->objid);
							u4 vstacksize = DB_ElementCount(vstack);
							if (vstacksize >= calledtypes->argsc) {
								NODE* calledarg = calledtypes + 1;
								NODE* stackbase = DB_Peek(vstack, calledtypes->argsc);
								if (debugmode & dbmd_checker) {
									NODE_PrintInfo(cu, ndop);
									Print("checking arguments passed to %$n\n", __fmtsym(cu, calledproc));
								}
								for (u8 i = 0; i < calledtypes->argsc; i++) {
									if (debugmode & dbmd_checker) {
										Print("called:");
										NODE_PrintInfo(cu, calledarg);
										Print("stack:");
										NODE_PrintInfo(cu, stackbase);
										Print("\n");
									}
									// TODO:: turn this if into a switch stament for better error report
									if (!SK_CompatibleTypes(stackbase, calledarg, 0)) {
										NODE_2Str(cu, stackbase, errbuff);
										NODE_2Str(cu, calledarg, errbuff + 256);
										SK_ErrorAtNode(cu, calledarg, "%s differs from the argument %s needed to call %$", errbuff, errbuff + 256, __fmtsym(cu, calledproc));
									}
									calledarg++;
									stackbase++;
								}
								vstack->occ -= calledtypes->argsc * sizeof(NODE);

								NODE* calledret = calledarg;
								for (u8 i = 0; i < calledtypes->retsc; i++) {
									DB_Push(vstack, calledret);
									calledret++;
								}
							}
							else {
								SK_ErrorAtNode(cu, ndop, "not enough operands to call %$", __fmtsym(cu, calledproc));
							}
						}
					} break;
					case ndtp_branchop:
					case ndtp_loopop: {
						ndop = SK_CheckOp(cu, ndop, vstack, errbuff, debugmode)-1;
					} break;
					case ndtp_cmd: {
						switch (ndop->cmdtp) {
							case skkw_mw: {
								if (DB_ElementCount(vstack) >= 2) {
									NODE* dst = (NODE*)DB_SoftPop(vstack);
									if (dst->primlvl < 1) {
										// see todo, for example here, idk what's on the stack exactly so this is the best i can do
										SK_ErrorAtNode(cu, dst, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
									}
									ndop->type = ndtp_op;
									ndop->op   = skvm_op_mw1 - 1 + SK_PrinSize2Off(sktpdesc[dst->prim].size);
									vstack->occ -= sizeof(NODE);
								}
								else {
									SK_ErrorAtNode(cu, ndop, "not enough arguments to use ! you need at least 1");
								}
							} break;
							case skkw_mr: {
								if (DB_ElementCount(vstack) >= 1) {
									NODE* scr = (NODE*)DB_Peek(vstack, 1);
									if (scr->primlvl < 1) {
										// see todo, for example here, idk what's on the stack exactly so this is the best i can do
										SK_ErrorAtNode(cu, scr, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
									}
									ndop->type = ndtp_op;
									ndop->op = skvm_op_mr1 - 1 + SK_PrinSize2Off(sktpdesc[scr->prim].size);
									scr->primlvl -= 1;
								}
								else {
									SK_ErrorAtNode(cu, ndop, "not enough arguments to use @ you need at least 2");
								}
							} break;
							default: {
								NODE_PrintInfo(cu, ndop);
								FatalError(0, "this command shouldn't had get here { %s })\n", skkw2str[ndop->cmdtp]);
							} break;
						}
					} break;
					case ndtp_cast: {
						ndop->type = ndtp_type;
						if (DB_ElementCount(vstack) > 0) {
							memcpy(DB_Peek(vstack, 1), ndop, sizeof(NODE));
						}
						else {
							SK_ErrorAtNode(cu, ndop, "you need at least one element on the stack to cast");
						}
					} break;
					default: {
						NODE_PrintInfo(cu, ndop);
						FatalError(0, "checking for the node type { %s } is not implemented yet (inner)\n", ndtp2str[ndop->type]);
					} break;
				}
				ndop++;
				if (debugmode & dbmd_checker) {
					Print("\n============vstack============\n");
					u8 vstacksize = DB_ElementCount(vstack);
					for (u8 i = 0; i < vstacksize; ++i) {
						NODE* nd = (NODE*)DB_Index(vstack, i);
						NODE_2Str(cu, nd, errbuff);
						Print("%s", errbuff);
					}
					Print("\n==============================\n");
				}
			}
			
			if (debugmode & dbmd_checker) Print("ops done\n");
			return ndop;
		}
		case ndtp_branchop:{
			NODE* ndnxtbranch;
			if (cnode->branchtype == skkw_else) {
				DYNBUFF* stacksnapshot = DB_Create(256, sizeof(NODE));
				DB_Extend(stacksnapshot, vstack);
				ndnxtbranch = cnode + 1;

				NODE* ndbranchbody = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
				SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "branch", debugmode);
				
				ndnxtbranch = SK_CheckOp(cu, ndbranchbody, vstack, errbuff, debugmode);

				DYNBUFF* bodystacksnapshot = DB_Create(256, sizeof(NODE));
				DB_Extend(bodystacksnapshot, vstack);

				for (s4 i = 0; i < (s4)(cnode->branchc - 1); i++) {
					vstack->occ = 0;
					DB_Extend(vstack, stacksnapshot);
					ndbranchbody = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
					SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "branch", debugmode);

					ndnxtbranch = SK_CheckOp(cu, ndbranchbody, vstack, errbuff, debugmode);
					SK_CheckBody(cu, cnode, vstack, bodystacksnapshot, "branch", debugmode);
				}
				vstack->occ = 0;
				DB_Extend(vstack, stacksnapshot);
				ndnxtbranch = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
				SK_CheckBody(cu, cnode, vstack, bodystacksnapshot, "branch", debugmode);
				
				vstack->occ = 0;
				DB_Extend(vstack, bodystacksnapshot);

				DB_Free(stacksnapshot);
				DB_Free(bodystacksnapshot);
			}
			else {
				DYNBUFF* stacksnapshot = DB_Create(256, sizeof(NODE));
				DB_Extend(stacksnapshot, vstack);
				ndnxtbranch = cnode + 1;
				
				NODE* ndbranchbody = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
				SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "branch", debugmode);
				
				ndnxtbranch = SK_CheckOp(cu, ndbranchbody, vstack, errbuff, debugmode);
				SK_CheckBody(cu, cnode, vstack, stacksnapshot, "branch", debugmode);
				
				for (s4 i = 0; i < cnode->branchc; i++) {
					vstack->occ = 0;
					DB_Extend(vstack, stacksnapshot);
					ndbranchbody = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
					SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "branch", debugmode);

					ndnxtbranch = SK_CheckOp(cu, ndbranchbody, vstack, errbuff, debugmode);
					SK_CheckBody(cu, cnode, vstack, stacksnapshot, "branch", debugmode);
				}

				vstack->occ = 0;
				DB_Extend(vstack, stacksnapshot);
				DB_Free(stacksnapshot);
			}
			return ndnxtbranch;
		}
		case ndtp_loopop: {
			DYNBUFF* stacksnapshot = DB_Create(256, sizeof(NODE));
			DB_Extend(stacksnapshot, vstack);

			NODE* ndcondition = cnode + 1;
			NODE* ndloopbody = SK_CheckOp(cu, ndcondition, vstack, errbuff, debugmode);
			
			SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "while", debugmode);
			NODE* ndendbody = SK_CheckOp(cu, ndloopbody, vstack, errbuff, debugmode);
			SK_CheckBody(cu, cnode, vstack, stacksnapshot, "while", debugmode);
			
			DB_Free(stacksnapshot);
			return ndendbody;
		}
		default: {
			NODE_PrintInfo(cu, cnode);
			FatalError(0, "checking for the node type { %s } is not implemented yet (outter)\n", ndtp2str[cnode->type]);
		} break;
	}
	return 0;
}

// TOCONSIDER:: reorganize stuff, remove and change nodes, i don't think that it is a really good idea since
// i would need patch the symbols offsets again but still i think it would be nice
// also i could add syntatic sugar so i don't need to do stupid pointer arithmetic all the time
// plus it would be convinient for later stages so they don't need to waste time skipping useless data
// maybe i should add this one stage after? or do it here idk.
// TODO:: for now i should aim for something basic but
// consider away to keep track where the nodes came from, for better error reporting
// TODO:: dereferencing a pointer (memory reads) should create proper nodes, atm i am only lowering the lvl
// but for genericc ptr shouldn't be like that since it would produce a ptr with lvl zero, that's just a u8, i guess or an error
// c as far as i recall doesn't even let you dereference void*, but it let's you do it wit higher order ptr like ****ptr
void SK_CheckStack(COMPUNIT* cu, const u2 debugmode) {
	if (debugmode & dbmd_checker) Print("\n==========checker==============\n");
	
	DYNBUFF* procsoff = DB_Create(256, sizeof(u8));
	u4 symbolcount = DB_ElementCount(cu->syms.entries);
	SYMTE* symte = DB_Index(cu->syms.entries, 0);
	for (u8 i = 0; i < symbolcount; i++) {
		if (symte->type== symtp_proc) { DB_Push(procsoff, &i); }
		symte++;
	}
	u1 errbuff[512];
	DYNBUFF* vstack = DB_Create(256, sizeof(NODE));
	
	u8 proccount = DB_ElementCount(procsoff);
	for (u8 proc = 0; proc < proccount; proc++) {
		SYMTE* wsymte = (SYMTE*)DB_Index(cu->syms.entries, *(u8*)DB_Index(procsoff, proc));
		if (debugmode & dbmd_checker) Print("\nchecking: %$\n", __fmtsym(cu, wsymte));
		
		
		NODE* ndtypes = (NODE*)DB_Index(cu->nodes, wsymte->objid);
		NODE* argtp = ndtypes+1;
		for (u4 i = 0; i < ndtypes->argsc; i++) { DB_Push(vstack, argtp++); }

		if (debugmode & dbmd_checker) {
			Print("\n============vstack============\n");
			u8 vstacksize = DB_ElementCount(vstack);
			for (u8 i = 0; i < vstacksize; ++i) {
				NODE* nd = (NODE*)DB_Index(vstack, i);
				NODE_2Str(cu, nd, errbuff);
				Print("%s", errbuff);
			}
			Print("\n==============================\n");
		}

		NODE* ndprocbody = ndtypes + ndtypes->argsc + ndtypes->retsc + 1;
		SK_CheckOp(cu, ndprocbody, vstack, errbuff, debugmode);

		u4 vstacksize = DB_ElementCount(vstack);
		if (vstacksize == ndtypes->retsc) {
			NODE* rettp = ndtypes + ndtypes->retsc;
			NODE* stackbase = DB_Peek(vstack, ndtypes->retsc);
			for (u8 i = 0; i < ndtypes->retsc; i++) {
				if (debugmode & dbmd_checker) {
					Print("retv:");
					NODE_PrintInfo(cu, rettp);
					Print("stack:");
					NODE_PrintInfo(cu, stackbase);
					Print("\n");
				}
				// TODO:: turn this if into a switch stament for better error report
				if (!SK_CompatibleTypes(stackbase, rettp, 0)) {
					NODE_2Str(cu, stackbase, errbuff);
					NODE_2Str(cu, rettp, errbuff + 256);
					SK_ErrorAtNode(cu, rettp, "%s differs from the return value %s from call %$", errbuff, errbuff + 256, __fmtsym(cu, wsymte));
				}
				rettp++;
				stackbase++;
			}
		}
		else {
			SK_ErrorAtNode(cu, ndtypes, "the procdedure %$ has an unbalanced stack of %8i", __fmtsym(cu, wsymte), (s8)(vstacksize - ndtypes->retsc));
		}
		vstack->occ = 0;
		if (debugmode & dbmd_checker) Print("\n===============================\n");
	}
	DB_Free(vstack);
	DB_Free(procsoff);
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
	}
	symte->icr = patch;
	symte->status = symstt_patched;
}

NODE* SK_GenOpsByteCode(COMPUNIT* cu, NODE* node, DYNBUFF* icr, DYNBUFF* unresolvedsymbols, u8* reservedstackspace, u1* errbuff, const u2 debugmode) {
	if ((debugmode & dbmd_vmcodegen) && (debugmode & dbmd_nodeinf)) NODE_PrintInfo(cu, node);
	switch (node->type) {
		case ndtp_ops: {
			NODE* op = node + 1;
			for (u8 k = 0; k < node->opsc; k++) {
				if ((debugmode & dbmd_vmcodegen) && (debugmode & dbmd_nodeinf)) NODE_PrintInfo(cu, op);
				switch (op->type) {
					case ndtp_type: {} break;
					case ndtp_num: {
						DB_Push(icr, &(u8){ skvm_op_push });
						DB_Push(icr, &(u8){ op->val });
					} break;
					case ndtp_call: {
						SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, op->callsym);
						DB_Push(icr, &(u8){ skvm_op_push });
						SK_TagSym(symte, icr, unresolvedsymbols, op->callsym);
						switch (symte->type) {
							case symtp_undef: {
								DB_Push(icr, &(u8){ skvm_op_nop });
							} break;
							case symtp_proc: {
								DB_Push(icr, &(u8){ skvm_op_call });
							} break;
							case symtp_eproc: {
								NODE* ndsignature = (NODE*)DB_Index(cu->nodes, symte->objid);
								DB_Push(icr, &(u8){ skvm_op_ecall });
								DB_Push(icr, &(u8){ ndsignature->val }); // both argsc and retsc
							} break;
							default: {
								if (symte->type < symtp_count) {
									NODE_PrintInfo(cu, op);
									FatalError(0, "unreachable %s::%$ not implemented yet, genops call %4u:%4u", symtp2str[symte->type], __fmtsym(cu, symte), op->row + 1, op->col + 1);
								}
								else {
									FatalError(0, "unreachable {%u} not implemented yet, genops call", symte->type);
								}
							} break;
						}
					} break;
					case ndtp_op: {
						DB_Push(icr, &(u8){ op->op });
					} break;
					case ndtp_sym: {
						SYMTE* symte = (SYMTE*)DB_Index(cu->syms.entries, op->val);
						switch (symte->type) {
							case symtp_proc:
							case symtp_eproc: {
								DB_Push(icr, &(u8){ skvm_op_push });
								SK_TagSym(symte, icr, unresolvedsymbols, op->val);
							} break;
							case symtp_dmlay: {
								DB_Push(icr, &(u8){ skvm_op_idx });
								SK_TagSym(symte, icr, unresolvedsymbols, op->val);
							} break;
							case symtp_smlay: {
								DB_Push(icr, &(u8){ skvm_op_lea });
								SK_TagSym(symte, icr, unresolvedsymbols, op->val);
							} break;
							default: {
								if (symte->type < symtp_count){
									NODE_PrintInfo(cu, op);
									FatalError(0, "unreachable %s::%$ not implemented yet, genops %4u:%4u",  symtp2str[symte->type], __fmtsym(cu, symte), op->row + 1, op->col+1);
								}
								else {
									FatalError(0, "unreachable {%u} not implemented yet, genops", symte->type);
								}
							} break;
						}
					} break;
					case ndtp_str:{
						SYMTE* strte = (SYMTE*)DB_Index(cu->syms.entries, op->stroff);
						DB_Push(icr, &(u8){ skvm_op_lea });
						SK_TagSym(strte, icr, unresolvedsymbols, op->stroff);
						DB_Push(icr, &(u8){ skvm_op_push });
						DB_Push(icr, &(u8){ strte->size });
					} break;
					case ndtp_mlay: case ndtp_loopop: case ndtp_branchop: {
						op = SK_GenOpsByteCode(cu, op, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode)-1;
					} break;
					case ndtp_ops: {
						// TODO?? is this even reachable? i think this is a block inside of a block no? just leave it as is for now
						SK_GenOpsByteCode(cu, op, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);
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
			NODE* block = SK_GenOpsByteCode(cu, node + 1, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);

			DB_Push(icr, &(u8){ skvm_op_jpz });
			u8 jpzargaddr = DB_ElementCount(icr);
			DB_Push(icr, &(u8){ 0 });

			NODE* endloopop = SK_GenOpsByteCode(cu, block, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);

			DB_Push(icr, &(u8){ skvm_op_jmp });
			DB_Push(icr, &(u8){ loopingaddr });
			u8 blockendaddr = DB_ElementCount(icr);
			*(u8*)DB_Index(icr, jpzargaddr) = blockendaddr;
			return endloopop;
		} break;
		case ndtp_mlay: {
			u8 maxsize =0 ;
			NODE* nd = node + 1;
			SYMTE* laysymte = (SYMTE*)DB_Index(cu->syms.entries, node->mlaymemsym);
			if (laysymte->type == symtp_dmlay) {
				SK_PatchUnresolvedSym(cu, icr, laysymte, *reservedstackspace);
			
				for (u8 i = 0; i < node->mlaymemc << 1; i += 2) {
					NODE* memdesc = nd;
					NODE* memtype = nd+1;

					if (memdesc->mlaymemf & 0b010) { //TODO:: if the memember has a symbol do stuff
						SYMTE* memsymte = (SYMTE*)DB_Index(cu->syms.entries, memdesc->mlaymemsym);
						FatalError(0, "memlayouts with named members are not implemented yet");
					}
					else {
						maxsize += memdesc->mlaymemsize * memtype->primsize;
					}
					nd += 2;
				}
				*reservedstackspace += maxsize;
			}
			return nd;
		} break;
		case ndtp_branchop: {
			NODE* nxtbranch_end = node + 1; 
			u1 haselsebranch = node->branchtype == skkw_else;
			u8 branchcount = node->branchc - haselsebranch;
			u8 jmpargaddr = (u8)(-1);
			u8 jpzargaddr = 0;
			do {
				NODE* ndbody = SK_GenOpsByteCode(cu, nxtbranch_end, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);
				DB_Push(icr, &(u8){ skvm_op_jpz });
				jpzargaddr = DB_ElementCount(icr);
				DB_Push(icr, &(u8){ 0 });
				nxtbranch_end = SK_GenOpsByteCode(cu, ndbody, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);
				
				if (branchcount) {
					DB_Push(icr, &(u8){ skvm_op_jmp });
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
				DB_Push(icr, &(u8){ skvm_op_jmp });
				if (jmpargaddr == (u8)(-1)) {
					jmpargaddr = DB_ElementCount(icr);
					DB_Push(icr, &(u8){ jmpargaddr });
				}
				else {
					DB_Push(icr, &(u8){ jmpargaddr });
					jmpargaddr = DB_ElementCount(icr) - 1;
				}
				*(u8*)DB_Index(icr, jpzargaddr) = DB_ElementCount(icr);
				nxtbranch_end = SK_GenOpsByteCode(cu, nxtbranch_end, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);
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

void SK_GenBytecode(SKVM_PROG* program, COMPUNIT* cu, const u2 debugmode) {
	u1 errbuff[512];
	DYNBUFF* icr = DB_Create(256, sizeof(u8));
	DYNBUFF* loadedlibs = DB_Create(256, sizeof(HMODULE));
	program->bytecode = icr;
	program->loadlibs = loadedlibs;
	program->symbols = DB_Create(256, sizeof(SKVM_SYM));


	DYNBUFF* unresolvedsymbols = DB_Create(256, sizeof(u8));
	DB_Push(unresolvedsymbols, &(u8){ cu->epindx });
	
	u1 mainprocf = 1;
	while (unresolvedsymbols->occ){
		u8 symidx = *(u8*)DB_SoftPop(unresolvedsymbols);
		SYMTE* wsym = (SYMTE*)DB_Index(cu->syms.entries, symidx);
		
		u8 procaddr = DB_ElementCount(icr);
		if (debugmode & dbmd_vmcodegen) {
			CstrFromRawBytes(cu->syms.pool->data + wsym->off, wsym->size, errbuff, 512);
			Print("\n\ncompiling: %s:%4u icr:%u\n\n", errbuff, symidx, procaddr);
		}

		switch (wsym->type) {
			case symtp_eproc:{
				NODE* procsign  = (NODE*)DB_Index(cu->nodes, wsym->objid);
				NODE* ndstrcmds = procsign + 1 + procsign->argsc + procsign->retsc;
				NODE* ndlib = ndstrcmds + 1;
				NODE* ndproc = ndlib + 1;
				
				SYMTE* strlib = DB_Index(cu->syms.entries, ndlib->stroff);
				SYMTE* strproc = DB_Index(cu->syms.entries, ndproc->stroff);
				
				u1 buff[256];
				CstrFmt(buff, "%$\0", __fmtsym(cu, strlib));
				HMODULE lib = LoadLibraryA(buff);
				CstrFmt(buff, "%$\0", __fmtsym(cu, strproc));
				u8 procaddr = (u8)GetProcAddress(lib, buff);
				SK_PatchUnresolvedSym(cu, icr, wsym, procaddr);
				DB_Push(loadedlibs, &(HMODULE){ lib });
			} break;
			case symtp_proc:{
				SKVM_SYM vmsym = { 0 };
				vmsym.bc = (u1*)(u8)icr->occ;
				vmsym.off = cu->syms.pool->data + wsym->off;
				vmsym.len = wsym->size;
				vmsym.type = skvm_symtp_proc;
				u8 resargaddr = 0;
				if (wsym->haslocals) {
					DB_Push(icr, &(u8){ skvm_op_res });
					resargaddr = DB_ElementCount(icr);
					DB_Push(icr, &(u8){ 0 });
				}
				NODE* procsign  = (NODE*)DB_Index(cu->nodes, wsym->objid);
				NODE* procblock = procsign + 1 + procsign->argsc + procsign->retsc;
				u8 reservedstackspace = 0;
				SK_GenOpsByteCode(cu, procblock, icr, unresolvedsymbols, &reservedstackspace, errbuff, debugmode);
		
				if (reservedstackspace) {
					DB_Push(icr, &(u8){ skvm_op_rel });
					DB_Push(icr, &(u8){ reservedstackspace });
					*(u8*)DB_Index(icr, resargaddr) = reservedstackspace;
				}
				if (mainprocf) {
					DB_Push(icr, &(u8){ skvm_op_hcf });
					mainprocf = 0;
				}
				else {
					DB_Push(icr, &(u8){ skvm_op_ret });
				}

				vmsym.bcend = (u1*)(u8)icr->occ;
				DB_Push(program->symbols, &vmsym);
				SK_PatchUnresolvedSym(cu, icr, wsym, procaddr);
			} break;
			case symtp_smlay: {
				SKVM_SYM vmsym = { 0 };
				vmsym.bc = (u1*)(u8)icr->occ;
				vmsym.off = cu->syms.pool->data + wsym->off;
				vmsym.len = wsym->size;
				vmsym.type = skvm_symtp_smlay;
				u8 maxsize = 0;
				u8 memlayaddr = DB_ElementCount(icr);
				NODE* node = (NODE*)DB_Index(cu->nodes, wsym->objid);
				NODE* nd = node + 1;

				// need to register the symbol
				for (u8 i = 0; i < node->mlaymemc << 1; i += 2) {
					NODE* memdesc = nd;
					NODE* memtype = nd + 1;

					if (memdesc->mlaymemf & 0b010) { //TODO:: if the memember has a symbol so stuff
						SYMTE* memsymte = (SYMTE*)DB_Index(cu->syms.entries, memdesc->mlaymemsym);
						FatalError(0, "memlayouts with named members are not implemented yet");
					}
					else {
						maxsize += memdesc->mlaymemsize * memtype->primsize;
					}
					nd += 2;
				}
				u8 oldocc = icr->occ;
				u8 newocc = ((icr->occ + maxsize + (u8)7) & (~(u8)7));
				while (newocc > icr->cap) { DB_Grow(icr); }
				icr->occ = newocc;
				memset(icr->data + oldocc, 0, newocc - oldocc);
				
				vmsym.bcend = (u1*)(u8)icr->occ;
				DB_Push(program->symbols, &vmsym);
				SK_PatchUnresolvedSym(cu, icr, wsym, memlayaddr);
			} break;
			case symtp_str: {
				SKVM_SYM vmsym = { 0 };
				vmsym.bc = (u1*)(u8)icr->occ;
				vmsym.off = cu->syms.pool->data + wsym->off;
				vmsym.len = wsym->size;
				vmsym.type = skvm_symtp_str;

				u8 straddr = DB_ElementCount(icr);
				DB_Append(icr, __fmtsym(cu, wsym));
				u8 remaining2nxtmultof8 = ((icr->occ + (u8)7) & (~(u8)7)) - icr->occ;
				u1 buff[256];
				memset(buff, 0, remaining2nxtmultof8);
				DB_Append(icr, buff, remaining2nxtmultof8);

				vmsym.bcend = (u1*)(u8)icr->occ;
				DB_Push(program->symbols, &vmsym);
				SK_PatchUnresolvedSym(cu, icr, wsym, straddr);
			} break;
			case symtp_dmlay:{ } break;
			case symtp_undef:{ } break;
			default: {
				FatalError(0, "unhandled symbol type %s at code gen", symtp2str[wsym->type]);
			}
		}
		if (debugmode & dbmd_vmcodegen) {
			CstrFromRawBytes(cu->syms.pool->data + wsym->off, wsym->size, errbuff, 512);
			Print("\n\ndone with: %s:%4u new icr:%u:%u:%u\n\n", errbuff, symidx, (u8)DB_ElementCount(icr), (u8)DB_ElementCount(icr) - procaddr, ((u8)DB_ElementCount(icr) - procaddr) << 3);
		}
	}
	DB_Free(unresolvedsymbols);
}

void SK_Free(COMPUNIT* cu) {
	DB_Free(cu->nodes);
	DB_Free(cu->syms.entries);
	DB_Free(cu->errpool);
	DB_Free(cu->syms.pool);
}

void SK_FlushErrors(COMPUNIT* cu) {
	u8 errlen = cu->errpool->occ;
	u1* erroff = cu->errpool->data;
	while (errlen) {
		u4 written = FS_Writeb(STDOUT, erroff, errlen);
		erroff += written;
		errlen -= written;
	}
	FS_Flush(STDOUT);
	cu->errpool->occ = 0;

	u1* messages[] = {
		"I would explain what is wrong, but I do not have the time, the patience, or the crayons to help you.",
		"Compiling this garbage consumed electricity that could have powered a hospital.",
		"Missing basic cognitive function between the keyboard and the chair.",
		"This did not compile. I suggest you close the IDE, go outside, stare at the sky, and seriously reflect on how you ended up here.",
		"Honestly, just delete the whole file and start over. Or better yet, do not.",
		"Type mismatch. Much like your career choice.",
		"Identifier not found. Much like your understanding of basic programming concepts.",
		"Expected expression. Found regret.",
		"404: Common sense not found.",
		"The problem is always at Layer 8: the user.",
		"Compilation failed. I'm beginning to suspect the problem is not the code.",
		"Our code works exactly as well as your understanding of it."
	};
	FatalError(0, "%s\n", messages[cu->errorcount % (sizeof(messages) / sizeof(u1*))]);
}

u4 main(u4 args, u1** vargs) {
	const u2 debugmode = dbmd_nodebug;// dbmd_runtests;//|dbmd_parser;
	COMPUNIT cu;
	SKVM_PROG prog;

	static const u1* testprograms[] = { 
		// ".\\tests\\scopes.sk",
		// ".\\tests\\conditionals.sk",
		// ".\\tests\\main.sk",
		// ".\\tests\\memlayout.sk",
		// ".\\tests\\fib.sk",
		// ".\\tests\\rand.sk",
		// ".\\tests\\strs.sk",
		// ".\\tests\\while.sk",
		".\\tests\\gol.sk",
	};
	static const u4 testprogramsize = sizeof(testprograms)/sizeof(u1*);
	
	u4 programcount = 0;
	u1** programs = 0;
	
	if (debugmode) {
		u4 mode = 1;
		for (u4 i = debugmode; i; i >>= 1) {
			if ((i & 1) && mode < dbmd_count) Print("mode: %s\n", dbmd2str[mode]);
			mode++;
		}
		programcount = testprogramsize;
		programs = testprograms;
	}
	else {
		programcount = args-1;
		programs = vargs+1;
	}
	void(*skvm[])(SKVM_PROG*) = { SKVM_Exe, SKVM_ExeDebug };

	for (u8 i = 0; i < programcount; i++) {
		Print("compiling: %s\n", programs[i]);
		SK_NodeWeber(&cu, programs[i], "main", debugmode);
		SK_CheckStack(&cu, debugmode);
		

		if (!cu.errorcount) { 
			SK_GenBytecode(&prog, &cu, debugmode);
			skvm[debugmode ? 1 : 0](&prog);
		}
		else SK_FlushErrors(&cu);
		
		SK_Free(&cu);
	}

	return 0x45;
}