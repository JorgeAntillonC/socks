
#ifndef SKCOMMONDEF_INCLUDE
#define SKCOMMONDEF_INCLUDE

#include "p:\stdwea\gdefwea.h"
#include "p:\stdwea\dynbuffwea.h"
#include "p:\stdwea\phashtablewea.h"
#include ".\skvm.h"

// TODO:: add source code equivalent
#define SK_KEYWORDS \
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
X(extrn)

#define X(x) sk_kw_##x,
typedef enum SK_KEYWORD SK_KEYWORD;
enum SK_KEYWORD { SK_KEYWORDS sk_kw_count };
#undef X

#define X(x) #x,
u1* sk_kw2str[] = { SK_KEYWORDS };
#undef X
#undef SK_KEYWORDS

// name, size in bytes, indirectionlvl
#define SK_TYPES \
X(u1,    1, 0)\
X(u2,    2, 0)\
X(u4,    4, 0)\
X(u8,    8, 0)\
X(s1,    1, 0)\
X(s2,    2, 0)\
X(s4,    4, 0)\
X(s8,    8, 0)\
X(ptr,   8, 1)\
X(proc,  8, 0)\
X(eproc, 8, 0)\
X(mlay,  8, 0)

#define X(x,...) sk_tp_##x,
typedef enum SK_TYPE SK_TYPE;
enum SK_TYPE { SK_TYPES sk_tp_count };
#undef X

typedef struct SK_TYPEDESC SK_TYPEDESC;
struct SK_TYPEDESC {
	u1* name;
	u4  size;
	u4  lvl;
};

#define X(x, _size, _lvl) { .name = #x, .size = _size, . lvl = _lvl },
SK_TYPEDESC sk_tpdesc[sk_tp_count] = { SK_TYPES };
#undef X
#undef SKTYPES

#define SK_NODETYPES \
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
X(sign)\
X(ops)\
X(types)\
X(smlay)\
X(dmlay)\
X(mlaymem)\
X(solo)\
X(paired)

#define X(x) sk_ndtp_##x,
typedef enum SK_NODETYPE SK_NODETYPE;
enum SK_NODETYPE { SK_NODETYPES sk_ndtp_count };
#undef X
#define X(x) #x,
u1* sk_ndtp2str[sk_ndtp_count] = { SK_NODETYPES };
#undef X

// TODO:: group better the sub parameters, i am basically writting the type as a prefix anyway
// may as well do it with named unions/structs to keep things in a neater order and remember what goes where
typedef struct SK_NODE SK_NODE;
struct SK_NODE {
	u2 col;
	u2 row;

	u2 type;

	union {
		u2 ext;
		u2 argsretsf;
		u2 mlaymemf;
		struct {
			u1 mlayf;
			u1 mlaytype; // static of dynamic
		};
		
		struct {
			u1 loopf;
			u1 loopdmemalloc;
		};
		u2 symf;
		u2 opsf;
		struct {
			u1 branchf;
			u1 branchtype;
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
		u8 val;  // lite ral int value, random bs
		struct {
			union {
				u4 prim; // the type of types lol
				u4 symoff;
				u4 mlaysym;
				u4 mlaymemsym; // for mem layout member
				u4 opsc;
				u4 cmdstrc;
				u4 argsc;
				u4 branchc;
				u4 op;
			};
			union {
				u4 primoff; // for funcs and memlay
				u4 mlaymemc;   // for mem layout
				u4 mlaymemsize; // for mem layout arrays
				u4 retsc;
				u4 cmdtp;
				u4 symcall;
			};
		};
		u4 argsrets[2];
	};
};

#define SK_SYMTYPES \
X(undef)\
X(seen)\
X(proc)\
X(eproc)\
X(rsym)\
X(mlaymem)\
X(dmlay)\
X(smlay)\
X(str)\
X(kw)\
X(count)

#define X(x) sk_symtp_##x,
typedef enum SK_SYMTYPE SK_SYMTYPE;
enum SK_SYMTYPE { SK_SYMTYPES };
#undef X
#define X(x) #x,
u1* sk_symtp2str[] = { SK_SYMTYPES };
#undef X
#undef SK_SYMTYPES


typedef enum SK_SYMSTT SK_SYMSTT;
enum SK_SYMSTT {
	sk_symstt_unpatched,
	sk_symstt_seen,
	sk_symstt_patched,
};

typedef struct SK_SYMBOL SK_SYMBOL;
struct SK_SYMBOL {
	u4 type;   // proc mem etc
	u4 state;
	u4 scopeid;
	u4 stroff;
	u8 objid;  // proc index, mem index, etc index nodepool
	u8 icr;    // proc index, mem index, etc index "final icr addr"
};

typedef struct SK_STR SK_STR;
struct SK_STR {
	u2 len;
	u1 str[];
};

typedef struct SK_COMPUNIT SK_COMPUNIT;
struct SK_COMPUNIT {
	DYNBUFF* procs;
	DYNBUFF* ndwebs;
	DYNBUFF* errpool;
	
