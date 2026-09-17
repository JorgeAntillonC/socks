#ifndef SK_COMPUNIT_INCLUDE
#define SK_COMPUNIT_INCLUDE
#include "P:\stdwea\gdefwea.h"
#include "P:\stdwea\dynbuffwea.h"
#include "P:\stdwea\phashtablewea.h"
#include "P:\stdwea\iowea.h"
#include ".\sk_commondef.h"

u1 SK_TableStrCmp(PHASHTABLE* table, PHTENTRY* entry, u1* key, u4 keysize);

u1 SK_TableSymbolCmp(PHASHTABLE* table, PHTENTRY* entry, u1* key, u4 keysize);

SK_COMPUNIT* SK_CompUnitCreate(u1* sourcefile, u1* entrypoint);

void SK_CompUnitFree(SK_COMPUNIT* cu);

void SK_CompUnitFlushErrors(SK_COMPUNIT* cu);
#endif // SK_COMPUNIT_INCLUDE

#ifndef SK_COMPUNIT_DEF
#define SK_COMPUNIT_DEF

u1 SK_TableStrCmp(PHASHTABLE* table, PHTENTRY* entry, u1* key, u4 keysize) {
	SK_STR* skstr = (SK_STR*)DB_Index(table->pool, entry->off);
	return skstr->len == keysize && memcmp(skstr->str, key, skstr->len);
}

u1 SK_TableSymbolCmp(PHASHTABLE* table, PHTENTRY* entry, u1* key, u4 keysize) {
	SK_SYMBOL* sksym = (SK_SYMBOL*)DB_Index(table->pool, entry->off);
	u8 symkey = ((u8)sksym->scopeid << 32) | sksym->stroff;
	return symkey == *(u8*)key;
}

SK_COMPUNIT* SK_CompUnitCreate(u1* sourcefile, u1* entrypoint) {
	SK_COMPUNIT* tmp = (SK_COMPUNIT*)Malloc(sizeof(SK_COMPUNIT));
	memset(tmp, 0, sizeof(SK_COMPUNIT));
	tmp->syms = PHT_Create(sizeof(SK_SYMBOL), PHT_Hash, SK_TableSymbolCmp);
	tmp->strs = PHT_Create(sizeof(u1), PHT_Hash, SK_TableStrCmp);
	tmp->ndwebs = DB_Create(256, sizeof(DYNBUFF*));
	tmp->procs = DB_Create(256, sizeof(u4));
	tmp->errpool = DB_Create(256, sizeof(u1));
	tmp->compunits = DB_Create(256, sizeof(u4));

	u1 partialpath[MAX_PATH];
	u1 fullpath[MAX_PATH];
	
	tmp->cwd    = Malloc(MAX_PATH);
	tmp->cwdlen = GetCurrentDirectoryA(MAX_PATH, tmp->cwd);
	CstrFmt(partialpath, "%$\\%$", tmp->cwd, (u8)tmp->cwdlen, sourcefile, CstrLen(sourcefile));
	
	u4 pathlen = GetFullPathNameA(partialpath, MAX_PATH, fullpath, 0);
	u4 stroff = SK_TableStrInsert(tmp->strs, fullpath, pathlen);
	tmp->currentcompunit = PHT_Insert(tmp->syms, &stroff, 8, &(SK_SYMBOL){ .type = sk_symtp_fdir, .stroff = stroff, .scopeid = 0 }, sizeof(SK_SYMBOL));
	DB_Push(tmp->compunits, &(u4){ tmp->currentcompunit });
	
	
	// TODO:: i think i should initiallize all reversed words here, that would simplify the the NxtNode func i think	
	u4 nulloff = SK_TableStrInsert(tmp->strs, "null", 4);
	PHT_Insert(tmp->syms, &(u8) { nulloff }, 8, &(SK_SYMBOL){.type = sk_symtp_kw, .stroff = nulloff, .scopeid = 0  }, sizeof(SK_SYMBOL));
	u4 entrypointoff = SK_TableStrInsert(tmp->strs, entrypoint, CstrLen(entrypoint)-1);
	tmp->entrypoint = PHT_Insert(tmp->syms, &(u8){ entrypointoff }, 8, &(SK_SYMBOL){.type = sk_symtp_undef, .scopeid = 0, .stroff = entrypointoff }, sizeof(SK_SYMBOL));
	return tmp;
}

void SK_CompUnitFree(SK_COMPUNIT* cu) {
	u4 webcount = DB_ElementCount(cu->ndwebs);
	DYNBUFF** web = (DYNBUFF**)DB_Index(cu->ndwebs, 0);
	for (u4 i = 0; i < webcount; i++) { DB_Free(*web++); }
	DB_Free(cu->ndwebs);
	DB_Free(cu->procs);
	DB_Free(cu->compunits);
	DB_Free(cu->errpool);
	PHT_Free(cu->syms);
	PHT_Free(cu->strs);
	Free(cu->cwd);
	Free(cu);
}

void SK_CompUnitFlushErrors(SK_COMPUNIT* cu) {
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
		"Trying to compile this garbage consumed electricity that could have powered a hospital.",
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
	Print("%s\n", messages[cu->errorcount % (sizeof(messages) / sizeof(u1*))]);
}
#endif // SK_COMPUNIT_DEF