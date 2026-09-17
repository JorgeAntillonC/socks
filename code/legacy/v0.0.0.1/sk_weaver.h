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

	u4 nodelen;
	SK_NODE cnode;
};

void SK_WeaverNxtNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver);

inline void SK_WeaverOpenNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u4 ndgrouptype, const u1 newscopef);

inline u4 SK_WeaverResolvedScopeSymbols(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u1* dynamicmemallocs);

inline u1 SK_WeaverCloseNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u1 newscopef);

inline u4 SK_WeaverRegisterSymbol(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u4 stroff, const u2 symboltype);

void SK_WeaverPrintState(SK_COMPUNIT* cu, SK_NDWEAVER* wv);

void SK_WeaverWeaveProgram(SK_COMPUNIT* cu, const u2 debugmode);

#endif //SK_WEAVER_INCLUDE

#ifndef SK_WEAVER_DEF
#define SK_WEAVER_DEF

void SK_WeaverNxtNode(SK_COMPUNIT* cu, SK_NDWEAVER* weaver) {
	SK_NODE* cnode = &weaver->cnode;
	cnode->col += weaver->nodelen;
	weaver->scoff += weaver->nodelen;
	do {
		while (*weaver->scoff && CharIsSpace(*weaver->scoff)) {
			weaver->scoff++;
			cnode->col++;
			if (CharIsNL(*weaver->scoff)) {
				cnode->col = 0;
				while (CharIsNL(*weaver->scoff)) { if (*weaver->scoff == '\n') { cnode->row++; } weaver->scoff++; }
			}
		}

		if (*weaver->scoff == ';') {
			weaver->scoff++;
			cnode->col++;
			if (*weaver->scoff == ';') {
				weaver->scoff++;
				cnode->col++;
				while (*weaver->scoff && !(*weaver->scoff == ';' && *(weaver->scoff + 1) == ';')) {
					weaver->scoff++;
					cnode->col++;
					if (CharIsNL(*weaver->scoff)) {
						cnode->col = 0;
						while (CharIsNL(*weaver->scoff)) { if (*weaver->scoff == '\n') { cnode->row++; } weaver->scoff++; }
					}
				}
				if (*weaver->scoff) {
					weaver->scoff += 2;
					cnode->col += 2;
				}
			}
			else {
				while (*weaver->scoff && !CharIsNL(*weaver->scoff)) { weaver->scoff++; }
				cnode->col = 0;
				while (*weaver->scoff && CharIsNL(*weaver->scoff)) { if (*weaver->scoff == '\n') { cnode->row++; } weaver->scoff++; }
			}
		}
	} while (*weaver->scoff && (CharIsSpace(*weaver->scoff) || *weaver->scoff == ';'));

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
			// TODO i should malloc this instead
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
				weaver->nodelen = (u8)(nodeend - weaver->scoff);
				cnode->type = sk_ndtp_str;
				cnode->symf = 0;
				cnode->symoff = SK_TableStrInsert(cu->strs, buff, (u8)(bh - buff));
			}
			else {
				FatalError(0, "SK_WeaverNxtNode:: premature eof while defining string literal at %s:%4u:%4u\n", *weaver->scoff, cu->sourcefile, cnode->row + 1, cnode->col + 1);
			}
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
		case ':': { weaver->nodelen = 1; cnode->type = sk_ndtp_delim; cnode->val = sk_kw_colon; } return;
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
		default: { FatalError(0, "TK_Nxt:: unhandled character { %c } at %s:%4u:%4u\n", *weaver->scoff, cu->sourcefile, cnode->row + 1, cnode->col + 1); }
	}
	return;
}

