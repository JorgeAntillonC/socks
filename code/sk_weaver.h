#ifndef SK_WEAVER_INCLUDE
#define SK_WEAVER_INCLUDE
#include "p:\stdwea\gdefwea.h"
#include "p:\stdwea\dynbuffwea.h"
#include "p:\stdwea\phashtablewea.h"
#include "p:\stdwea\iowea.h"
#include ".\sk_commondef.h"

typedef struct SK_WEAVERSTKSTT SK_WEAVERSTKSTT;
struct SK_WEAVERSTKSTT {
	union {
		struct { u4 off; u4 len; } ndgroup;
		struct { u4 procidx; u4 procoff; } usym;
		struct { u4 procidx; u4 procoff; } proc;
		struct { u4 id; u4 symc; } scope;
	};
};

typedef struct SK_NDWEAVER SK_NDWEAVER;
struct SK_NDWEAVER {
	DYNBUFF* syntaxstk;
	DYNBUFF* peekedstk;

	DYNBUFF* ndgroupstk;
	DYNBUFF* usymstk;
	DYNBUFF* scopestk;

	DYNBUFF* wwebstk;
	DYNBUFF* wweb;

	u1* sourcecode;
	u1* scoff;

	struct {
		u4 id;
		u4 symc;
		u4 counter;
	} scope;

	struct {
		u4 procidx;
		u4 off;
		u4 len;
	} ndgroup;

	u2 col;
	u2 row;
	u4 nodelen;
	u4 ssp; // syntax stack pointer
	u4 peeked; // how many tokens has it peeked
};

inline void SK_WeaverGetNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u1 peek);

void SK_WeaverPeekNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver);

void SK_WeaverNxtNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver);

inline void SK_WeaverOpenNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u4 ndgrouptype, const u1 newscopef);

inline u4 SK_WeaverResolvedScopeSymbols(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u1* dynamicmemallocs);

inline u1 SK_WeaverCloseNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u1 newscopef);

inline void SK_WeaverRegisterSymbol(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, SK_NODE* symnd, const u2 symboltype);

void SK_WeaverPrintState(SK_COMPUNIT* cu, SK_NDWEAVER* wv);

void SK_WeaverWeaveProgram(SK_COMPUNIT* cu, const u2 debugmode);

#endif //SK_WEAVER_INCLUDE

#ifndef SK_WEAVER_DEF
#define SK_WEAVER_DEF

inline void SK_WeaverDrop(SK_NDWEAVER* weaver, u4 num) {
	DB_Drop(weaver->syntaxstk, num);
	weaver->ssp -= num;
		if (weaver->ssp + num < weaver->ssp) {
		FatalError(0, "SK_WeaverDrop:: you drop too many elements");
	}
}

inline void SK_WeaverPush(SK_NDWEAVER* weaver, SK_NODE* node) {
	DB_Push(weaver->syntaxstk, node);
	weaver->ssp++;
}

inline void SK_WeaverDropPeek(SK_NDWEAVER* weaver, u4 num){
	weaver->peeked += num;
	if (weaver->peeked * sizeof(SK_NODE) == weaver->peekedstk->occ) {
		weaver->peeked = 0;
		weaver->peekedstk->occ = 0;
	}
}

inline void SK_WeaverPushPeek(SK_NDWEAVER* weaver, u4 num) {
	if (weaver->peekedstk->occ) {
		if (weaver->peeked < num) {
			FatalError(0, "SK_WeaverPushPeek:: overflow!!!\n");
		}
		weaver->peeked -= num;
		memcpy(weaver->peekedstk->data + weaver->peeked * sizeof(SK_NODE), weaver->syntaxstk->data + weaver->syntaxstk->occ - num * sizeof(SK_NODE), num*sizeof(SK_NODE));
	}
	else {
		DB_Append(weaver->peekedstk, DB_Peek(weaver->syntaxstk, num), num*sizeof(SK_NODE));
	}
}