	PHASHTABLE* syms;
	PHASHTABLE* strs;
	
	u4 entrypoint;
	u1* sourcefile;
	u4 errorcount;
};

#define DEBUGMODES \
X(nodebug,     0x0000) \
X(nodeinf,     0x0001) \
X(weaver,      0x0002) \
X(checker,     0x0004) \
X(bcgenerator, 0x0008) \
X(x64codegen,  0x0010) \
X(debugger,    0x0020) \
X(bytecode,    0x0040)

#define X(x,...) #x,
u1* sk_dbmd2str[] = { DEBUGMODES };
#undef X

#define X(x, v) sk_dbmd_##x = v,
typedef enum SKDEBUGMODE SKDEBUGMODE;
enum SKDEBUGMODE { DEBUGMODES sk_dbmd_count = sizeof(sk_dbmd2str)/sizeof(u1*) };
#undef X
#undef DEBUGMODES

u4 SK_TableStrInsert(PHASHTABLE* table, u1* str, u4 strlen);

inline static u8 SK_Node2Str(SK_COMPUNIT* cu, SK_NODE* node, u1* buff);

inline static u8 SK_Node2StrSimp(SK_COMPUNIT* cu, SK_NODE* node, u1* buff);

inline static void SK_NodePrintInfo(SK_COMPUNIT* cu, SK_NODE* node);

static void SK_ErrorAtNode(SK_COMPUNIT* cu, SK_NODE* node, u1* errmsg, ...);

#endif // SKCOMMONDEF_INCLUDE

#ifndef  SKCOMMONDEF_DEF
#define  SKCOMMONDEF_DEF

u4 SK_TableStrInsert(PHASHTABLE* table, u1* str, u4 strlen) {
	PHTENTRY* strentry = PHT_Reserve(table, str, strlen, strlen + sizeof(u2));
	if (!strentry->state) {
		strentry->state = 1;
		u1* entryptr = DB_Index(table->pool, strentry->off);
		*(u2*)entryptr = strlen;
		memcpy(entryptr + sizeof(u2), str, strlen);
	}
	return strentry->off;
}

inline static u8 SK_Node2StrSimp(SK_COMPUNIT* cu, SK_NODE* node, u1* buff) {
	switch (node->type) {
		case sk_ndtp_str: {
			u1 strbuff[512];
			if (node->symf) {
				SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->symoff);
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
				CstrFromRawBytes(strte->str, strte->len, strbuff, 512);
				return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], strbuff);
			}
			else {
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, node->symoff);
				CstrFromRawBytes(strte->str, strte->len, strbuff, 512);
				return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], strbuff);
			}
		} break;
		case sk_ndtp_sym: {
			if (node->symf) {
				SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->symoff);
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
				return CstrFmt(buff, "(%s:%$)", sk_ndtp2str[node->type], strte->str, (u8)strte->len);
			}
			else {
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, node->symoff);
				return CstrFmt(buff, "(%s:%$)", sk_ndtp2str[node->type], strte->str, (u8)strte->len);
			}
		} break;
		case sk_ndtp_num: {
			if (sk_tp_s1 <= node->vtp && node->vtp <= sk_tp_s8) {
				return CstrFmt(buff, "(%s:%i)", sk_ndtp2str[node->type], node->val);
			}
			else {
				return CstrFmt(buff, "(%s:%u)", sk_ndtp2str[node->type], node->val);
			}
		}
		case sk_ndtp_smlay:
		case sk_ndtp_dmlay: {
			SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->mlaysym);
			SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
			return CstrFmt(buff, "(%s:%$)", sk_ndtp2str[node->type], strte->str, (u8)strte->len);
		}
		case sk_ndtp_cast:
		case sk_ndtp_type: {
			return CstrFmt(buff, "(%s:%s lvl:%1u)", sk_ndtp2str[node->type], sk_tpdesc[node->prim].name, node->primlvl);
		}
		case sk_ndtp_cmd:
		case sk_ndtp_cmdstrs: {
			return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], sk_kw2str[node->cmdtp]);
		}
		case sk_ndtp_op: {
			return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], skvm_opdesc[node->op].name);
		}
		case sk_ndtp_flow:
		case sk_ndtp_delim: {
			return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], sk_kw2str[node->val]);
		}
		default: {
			if (node->type < sk_ndtp_count) {
				return CstrFmt(buff, "(%s)", sk_ndtp2str[node->type]);
			}
			return CstrFmt(buff, "(?? %u)", node->type);
		}
	}
}

