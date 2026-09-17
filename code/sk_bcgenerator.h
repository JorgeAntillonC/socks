#ifndef SK_BCGENERATOR_INCLUDE
#define SK_BCGENERATOR_INCLUDE
#include "p:\stdwea\gdefwea.h"
#include "p:\stdwea\dynbuffwea.h"
#include ".\sk_commondef.h"

inline static void SK_TagSym(SK_SYMBOL* symte, DYNBUFF* icr, DYNBUFF* unresolvedsymbols, u4 symidx);

void SK_PatchUnresolvedSym(SK_COMPUNIT* cu, DYNBUFF* icr, SK_SYMBOL* symte, u8 patch);

SK_NODE* SK_GenOpsByteCode(SK_COMPUNIT* cu, SK_NODE* node, DYNBUFF* icr, DYNBUFF* unresolvedsymbols, u8* reservedstackspace, u1* errbuff, const u2 debugmode);

void SK_BCGeneratorGenerateBytecode(SKVM_PROG* program, SK_COMPUNIT* cu, const u2 debugmode);

#endif //SK_BCGENERATOR_INCLUDE

#ifndef SK_BCGENERATOR_DEF
#define SK_BCGENERATOR_DEF
inline static void SK_TagSym(SK_SYMBOL* symte, DYNBUFF* icr, DYNBUFF* unresolvedsymbols, u4 symidx) {
	switch (symte->state) {
		case sk_symstt_unpatched: {
			DB_Push(unresolvedsymbols, &(u4){ symidx });
			symte->icr = DB_ElementCount(icr);
			DB_Push(icr, &(u8){ symte->icr });
			symte->state = sk_symstt_seen;
		} break;
		case sk_symstt_seen: {
			DB_Push(icr, &(u8){ symte->icr });
			symte->icr = DB_ElementCount(icr) - 1;
		} break;
		case sk_symstt_patched: {
			DB_Push(icr, &(u8){ symte->icr });
		} break;
		default: {
			FatalError(0, "unreachable symbol state patchunresolved");
		} break;
	}
}

void SK_PatchUnresolvedSym(SK_COMPUNIT* cu, DYNBUFF* icr, SK_SYMBOL* symte, u8 patch) {
	if (symte->state == sk_symstt_seen) {
		while (*(u8*)DB_Index(icr, symte->icr) != symte->icr) {
			u8 tmp = *(u8*)DB_Index(icr, symte->icr);
			*(u8*)DB_Index(icr, symte->icr) = patch;
			symte->icr = tmp;
		}
		*(u8*)DB_Index(icr, symte->icr) = patch;
	}
	symte->icr = patch;
	symte->state = sk_symstt_patched;
}