inline void SK_WeaverGetNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u1 peek) {
	if (weaver->peekedstk->occ && !peek) {
		DB_Push(weaver->syntaxstk, DB_Index(weaver->peekedstk, weaver->peeked++));
		weaver->ssp++;
		if (weaver->peeked * sizeof(SK_NODE) == weaver->peekedstk->occ) {
			weaver->peeked = 0;
			weaver->peekedstk->occ = 0;
		}
		return;
	}
	weaver->col += weaver->nodelen;
	weaver->scoff += weaver->nodelen;
	do {
		while (*weaver->scoff && CharIsSpace(*weaver->scoff)) {
			weaver->scoff++;
			weaver->col++;
			if (CharIsNL(*weaver->scoff)) {
				weaver->col = 0;
				while (CharIsNL(*weaver->scoff)) { if (*weaver->scoff == '\n') { weaver->row++; } weaver->scoff++; }
			}
		}

		if (*weaver->scoff == ';') {
			weaver->scoff++;
			weaver->col++;
			if (*weaver->scoff == ';') {
				weaver->scoff++;
				weaver->col++;
				while (*weaver->scoff && !(*weaver->scoff == ';' && *(weaver->scoff + 1) == ';')) {
					weaver->scoff++;
					weaver->col++;
					if (CharIsNL(*weaver->scoff)) {
						weaver->col = 0;
						while (CharIsNL(*weaver->scoff)) { if (*weaver->scoff == '\n') { weaver->row++; } weaver->scoff++; }
					}
				}
				if (*weaver->scoff) {
					weaver->scoff += 2;
					weaver->col += 2;
				}
			}
			else {
				while (*weaver->scoff && !CharIsNL(*weaver->scoff)) { weaver->scoff++; }
				weaver->col = 0;
				while (*weaver->scoff && CharIsNL(*weaver->scoff)) { if (*weaver->scoff == '\n') { weaver->row++; } weaver->scoff++; }
			}
		}
	} while (*weaver->scoff && (CharIsSpace(*weaver->scoff) || *weaver->scoff == ';'));

	SK_NODE* cnode;
	if (peek) {
		cnode = (SK_NODE*)DB_Reserve(weaver->peekedstk, sizeof(SK_NODE));
	}
	else {
		weaver->ssp++;
		cnode = (SK_NODE*)DB_Reserve(weaver->syntaxstk, sizeof(SK_NODE));
	}
	cnode->row = weaver->row;
	cnode->col = weaver->col;
	u1* nodeend = weaver->scoff;

	if (CharIsAlp(*weaver->scoff)) {
		while (CharIsAlpNum(*nodeend) || *nodeend == '_') { nodeend++; }
		weaver->nodelen = (u8)nodeend - (u8)weaver->scoff;

#define X(x) (CstrLen(x)-1 == weaver->nodelen && CstrCmpS(x, weaver->scoff, weaver->nodelen))
		if (X("oogabuga")) {
			cnode->type = sk_ndtp_op;
			cnode->op = skvm_op_oogabuga;
			return;
		}
		if (X("dmp")) {
			cnode->type = sk_ndtp_op;
			cnode->op = skvm_op_dmp;
			return;
		}
		if (X("null")) {
			cnode->type = sk_ndtp_num;
			cnode->vtp = sk_tp_ptr;
			cnode->rsz = 1;
			cnode->val = 0;
			return;
		}

		for (u4 i = 0; i < sk_tp_count; i++) {
			if (X(sk_tpdesc[i].name)) {
				cnode->type = sk_ndtp_type;
				cnode->prim = i;
				cnode->primsize = sk_tpdesc[i].size;
				cnode->primlvl = sk_tpdesc[i].lvl;
				return;
			}
		}

		if (X("if")) {
			cnode->type = sk_ndtp_flow;
			cnode->val = sk_kw_if;
			return;
		}
		if (X("elif")) {
			cnode->type = sk_ndtp_flow;
			cnode->val = sk_kw_elif;
			return;
		}
		if (X("else")) {
			cnode->type = sk_ndtp_flow;
			cnode->val = sk_kw_else;
			return;
		}
		if (X("while")) {
			cnode->type = sk_ndtp_flow;
			cnode->val = sk_kw_while;
			return;
		}
		if (X("extrn")) {
			cnode->type = sk_ndtp_cmd;
			cnode->cmdtp = sk_kw_extrn;
			return;
		}
		if (X("include")) {
			cnode->type = sk_ndtp_cmd;
			cnode->cmdtp = sk_kw_include;
			return;
		}
#undef X
		cnode->symf = 0;
		cnode->type = sk_ndtp_sym;
		cnode->symoff = SK_TableStrInsert(cu->strs, weaver->scoff, weaver->nodelen);
		return;
	}
	if (CharIsNum(*weaver->scoff)) {
		cnode->val = CstrParseNum(weaver->scoff, &weaver->nodelen);
		cnode->type = sk_ndtp_num;
		cnode->vtp = sk_tp_s4;
		if (cnode->val & ~(0xFFFFFFFFULL)) { cnode->rsz = 8; cnode->vtp = sk_tp_s8; }
		else if (cnode->val & ~(0xFFFFULL)) { cnode->rsz = 4; }
		else if (cnode->val & ~(0xFFULL)) { cnode->rsz = 2; }
		else { cnode->rsz = 1; }
		return;
	}

	if (CstrCmpS(">>", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_shr;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("<<", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_shl;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("*>", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_dup;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("@>", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_rtf;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("<@", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_rtb;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("'>", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_leap;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("><", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_swap;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("//", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_drop;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("@1", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mw1;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("@2", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mw2;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("@4", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mw4;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("@8", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mw8;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("!1", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mr1;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("!2", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mr2;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("!4", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mr4;
		weaver->nodelen = 2;
		return;
	}
	if (CstrCmpS("!8", weaver->scoff, 2)) {
		cnode->type = sk_ndtp_op;
		cnode->val = skvm_op_mr8;
		weaver->nodelen = 2;
		return;
	}

	switch (*weaver->scoff) {
		case '"': {
			nodeend++;
			DYNBUFF* strlit = DB_Create(256, sizeof(u1));
			u1* lastcopy = nodeend;
			while (*nodeend && *nodeend != '"') {
				if (*nodeend == '\\') {
					DB_Append(strlit, lastcopy, nodeend - lastcopy);
					switch (*++nodeend) {
						case 'n': {
							DB_Push(strlit, &(u1){ '\n' });
						} break;
						case 'r': {
							DB_Push(strlit, &(u1){ '\r' });
						} break;
						case '0': {
							nodeend++;
							u1 charc = 0;
							u1 v = 0;
							while (CharIsNumO(*nodeend) && charc < 2) {
								u1 c = *nodeend++;
								v = (v << 3) + c - '0';
								charc++;
							}
							DB_Push(strlit, &(u1){ v });
							nodeend--;
						} break;
						case 'x': {
							nodeend++;
							u1 v = 0;
							u1 charc = 0;
							while (CharIsNumX(*nodeend) && charc < 2) {
								u1 c = *nodeend++;
								if ((u1)((c & 0xDF) - 'A') < 6) { c = (c & 0xDF) - 7; }
								v = (v << 4) + c - '0';
								charc++;
							}
							DB_Push(strlit, &(u1){ v });
							nodeend--;
						} break;
						case '\\': {
							DB_Push(strlit, &(u1){ '\\' });
						} break;
						default: {
							SK_ErrorAtNode(cu, cnode, "unknown scapecode { %c }", *nodeend);
							DB_Push(strlit, nodeend);
						} break;
					}
					nodeend++;
					lastcopy = nodeend;
				}
				else {
					*nodeend++;
				}
			}
			DB_Append(strlit, lastcopy, nodeend - lastcopy);
			if (*nodeend == '"') {
				nodeend++;
			}
			else {
				SK_ErrorAtNode(cu, cnode, "premature eof while defining string literal");
			}
			weaver->nodelen = (u8)(nodeend - weaver->scoff);
			cnode->type = sk_ndtp_str;
			cnode->strf = 0;
			cnode->strlit = strlit;
		} return;
		case '-': {
			if (CharIsNum(*(weaver->scoff + 1))) {
				cnode->type = sk_ndtp_num;
				cnode->val = CstrParseNum(weaver->scoff + 1, &weaver->nodelen);
				cnode->vtp = sk_tp_s4;
				if (cnode->val & ~(0xFFFFFFFFULL)) { cnode->rsz = 8; cnode->vtp = sk_tp_s8; }
				else if (cnode->val & ~(0xFFFFULL)) { cnode->rsz = 4; }
				else if (cnode->val & ~(0xFFULL)) { cnode->rsz = 2; }
				else { cnode->rsz = 1; }
				cnode->val = -cnode->val;
				weaver->nodelen += 1;
			}
			else {
				weaver->nodelen = 1;
				cnode->type = sk_ndtp_op;
				cnode->op = skvm_op_sub;
			}
		} return;
		case '@': { weaver->nodelen = 1; cnode->type = sk_ndtp_cmd;   cnode->cmdtp = sk_kw_mw; } return;
		case '!': { weaver->nodelen = 1; cnode->type = sk_ndtp_cmd;   cnode->cmdtp = sk_kw_mr; } return;
		case '(': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_lpar; } return;
		case ')': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_rpar; } return;
		case '[': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_lsqr; } return;
		case ']': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_rsqr; } return;
		case '{': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_lbra; } return;
		case '}': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_rbra; } return;
		case '.': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_dot;} return;
		case ':': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_colon;  } return;
		case '+': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_add; } return;
		case '*': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_mlt; } return;
		case '/': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_div; } return;
		case '%': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_mod; } return;
		case '&': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_and; } return;
		case '|': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_ior; } return;
		case '^': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_xor; } return;
		case '~': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_not; } return;
		case '=': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_eq; } return;
		case '<': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_lt; } return;
		case '>': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_gt; } return;
		case '#': { weaver->nodelen = 1; cnode->type = sk_ndtp_op;    cnode->op = skvm_op_call; } return;
		case '\0': { weaver->nodelen = 0; cnode->type = sk_ndtp_eof;   cnode->val = skvm_op_hcf; } return;
		default: {
			SK_STR* custr = SK_StrFromSymoff(cu, cu->currentcompunit);
			FatalError(0, "SK_WeaverGetNxtNode:: unhandled character { %c } at %$:%4u:%4u\n", *weaver->scoff, custr->str, (u8)custr->len, weaver->col + 1, weaver->row + 1);
		}
	}
	return;
}

void SK_WeaverPeekNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver) {
	SK_WeaverGetNode(cu, weaver, 1);
}

void SK_WeaverNxtNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver) {
	SK_WeaverGetNode(cu, weaver, 0);
}

inline void SK_WeaverOpenNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u4 ndgrouptype, const u1 newscopef) {
	if (newscopef) {
		DB_Push(weaver->scopestk, &(SK_WEAVERSTKSTT){.scope.id = weaver->scope.id, .scope.symc = weaver->scope.symc });
		weaver->scope.symc = 0;
		weaver->scope.id = ++weaver->scope.counter;
	}
	SK_NODE* topnd = (SK_NODE*)DB_Peek(weaver->syntaxstk, 1);
	topnd->type = ndgrouptype;
	topnd->ext = 0;
	topnd->val = 0;
	if (ndgrouptype == sk_ndtp_branchop) {
		topnd->branchtype = sk_kw_if;
	}
	DB_Push(weaver->ndgroupstk, &(SK_WEAVERSTKSTT){.ndgroup.off = weaver->ndgroup.off, .ndgroup.len = weaver->ndgroup.len });
	weaver->ndgroup.off = DB_ElementCount(weaver->wweb);
	weaver->ndgroup.len = 0;
	DB_Push(weaver->wweb, topnd);
}

inline u4 SK_WeaverResolvedScopeSymbols(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u1* dynamicmemallocs) {
	u4 resolvedsymc = 0;
	if (weaver->scope.symc) {
		SK_WEAVERSTKSTT* head = (SK_WEAVERSTKSTT*)DB_Peek(weaver->usymstk, weaver->scope.symc);
		SK_WEAVERSTKSTT* tail = (SK_WEAVERSTKSTT*)DB_Peek(weaver->usymstk, 1);
		while (head <= tail) {
			DYNBUFF* db = *(DYNBUFF**)DB_Index(cu->ndwebs, head->usym.procidx);
			SK_NODE* nd = (SK_NODE*)DB_Index(db, head->usym.procoff);
			u8 symkey = ((u8)weaver->scope.id << 32) | nd->symoff;
			u4 symoff = PHT_Index(cu->syms, &symkey, 8);
			if (symoff + 1) {
				*dynamicmemallocs |= ((SK_SYMBOL*)DB_Index(cu->syms->pool, symoff))->type == sk_symtp_dmlay;
				nd->symf = 1;
				nd->symoff = symoff;
				resolvedsymc++;
				*head = *tail--;
			}
			else {
				head++;
			}
		}
		weaver->usymstk->occ -= resolvedsymc * sizeof(SK_WEAVERSTKSTT);
	}
	return resolvedsymc;
}