inline static u8 SK_Node2Str(SK_COMPUNIT* cu, SK_NODE* node, u1* buff) {
	switch (node->type) {
		case sk_ndtp_str: {
			u1 strbuff[512];
			if (node->symf) {
				SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->symoff);
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
				CstrFromRawBytes(strte->str, strte->len, strbuff, 512);
				return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], strbuff);
			}
			else {
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, node->symoff);
				CstrFromRawBytes(strte->str, strte->len, strbuff, 512);
				return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], strbuff);
			}
		} break;
		case sk_ndtp_symdef:
		case sk_ndtp_procdef:
		case sk_ndtp_sym: {
			if (node->symf) {
				SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->symoff);
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
				return CstrFmt(buff, "(%s:%$ %4u %2u)", sk_ndtp2str[node->type], strte->str, (u8)strte->len, node->symoff, node->symf);
			}
			else {
				SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, node->symoff);
				return CstrFmt(buff, "(%s:%$ %4u %2u)", sk_ndtp2str[node->type], strte->str, (u8)strte->len, node->symoff, node->symf);
			}
		} break;
		case sk_ndtp_num: {
			if (sk_tp_s1 <= node->vtp && node->vtp <= sk_tp_s8) {
				return CstrFmt(buff, "(%s:%i)", sk_ndtp2str[node->type], node->val);
			}
			else {
				return CstrFmt(buff, "(%s:%u)", sk_ndtp2str[node->type], node->val);
			}
		}
		case sk_ndtp_sign: {
			return CstrFmt(buff, "(%s:%4u %4u)", sk_ndtp2str[node->type], node->argsc, node->retsc);
		}
		case sk_ndtp_smlay:
		case sk_ndtp_dmlay: {
			SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->mlaysym);
			SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
			return CstrFmt(buff, "(%s:%$ %u)", sk_ndtp2str[node->type], strte->str, (u8)strte->len, (u8)node->mlaymemc);
		}
		case sk_ndtp_mlaymem: {
			SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->mlaysym);
			SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
			return CstrFmt(buff, "(%s:%u %$ %1br0)", sk_ndtp2str[node->type], node->mlaymemsize, strte->str, (u8)strte->len, node->mlaymemf, (u8)3);
		}
		case sk_ndtp_ops: {
			return CstrFmt(buff, "(%s:%4u %2u)", sk_ndtp2str[node->type], node->opsc, node->opsf);
		}
		case sk_ndtp_types: {
			return CstrFmt(buff, "(%s:%4u %4u)", sk_ndtp2str[node->type], node->argsc, node->retsc);
		}
		case sk_ndtp_cast:
		case sk_ndtp_type: {
			return CstrFmt(buff, "(%s:%s %1u)", sk_ndtp2str[node->type], sk_tpdesc[node->prim].name, node->primlvl);
		}
		case sk_ndtp_cmd:
		case sk_ndtp_cmdstrs: {
			return CstrFmt(buff, "(%s:%s %u)", sk_ndtp2str[node->type], sk_kw2str[node->cmdtp], node->cmdstrc);
		}
		case sk_ndtp_op: {
			return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], skvm_opdesc[node->op].name);
		}
		case sk_ndtp_flow:
		case sk_ndtp_delim: {
			return CstrFmt(buff, "(%s:%s)", sk_ndtp2str[node->type], sk_kw2str[node->val]);
		}
		case sk_ndtp_branchop: {
			return CstrFmt(buff, "(%s:%4u %4u %s)", sk_ndtp2str[node->type], node->branchf, node->branchc, sk_kw2str[node->branchtype]);
		}
		default: {
			if (node->type < sk_ndtp_count) {
				return CstrFmt(buff, "(%s)", sk_ndtp2str[node->type]);
			}
			return CstrFmt(buff, "(?? %u)", node->type);
		}
	}
}

inline static void SK_NodePrintInfo(SK_COMPUNIT* cu, SK_NODE* node) {
	u1 buff[256];
	SK_Node2Str(cu, node, buff);
	Print("%s at %s:%u:%u\n", buff, cu->sourcefile, (u8)node->row + 1, (u8)node->col + 1);
}

static void SK_ErrorAtNode(SK_COMPUNIT* cu, SK_NODE* node, u1* errmsg, ...) {
	SK_NODE* cnode = node;
	u1 errbuff[1024];
	u4 errlen;
	if (node) {
		if (errmsg) {
			VAR_ARG(errmsg, stack);
			CstrFmtS(errbuff + 512, errmsg, stack);
			errlen = CstrFmt(errbuff, "\x1B[31mERROR\x1B[37m::%s at %s:%2u:%2u\n\n", errbuff + 512, cu->sourcefile, cnode->row + 1, cnode->col + 1);
		}
		else {
			SK_Node2StrSimp(cu, node, errbuff + 512);
			errlen = CstrFmt(errbuff, "\x1B[31mERROR\x1B[37m::rogue %s at %s:%2u:%2u\n\n", errbuff + 512, cu->sourcefile, cnode->row + 1, cnode->col + 1);
		}
	}
	else {
		VAR_ARG(errmsg, stack);
		CstrFmtS(errbuff + 512, errmsg, stack);
		errlen = CstrFmt(errbuff, "\x1B[31mERROR\x1B[37m::%s at %s\n\n", errbuff + 512, cu->sourcefile);
	}
	DB_Append(cu->errpool, errbuff, errlen);
	cu->errorcount++;
}

#endif // SKCOMMONDEF_DEF