SK_NODE* SK_GenOpsByteCode(SK_COMPUNIT* cu, SK_NODE* node, DYNBUFF* icr, DYNBUFF* unresolvedsymbols, u8* reservedstackspace, u1* errbuff, const u2 debugmode) {
	if ((debugmode & sk_dbmd_bcgenerator) && (debugmode & sk_dbmd_nodeinf)) SK_NodePrintInfo(cu, node);
	switch (node->type) {
		case sk_ndtp_ops: {
			SK_NODE* op = node + 1;
			u8 resargaddr = 0;
			u8 spacetoreserve = 0;
			if (node->opsf) {
				DB_Push(icr, &(u8){ skvm_op_res });
				resargaddr = DB_ElementCount(icr);
				DB_Push(icr, &(u8){ 0 });
			}
			for (u8 k = 0; k < node->opsc; k++) {
				if ((debugmode & sk_dbmd_bcgenerator) && (debugmode & sk_dbmd_nodeinf)) SK_NodePrintInfo(cu, op);
				switch (op->type) {
					case sk_ndtp_num: {
						DB_Push(icr, &(u8){ skvm_op_push });
						DB_Push(icr, &(u8){ op->val });
					} break;
					case sk_ndtp_op: {
						if (op->op == skvm_op_call || op->op == skvm_op_ecall) {
							SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, op->symcall);
							switch (symte->type) {
								case sk_symtp_undef: {
									DB_Push(icr, &(u8){ skvm_op_nop });
								} break;
								case sk_symtp_proc: {
									DB_Push(icr, &(u8){ skvm_op_call });
								} break;
								case sk_symtp_eproc: {
									SK_NODE* ndsignature = (SK_NODE*)DB_Index(*(DYNBUFF**)DB_Index(cu->ndwebs, symte->objid), 1);
									DB_Push(icr, &(u8){ skvm_op_ecall });
									DB_Push(icr, &(u8){ ndsignature->val }); // both argsc and retsc
								} break;
								default: {
									if (symte->type < sk_symtp_count) {
										SK_NodePrintInfo(cu, op);
										FatalError(0, "unreachable { %s } not implemented yet, genops call %4u:%4u", sk_symtp2str[symte->type], op->row + 1, op->col + 1);
									}
									else {
										FatalError(0, "unreachable { %u } not implemented yet, genops call", symte->type);
									}
								} break;
							}
						}
						else {
							DB_Push(icr, &(u8){ op->op });
						}
					} break;
					case sk_ndtp_sym: {
						SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, op->symoff);
						switch (symte->type) {
							case sk_symtp_proc:
							case sk_symtp_eproc: {
								DB_Push(icr, &(u8){ skvm_op_push });
								SK_TagSym(symte, icr, unresolvedsymbols, op->symoff);
							} break;
							case sk_symtp_dmlay: {
								DB_Push(icr, &(u8){ skvm_op_idx });
								SK_TagSym(symte, icr, unresolvedsymbols, op->symoff);
							} break;
							case sk_symtp_smlay: {
								DB_Push(icr, &(u8){ skvm_op_lea });
								SK_TagSym(symte, icr, unresolvedsymbols, op->symoff);
							} break;
							default: {
								if (symte->type < sk_symtp_count) {
									SK_NodePrintInfo(cu, op);
									FatalError(0, "unreachable %s not implemented yet, genops %4u:%4u", sk_symtp2str[symte->type], op->row + 1, op->col + 1);
								}
								else {
									FatalError(0, "unreachable {%u} not implemented yet, genops", symte->type);
								}
							} break;
						}
				} break;
					case sk_ndtp_str: {
						SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, op->symoff);
						DB_Push(icr, &(u8){ skvm_op_lea });
						SK_TagSym(symte, icr, unresolvedsymbols, op->symoff);
						SK_STR* strte = (SK_STR*)DB_Index(cu->strs->pool, symte->stroff);
						DB_Push(icr, &(u8){ skvm_op_push });
						DB_Push(icr, &(u8){ strte->len });
					} break;
					case sk_ndtp_dmlay: case sk_ndtp_loopop: case sk_ndtp_branchop: {
						op = SK_GenOpsByteCode(cu, op, icr, unresolvedsymbols, &spacetoreserve, errbuff, debugmode) - 1;
					} break;
					case sk_ndtp_ops: {
						// TODO?? is this even reachable? i think this is a block inside of a block no? just leave it as is for now
						op = SK_GenOpsByteCode(cu, op, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode)-1;
					} break;
					case sk_ndtp_type: { } break; // droping the casts
					default: {
						SK_NodePrintInfo(cu, op);
						FatalError(0, "SK_GenBytecode:: \"%s\" not immplented (inner)\n", sk_ndtp2str[op->type]);
					} break;
				}
				op += 1;
			}
			if (node->opsf) {
				DB_Push(icr, &(u8){ skvm_op_rel });
				DB_Push(icr, &(u8){ spacetoreserve });
				*(u8*)DB_Index(icr, resargaddr) = spacetoreserve;
			}
			return op;
		} break;
		case sk_ndtp_loopop: {
			u8 loopingaddr = DB_ElementCount(icr);
			SK_NODE* block = SK_GenOpsByteCode(cu, node + 1, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);

			DB_Push(icr, &(u8){ skvm_op_jpz });
			u8 jpzargaddr = DB_ElementCount(icr);
			DB_Push(icr, &(u8){ 0 });

			SK_NODE* endloopop = SK_GenOpsByteCode(cu, block, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);

			DB_Push(icr, &(u8){ skvm_op_jmp });
			DB_Push(icr, &(u8){ loopingaddr });
			u8 blockendaddr = DB_ElementCount(icr);
			*(u8*)DB_Index(icr, jpzargaddr) = blockendaddr;
			return endloopop;
		} break;
		case sk_ndtp_dmlay: {
			SK_SYMBOL* mlaysym = (SK_SYMBOL*)DB_Index(cu->syms->pool, node->mlaysym);
			SK_PatchUnresolvedSym(cu, icr, mlaysym, *reservedstackspace);
			DYNBUFF* ndwebmlay = *(DYNBUFF**)DB_Index(cu->ndwebs, mlaysym->objid);
			SK_NODE* ndmlay = (SK_NODE*)DB_Index(ndwebmlay, 1);
			SK_NODE* ndmemdesc = ndmlay + 1;
			SK_NODE* ndmemtype = ndmlay + 2;
			
			u8 totalmemrequired = 0;

			for (u4 i = 0; i < ndmlay->mlaymemc; i++) {
				totalmemrequired += ndmemdesc->mlaymemsize * ndmemtype->primsize;
				ndmemdesc += 2;
				ndmemtype += 2;
			}
			*reservedstackspace += totalmemrequired;
			return node+1;
		} break;
		case sk_ndtp_branchop: {
			SK_NODE* nxtbranch_end = node + 1;
			u1 haselsebranch = node->branchtype == sk_kw_else;
			u8 branchcount = node->branchc - haselsebranch;
			u8 jmpargaddr = (u8)(-1);
			u8 jpzargaddr = 0;
			do {
				SK_NODE* ndbody = SK_GenOpsByteCode(cu, nxtbranch_end, icr, unresolvedsymbols, reservedstackspace, errbuff, debugmode);
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
			FatalError(0, "SK_GenBytecode:: \"%s\" not immplented (outer)\n", sk_ndtp2str[node->type]);
			return 0;
		} break;
	}
	return 0;
}