inline u1 SK_WeaverCloseNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u1 newscopef) {
	SK_WeaverDrop(weaver, 1);
	SK_WEAVERSTKSTT* prevwnodestt = (SK_WEAVERSTKSTT*)DB_SoftPop(weaver->ndgroupstk);
	weaver->ndgroup.off = prevwnodestt->ndgroup.off;
	weaver->ndgroup.len = prevwnodestt->ndgroup.len;

	if (newscopef) {
		u1 dmemallocs = 0;
		u4 resolvedsymc = SK_WeaverResolvedScopeSymbols(cu, weaver, &dmemallocs);
		SK_WEAVERSTKSTT* prevscopestt = (SK_WEAVERSTKSTT*)DB_SoftPop(weaver->scopestk);
		weaver->scope.id = prevscopestt->scope.id;
		weaver->scope.symc += prevscopestt->scope.symc - resolvedsymc;
		return dmemallocs;
	}
	return 0;
}

inline void SK_WeaverRegisterSymbol(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, SK_NODE* symnd, const u2 symboltype) {
	u8 symkey = ((u8)weaver->scope.id << 32) | symnd->symoff;
	u4 symoff = PHT_Index(cu->syms, &symkey, 8);

	if (symoff + 1) {
		SK_SYMBOL* sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symoff);
		if (sym->type != sk_symtp_undef) {
			SK_STR* symstr = (SK_STR*)DB_Index(cu->strs->pool, sym->stroff);
			SK_ErrorAtNode(cu, symnd, "redefinition of { %$ }", symstr->str, (u8)symstr->len);
			u4 redefcount = 0;
			u1 buff[512];
			do {
				symnd->symoff = SK_TableStrInsert(cu->strs, buff, CstrFmt(buff, "%$_0x%4xr0", symstr->str, (u8)symstr->len, redefcount++, (u8)8));
				symkey = ((u8)weaver->scope.id << 32) | symnd->symoff;
				symoff = PHT_Index(cu->syms, &symkey, 8);
				sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symoff);
			} while (symoff + 1 && sym->type != sk_symtp_undef);
		}
	}
	
	DB_Push(weaver->wwebstk, &(SK_WEAVERSTKSTT){ .proc.procidx = weaver->ndgroup.procidx });
	weaver->ndgroup.procidx = DB_ElementCount(cu->ndwebs);
	weaver->wweb = DB_Create(256, sizeof(SK_NODE));
	DB_Push(cu->ndwebs, &(DYNBUFF*){ weaver->wweb });
	
	symnd->symoff = PHT_Insert(cu->syms, &(u8){ symkey }, 8, & (SK_SYMBOL){.type = symboltype, .scopeid = weaver->scope.id, .stroff = symnd->symoff, .objid = weaver->ndgroup.procidx }, sizeof(SK_SYMBOL));
	DB_Push(weaver->wweb, &(SK_NODE){.type = sk_ndtp_sym, .symf = 1, .symoff = symnd->symoff });
	if (symboltype == sk_symtp_proc) {
		DB_Push(cu->procs, &(u4){ weaver->ndgroup.procidx });
	}
}

inline void SK_WeaverRegisterMemLayField(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, SK_NODE* memlaynd, SK_NODE* symnd) {
	u8 symkey = ((u8)(weaver->scope.id ^ memlaynd->mlaysym) << 32) | symnd->symoff;
	u4 symoff = PHT_Index(cu->syms, &symkey, 8);

	if (symoff + 1) {
		SK_SYMBOL* sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symoff);
		if (sym->type != sk_symtp_undef) {
			SK_STR* symstr = (SK_STR*)DB_Index(cu->strs->pool, sym->stroff);
			SK_ErrorAtNode(cu, symnd, "redefinition of { %$ }", symstr->str, (u8)symstr->len);
			u4 redefcount = 0;
			u1 buff[512];
			do {
				symnd->symoff = SK_TableStrInsert(cu->strs, buff, CstrFmt(buff, "%$_0x%4xr0", symstr->str, (u8)symstr->len, redefcount++, (u8)8));
				symkey = ((u8)(weaver->scope.id ^ memlaynd->mlaysym) << 32) | symnd->symoff;
				symoff = PHT_Index(cu->syms, &symkey, 8);
				sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symoff);
			} while (symoff + 1 && sym->type != sk_symtp_undef);
		}
	}
	symnd->symf = 1;
	symnd->symoff = PHT_Insert(cu->syms, &(u8){ symkey }, 8, & (SK_SYMBOL){.type = sk_symtp_mlaymem, .scopeid = weaver->scope.id, .stroff = symnd->symoff, .objid = weaver->ndgroup.procidx, .mlaymemoff = weaver->ndgroup.len }, sizeof(SK_SYMBOL));
}

void SK_WeaverPrintState(SK_COMPUNIT* cu, SK_NDWEAVER* wv) {
	u1 buff[512];

	SK_STR* custr = SK_StrFromSymoff(cu, cu->currentcompunit);
	Print("\n\n%$:%2u:%2u\n", custr->str, (u8)custr->len, wv->row + 1, wv->col + 1);
	u8 peekedstksize = DB_ElementCount(wv->peekedstk);
	u8 syntaxstksize = DB_ElementCount(wv->syntaxstk);
	Print("\n\x1b[31m============syntaxstack=========\ncurrent %4u %4u %4u\n", wv->ssp, wv->peeked, peekedstksize);
	for (u8 i = 0; i < syntaxstksize; ++i) {
		SK_NODE* nd = (SK_NODE*)DB_Index(wv->syntaxstk, i);
		SK_Node2StrSimp(cu, nd, buff);
		Print("%s", buff);
	}
	Print("\n\x1b[31m============peekedstack=========\n");
	for (u8 i = wv->peeked; i < peekedstksize; ++i) {
		SK_NODE* nd = (SK_NODE*)DB_Index(wv->peekedstk, i);
		SK_Node2Str(cu, nd, buff);
		Print("%s", buff);
	}
	Print("\n\x1b[32m============wweb============\n");
	Print("current: %4i %4i\n", wv->ndgroup.procidx, DB_ElementCount(cu->ndwebs));
	u4 wprocstacksize = DB_ElementCount(wv->wwebstk);
	for (u4 i = 0; i < wprocstacksize; i++) {
		SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_Index(wv->wwebstk, i);
		Print("(%4i)", stkstt->proc.procidx);
	}
	Print("\n");
	if ((u8)wv->wweb != (u8)(-1)) {
		u8 nodepoolsize = DB_ElementCount(wv->wweb);
		for (u8 i = 0; i < nodepoolsize; ++i) {
			SK_NODE* nd = (SK_NODE*)DB_Index(wv->wweb, i);
			SK_Node2Str(cu, nd, buff);
			Print("%s", buff);
			if (i % 10 == 0) { Print("\n"); }
		}
	}
	Print("\n\x1b[33m============ndgroupstack==========\n");
	Print("current: %4u %4u\n", wv->ndgroup.off, wv->ndgroup.len);
	u8 wnodestksize = DB_ElementCount(wv->ndgroupstk);
	for (u8 i = 0; i < wnodestksize; ++i) {
		SK_WEAVERSTKSTT* entry = (SK_WEAVERSTKSTT*)DB_Index(wv->ndgroupstk, i);
		Print("(%4u %4u)", entry->ndgroup.off, entry->ndgroup.len);
	}
	Print("\n\x1b[34m============strentries==========\n");
	u4 strsentrycount = DB_Size(cu->strs->entries);
	u4 stroccentries = DB_ElementCount(cu->strs->entries);
	Print("%4u/%4u\n", stroccentries, strsentrycount);
	PHTENTRY* strentry = (PHTENTRY*)DB_Index(cu->strs->entries, 0);
	for (u4 i = 0; i < strsentrycount; i++) {
		if (strentry->state) {
			SK_STR* skstr = (SK_STR*)DB_Index(cu->strs->pool, strentry->off);
			CstrFromRawBytes(skstr->str, (u4)skstr->len, buff, 256);
			Print("(%4u:%s)", strentry->off, buff);
		}
		strentry++;
	}
	Print("\n\x1b[35m===========symentries===========\n");
	u4 symsentrycount = DB_Size(cu->syms->entries);
	u4 symsoccentries = DB_ElementCount(cu->syms->entries);
	Print("%4u/%4u\n", symsoccentries, symsentrycount);
	PHTENTRY* symentry = (PHTENTRY*)DB_Index(cu->syms->entries, 0);
	for (u4 i = 0; i < symsentrycount; i++) {
		if (symentry->state) {
			SK_SYMBOL* sksym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symentry->off);
			SK_STR* skstr = (SK_STR*)DB_Index(cu->strs->pool, sksym->stroff);
			CstrFromRawBytes(skstr->str, (u4)skstr->len, buff, 256);
			Print("(%4u:%4u:%4u %s %s)", symentry->off, sksym->objid, sksym->scopeid, buff, sk_symtp2str[sksym->type]);
		}
		symentry++;
	}
	Print("\n\x1b[36m========scopestack==============\n");
	Print("current: %4u %4u %4u\n", wv->scope.id, wv->scope.counter, wv->scope.symc);
	u8 scopestksize = DB_ElementCount(wv->scopestk);
	for (u8 i = 0; i < scopestksize; ++i) {
		SK_WEAVERSTKSTT* entry = (SK_WEAVERSTKSTT*)DB_Index(wv->scopestk, i);
		Print("(%4u %4u)", entry->scope.id, entry->scope.symc);
	}
	Print("\n============unresolvedsyms======\n");
	u8 symstksize = DB_ElementCount(wv->usymstk);
	for (u8 i = 0; i < symstksize; ++i) {
		SK_WEAVERSTKSTT* symoff = (SK_WEAVERSTKSTT*)DB_Index(wv->usymstk, i);
		Print("(%4u %4u)", symoff->usym.procidx, symoff->usym.procoff);
	}
	Print("\n================================\x1b[37m\n\n\n\n");
}