inline void SK_WeaverOpenNodeGroup(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, u4 ndgrouptype, const u1 newscopef) {
	if (newscopef) {
		DB_Push(weaver->scopestk, &(SK_WEAVERSTKSTT){ .scope.id = weaver->scope.id, .scope.symc = weaver->scope.symc });
		weaver->scope.symc = 0;
		weaver->scope.id = ++weaver->scope.counter;
	}
	weaver->cnode.type = ndgrouptype;
	weaver->cnode.ext = 0;
	weaver->cnode.val = 0;
	if (ndgrouptype == sk_ndtp_branchop) {
		weaver->cnode.branchtype = sk_kw_if;
	}
	DB_Push(weaver->syntaxstk, &weaver->cnode);
	DB_Push(weaver->ndgroupstk, &(SK_WEAVERSTKSTT){ .ndgroup.off = weaver->ndgroup.off, .ndgroup.len = weaver->ndgroup.len });
	weaver->ndgroup.off = DB_ElementCount(weaver->wweb);
	weaver->ndgroup.len = 0;
	DB_Push(weaver->wweb, &weaver->cnode);
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
	weaver->syntaxstk->occ -= 1 * sizeof(SK_NODE);
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

inline u4 SK_WeaverRegisterSymbol(SK_COMPUNIT* cu, SK_NDWEAVER* weaver, const u4 stroff, const u2 symboltype) {
	if (stroff == (u4)(-1)) {
		FatalError(0, "SK_WeaverRegisterSymbol: we got an invalid stroff entry { %4u }\n", stroff);
	}

	u8 symkey = ((u8)weaver->scope.id << 32) | stroff;
	u4 symoff = PHT_Index(cu->syms, &symkey, 8);
	SK_SYMBOL* sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symoff);
	if (!sym) {
		SK_STR* strentry = (SK_STR*)DB_Index(cu->strs->pool, stroff);
		FatalError(0, "SK_WeaverRegisterSymbol: we got a null! since we haven't seen { %$ } yet\n", strentry->str, (u8)strentry->len);
	}
	DB_Push(weaver->wwebstk, &(SK_WEAVERSTKSTT){.proc.procidx = weaver->ndgroup.procidx });
	weaver->ndgroup.procidx = DB_ElementCount(cu->ndwebs);
	weaver->wweb = DB_Create(256, sizeof(SK_NODE));
	DB_Push(cu->ndwebs, &(DYNBUFF*){ weaver->wweb });
	sym->objid = weaver->ndgroup.procidx;
	sym->type = symboltype;
	DB_Push(weaver->wweb, &(SK_NODE){.type = sk_ndtp_sym, .symf = 1, .symoff = symoff });
	if (symboltype == sk_symtp_proc) {
		DB_Push(cu->procs, &(u4){ weaver->ndgroup.procidx });
	}
	return symoff;
}