void SK_BCGeneratorGenerateBytecode(SKVM_PROG* program, SK_COMPUNIT* cu, const u2 debugmode) {
	u1 errbuff[512];
	DYNBUFF* icr = DB_Create(256, sizeof(u8));
	DYNBUFF* loadedlibs = DB_Create(256, sizeof(HMODULE));
	program->bytecode = icr;
	program->loadlibs = loadedlibs;
	program->symbols = DB_Create(256, sizeof(SKVM_SYM));


	DYNBUFF* unresolvedsymbols = DB_Create(256, sizeof(u4));
	DB_Push(unresolvedsymbols, &(u4){ cu->entrypoint });

	u1 mainprocf = 1;
	while (unresolvedsymbols->occ) {
		u4 symidx = *(u4*)DB_SoftPop(unresolvedsymbols);
		SK_SYMBOL* wsym = (SK_SYMBOL*)DB_Index(cu->syms->pool, symidx);
		SK_STR* wsymstr = (SK_STR*)DB_Index(cu->strs->pool, wsym->stroff);

		u8 procaddr = DB_ElementCount(icr);
		if (debugmode & sk_dbmd_bcgenerator) {
			SK_STR* strte = DB_Index(cu->strs->pool, wsym->stroff);
			CstrFromRawBytes(strte->str, strte->len, errbuff, 512);
			Print("\n\ncompiling: %s:%4u icr:%u\n\n", errbuff, symidx, procaddr);
		}

		switch (wsym->type) {
			case sk_symtp_eproc: {
				SK_NODE* procsign = (SK_NODE*)DB_Index(*(DYNBUFF**)DB_Index(cu->ndwebs, wsym->objid), 1);
				SK_NODE* ndstrcmds = procsign + 1 + procsign->argsc + procsign->retsc;
				SK_NODE* ndlib = ndstrcmds + 1;
				SK_NODE* ndproc = ndlib + 1;

				SK_STR* strlib  = (SK_STR*)DB_Index(cu->strs->pool, ((SK_SYMBOL*)DB_Index(cu->syms->pool, ndlib->symoff))->stroff);
				SK_STR* strproc = (SK_STR*)DB_Index(cu->strs->pool, ((SK_SYMBOL*)DB_Index(cu->syms->pool, ndproc->symoff))->stroff);

				u1 buff[256];
				CstrFmt(buff, "%$\0", strlib->str, (u8)strlib->len);
				HMODULE lib = LoadLibraryA(buff);
				CstrFmt(buff, "%$\0", strproc->str, (u8)strproc->len);
				u8 procaddr = (u8)GetProcAddress(lib, buff);
				SK_PatchUnresolvedSym(cu, icr, wsym, procaddr);
				DB_Push(loadedlibs, &(HMODULE){ lib });
			} break;
			case sk_symtp_proc: {
				SKVM_SYM vmsym = { 0 };
				vmsym.bc = (u1*)(u8)icr->occ;
				vmsym.off = wsymstr->str;
				vmsym.len = wsymstr->len;
				vmsym.type = skvm_symtp_proc;
				u8 resargaddr = 0;
			
				SK_NODE* procsign = (SK_NODE*)DB_Index(*(DYNBUFF**)DB_Index(cu->ndwebs, wsym->objid), 1);
				SK_NODE* procblock = procsign + 1 + procsign->argsc + procsign->retsc;
				SK_GenOpsByteCode(cu, procblock, icr, unresolvedsymbols, &(u8){ 0 }, errbuff, debugmode);

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
			case sk_symtp_smlay: {
				SKVM_SYM vmsym = { 0 };
				vmsym.bc = (u1*)(u8)icr->occ;
				vmsym.off = wsymstr->str;
				vmsym.len = wsymstr->len;
				vmsym.type = skvm_symtp_smlay;
				u8 maxsize = 0;
				u8 memlayaddr = DB_ElementCount(icr);
				SK_NODE* node = (SK_NODE*)DB_Index(*(DYNBUFF**)DB_Index(cu->ndwebs, wsym->objid), 1);
				SK_NODE* nd = node + 1;

				// need to register the symbol
				for (u8 i = 0; i < node->mlaymemc << 1; i += 2) {
					SK_NODE* memdesc = nd;
					SK_NODE* memtype = nd + 1;

					if (memdesc->mlaymemf & 0b010) { //TODO:: if the memember has a symbol so stuff
						SK_SYMBOL* memsymte = (SK_SYMBOL*)DB_Index(cu->syms->pool, memdesc->mlaymemsym);
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
			case sk_symtp_str: {
				SKVM_SYM vmsym = { 0 };
				vmsym.bc = (u1*)(u8)icr->occ;
				vmsym.off = wsymstr->str;
				vmsym.len = wsymstr->len;
				vmsym.type = skvm_symtp_str;

				u8 straddr = DB_ElementCount(icr);
				DB_Append(icr, wsymstr->str, wsymstr->len);
				u8 remaining2nxtmultof8 = ((icr->occ + (u8)7) & (~(u8)7)) - icr->occ;
				u1 buff[256];
				memset(buff, 0, remaining2nxtmultof8);
				DB_Append(icr, buff, remaining2nxtmultof8);

				vmsym.bcend = (u1*)(u8)icr->occ;
				DB_Push(program->symbols, &vmsym);
				SK_PatchUnresolvedSym(cu, icr, wsym, straddr);
			} break;
			case sk_symtp_dmlay: {} break;
			case sk_symtp_undef: {} break;
			default: {
				FatalError(0, "unhandled symbol type %s at code gen", sk_symtp2str[wsym->type]);
			}
		}
		if (debugmode & sk_dbmd_bcgenerator) {
			CstrFromRawBytes(wsymstr->str, wsymstr->len, errbuff, 512);
			Print("\n\ndone with: %s:%4u new icr:%u:%u:%u\n\n", errbuff, symidx, (u8)DB_ElementCount(icr), (u8)DB_ElementCount(icr) - procaddr, ((u8)DB_ElementCount(icr) - procaddr) << 3);
		}
	}
	DB_Free(unresolvedsymbols);
}
#endif //SK_BCGENERATOR_DEF