inline SK_NODE* SK_WeaverCloseWellFormedMemLayout(SK_COMPUNIT* cu, SK_NDWEAVER* wv) {
	SK_NODE* mlaymemdesc = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
	mlaymemdesc->mlaymemc = wv->ndgroup.len;

	if (!mlaymemdesc->mlaymemc) {
		mlaymemdesc->mlaymemc++;
		SK_ErrorAtNode(cu, mlaymemdesc, "memory layout is missing members");
		DB_Push(wv->wweb, &(SK_NODE){.type = sk_ndtp_mlaymem, .mlaymemsize = 1, .mlaymemsym = 0, .mlaymemf = 0b001 });
		DB_Push(wv->wweb, &(SK_NODE){.type = sk_ndtp_type, .prim = sk_tp_u4, .primlvl = 0, .primsize = 4 });
		mlaymemdesc = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);// incase of regrow of the buffer
	}

	SK_WeaverCloseNodeGroup(cu, wv, 0);

	SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv->wwebstk);
	wv->ndgroup.procidx = stkstt->proc.procidx;
	if (wv->ndgroup.procidx == (u4)(-1)) {
		wv->wweb = (DYNBUFF*)(-1);
	}
	else {
		wv->wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv->ndgroup.procidx);
	}
	return mlaymemdesc;
}

inline SK_NODE* SK_WeaverCloseMalFormedMemLayout(SK_COMPUNIT* cu, SK_NDWEAVER* wv) {
	SK_NODE* mlaydesc = (SK_NODE*)DB_Peek(wv->wweb, 1);
	if (mlaydesc->type == sk_ndtp_dmlay || mlaydesc->type == sk_ndtp_smlay) {
		wv->ndgroup.len++;
		DB_Push(wv->ndgroupstk, &(SK_WEAVERSTKSTT){.ndgroup.off = wv->ndgroup.off, .ndgroup.len = wv->ndgroup.len });
		wv->ndgroup.off = DB_ElementCount(wv->wweb);
		wv->ndgroup.len = 0;
		SK_NODE tmp = (SK_NODE){ .type = sk_ndtp_mlaymem, .mlayf = 0b001, .mlaymemsize = 1, .mlaymemsym = 0 };
		SK_WeaverPush(wv, &tmp);
		DB_Push(wv->wweb, &tmp);
	}
	DB_Push(wv->wweb, &(SK_NODE){.type = sk_ndtp_type, .prim = sk_tp_u4 });
	SK_WeaverCloseNodeGroup(cu, wv, 0);

	SK_NODE* ndmemlay = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
	SK_WeaverCloseWellFormedMemLayout(cu, wv);
	return ndmemlay;
}

// this assumes that the top of the syntax stack is a symbol
inline u4 SK_WeaverTrySymWeaving(SK_COMPUNIT* cu, SK_NDWEAVER* wv) {
	SK_WeaverNxtNode(cu, wv);
	SK_NODE* nxtnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
	SK_NODE* symnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);

	// sym nxt
	if (nxtnd->type == sk_ndtp_delim) {
		u4 retv = 0;
		if (nxtnd->val == sk_kw_colon) {
			SK_WeaverNxtNode(cu, wv);
			nxtnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
			symnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 3);
			switch (nxtnd->type) {
				case sk_ndtp_delim: {
					switch (nxtnd->val) {
						case sk_kw_lsqr: { retv = sk_ndtp_smlay; } break;
						case sk_kw_lbra: { retv = sk_ndtp_dmlay; } break;
						case sk_kw_lpar: { retv = sk_ndtp_procdef; } break;
						default: { retv = sk_ndtp_solo; } break;
					}
				} break;
				case sk_ndtp_type: { // for variables, not implemented yet
					SK_ErrorAtNode(cu, symnd, "var definitons are not implemented yet\n");
					SK_CompUnitFlushErrors(cu);
					FatalError(0, "");
				} return 0;
				default: {
					retv = sk_ndtp_solo;
				} break;
			}
		}
		else if (nxtnd->val == sk_kw_dot) { // inside of a mlaymem
			SK_WeaverNxtNode(cu, wv);
			nxtnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
			if (nxtnd->type == sk_ndtp_sym) {
				retv = sk_ndtp_memoff;
			}
			else {
				retv = sk_ndtp_solo;
			}
		}
		SK_WeaverPushPeek(wv, 2);
		SK_WeaverDrop(wv, 2);
		return retv;
	}

	if (nxtnd->type == sk_ndtp_type) { // inside of a mlaymem
		SK_WeaverPushPeek(wv, 1);
		SK_WeaverDrop(wv, 1);
		return sk_ndtp_mlaymem;
	}
	SK_WeaverPushPeek(wv, 1);
	SK_WeaverDrop(wv, 1);
	return sk_ndtp_solo;
}

void SK_WeaverErrorRecovery(SK_COMPUNIT* cu, SK_NDWEAVER* wv, u4 contxt, u4 failedweb) {
	switch (contxt) {
		case sk_ndtp_sym: {
			if (wv->ssp >= 2) {
				SK_NODE* tmp = { 0 };
				// memcpy(tmp, nxtnd, sizeof(SK_NODE));
				// ... sym
				SK_NODE* symnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_NODE* prvnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
				switch (prvnd->type) {
					case sk_ndtp_mlaymem: {
						FatalError(0, "SK_WeaverErrorRecovery: sym 1\n");
					} break;
					case sk_ndtp_smlay: {
						// FatalError(0, "SK_WeaverErrorRecovery: sym 2\n");
						if (failedweb == sk_ndtp_solo) {
							SK_ErrorAtNode(cu, symnd, 0);
							SK_WeaverDrop(wv, 1);
							return;
						}
						else {
							SK_ErrorAtNode(cu, prvnd, "mising ] for static memory layout definiton's block");
							symnd->type = sk_ndtp_symdef;
							SK_NODE tmp = { 0 };
							memcpy(&tmp, symnd, sizeof(SK_NODE));
							SK_WeaverDrop(wv, 1);//drops the symbol
							SK_WeaverCloseMalFormedMemLayout(cu, wv);
							SK_WeaverPush(wv, &tmp);
							SK_WeaverDropPeek(wv, 1);
						}
					} break;
					case sk_ndtp_dmlay: {
						FatalError(0, "SK_WeaverErrorRecovery: sym 3\n");
					} break;
				}
				// FatalError(0, "SK_WeaverErrorRecovery: sym 4\n");
			}
			else {
				// syntax: sym
				// peeked: ??? ...
				SK_WeaverNxtNode(cu, wv);
				SK_NODE* nxtnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_NODE* symnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
				if (nxtnd->type == sk_ndtp_delim && (nxtnd->val == sk_kw_lpar || nxtnd->val == sk_kw_lsqr)) {
					SK_STR* symstr = (SK_STR*)DB_Index(cu->strs->pool, symnd->symoff);
					SK_ErrorAtNode(cu, symnd, "missing { : } after symbol { %$ }", symstr->str, (u8)symstr->len);
					symnd->type = sk_ndtp_symdef;
					SK_WeaverPushPeek(wv, 1);
					SK_WeaverDrop(wv, 1);
					return;
				}
				SK_WeaverNxtNode(cu, wv);
				// sym ??? nxt
				nxtnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_NODE* foreignnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
				symnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 3);

				if (nxtnd->type == sk_ndtp_delim && (nxtnd->val == sk_kw_lpar || nxtnd->val == sk_kw_lsqr)) {
					SK_STR* symstr = (SK_STR*)DB_Index(cu->strs->pool, symnd->symoff);
					u1 buff[512];
					SK_Node2StrSimp(cu, foreignnd, buff);
					SK_ErrorAtNode(cu, foreignnd, "misplaced { %s } after symbol { %$ }; expected { : } instead", buff, symstr->str, (u8)symstr->len);

					symnd->type = sk_ndtp_symdef;
					SK_WeaverPushPeek(wv, 1);
					SK_WeaverDrop(wv, 2);
					return;
				}

				SK_ErrorAtNode(cu, symnd, 0);
				// sym ??? ???
				SK_WeaverPushPeek(wv, 2);
				SK_WeaverDrop(wv, 3);
				return;
			}
		} break;
	}
}


