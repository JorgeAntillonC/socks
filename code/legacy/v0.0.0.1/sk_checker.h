#ifndef SK_CHECKER_INCLUDE
#define SK_CHECKER_INCLUDE
#include "p:\stdwea\gdefwea.h"
#include "p:\stdwea\dynbuffwea.h"
#include ".\sk_commondef.h"

inline static u1 SK_IsTypeSigned(u8 t);
inline static u1 SK_IsTypeUnsigned(u8 t);

u1 SK_CompatibleTypes(SK_NODE* scrtype, SK_NODE* tgtype, u1 tglowerlvl);

inline static void SK_CheckTypeArithmetic(SK_NODE* a, SK_NODE* b);

inline static void SK_CheckBody(SK_COMPUNIT* cu, SK_NODE* ndobj, DYNBUFF* vstack, DYNBUFF* stacksnapshot, u1* bodytype, const u2 debugmode);

inline static void SK_CheckCondition(SK_COMPUNIT* cu, SK_NODE* ndobj, DYNBUFF* vstack, DYNBUFF* stacksnapshot, u1* conditiontype, const u2 debugmode);

inline static u4 SK_PrimSize2Off(u4 primsize);

SK_NODE* SK_CheckOp(SK_COMPUNIT* cu, SK_NODE* cnode, DYNBUFF* vstack, u1* errbuff, const u2 debugmode);

void SK_CheckerCheck(SK_COMPUNIT* cu, const u2 debugmode);
#endif //SK_CHECKER_INCLUDE

#ifndef SK_CHECKER_DEF
#define SK_CHECKER_DEF

inline static u1 SK_IsTypeSigned(u8 t) { return sk_tp_s1 <= t && t <= sk_tp_s8; }
inline static u1 SK_IsTypeUnsigned(u8 t) { return sk_tp_u1 <= t && t <= sk_tp_u8; }

// TODO:: return different values to know where it failed
u1 SK_CompatibleTypes(SK_NODE* scrtype, SK_NODE* tgtype, u1 tglowerlvl) {
	if (scrtype->prim == tgtype->prim && scrtype->primlvl == tgtype->primlvl) return 1;
	if (tgtype->prim == sk_tp_ptr) return scrtype->primlvl > 0;
	else {
		if (scrtype->primlvl != tgtype->primlvl - tglowerlvl) return 0;
		if (scrtype->primsize > tgtype->primsize) return 0;
		if (SK_IsTypeSigned(scrtype->prim) != SK_IsTypeSigned(tgtype->prim)) return 0;
		return 1;
	}
}