void SK_WeaverPrintState(SK_COMPUNIT* cu, SK_NDWEAVER* wv) {
	u1 buff[512];
	Print("\n\x1b[31m============syntaxstack=========\n");
	u8 syntaxstksize = DB_ElementCount(wv->syntaxstk);
	for (u8 i = 0; i < syntaxstksize; ++i) {
		SK_NODE* nd = (SK_NODE*)DB_Index(wv->syntaxstk, i);
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

void SK_WeaverWeaveProgram(SK_COMPUNIT* cu, const u2 debugmode) {
	u1 buff[256];
	SK_NDWEAVER wv = { 0 };
	wv.sourcecode = LoadFile(cu->sourcefile);
	if (!wv.sourcecode) {
		Print("couln't open the file { %s }\n", cu->sourcefile);
		cu->errorcount++;
		return;
	}
	wv.wweb = (DYNBUFF*)(-1);
	wv.ndgroup.procidx = -1;
	wv.scoff = wv.sourcecode;
	wv.syntaxstk  = DB_Create(256, sizeof(SK_NODE));
	wv.wwebstk    = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // prev stack and off
	wv.scopestk   = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // scope
	wv.ndgroupstk = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // ndgroup wnodecount
	wv.usymstk    = DB_Create(256, sizeof(SK_WEAVERSTKSTT)); // proc and offset

	if (debugmode & sk_dbmd_weaver) {
		Print("\n\n===========source==========\n\n");
		u4 sourcedoesize = CstrLen(wv.sourcecode);
		FS_Write(STDOUT, wv.sourcecode, sourcedoesize);
		Print("\n\n===========================\n\n");
	}

	if (debugmode & sk_dbmd_weaver) Print("\n\n===========parsing===========\n\n");
	do {
		SK_WeaverNxtNode(cu, &wv);

	skp:if (debugmode & sk_dbmd_weaver) {
			SK_WeaverPrintState(cu, &wv);
			Print("next node:\n");
			SK_NodePrintInfo(cu, &wv.cnode);
		}

		switch (wv.cnode.type) {
			case sk_ndtp_sym: {
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);

				u4 stroff = wv.cnode.symoff;
				SK_STR* skstr = (SK_STR*)DB_Index(cu->strs->pool, stroff);
				DB_Push(wv.syntaxstk, &wv.cnode);

				SK_WeaverNxtNode(cu, &wv);
				if (syntaxstksize) {
					switch (topnode->type) {
					case sk_ndtp_ops: {
						if (wv.cnode.type == sk_ndtp_delim && wv.cnode.val == sk_kw_colon) {
							u8 symbolkey = ((u8)wv.scope.id << 32) | stroff;
							u4 symoff = PHT_Index(cu->syms, &(u8){ symbolkey }, 8);
							if (symoff + 1) {
								SK_ErrorAtNode(cu, &wv.cnode, "redefinition of %$", skstr->str, skstr->len);
							}
							else {
								PHT_Insert(cu->syms, &(u8){ symbolkey }, 8, & (SK_SYMBOL){.type = sk_symtp_undef, .scopeid = wv.scope.id, .stroff = stroff }, sizeof(SK_SYMBOL));
								SK_NODE* symnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
								symnode->type = sk_ndtp_symdef;
							}
						}
						else {
							DB_Push(wv.usymstk, &(SK_WEAVERSTKSTT){ .usym.procidx = wv.ndgroup.procidx, .usym.procoff = DB_ElementCount(wv.wweb) });
							wv.scope.symc++;
							DB_Push(wv.wweb, (SK_NODE*)DB_SoftPop(wv.syntaxstk));
							wv.ndgroup.len++;
							goto skp;
						}
					} break;
									// TODO:: implement this for named mem layout members
					default: {
						// TODO:: this is point to the wrong wv.cnode ergo the error is not at the right place
						SK_ErrorAtNode(cu, &wv.cnode, 0);
						wv.syntaxstk->occ -= sizeof(SK_NODE);
					} goto skp;
					}
				}
				else if (wv.cnode.type == sk_ndtp_delim && wv.cnode.val == sk_kw_colon) {
					// TOCONSIDER:: maybe turn this into a func, since this pattern appears at least twice in the code
					u8 symbolkey = ((u8)wv.scope.id << 32) | stroff;
					u4 symoff = PHT_Index(cu->syms, &(u8){ symbolkey }, 8);

					if (symoff + 1) {
						SK_SYMBOL* sym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symoff);
						if (sym->type == sk_symtp_undef) {
							SK_NODE* symnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
							symnode->type = sk_ndtp_symdef;
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, "redefinition of %$", skstr->str, (u8)skstr->len);
						}
					}
					else {
						PHT_Insert(cu->syms, &(u8){ symbolkey }, 8, & (SK_SYMBOL){.type = sk_symtp_undef, .scopeid = wv.scope.id, .stroff = stroff }, sizeof(SK_SYMBOL));
						SK_NODE* symnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						symnode->type = sk_ndtp_symdef;
					}
				}
				else {
					// TODO:: this is point to the wrong wv.cnode ergo the error is not at the right place
					SK_ErrorAtNode(cu, &wv.cnode, 0);
					wv.syntaxstk->occ -= sizeof(SK_NODE);
					goto skp;
				}
			} break;
			case sk_ndtp_delim: {
				switch (wv.cnode.val) {
					case sk_kw_colon: {
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						if (topnode && topnode->type == sk_ndtp_types) {
							SK_NODE* ndtypes = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							// TODO:: Error report duplicates ":" 
							ndtypes->argsretsf = 1;
							ndtypes->argsc = wv.ndgroup.len;
							wv.ndgroup.len = 0;
						}
						else if (topnode && topnode->type == sk_ndtp_ops) {
							SK_NODE tmp;
							memcpy(&tmp, &wv.cnode, sizeof(SK_NODE));
							SK_WeaverNxtNode(cu, &wv);
							if (wv.cnode.type == sk_ndtp_type) {
								wv.ndgroup.len++;
								wv.cnode.type = sk_ndtp_cast;
								DB_Push(wv.wweb, &wv.cnode);
							}
							else {
								SK_ErrorAtNode(cu, &tmp, 0);
								goto skp;
							}
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					case sk_kw_lpar: {
						// command( -> command, symdef(->procdef
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						if (topnode) {
							switch (topnode->type) {
								case sk_ndtp_symdef: {
									topnode->symoff = SK_WeaverRegisterSymbol(cu, &wv, topnode->symoff, sk_symtp_proc);
									SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_types, 0);
									topnode->type = sk_ndtp_procdef;
									topnode->symf = 1;
									if (topnode->symoff == cu->entrypoint && !wv.scope.id) cu->epidx = wv.ndgroup.procidx;
								} break;
								case sk_ndtp_cmd: {
									u8 tmp = topnode->cmdtp;
									wv.syntaxstk->occ -= sizeof(SK_NODE);
									SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_cmdstrs, 0);
									SK_NODE* ndcmdstrs = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
									ndcmdstrs->cmdtp = tmp;
								} break;
								default: {
									SK_ErrorAtNode(cu, &wv.cnode, 0);
								} break;
							}
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					case sk_kw_rpar: {
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
						if (syntaxstksize >= 1 && topnode->type == sk_ndtp_types) {
							SK_NODE* ndtypes = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							ndtypes->argsrets[ndtypes->argsretsf] = wv.ndgroup.len;
							if (debugmode & sk_dbmd_weaver) {
								Print("signature:\n");

								Print("args %4u:\n", ndtypes->argsc);
								for (u8 i = 0; i < ndtypes->argsc; ++i) {
									SK_Node2Str(cu, ndtypes + 1 + i, buff);
									Print("%s", buff);
								}
								Print("\nrets %4u:\n", ndtypes->retsc);
								for (u8 i = ndtypes->argsc; i < ndtypes->argsc + ndtypes->retsc; ++i) {
									SK_Node2Str(cu, ndtypes + 1 + i, buff);
									Print("%s", buff);
								}
							}
							SK_WeaverCloseNodeGroup(cu, &wv, 0);
						}
						else if (syntaxstksize >= 1 && topnode->type == sk_ndtp_cmdstrs) {
							wv.syntaxstk->occ -= 1 * sizeof(SK_NODE);
							SK_NODE* ndcmdstrs = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							ndcmdstrs->cmdstrc = wv.ndgroup.len;
							if (debugmode & sk_dbmd_weaver) {
								Print("cmds %4u:\n", ndcmdstrs->cmdstrc);
								for (u8 i = 0; i < ndcmdstrs->cmdstrc; ++i) {
									SK_Node2Str(cu, ndcmdstrs + 1 + i, buff);
									Print("%s", buff);
								}
							}
							SK_WeaverCloseNodeGroup(cu, &wv, 0);

							SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv.wwebstk);
							wv.ndgroup.procidx = stkstt->proc.procidx;
							if (wv.ndgroup.procidx == (u4)(-1)) {
								wv.wweb = (DYNBUFF*)(-1);
							}
							else {
								wv.wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv.ndgroup.procidx);
							}
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					case sk_kw_lsqr: {
						// symdef [ -> smlay
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						if (topnode && topnode->type == sk_ndtp_symdef) {
							wv.syntaxstk->occ -= sizeof(SK_NODE);
							u4 stroff = topnode->symoff;
							
							u4 symoff = SK_WeaverRegisterSymbol(cu, &wv, stroff, sk_symtp_smlay);
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_mlay, 0);
							((SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off))->mlaysym = symoff;
							topnode->mlaysym = symoff;
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					case sk_kw_rsqr: {
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						if (topnode && topnode->type == sk_ndtp_mlay) {
							SK_NODE* mlaymemdesc = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							mlaymemdesc->mlaymemc = wv.ndgroup.len;

							if (debugmode & sk_dbmd_weaver) {
								Print("mlaymem %4u:\n", mlaymemdesc->mlaymemc);
								for (u8 i = 0; i < mlaymemdesc->mlaymemc << 1; i += 2) {
									SK_Node2Str(cu, mlaymemdesc + i + 1, buff);
									Print("%s", buff);
									SK_Node2Str(cu, mlaymemdesc + i + 2, buff);
									Print("%s", buff);
								}
							}

							SK_WeaverCloseNodeGroup(cu, &wv, 0);

							SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv.wwebstk);
							wv.ndgroup.procidx = stkstt->proc.procidx;
							if (wv.ndgroup.procidx == (u4)(-1)) {
								wv.wweb = (DYNBUFF*)(-1);
							}
							else {
								wv.wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv.ndgroup.procidx);
							}
						}
					} break;
					case sk_kw_lbra: {
						// procdef {, symdef {, flow ops {, flow {
						// TOCONSIDER add support for blocks only for the sake of temporal scopes
						u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						if (syntaxstksize >= 1 && topnode->type == sk_ndtp_procdef) {
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 1);
						}
						else if (syntaxstksize >= 2 && topnode->type == sk_ndtp_ops && (topnode - 1)->type == sk_ndtp_loopop) {
							SK_NODE* ndops = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							ndops->opsc = wv.ndgroup.len;
							SK_WeaverCloseNodeGroup(cu, &wv, 0);
							SK_NODE* ndloopop = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							if (ndloopop->loopf) { // TODO:: for now this an error
								FatalError(0, "blocks are not implemented inside of a loopop yet");
							}
							ndloopop->loopf = 1;
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 1);
						}
						else if (syntaxstksize >= 2 && topnode->type == sk_ndtp_ops && (topnode - 1)->type == sk_ndtp_branchop) {
							SK_NODE* ndops = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							ndops->opsc = wv.ndgroup.len;
							SK_WeaverCloseNodeGroup(cu, &wv, 0);
							SK_NODE* ndbranchop = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							if (ndbranchop->branchf) { // TODO:: for now this an error
								FatalError(0, "blocks are not implemented inside of a branchop yet");
							}
							ndbranchop->branchf = 1;
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 1);
						}
						else if (syntaxstksize >= 1 && topnode->type == sk_ndtp_branchop) {
							SK_NODE* ndbranchop = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
							ndbranchop->branchf = 1;
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 1);
						}
						else if (syntaxstksize && topnode->type == sk_ndtp_symdef && (topnode - 1)->type == sk_ndtp_ops) {
							wv.syntaxstk->occ -= sizeof(SK_NODE);
							wv.ndgroup.len++;
							u4 stroff = topnode->symoff;
							u4 symoff = SK_WeaverRegisterSymbol(cu, &wv, stroff, sk_symtp_dmlay);
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_mlay, 0);
							((SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off))->mlaysym = symoff;
							topnode->mlaysym = symoff;
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					case sk_kw_rbra: {
						// procdef ops, flow ops, smlaydef mlay
						u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
						SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
						// TODO:: unnamed proc? let's not support this for now
						// it could be good if i need to create arrays of procs or pass it as an argument, etc

						switch (topnode->type) {
							case sk_ndtp_ops: {
								SK_NODE* ndops = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
								ndops->opsc = wv.ndgroup.len;
								if (debugmode & sk_dbmd_weaver) {
									Print("ops %4u:\n", ndops->opsc);
									for (u8 i = 0; i < ndops->opsc; ++i) {
										SK_Node2Str(cu, ndops + 1 + i, buff);
										Print("%s", buff);
									}
								}

								if (syntaxstksize >= 2 && (topnode - 1)->type == sk_ndtp_procdef) {
									SK_WeaverCloseNodeGroup(cu, &wv, 1);
									wv.syntaxstk->occ -= sizeof(SK_NODE);
									SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv.wwebstk);
									wv.ndgroup.procidx = stkstt->proc.procidx;
									if (wv.ndgroup.procidx == (u4)(-1)) {
										wv.wweb = (DYNBUFF*)(-1);
									}
									else {
										wv.wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv.ndgroup.procidx);
									}
								}
								else if (syntaxstksize >= 2 && (topnode - 1)->type == sk_ndtp_loopop) {
									u1 dmemallocs = SK_WeaverCloseNodeGroup(cu, &wv, 1);
									SK_NODE* ndloop = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
									ndloop->loopdmemalloc = dmemallocs;
									if (!ndloop->loopf) {
										SK_ErrorAtNode(cu, ndloop, "missing \"{\" after while condition");
									}
									SK_WeaverCloseNodeGroup(cu, &wv, 0);
								}
								else if (syntaxstksize >= 2 && (topnode - 1)->type == sk_ndtp_branchop) {
									SK_WeaverCloseNodeGroup(cu, &wv, 1);
									SK_NODE* ndbranchop = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
									if (!ndbranchop->branchf) {
										SK_ErrorAtNode(cu, ndbranchop, "missing \"{\" after %s condition", sk_kw2str[ndbranchop->branchtype]);
									}
									// ops branchop
									switch (ndbranchop->branchtype) {
									case sk_kw_if:
									case sk_kw_elif: {
										SK_WeaverNxtNode(cu, &wv);
										if (wv.cnode.type == sk_ndtp_flow && (wv.cnode.val == sk_kw_else || wv.cnode.val == sk_kw_elif)) {
											ndbranchop->branchc++;
											ndbranchop->branchf = wv.cnode.val == sk_kw_else;
											ndbranchop->branchtype = wv.cnode.val;
											if (wv.cnode.val == sk_kw_elif) {
												SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 0);
											}
											SK_WeaverNxtNode(cu, &wv);
										}
										else {
											ndbranchop->branchf = 2;
											SK_WeaverCloseNodeGroup(cu, &wv, 0);
										}
										goto skp;
									} break;
									case sk_kw_else: {
										// merge ops branchop ops
										ndbranchop->branchf = 2;
										SK_WeaverCloseNodeGroup(cu, &wv, 0);
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
							case sk_ndtp_mlay: {
								if (syntaxstksize && topnode->type == sk_ndtp_mlay) {
									SK_NODE* mlaymemdesc = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
									mlaymemdesc->mlaymemc = wv.ndgroup.len;
									if (debugmode & sk_dbmd_weaver) {
										Print("mlaymem %4u:\n", mlaymemdesc->mlaymemc);
										for (u8 i = 0; i < mlaymemdesc->mlaymemc << 1; i += 2) {
											SK_Node2Str(cu, mlaymemdesc + i + 1, buff);
											Print("%s", buff);
											SK_Node2Str(cu, mlaymemdesc + i + 2, buff);
											Print("%s", buff);
										}
									}
									SK_WeaverCloseNodeGroup(cu, &wv, 0);
									SK_WEAVERSTKSTT* stkstt = (SK_WEAVERSTKSTT*)DB_SoftPop(wv.wwebstk);
									wv.ndgroup.procidx = stkstt->proc.procidx;
									if (wv.ndgroup.procidx == (u4)(-1)) {
										wv.wweb = (DYNBUFF*)(-1);
									}
									else {
										wv.wweb = *(DYNBUFF**)DB_Index(cu->ndwebs, wv.ndgroup.procidx);
									}
									DB_Push(wv.wweb, mlaymemdesc);
									SK_NODE* ndops = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
									ndops->opsf |= 1;
								}
							} break;
							default: {
								SK_ErrorAtNode(cu, &wv.cnode, 0);
							} break;
						}
					} break;
					default: {
						FatalError(0, "SK_Weaver_WebProgram::unreachable { %s:\"%$\" }", sk_ndtp2str[wv.cnode.type], wv.scoff, (u8)wv.nodelen);
					} break;
				}
			}break;
			case sk_ndtp_type: {
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
				if (topnode && topnode->type == sk_ndtp_types) {
					DB_Push(wv.wweb, &wv.cnode);
					wv.ndgroup.len++;
				}
				else if (topnode && topnode->type == sk_ndtp_mlaymem) {
					((SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off))->mlaymemf |= 0b001;
					DB_Push(wv.wweb, &wv.cnode);
					SK_WeaverCloseNodeGroup(cu, &wv, 0);
				}
				else if (topnode && topnode->type == sk_ndtp_mlay) {
					DB_Push(wv.wweb, &(SK_NODE){.type = sk_ndtp_mlaymem, .mlaymemsize = 1, .mlaymemsym = 0, .mlaymemf = 0b001 });
					DB_Push(wv.wweb, &wv.cnode);
					wv.ndgroup.len++;
				}
				else {
					SK_ErrorAtNode(cu, &wv.cnode, 0);
				}
			} break;
			case sk_ndtp_num: {
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				if (syntaxstksize && topnode->type == sk_ndtp_ops) {
					DB_Push(wv.wweb, &wv.cnode);
					wv.ndgroup.len++;
				}
				else if (syntaxstksize && topnode->type == sk_ndtp_mlay) {
					// TODO:: do some error handling here you can't have more than 1 number at any given time
					wv.ndgroup.len++;
					u8 val = wv.cnode.val;
					SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_mlaymem, 0);
					SK_NODE* nmlaymem = (SK_NODE*)DB_Index(wv.wweb, wv.ndgroup.off);
					nmlaymem->mlaymemsize = val;
					nmlaymem->mlaymemf = 0b100;
				}
				else {
					SK_ErrorAtNode(cu, &wv.cnode, 0);
				}
			} break;
			case sk_ndtp_op: {
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				if (syntaxstksize && topnode->type == sk_ndtp_ops) {
					DB_Push(wv.wweb, &wv.cnode);
					wv.ndgroup.len++;
				}
				else {
					SK_ErrorAtNode(cu, &wv.cnode, 0);
				}
			} break;
			case sk_ndtp_str: {
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				if (syntaxstksize && (topnode->type == sk_ndtp_ops || topnode->type == sk_ndtp_cmdstrs)) {
					wv.cnode.symf = 1;
					u8 symkey = wv.cnode.symoff;
					u4 symoff = PHT_Index(cu->syms, &symkey, 8);
					if (symoff + 1) {
						wv.cnode.symoff = symoff;
					}
					else {
						wv.cnode.symoff = PHT_Insert(cu->syms, &symkey, 8, &(SK_SYMBOL){.type = sk_symtp_str, .stroff = wv.cnode.symoff, .scopeid = wv.scope.id }, sizeof(SK_SYMBOL));
					}
					DB_Push(wv.wweb, &wv.cnode);
					wv.ndgroup.len++;
				}
				else {
					SK_SYMBOL* strte = (SK_SYMBOL*)DB_Index(cu->syms->entries, wv.cnode.symoff);
					SK_ErrorAtNode(cu, &wv.cnode, 0);
				}
			} break;
			case sk_ndtp_flow: {
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				switch (wv.cnode.val) {
					case sk_kw_while: {
						if (syntaxstksize && topnode->type == sk_ndtp_ops) {
							wv.ndgroup.len++;
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_loopop, 0);
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 0);
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					case sk_kw_if: {
						if (syntaxstksize && topnode->type == sk_ndtp_ops) {
							wv.ndgroup.len++;
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_branchop, 0);
							SK_WeaverOpenNodeGroup(cu, &wv, sk_ndtp_ops, 0);
						}
						else {
							SK_ErrorAtNode(cu, &wv.cnode, 0);
						}
					} break;
					default: {
						SK_ErrorAtNode(cu, &wv.cnode, 0);
					} break;
				}
			} break;
			case sk_ndtp_cmd: {
				SK_NODE* topnode = (SK_NODE*)DB_Peek(wv.syntaxstk, 1);
				u8 syntaxstksize = DB_ElementCount(wv.syntaxstk);
				if (syntaxstksize && topnode->type == sk_ndtp_ops && (wv.cnode.cmdtp == sk_kw_mw || wv.cnode.cmdtp == sk_kw_mr)) {
					DB_Push(wv.wweb, &wv.cnode);
					wv.ndgroup.len++;
				}
				else if (syntaxstksize && topnode->type == sk_ndtp_procdef && wv.cnode.cmdtp == sk_kw_extrn) {
					SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, topnode->symoff);
					symte->type = sk_symtp_eproc;
					DB_Push(wv.syntaxstk, &wv.cnode);
				}
				else {
					SK_ErrorAtNode(cu, &wv.cnode, 0);
				}
			} break;
			case sk_ndtp_eof: {
				if (DB_ElementCount(wv.syntaxstk) > 0) {
					SK_ErrorAtNode(cu, &wv.cnode, "premature eof; %u nodes left on the syntax stack", DB_ElementCount(wv.syntaxstk));
				}
				if (DB_ElementCount(wv.scopestk) > 0) {
					SK_ErrorAtNode(cu, &wv.cnode, "premature eof; %u nodes left on the scope stack", DB_ElementCount(wv.scopestk));
				}
				if (wv.scope.symc) {
					wv.scope.symc -= SK_WeaverResolvedScopeSymbols(cu, &wv, &(u1){ 0 });
					if (wv.scope.symc) {
						u8 symstksize = DB_ElementCount(wv.usymstk);
						for (u8 i = 0; i < symstksize; ++i) {
							SK_WEAVERSTKSTT* symoff = (SK_WEAVERSTKSTT*)DB_Index(wv.usymstk, i);
							DYNBUFF* ndwebbuff = *(DYNBUFF**)DB_Index(cu->ndwebs, symoff->usym.procidx);
							SK_NODE* ndprocname = (SK_NODE*)DB_Index(ndwebbuff, 0);
							SK_NODE* ndsym      = (SK_NODE*)DB_Index(ndwebbuff, symoff->usym.procoff);
							
							SK_SYMBOL* procnamesym = (SK_SYMBOL*)DB_Index(cu->syms->pool, ndprocname->symoff);
							SK_STR* procnamestr    = (SK_STR*)DB_Index(cu->strs->pool, procnamesym->stroff);
							
							SK_Node2Str(cu, ndsym, buff);
							SK_ErrorAtNode(cu, ndsym, "you used the symbol { %s } in the procedure %$ with out declaring it first", buff, procnamestr->str, (u8)procnamestr->len);
						}
						Print("think about gardening instead\n");
					}
				}
				if (debugmode & sk_dbmd_weaver) {
					SK_WeaverPrintState(cu, &wv);
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
				FatalError(0, "SK_BuildBlocks:: unhandled symbol { %s:\"%$\" }", sk_ndtp2str[wv.cnode.type], wv.scoff, (u8)wv.nodelen);
			} break;
		}
	} while (wv.cnode.type != sk_ndtp_eof);

	if (cu->epidx == (u8)(-1)) {
		SK_STR* entrypointstr = (SK_STR*)DB_Index(cu->strs->pool, cu->entrypoint);
		SK_ErrorAtNode(cu, &wv.cnode, "entry point { %$ } was not found", entrypointstr->str, (u8)entrypointstr->len);
	}

	Free(wv.sourcecode);
	DB_Free(wv.syntaxstk);
	DB_Free(wv.wwebstk);
	DB_Free(wv.scopestk);
	DB_Free(wv.ndgroupstk);
	DB_Free(wv.usymstk);
}

#endif //SK_WEAVER_DEF