// TODO:: take advange from the new design, an emmit things the momment you know you can do it
// instead of following the old path this implies a rewrite but it would be nicer
// TODO:: make funcs to close and open a func/branch/loop/etc
// TODO:: handle all cases where matching branches fall through,and replace the 'rogue node' error for proper recovery when need it 
void SK_WeaverWeaveProgram(SK_COMPUNIT* cu, const u2 debugmode) {
	u1 buff[256];

	SK_NDWEAVER* wv = (SK_NDWEAVER*)Malloc(sizeof(SK_NDWEAVER));
	memset(wv, 0, sizeof(SK_NDWEAVER));
		
	SK_STR* custr = SK_StrFromSymoff(cu, cu->currentcompunit);
	CstrFmt(buff, "%$", custr->str, (u8)custr->len);
	wv->sourcecode = LoadFile(buff);
	if (!wv->sourcecode) {
		SK_ErrorAtNode(cu, 0, "couln't open the file { %s }\n", buff);
		cu->errorcount++;
		Free(wv);
		return;
	}
	wv->wweb = (DYNBUFF*)(-1);
	wv->ndgroup.procidx = -1;
	wv->scoff = wv->sourcecode;
	wv->peekedstk  = DB_Create(256, sizeof(SK_NODE));
	wv->syntaxstk  = DB_Create(256, sizeof(SK_NODE));
	wv->wwebstk    = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // prev stack and off
	wv->scopestk   = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // scope
	wv->ndgroupstk = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // ndgroup wnodecount
	wv->usymstk    = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // proc and offset

	DB_Push(wv->wwebstk, &(SK_WEAVERSTKSTT){.proc.procidx = wv->ndgroup.procidx }); // global working web
	wv->ndgroup.procidx = DB_ElementCount(cu->ndwebs);
	wv->wweb = DB_Create(256, sizeof(SK_NODE));
	DB_Push(cu->ndwebs, &(DYNBUFF*){ wv->wweb });

	if (debugmode & sk_dbmd_weaver) {
		Print("\n\n===========source==========\n\n");
		u4 sourcedoesize = CstrLen(wv->sourcecode);
		FS_Write(STDOUT, wv->sourcecode, sourcedoesize);
		Print("\n\n===========================\n\n");
		Print("\n\n===========weaving===========\n\n");
	}
	u1 eoffound = 0;
	do {
	nxtnd:
		SK_WeaverNxtNode(cu, wv);
	skp:if (debugmode & sk_dbmd_weaver) { SK_WeaverPrintState(cu, wv); }

		switch (((SK_NODE*)DB_Peek(wv->syntaxstk, 1))->type) {
			case sk_ndtp_sym: {
				u4 symweb = SK_WeaverTrySymWeaving(cu, wv);

				SK_NODE* symnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_NODE* prvnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
				switch (symweb) {
					case sk_ndtp_solo: {
						if (prvnd && prvnd->type == sk_ndtp_ops) {
							DB_Push(wv->usymstk, &(SK_WEAVERSTKSTT){.usym.procidx = wv->ndgroup.procidx, .usym.procoff = DB_ElementCount(wv->wweb) });
							DB_Push(wv->wweb, symnd);
							SK_WeaverDrop(wv, 1);
							wv->scope.symc++;
							wv->ndgroup.len++;
							goto nxtnd;
						}
						// TODO:: for now we only admit primitive types as memlaymems, add recursive memlayouts and proper proctypes
						if (prvnd && prvnd->type == sk_ndtp_memoff) {
							// TODO:: here we should also divide this into many nodes instead for constructs like
							// struct.field.subfield0.subfield1.subfield2. ... but for now let's just restrict it to 1 offset
							// until i create proper mememory layouts types and proc types
							DB_Push(wv->wweb, symnd);
							SK_WeaverDrop(wv, 2); // the symbol the memoff
							goto nxtnd;
						}
					} break;
					case sk_ndtp_memoff: {
						if (!prvnd || (prvnd && prvnd->type == sk_ndtp_ops)) {
							DB_Push(wv->usymstk, &(SK_WEAVERSTKSTT){ .usym.procidx = wv->ndgroup.procidx, .usym.procoff = DB_ElementCount(wv->wweb) });
							symnd->type = sk_ndtp_memoff;
							DB_Push(wv->wweb, symnd);
							SK_WeaverDropPeek(wv, 1);
							goto nxtnd;
						}
					} break;
					case sk_ndtp_procdef:
					case sk_ndtp_smlay: {
						if (!prvnd || (prvnd && prvnd->type == sk_ndtp_ops)) {
							symnd->type = sk_ndtp_symdef;
							SK_WeaverDropPeek(wv, 1);
							goto nxtnd;
						}
					} break;
					case sk_ndtp_dmlay: {
						if (prvnd && prvnd->type == sk_ndtp_ops) {
							symnd->type = sk_ndtp_symdef;
							SK_WeaverDropPeek(wv, 1);
							goto nxtnd;
						}
					} break;
					case sk_ndtp_mlaymem: {
						// TODO:: add the case when there's a mlaymem at the top, it must only a size descriptor for now. see the TODO below 
						if (prvnd && (prvnd->type == sk_ndtp_dmlay || prvnd->type == sk_ndtp_smlay)) {
							// TODO:: do some error handling here that is for now a field can't have 2 names later nth names for the same field could be seen as unions
							wv->ndgroup.len++;
							SK_WeaverRegisterMemLayField(cu, wv, prvnd, symnd);
							u4 symoff = symnd->symoff;
							SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_mlaymem, 0);
							SK_NODE* ndmlaymem = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
							ndmlaymem->mlaymemsym = symoff;
							ndmlaymem->mlaymemf = 0b010;
							ndmlaymem->mlaymemsize = 1;
							goto nxtnd;
						}
					} break;
				}
				SK_WeaverErrorRecovery(cu, wv, sk_ndtp_sym, symweb);
			} break;
			case sk_ndtp_delim: {
				SK_NODE* delimnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				if (wv->ssp >= 2) {
					SK_NODE* prvnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
					switch (delimnd->val) {
						case sk_kw_colon: {
							if (prvnode->type == sk_ndtp_proctype) {
								SK_NODE* ndproctype = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								// TODO:: Error report duplicates ":"
								ndproctype->argsretsf = 1;
								ndproctype->argsc = wv->ndgroup.len;
								wv->ndgroup.len = 0;
								SK_WeaverDrop(wv, 1); // ignores the delim
							}
							else {
								SK_ErrorAtNode(cu, delimnd, 0);
								SK_WeaverDrop(wv, 1); // ignores the delim
							}
						} break;
						case sk_kw_lpar: {
							// cmd( -> cmd, cmdstrs ; symdef(->procdef, proctype						
							switch (prvnode->type) {
								case sk_ndtp_symdef: {
									prvnode->symf = 1;
									prvnode->type = sk_ndtp_procdef;
									SK_WeaverRegisterSymbol(cu, wv, prvnode, sk_symtp_proc);
									SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_proctype, 0);
								} break;
								case sk_ndtp_cmd: {
									u8 tmp = prvnode->cmdtp;
									SK_WeaverDrop(wv, 1);
									SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_cmdstrs, 0);
									SK_NODE* ndcmdstrs = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
									ndcmdstrs->cmdtp = tmp; 
								} break;
								case sk_ndtp_ops: {
									wv->ndgroup.len++;
									SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_cast, 0);
								} break;
								default: {
									SK_ErrorAtNode(cu, delimnd, 0);
									SK_WeaverDrop(wv, 1);
								} break;
							}
						} break;
						case sk_kw_rpar: {
							SK_WeaverDrop(wv, 1); // )
							if (prvnode->type == sk_ndtp_proctype) {
								// procdef proctype -> procdef
								SK_NODE* ndproctype = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								ndproctype->argsrets[ndproctype->argsretsf] = wv->ndgroup.len;
								if (debugmode & sk_dbmd_weaver) {
									Print("signature:\n");

									Print("args %4u:\n", ndproctype->argsc);
									for (u8 i = 0; i < ndproctype->argsc; ++i) {
										SK_Node2Str(cu, ndproctype + 1 + i, buff);
										Print("%s", buff);
									}
									Print("\nrets %4u:\n", ndproctype->retsc);
									for (u8 i = ndproctype->argsc; i < ndproctype->argsc + ndproctype->retsc; ++i) {
										SK_Node2Str(cu, ndproctype + 1 + i, buff);
										Print("%s", buff);
									}
								}
								SK_WeaverCloseNodeGroup(cu, wv, 0);
							}
							else if (prvnode->type == sk_ndtp_cmdstrs) {
								SK_WeaverDrop(wv, 1);
								
								SK_NODE * ndcmdstrs = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								ndcmdstrs->cmdstrc = wv->ndgroup.len;
								if (debugmode & sk_dbmd_weaver) {
									Print("cmds %4u:\n", ndcmdstrs->cmdstrc);
									for (u8 i = 0; i < ndcmdstrs->cmdstrc; ++i) {
										SK_Node2Str(cu, ndcmdstrs + 1 + i, buff);
										Print("%s", buff);
									}
								}
								if (ndcmdstrs->cmdtp == sk_kw_extrn) {
									if (ndcmdstrs->cmdstrc != 2) {
										SK_ErrorAtNode(cu, ndcmdstrs, "the extern command needs exactly 2 arguments");
									}

									SK_WeaverCloseNodeGroup(cu, wv, 0);

									SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv->wwebstk);
									wv->ndgroup.procidx = stkstt->proc.procidx;
									if (wv->ndgroup.procidx == (u4)(-1)) {
										wv->wweb = (DYNBUFF*)(-1);
									}
									else {
										wv->wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv->ndgroup.procidx);
									}
								}
								if (ndcmdstrs->cmdtp == sk_kw_include) {
									if (!ndcmdstrs->cmdstrc) {
										SK_ErrorAtNode(cu, ndcmdstrs, "the include command needs at least 1 argument");
									}
									else {
										u4 totaldir = ndcmdstrs->cmdstrc;
										u4 fdiroff = wv->ndgroup.off;
										for (u4 i = 1; i <= totaldir; i++) {
											SK_NODE* fdirnd = (SK_NODE*)DB_Index(wv->wweb, fdiroff + i);
											SK_SYMBOL* fdirsym = (SK_SYMBOL*)DB_Index(cu->syms->pool, fdirnd->symoff);
											if (fdirsym->state == 2) { continue; }
											if (fdirsym->state == 1) {
												SK_STR* fdirstr = (SK_STR*)DB_Index(cu->strs->pool, fdirsym->stroff);
												SK_ErrorAtNode(cu, fdirnd, "circular include of { %$ }", fdirstr->str, (u8)fdirstr->len);
												continue;
											}


											u4 main_cwdlen = cu->cwdlen;
											u1* main_cwd = cu->cwd;
											
											SK_STR* fdirstr = (SK_STR*)DB_Index(cu->strs->pool, fdirsym->stroff);

											cu->cwd = Malloc(MAX_PATH);
											cu->cwdlen = fdirstr->len-1;
											while (cu->cwdlen && fdirstr->str[cu->cwdlen] != '\\') { cu->cwdlen--; }
											memcpy(cu->cwd, fdirstr->str, cu->cwdlen);

											fdirsym->state = 1;
											u4 maincu = cu->currentcompunit;
											cu->currentcompunit = fdirnd->symoff;
											SK_WeaverWeaveProgram(cu, debugmode);
											DB_Push(cu->compunits, &(u4){ cu->currentcompunit });
											cu->currentcompunit = maincu;
											fdirsym->state = 2;

											Free(cu->cwd);
											cu->cwdlen = main_cwdlen;
											cu->cwd = main_cwd;
										}
									}
								}
							}
							else if (prvnode->type == sk_ndtp_cast) {
								SK_NODE* castnd = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								if (!castnd->castf) {
									SK_ErrorAtNode(cu, castnd, "casting needs a type");
									castnd->prim = sk_tp_u4;
									castnd->castf = 1;
								}
								SK_WeaverCloseNodeGroup(cu, wv, 0);
							}
							else {
								SK_ErrorAtNode(cu, delimnd, 0);
							}
						} break;
						case sk_kw_lsqr: {
							// symdef [ -> smlay
							SK_WeaverDrop(wv, 1);
							if (prvnode->type == sk_ndtp_symdef) {
								SK_WeaverRegisterSymbol(cu, wv, prvnode, sk_symtp_smlay);
								u4 symoff = prvnode->symoff;
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_smlay, 0);
								SK_NODE* memlaynd = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								memlaynd->mlaysym = symoff;
							}
							else {
								SK_ErrorAtNode(cu, delimnd, 0);
							}
						} break;
						case sk_kw_rsqr: {
							SK_WeaverDrop(wv, 1);
							if (prvnode->type == sk_ndtp_smlay) {
								SK_WeaverCloseWellFormedMemLayout(cu, wv);
							}
							else if (prvnode->type == sk_ndtp_mlaymem) {
								SK_ErrorAtNode(cu, (SK_NODE*)DB_Peek(wv->syntaxstk, 1), "malformed memory layout member missing typing");
								SK_WeaverCloseMalFormedMemLayout(cu, wv);
							}
							else {
								SK_ErrorAtNode(cu, delimnd, 0);
							}
						} break;
						case sk_kw_lbra: {
							// procdef {-> procdef ops; symdef { -> smemlay; flow ops { -> flow ops; flow { -> flow ops
							// TODO:: add support for just blocks {} to have access to temporal scopes
							if (prvnode->type == sk_ndtp_procdef) {
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 1);
							}
							else if (wv->ssp >= 3 && prvnode->type == sk_ndtp_ops && (prvnode - 1)->type == sk_ndtp_loopop) {
								SK_WeaverDrop(wv, 1);
								SK_NODE* ndops = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								ndops->opsc = wv->ndgroup.len;
								if (!wv->ndgroup.len) {
									SK_ErrorAtNode(cu, ndops, "missing while's condtion");
								}
								SK_WeaverCloseNodeGroup(cu, wv, 0); // drops the condtion
								SK_NODE* ndloopop = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								if (ndloopop->loopf) { // TODO:: for now this an error
									FatalError(0, "blocks are not implemented inside of a loopop yet");
								}
								ndloopop->loopf = 1;
								// SK_WeaverPush should never grow the stack since i just made space
								// there's no need for this other than to keep track of the proper row and col 
								SK_WeaverPush(wv, delimnd);
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 1);
							}
							// TODO::flow ops { could get into it's on func to ommit the code repetition
							else if (wv->ssp >= 3 && prvnode->type == sk_ndtp_ops && (prvnode - 1)->type == sk_ndtp_branchop) {
								SK_WeaverDrop(wv, 1);
								SK_NODE* ndops = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								ndops->opsc = wv->ndgroup.len;
								if (!wv->ndgroup.len) {
									SK_ErrorAtNode(cu, ndops, "missing %s's condtion", sk_kw2str[(prvnode - 1)->branchtype]);
								}
								SK_WeaverCloseNodeGroup(cu, wv, 0);
								SK_NODE* ndbranchop = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								if (ndbranchop->branchf) { // TODO:: for now this an error
									FatalError(0, "blocks are not implemented inside of a branchop yet");
								}
								ndbranchop->branchf = 1;
								SK_WeaverPush(wv, delimnd);
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 1);
							}
							else if (prvnode->type == sk_ndtp_branchop) {
								SK_NODE* ndbranchop = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
								ndbranchop->branchf = 1;
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 1); // no need for push here since { would be overwritten
							}
							else if (prvnode->type == sk_ndtp_symdef && (prvnode - 1)->type == sk_ndtp_ops) {
								SK_WeaverDrop(wv, 1);
								wv->ndgroup.len++;
								SK_WeaverRegisterSymbol(cu, wv, prvnode, sk_symtp_dmlay);
								u4 symoff = prvnode->symoff;
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_dmlay, 0);
								((SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off))->mlaysym = symoff;
								prvnode->mlaysym = symoff;
							}
							else {
								SK_ErrorAtNode(cu, delimnd, 0);
								SK_WeaverDrop(wv, 1);
							}
						} break;
						case sk_kw_rbra: {
							// TODO:: unnamed procs? let's not support this for now
							// it could be good if i need to create arrays of procs or pass them as an argument, etc
							SK_WeaverDrop(wv, 1); // drops the }
							switch (prvnode->type) {
								case sk_ndtp_ops: {
									// ... ops
									SK_NODE* ndops = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
									ndops->opsc = wv->ndgroup.len;
									if (debugmode & sk_dbmd_weaver) {
										Print("ops %4u:\n", ndops->opsc);
										for (u8 i = 0; i < ndops->opsc; ++i) {
											SK_Node2Str(cu, ndops + 1 + i, buff);
											Print("%s", buff);
										}
									}

									if (wv->ssp >= 2 && (prvnode - 1)->type == sk_ndtp_procdef) {
										SK_WeaverCloseNodeGroup(cu, wv, 1); // closeses the ops
										SK_WeaverDrop(wv, 1); // drops the procdef

										// TODO:: i should put this inside of a func
										SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv->wwebstk);
										wv->ndgroup.procidx = stkstt->proc.procidx;
										if (wv->ndgroup.procidx == (u4)(-1)) {
											wv->wweb = (DYNBUFF*)(-1);
										}
										else {
											wv->wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv->ndgroup.procidx);
										}
									}
									else if (wv->ssp >= 2 && (prvnode - 1)->type == sk_ndtp_loopop) {
										u1 dmemallocs = SK_WeaverCloseNodeGroup(cu, wv, 1);
										SK_NODE* ndloop = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
										ndloop->loopdmemalloc = dmemallocs;
										if (!ndloop->loopf) {
											SK_ErrorAtNode(cu, ndloop, "missing \"{\" after while condition");
										}
										SK_WeaverCloseNodeGroup(cu, wv, 0);
									}
									else if (wv->ssp >= 2 && (prvnode - 1)->type == sk_ndtp_branchop) {
										// branchop ops
										u1 dmemallocs = SK_WeaverCloseNodeGroup(cu, wv, 1);
										SK_NODE* ndbranchop = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
										if (!ndbranchop->branchf) {
											SK_ErrorAtNode(cu, ndbranchop, "missing \"{\" after %s condition", sk_kw2str[ndbranchop->branchtype]);
										}
										// ops branchop
										switch (ndbranchop->branchtype) {
										case sk_kw_if:
										case sk_kw_elif: {
											SK_WeaverNxtNode(cu, wv);
											SK_NODE* topnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
											if (topnode->type == sk_ndtp_flow && (topnode->val == sk_kw_else || topnode->val == sk_kw_elif)) {
												ndbranchop->branchc++;
												ndbranchop->branchf = topnode->val == sk_kw_else;
												ndbranchop->branchtype = topnode->val;
												if (topnode->val == sk_kw_elif) {
													SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 0); // here the kwnd becomes the condition nd
												}
												else {
													SK_WeaverDrop(wv, 1); // here we got a dangling else kwnd thus drop it 
												}
												SK_WeaverNxtNode(cu, wv);
											}
											else {
												ndbranchop->branchf = 2;
												SK_WeaverDrop(wv, 1);
												SK_WeaverCloseNodeGroup(cu, wv, 0);
												SK_WeaverPush(wv, topnode); // weaverClose and drop, only reduce the syntax stack so it's safe to use topnode
											}
											goto skp;
										} break;
										case sk_kw_else: {
											// merge ops branchop ops
											ndbranchop->branchf = 2;
											SK_WeaverCloseNodeGroup(cu, wv, 0);
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
								case sk_ndtp_dmlay: {
									if (prvnode->type == sk_ndtp_dmlay) {
										SK_NODE* mlaymemdesc = SK_WeaverCloseWellFormedMemLayout(cu, wv);
										if ((u8)wv->wweb != (u8)(-1)) {
											DB_Push(wv->wweb, mlaymemdesc);
											SK_NODE* ndops = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
											ndops->opsf |= 1;
										}
									}
									else {
										SK_ErrorAtNode(cu, delimnd, 0);
									}
								} break;
								default: {
									if (prvnode->type == sk_ndtp_mlaymem) {
										SK_ErrorAtNode(cu, (SK_NODE*)DB_Peek(wv->syntaxstk, 1), "malformed memory layout member missing typing");
										SK_NODE* mlaymemdesc = SK_WeaverCloseMalFormedMemLayout(cu, wv);
										if ((u8)wv->wweb != (u8)(-1)) {
											DB_Push(wv->wweb, mlaymemdesc);
											SK_NODE* ndops = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
											ndops->opsf |= 1;
										}
									}
									else {
										SK_ErrorAtNode(cu, delimnd, 0);
									}
								} break;
							}
						} break;
						default: {
							FatalError(0, "SK_Weaver_WebProgram::unreachable { %s:\"%$\" }", sk_ndtp2str[delimnd->type], wv->scoff, (u8)wv->nodelen);
						} break;
					}
				}
				else {
					SK_ErrorAtNode(cu, delimnd, 0);
					SK_WeaverDrop(wv, 1);
				}
			}break;
			case sk_ndtp_type: {
				SK_NODE* typend = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_NODE* prvnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
				if (wv->ssp >= 2) {
					if (prvnode->type == sk_ndtp_proctype) {
						DB_Push(wv->wweb, typend);
						wv->ndgroup.len++;
						SK_WeaverDrop(wv, 1);
					}
					else if (prvnode->type == sk_ndtp_mlaymem) {
						((SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off))->mlaymemf |= 0b001;
						DB_Push(wv->wweb, typend);
						SK_WeaverDrop(wv, 1);
						SK_WeaverCloseNodeGroup(cu, wv, 0);
					}
					else if (prvnode->type == sk_ndtp_smlay || prvnode->type == sk_ndtp_dmlay) {
						DB_Push(wv->wweb, &(SK_NODE){.type = sk_ndtp_mlaymem, .mlaymemsize = 1, .mlaymemsym = 0, .mlaymemf = 0b001 });
						DB_Push(wv->wweb, typend);
						SK_WeaverDrop(wv, 1);
						wv->ndgroup.len++;
					}
					else if (prvnode->type == sk_ndtp_cast) {
						SK_NODE* castnd = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
						castnd->castf = 1;
						castnd->prim = typend->prim;
						if (typend->prim == sk_tp_ptr) {
							castnd->primlvl += 1;
						}
						SK_WeaverDrop(wv, 1);
					}
					else {
						SK_ErrorAtNode(cu, typend, 0);
						SK_WeaverDrop(wv, 1);
					}
				}
				else {
					SK_ErrorAtNode(cu, typend, 0);
					SK_WeaverDrop(wv, 1);
				}
			} break;
			case sk_ndtp_num: {
				SK_NODE* numnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				if (wv->ssp >= 2) {
					SK_NODE* prvnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
					u8 syntaxstksize = DB_ElementCount(wv->syntaxstk);
					if (prvnode->type == sk_ndtp_ops) {
						DB_Push(wv->wweb, numnd);
						SK_WeaverDrop(wv, 1);
						wv->ndgroup.len++;
					}
					else if (prvnode->type == sk_ndtp_smlay || prvnode->type == sk_ndtp_dmlay) {
						// TODO:: do some error handling here you can't have more than 1 number at any given time
						wv->ndgroup.len++;
						u8 val = numnd->val;
						SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_mlaymem, 0);
						SK_NODE* nmlaymem = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
						nmlaymem->mlaymemsize = val;
						nmlaymem->mlaymemf = 0b100;
					}
					else {
						SK_ErrorAtNode(cu, numnd, 0);
						SK_WeaverDrop(wv, 1);
					}
				}
				else {
					SK_ErrorAtNode(cu, numnd, 0);
					SK_WeaverDrop(wv, 1);
				}
			} break;
			case sk_ndtp_op: {
				SK_NODE* opnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_NODE* prvnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
				if (prvnode && prvnode->type == sk_ndtp_ops) {
					DB_Push(wv->wweb, opnd);
					SK_WeaverDrop(wv, 1);
					wv->ndgroup.len++;
				}
				else {
					SK_ErrorAtNode(cu, opnd, 0);
					SK_WeaverDrop(wv, 1);
				}
			} break;
			case sk_ndtp_flow: {
				SK_NODE* flownd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				if (wv->ssp >= 2) {
					SK_NODE* prvnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
					switch (flownd->val) {
					case sk_kw_if:
					case sk_kw_while: {
						if (prvnode->type == sk_ndtp_ops) {
							wv->ndgroup.len++;
							SK_WeaverOpenNodeGroup(cu, wv, flownd->val == sk_kw_while ? sk_ndtp_loopop : sk_ndtp_branchop, 0);
							SK_NODE tmp = { 0 };
							tmp.col = flownd->col;
							tmp.row = flownd->row;
							SK_WeaverPush(wv, &tmp);
							SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 0);
						}
						else {
							SK_ErrorAtNode(cu, flownd, 0);
							SK_WeaverDrop(wv, 1);
						}
					} break;
					default: {
						// TODO:: i am not sure if this is okay? it's from the previous iteration, i should review this part later
						SK_ErrorAtNode(cu, flownd, 0);
						if (prvnode->type == sk_ndtp_ops && (flownd->val == sk_kw_elif || flownd->val == sk_kw_else)) {
							wv->ndgroup.len++;
							u8 branchtype = flownd->val;
							SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_branchop, 0);
							((SK_NODE*)DB_Peek(wv->syntaxstk, 1))->branchtype = branchtype;
							if (branchtype == sk_kw_elif) {
								SK_NODE tmp = { 0 };
								tmp.col = flownd->col;
								tmp.row = flownd->row;
								SK_WeaverPush(wv, &tmp);
								SK_WeaverOpenNodeGroup(cu, wv, sk_ndtp_ops, 0);
							}
						}
					} break;
					}
				}
				else {
					SK_ErrorAtNode(cu, flownd, 0);
					SK_WeaverDrop(wv, 1);
				}
			} break;
			case sk_ndtp_str: {
				SK_NODE* strnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				if (wv->ssp >= 2) {
					if ((strnd - 1)->type == sk_ndtp_ops || (strnd - 1)->type == sk_ndtp_cmdstrs) {
						strnd->strf = 1;
						
						u4 symtp = sk_symtp_str;
						SK_NODE*  cmdnd = (SK_NODE*)DB_Index(wv->wweb, wv->ndgroup.off);
						u4 stroff;
						u4 symoff;
						if (cmdnd->type == sk_ndtp_cmdstrs && cmdnd->cmdtp == sk_kw_include) {
							u1 partialpath[MAX_PATH];
							u1 fullpath[MAX_PATH];

							CstrFmt(partialpath, "%$\\%$", cu->cwd, (u8)cu->cwdlen, strnd->strlit->data, (u8)strnd->strlit->occ);
							u4 pathlen = GetFullPathNameA(partialpath, MAX_PATH, fullpath, 0);
							stroff = SK_TableStrInsert(cu->strs, fullpath, pathlen);
							symtp = sk_symtp_fdir;
						}
						else {
							stroff = SK_TableStrInsert(cu->strs, strnd->strlit->data, strnd->strlit->occ);
						}
						symoff = PHT_Index(cu->syms, &stroff, 8);
						
						DB_Free(strnd->strlit);
						strnd->strlit = 0;

						if (symoff + 1) {
							strnd->symoff = symoff;
						}
						else {
							strnd->symoff = PHT_Insert(cu->syms, &stroff, 8, &(SK_SYMBOL){.type = symtp, .stroff = stroff, .scopeid = wv->scope.id }, sizeof(SK_SYMBOL));
						}
						DB_Push(wv->wweb, strnd);
						wv->ndgroup.len++;
						SK_WeaverDrop(wv, 1);
					}
					else {
						SK_ErrorAtNode(cu, strnd, 0);
						DB_Free(strnd->strlit);
						strnd->strlit = 0;
						SK_WeaverDrop(wv, 1);
					}
				}
				else {
					SK_ErrorAtNode(cu, strnd, 0);
					DB_Free(strnd->strlit);
					strnd->strlit = 0;
					SK_ErrorAtNode(cu, strnd, 0);
					SK_WeaverDrop(wv, 1);
				}
			} break;
			case sk_ndtp_cmd: {
				SK_NODE* cmdnd = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				
				if (cmdnd->cmdtp == sk_kw_include) {

				}
				else if (wv->ssp >= 2) {
					SK_NODE* prvnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 2);
					if (prvnode->type == sk_ndtp_ops && (cmdnd->cmdtp == sk_kw_mw || cmdnd->cmdtp == sk_kw_mr)) {
						DB_Push(wv->wweb, cmdnd);
						wv->ndgroup.len++;
						SK_WeaverDrop(wv, 1);
					}
					else if (prvnode->type == sk_ndtp_procdef && cmdnd->cmdtp == sk_kw_extrn) {
						SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, prvnode->symoff);
						symte->type = sk_symtp_eproc;
						SK_WeaverPush(wv, cmdnd);
						SK_WeaverDrop(wv, 1);
					}
					else {
						SK_ErrorAtNode(cu, cmdnd, 0);
						SK_WeaverDrop(wv, 1);
					}
				}
				else {
					SK_ErrorAtNode(cu, cmdnd, 0);
					SK_WeaverDrop(wv, 1);
				}
			} break;
			case sk_ndtp_eof: {
				eoffound = 1;
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				SK_WeaverDrop(wv, 1);
				if (wv->ssp > 0) {// cause the endof the file goes with it
					SK_ErrorAtNode(cu, topnode, "premature eof");
					SK_NODE* syntaxstknd = (SK_NODE*)DB_Index(wv->syntaxstk, 0);
					for (u4 i = 0; i < wv->ssp; i++) {
						switch (syntaxstknd->type) {
							case sk_ndtp_loopop: {
								SK_ErrorAtNode(cu, syntaxstknd, "mising } pair for while's block");
							} break;
							case sk_ndtp_branchop: {
								SK_ErrorAtNode(cu, syntaxstknd, "mising } pair for %s's block", sk_kw2str[syntaxstknd->branchtype]);
							} break;
							case sk_ndtp_procdef: {
								SK_SYMBOL* sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, syntaxstknd->symoff);
								SK_STR* symstr = (SK_STR*)DB_Index(cu->strs->pool, sym->stroff);
								SK_ErrorAtNode(cu, syntaxstknd, "mising } pair for %$'s block", symstr->str, (u8)symstr->len);
							} break;
							case sk_ndtp_dmlay: {
								SK_ErrorAtNode(cu, syntaxstknd, "mising } for dynamic memory layout definiton's block");
							} break;
							case sk_ndtp_smlay: {
								SK_ErrorAtNode(cu, syntaxstknd, "mising ] for static memory layout definiton's block");
							} break;
						}
						syntaxstknd++;;
					}
					wv->syntaxstk->occ = 0;
					wv->ssp = 0;
				}
				if (wv->scope.symc) {
					wv->scope.symc -= SK_WeaverResolvedScopeSymbols(cu, wv, &(u1){ 0 });
					if (wv->scope.symc) {
						u8 symstksize = DB_ElementCount(wv->usymstk);
						for (u8 i = 0; i < symstksize; ++i) {
							SK_WEAVERSTKSTT* symoff = (SK_WEAVERSTKSTT*)DB_Index(wv->usymstk, i);
							DYNBUFF* ndwebbuff = *(DYNBUFF**)DB_Index(cu->ndwebs, symoff->usym.procidx);
							SK_NODE* ndprocname = (SK_NODE*)DB_Index(ndwebbuff, 0);
							SK_NODE* ndsym = (SK_NODE*)DB_Index(ndwebbuff, symoff->usym.procoff);

							SK_SYMBOL* procnamesym = (SK_SYMBOL*)DB_Index(cu->syms->pool, ndprocname->symoff);
							SK_STR* procnamestr = (SK_STR*)DB_Index(cu->strs->pool, procnamesym->stroff);

							SK_STR* ndnamestr;
							if (ndsym->symf) {
								SK_SYMBOL* ndnamesym = (SK_SYMBOL*)DB_Index(cu->syms->pool, ndsym->symoff);
								ndnamestr = (SK_STR*)DB_Index(cu->strs->pool, ndnamesym->stroff);
							}
							else {
								ndnamestr = (SK_STR*)DB_Index(cu->strs->pool, ndsym->symoff);
							}
							SK_ErrorAtNode(cu, ndsym, "the symbol { %$ } was used in the procedure { %$ } with out declaring it first", ndnamestr->str, (u8)ndnamestr->len, procnamestr->str, (u8)procnamestr->len);
						}
						Print("think about gardening instead\n");
					}
				}
				if (debugmode & sk_dbmd_weaver) {
					SK_WeaverPrintState(cu, wv);
					for (u4 i = 0; i < DB_ElementCount(cu->procs); i++) {
						DYNBUFF* buff = *(DYNBUFF**)DB_Index(cu->ndwebs, *(u4*)DB_Index(cu->procs, i));
						for (u4 j = 0; j < DB_ElementCount(buff); j++) {
							SK_NODE* nd = DB_Index(buff, j);
							SK_NodePrintInfo(cu, nd);
						}
						Print("\n\n");
					}
				}
			} break;
			default: {
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv->syntaxstk, 1);
				FatalError(0, "SK_WeaverWeaveProgram:: unhandled symbol { %s:\"%$\" }", sk_ndtp2str[topnode->type], wv->scoff, (u8)wv->nodelen);
			} break;
		}
	} while (!eoffound);

	Free(wv->sourcecode);
	DB_Free(wv->peekedstk);
	DB_Free(wv->syntaxstk);
	DB_Free(wv->wwebstk);
	DB_Free(wv->scopestk);
	DB_Free(wv->ndgroupstk);
	DB_Free(wv->usymstk);
	Free(wv);
}

#endif //SK_WEAVER_DEF