inline static void SK_CheckTypeArithmetic(SK_NODE* a, SK_NODE* b) {
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

inline static void SK_CheckBody(SK_COMPUNIT* cu, SK_NODE* ndobj, DYNBUFF* vstack, DYNBUFF* stacksnapshot, u1* bodytype, const u2 debugmode) {
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
		SK_NODE* vstackbase = (SK_NODE*)DB_Index(vstack, 0);
		SK_NODE* snapshotbase = (SK_NODE*)DB_Index(stacksnapshot, 0);
		u1 mismatchestack = 0;
		for (u4 i = 0; i < ogvstacksize; i++) {
			if (!SK_CompatibleTypes(snapshotbase, vstackbase, 0)) {
				//TODO:: better error report
				SK_NodePrintInfo(cu, snapshotbase);
				SK_NodePrintInfo(cu, vstackbase);
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

inline static void SK_CheckCondition(SK_COMPUNIT* cu, SK_NODE* ndobj, DYNBUFF* vstack, DYNBUFF* stacksnapshot, u1* conditiontype, const u2 debugmode) {
	u4 vstacksize = DB_ElementCount(vstack);
	u4 ogvstacksize = DB_ElementCount(stacksnapshot);
	if (vstacksize < ogvstacksize + 1) {
		SK_ErrorAtNode(cu, ndobj, "condition underflow: expected exactly one extra value on top, but got %u fewer", (ogvstacksize + 1) - vstacksize);
		vstack->occ = 0;
		DB_Extend(vstack, stacksnapshot);
		vstack->occ += sizeof(SK_NODE);
	}
	else if (vstacksize > ogvstacksize + 1) {
		SK_ErrorAtNode(cu, ndobj, "condition overflow: expected exactly one extra value on top, but got %u more", vstacksize - (ogvstacksize + 1));
		vstack->occ = 0;
		DB_Extend(vstack, stacksnapshot);
		vstack->occ += sizeof(SK_NODE);
	}
	vstack->occ -= sizeof(SK_NODE);

	SK_NODE* vstackbase = (SK_NODE*)DB_Index(vstack, 0);
	SK_NODE* snapshotbase = (SK_NODE*)DB_Index(stacksnapshot, 0);
	u1 mismatchestack = 0;
	for (u4 i = 0; i < ogvstacksize; i++) {
		if (debugmode & sk_dbmd_checker) {
			SK_NodePrintInfo(cu, snapshotbase);
			SK_NodePrintInfo(cu, vstackbase);
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

inline static u4 SK_PrimSize2Off(u4 primsize) {
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
SK_NODE* SK_CheckOp(SK_COMPUNIT* cu, SK_NODE* cnode, DYNBUFF* vstack, u1* errbuff, const u2 debugmode) {
	if (!cnode) return 0;
	if (debugmode & sk_dbmd_checker) {
		Print("\nnext node:");
		SK_NodePrintInfo(cu, cnode);
	}
	switch (cnode->type) {
		case sk_ndtp_ops: {
			SK_NODE* ndop = cnode + 1;
			for (u8 i = 0; i < cnode->opsc; i++) {
				if (debugmode & sk_dbmd_checker) {
					Print("\nnext node:");
					SK_NodePrintInfo(cu, ndop);
				}
				switch (ndop->type) {
					case sk_ndtp_op: {
						if (DB_ElementCount(vstack) >= skvm_opdesc[ndop->op].argc) {
							// TODO:: at language level i bevieve i dont' need to make a diffecerence between !n and @n,
							// and i could just create syntactic sugar and only write ! and @, and i should infer the size from what's on the stack
							// maybe i could left !n and @n, for a more strict user
							switch (ndop->op) {
								case skvm_op_oogabuga: {
									u4 vstackec = DB_ElementCount(vstack);
									Print("oogabuga!!\n===============\n");
									for (u4 i = 0; i < vstackec; i++) {
										SK_NodePrintInfo(cu, DB_Index(vstack, i));
									}
									Print("===============\n");
								} break;
								case skvm_op_add: {
									SK_NODE* b = (SK_NODE*)DB_SoftPop(vstack);
									SK_NODE* a = (SK_NODE*)DB_Peek(vstack, 1);
									if (a->primlvl && b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "you can't add 2 pointers");
										a->prim = sk_tp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else if (a->primlvl) {}
									else if (b->primlvl) {
										memcpy(a, b, sizeof(SK_NODE));
									}
									else {
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								case skvm_op_sub: {
									SK_NODE* b = (SK_NODE*)DB_SoftPop(vstack);
									SK_NODE* a = (SK_NODE*)DB_Peek(vstack, 1);
									if (a->primlvl && b->primlvl) {
										a->prim = sk_tp_u8;
										a->primlvl = 0;
										a->primsize = 8;
									}
									else if (a->primlvl) {}
									else if (b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "subtracting a pointer is not a valid operation\n");
										a->prim = sk_tp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else {
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								case skvm_op_gt: case skvm_op_eq:
								case skvm_op_lt: {
									SK_NODE* b = (SK_NODE*)DB_SoftPop(vstack);
									SK_NODE* a = (SK_NODE*)DB_Peek(vstack, 1);
									if (a->primlvl && b->primlvl) {
										a->prim = sk_tp_u1;
										a->primlvl = 0;
										a->primsize = 1;
									}
									else if (a->primlvl || b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "comparing a pointer and an integer is not a valid operation\n");
										a->prim = sk_tp_ptr;
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
									SK_NODE* dst = (SK_NODE*)DB_SoftPop(vstack);
									if (dst->primlvl < 1) {
										if (debugmode & sk_dbmd_checker) {
											Print("\n\n\n====================\n");
											SK_NodePrintInfo(cu, ndop);
											Print("\n\n\n====================\n");
										}
										// see todo, for example here, idk what's on the stack exactly so this is the best i can do
										SK_ErrorAtNode(cu, dst, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
									}
									SK_NODE* scr = (SK_NODE*)DB_SoftPop(vstack);
									if (dst->prim != sk_tp_ptr && !SK_CompatibleTypes(scr, dst, 1)) {
										SK_NodePrintInfo(cu, ndop);
										SK_NodePrintInfo(cu, scr);
										SK_NodePrintInfo(cu, dst);
										SK_Node2Str(cu, scr, errbuff);
										SK_Node2Str(cu, dst, errbuff + 256);
										SK_ErrorAtNode(cu, scr, "%s and %s are not compatible for a write to memory", sk_tpdesc[scr->prim].name, sk_tpdesc[dst->prim].name, skvm_opdesc[ndop->op].crname);
									}
								} break;
								case skvm_op_mr1: case skvm_op_mr2:
								case skvm_op_mr4: case skvm_op_mr8:
								{
									SK_NODE* scr = (SK_NODE*)DB_Peek(vstack, 1);
									if (scr->primlvl < 1) {
										// see todo, for example here, idk what's on the stack exactly so this is the best i can do
										if (debugmode & sk_dbmd_checker) {
											Print("\n\n\n====================\n");
											SK_NodePrintInfo(cu, ndop);
											Print("\n\n\n====================\n");
										}
										SK_ErrorAtNode(cu, scr, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
									}
									else {
										scr->primlvl -= 1;
									}
								} break;
								case skvm_op_dmp: case skvm_op_drop: {
									vstack->occ -= sizeof(SK_NODE);
								} break;
								case skvm_op_rtb: {
									SK_NODE tmp;
									SK_NODE* ndbase = DB_Peek(vstack, 3);
									memcpy(&tmp, ndbase, sizeof(SK_NODE));
									memcpy(ndbase, ndbase + 1, sizeof(SK_NODE) * 2);
									memcpy(ndbase + 2, &tmp, sizeof(SK_NODE));
								} break;
								case skvm_op_rtf: {
									SK_NODE tmp;
									SK_NODE* ndtop = DB_Peek(vstack, 1);
									memcpy(&tmp, ndtop, sizeof(SK_NODE));
									memcpy(ndtop, ndtop - 1, sizeof(SK_NODE));
									memcpy(ndtop - 1, ndtop - 2, sizeof(SK_NODE));
									memcpy(ndtop - 2, &tmp, sizeof(SK_NODE));
								} break;
								case skvm_op_swap: {
									SK_NODE tmp;
									SK_NODE* ndtop = DB_Peek(vstack, 1);
									memcpy(&tmp, ndtop, sizeof(SK_NODE));
									memcpy(ndtop, ndtop - 1, sizeof(SK_NODE));
									memcpy(ndtop - 1, &tmp, sizeof(SK_NODE));
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
								case skvm_op_div: case skvm_op_ior:
								{
									SK_NODE* b = (SK_NODE*)DB_SoftPop(vstack);
									SK_NODE* a = (SK_NODE*)DB_Peek(vstack, 1);
									if (a->primlvl || b->primlvl) {
										SK_ErrorAtNode(cu, ndop, "you can't { %s } pointers", skvm_opdesc[ndop->op].name);
										a->prim = sk_tp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
									else {
										// TODO:: think how to handle this better
										SK_CheckTypeArithmetic(a, b);
									}
								} break;
								case skvm_op_not:
								{
									SK_NODE* a = (SK_NODE*)DB_Peek(vstack, 1);
									if (a->primlvl) {
										SK_ErrorAtNode(cu, ndop, "you can't { not } pointers", skvm_opdesc[ndop->op].name);
										a->prim = sk_tp_ptr;
										a->primlvl = 1;
										a->primsize = 8;
									}
								} break;
								case skvm_op_call: {
									SK_NODE* procaddr = (SK_NODE*)DB_SoftPop(vstack);
									SK_SYMBOL* calledproc = (SK_SYMBOL*)DB_Index(cu->syms->pool, procaddr->primoff);
									SK_STR* procstr = (SK_STR*)DB_Index(cu->strs->pool, calledproc->stroff);
									if (procaddr->prim != sk_tp_proc && procaddr->prim != sk_tp_eproc) {
										SK_ErrorAtNode(cu, procaddr, "an uncallable address reach the call");
										// FatalError(0, "for now fatal error\n");
										return 0;
									}
									ndop->symcall = procaddr->primoff;
									SK_NODE* calledtypes = (SK_NODE*)DB_Index(*(DYNBUFF**)DB_Index(cu->ndwebs, calledproc->objid), 1);
									u4 vstacksize = DB_ElementCount(vstack);
									if (vstacksize >= calledtypes->argsc) {
										SK_NODE* calledarg = calledtypes + 1;
										SK_NODE* stackbase = DB_Peek(vstack, calledtypes->argsc);
										if (debugmode & sk_dbmd_checker) {
											SK_NodePrintInfo(cu, ndop);
											Print("checking arguments passed to %$\n", procstr->str, (u8)procstr->len);
										}
										for (u8 i = 0; i < calledtypes->argsc; i++) {
											if (debugmode & sk_dbmd_checker) {
												Print("called:");
												SK_NodePrintInfo(cu, calledarg);
												Print("stack:");
												SK_NodePrintInfo(cu, stackbase);
												Print("\n");
											}
											// TODO:: turn this if into a switch stament for better error report
											if (!SK_CompatibleTypes(stackbase, calledarg, 0)) {
												SK_Node2Str(cu, stackbase, errbuff);
												SK_Node2Str(cu, calledarg, errbuff + 256);
												SK_ErrorAtNode(cu, calledarg, "%s differs from the argument %s needed to call { %$ }", errbuff, errbuff + 256, procstr->str, (u8)procstr->len);
												if (debugmode & sk_dbmd_checker)
													Print("%s differs from the argument %s needed to call { %$ }", errbuff, errbuff + 256, procstr->str, (u8)procstr->len);
											}
											calledarg++;
											stackbase++;
										}
										vstack->occ -= calledtypes->argsc * sizeof(SK_NODE);

										SK_NODE* calledret = calledarg;
										for (u8 i = 0; i < calledtypes->retsc; i++) {
											DB_Push(vstack, calledret);
											calledret++;
										}
									}
									else {
										SK_ErrorAtNode(cu, ndop, "not enough operands to call { %$ }", procstr->str, (u8)procstr->len);
										vstack->occ = 0;
										SK_NODE* calledret = calledtypes + calledtypes->argsc + 1;
										for (u8 i = 0; i < calledtypes->retsc; i++) {
											DB_Push(vstack, calledret);
											calledret++;
										}
									}
								} break;
								default: {
									FatalError(0, "checking for the operation { %s } is not implemented yet\n", skvm_opdesc[ndop->op].name);
								} break;
							}
						}
						else {
							SK_ErrorAtNode(cu, ndop, "not enough operands for { %s }", skvm_opdesc[ndop->op].name);
						}
					} break;
					case sk_ndtp_sym: {
						SK_SYMBOL* symte = (SK_SYMBOL*)DB_Index(cu->syms->pool, ndop->symoff);
						switch (symte->type) {
							case sk_symtp_smlay:
							case sk_symtp_dmlay: {
								DYNBUFF* ndwebmaly = *(DYNBUFF**)DB_Index(cu->ndwebs, symte->objid);
								SK_NODE* ndmlay = (SK_NODE*)DB_Index(ndwebmaly, 1);// since zero is just the name
								if (ndmlay->mlaymemc == 1) {
									SK_NODE* nddmlaymemdesc = ndmlay + 1;
									SK_NODE* nddmlaymemtype = nddmlaymemdesc + 1;

									if (debugmode & sk_dbmd_checker) {
										SK_NodePrintInfo(cu, nddmlaymemdesc);
										SK_NodePrintInfo(cu, nddmlaymemtype);
									}

									DB_Push(vstack, nddmlaymemtype);
									SK_NODE* ndtype = (SK_NODE*)DB_Peek(vstack, 1);
									ndtype->type = sk_ndtp_type;
									ndtype->primlvl = 1 + nddmlaymemtype->primlvl;
									ndtype->primsize = 8;
								}
								else {
									FatalError(0, "structs are not implemented yet { %s }\n", sk_symtp2str[symte->type]);
								}
							} break;
							case sk_symtp_undef: {
								DB_Push(vstack, &(SK_NODE){.type = sk_ndtp_type, .prim = sk_tp_ptr, .primlvl = 1, .primsize = 8, .col = ndop->col, .row = ndop->row });
							} break;
							case sk_symtp_eproc: {
								DB_Push(vstack, &(SK_NODE){.type = sk_ndtp_type, .prim = sk_tp_eproc, .primoff = ndop->symoff, .primlvl = 1, .primsize = 8, .col = ndop->col, .row = ndop->row });
							} break;
							case sk_symtp_proc: {
								DB_Push(vstack, &(SK_NODE){.type = sk_ndtp_type, .prim = sk_tp_proc, .primoff = ndop->symoff, .primlvl = 1, .primsize = 8, .col = ndop->col, .row = ndop->row });
							} break;
							default: {
								FatalError(0, "checking for symbols of the type { %s } is not implemented yet (inner)\n", sk_symtp2str[symte->type]);
							} break;
						}
					} break;
					case sk_ndtp_str: {
						DB_Push(vstack, ndop);
						SK_NODE* ndtype = (SK_NODE*)DB_Peek(vstack, 1);
						ndtype->type = sk_ndtp_type;
						ndtype->prim = sk_tp_u1;
						ndtype->primlvl = 1;
						ndtype->primsize = 8;

						DB_Push(vstack, ndop);
						ndtype = (SK_NODE*)DB_Peek(vstack, 1);
						ndtype->type = sk_ndtp_type;
						ndtype->prim = sk_tp_u4;
						ndtype->primlvl = 0;
						ndtype->primsize = sk_tpdesc[ndtype->prim].size;
					} break;
					case sk_ndtp_num: {
						DB_Push(vstack, ndop);
						SK_NODE* ndtype = (SK_NODE*)DB_Peek(vstack, 1);
						ndtype->type = sk_ndtp_type;
						ndtype->prim = ndop->vtp;
						ndtype->primlvl = ndop->vtp == sk_tp_ptr;
						ndtype->primsize = sk_tpdesc[ndop->vtp].size;
					} break;
					case sk_ndtp_mlay: {
						// TODO:: i think this node should be just ignore atm
						// maybe later i should register it's members as a proper 'type' with a hash func or something
						// then again i should've done that at the previous stage
					} break;
					case sk_ndtp_branchop:
					case sk_ndtp_loopop: {
						ndop = SK_CheckOp(cu, ndop, vstack, errbuff, debugmode) - 1;
					} break;
					case sk_ndtp_cmd: {
						switch (ndop->cmdtp) {
						case sk_kw_mw: {
							if (DB_ElementCount(vstack) >= 2) {
								SK_NODE* dst = (SK_NODE*)DB_SoftPop(vstack);
								if (dst->primlvl < 1) {
									// see todo, for example here, idk what's on the stack exactly so this is the best i can do
									SK_ErrorAtNode(cu, dst, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
								}
								ndop->type = sk_ndtp_op;
								ndop->op = skvm_op_mw1 - 1 + SK_PrimSize2Off(sk_tpdesc[dst->prim].size);
								vstack->occ -= sizeof(SK_NODE);
							}
							else {
								SK_ErrorAtNode(cu, ndop, "not enough arguments to use ! you need at least 1");
							}
						} break;
						case sk_kw_mr: {
							if (DB_ElementCount(vstack) >= 1) {
								SK_NODE* scr = (SK_NODE*)DB_Peek(vstack, 1);
								if (scr->primlvl < 1) {
									// see todo, for example here, idk what's on the stack exactly so this is the best i can do
									SK_ErrorAtNode(cu, scr, "%s needs a proper destinaiton", skvm_opdesc[ndop->op].crname);
								}
								ndop->type = sk_ndtp_op;
								ndop->op = skvm_op_mr1 - 1 + SK_PrimSize2Off(sk_tpdesc[scr->prim].size);
								scr->primlvl -= 1;
							}
							else {
								SK_ErrorAtNode(cu, ndop, "not enough arguments to use @ you need at least 2");
							}
						} break;
						default: {
							SK_NodePrintInfo(cu, ndop);
							FatalError(0, "this command shouldn't had get here { %s })\n", sk_kw2str[ndop->cmdtp]);
						} break;
						}
					} break;
					case sk_ndtp_cast: {
						ndop->type = sk_ndtp_type;
						if (DB_ElementCount(vstack) > 0) {
							memcpy(DB_Peek(vstack, 1), ndop, sizeof(SK_NODE));
						}
						else {
							SK_ErrorAtNode(cu, ndop, "you need at least one element on the stack to cast");
						}
					} break;
					default: {
						SK_NodePrintInfo(cu, ndop);
						FatalError(0, "checking for the node type { %s } is not implemented yet (outer)\n", sk_ndtp2str[ndop->type]);
					} break;
				}
				ndop++;
				if (debugmode & sk_dbmd_checker) {
					Print("\n============vstack============\n");
					u8 vstacksize = DB_ElementCount(vstack);
					for (u8 i = 0; i < vstacksize; ++i) {
						SK_NODE* nd = (SK_NODE*)DB_Index(vstack, i);
						SK_Node2Str(cu, nd, errbuff);
						Print("%s", errbuff);
					}
					Print("\n==============================\n");
				}
			}

			if (debugmode & sk_dbmd_checker) Print("ops done\n");
			return ndop;
		}
		case sk_ndtp_branchop: {
			SK_NODE* ndnxtbranch;
			if (cnode->branchtype == sk_kw_else) {
				DYNBUFF* stacksnapshot = DB_Create(256, sizeof(SK_NODE));
				DB_Extend(stacksnapshot, vstack);
				ndnxtbranch = cnode + 1;

				SK_NODE* ndbranchbody = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
				SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "branch", debugmode);

				ndnxtbranch = SK_CheckOp(cu, ndbranchbody, vstack, errbuff, debugmode);

				DYNBUFF* bodystacksnapshot = DB_Create(256, sizeof(SK_NODE));
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
				DYNBUFF* stacksnapshot = DB_Create(256, sizeof(SK_NODE));
				DB_Extend(stacksnapshot, vstack);
				ndnxtbranch = cnode + 1;

				SK_NODE* ndbranchbody = SK_CheckOp(cu, ndnxtbranch, vstack, errbuff, debugmode);
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
		case sk_ndtp_loopop: {
			DYNBUFF* stacksnapshot = DB_Create(256, sizeof(SK_NODE));
			DB_Extend(stacksnapshot, vstack);

			SK_NODE* ndcondition = cnode + 1;
			SK_NODE* ndloopbody = SK_CheckOp(cu, ndcondition, vstack, errbuff, debugmode);

			SK_CheckCondition(cu, cnode, vstack, stacksnapshot, "while", debugmode);
			SK_NODE* ndendbody = SK_CheckOp(cu, ndloopbody, vstack, errbuff, debugmode);
			SK_CheckBody(cu, cnode, vstack, stacksnapshot, "while", debugmode);

			DB_Free(stacksnapshot);
			return ndendbody;
		}
		default: {
			SK_NodePrintInfo(cu, cnode);
			FatalError(0, "checking for the node type { %s } is not implemented yet (outter)\n", sk_ndtp2str[cnode->type]);
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
void SK_CheckerCheck(SK_COMPUNIT* cu, const u2 debugmode) {
	if (debugmode & sk_dbmd_checker) Print("\n==========checker==============\n");

	u1 errbuff[512];
	DYNBUFF* vstack = DB_Create(256, sizeof(SK_NODE));

	u8 proccount = DB_ElementCount(cu->procs);
	for (u8 proc = 0; proc < proccount; proc++) {
		SK_NODE* procnamend = (SK_NODE*)DB_Index(*(DYNBUFF**)DB_Index(cu->ndwebs, *(u4*)DB_Index(cu->procs, proc)), 0);
		SK_SYMBOL* wsym = (SK_SYMBOL*)DB_Index(cu->syms->pool, procnamend->symoff);
		if (wsym->type == sk_symtp_eproc) continue;
		SK_STR* procnamestr = (SK_STR*)DB_Index(cu->strs->pool, wsym->stroff);
		if (debugmode & sk_dbmd_checker) {
			Print("checking: (%4u:%4u:%4u %$ %s)", procnamend->symoff, wsym->objid, wsym->scopeid, procnamestr->str, (u8)procnamestr->len, sk_symtp2str[wsym->type]);
		}

		SK_NODE* ndtypes = procnamend + 1;
		SK_NODE* argtp = ndtypes + 1;
		for (u4 i = 0; i < ndtypes->argsc; i++) { DB_Push(vstack, argtp++); }

		if (debugmode & sk_dbmd_checker) {
			Print("\n============vstack============\n");
			u8 vstacksize = DB_ElementCount(vstack);
			for (u8 i = 0; i < vstacksize; ++i) {
				SK_NODE* nd = (SK_NODE*)DB_Index(vstack, i);
				SK_Node2Str(cu, nd, errbuff);
				Print("%s", errbuff);
			}
			Print("\n==============================\n");
		}

		SK_NODE* ndprocbody = ndtypes + ndtypes->argsc + ndtypes->retsc + 1;
		SK_CheckOp(cu, ndprocbody, vstack, errbuff, debugmode);

		u4 vstacksize = DB_ElementCount(vstack);
		if (vstacksize == ndtypes->retsc) {
			SK_NODE* rettp = ndtypes + ndtypes->argsc + 1;
			SK_NODE* stackbase = DB_Peek(vstack, ndtypes->retsc);
			for (u8 i = 0; i < ndtypes->retsc; i++) {
				if (debugmode & sk_dbmd_checker) {
					Print("retv:");
					SK_NodePrintInfo(cu, rettp);
					Print("stack:");
					SK_NodePrintInfo(cu, stackbase);
					Print("\n");
				}
				// TODO:: turn this if into a switch stament for better error report
				if (!SK_CompatibleTypes(stackbase, rettp, 0)) {
					SK_Node2Str(cu, stackbase, errbuff);
					SK_Node2Str(cu, rettp, errbuff + 256);
					SK_ErrorAtNode(cu, rettp, "%s differs from the return value %s from call %$", errbuff, errbuff + 256, procnamestr->str, (u8)procnamestr->len);
				}
				rettp++;
				stackbase++;
			}
		}
		else {
			SK_ErrorAtNode(cu, ndtypes, "the procdedure %$ has an unbalanced stack of %8i", procnamestr->str, (u8)procnamestr->len, (s8)(vstacksize - ndtypes->retsc));
		}
		vstack->occ = 0;
		if (debugmode & sk_dbmd_checker) Print("\n===============================\n");
	}
	DB_Free(vstack);
}
#endif // SK_CHECKER_DEF