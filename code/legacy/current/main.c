#include "P:\stdwea\stdwea.h"
#include ".\sk_compunit.h"
#include ".\sk_weaver.h"
#include ".\sk_checker.h"
#include ".\sk_bcgenerator.h"
#include ".\skvm.h"

/*

**: empty
x, : 'or could be' x
x? : 1 or 0 of x
x* : '0 or more of x'
x+ : '1 or more of x'
\x : literal x
(x): grouping of x
counter: *, ?, +
...: many others, but i am lazy to write them up

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
stackop:    \*>, ><, '>, @>, <@, //, ...
memop:      !(1,2,4,8), @(1,2,4,8), #
binop:      \+, -, <, >, &, <<, >>, ^, ...
intrinsic:  dmp
branchop:   if ops block (elif ops block)* (else block)?
loopop:     while ops block
dmemlay:    sym : { num? type }
extrn:      extrn \( string string \)
*/

u4 main(u4 args, u1** vargs) {

	u2 debugmode = sk_dbmd_nodebug;//sk_dbmd_runtests | sk_dbmd_weaver;//sk_dbmd_nodebug| sk_dbmd_checker ;//   | sk_dbmd_bcgenerator;
	// TODO implement all of these
	if (args == 1) {
		Print(
			"\x1b[1musage:\x1b[0m\n"
			"  socks \x1b[33m-flag*\x1b[0m (\x1b[36mfile.sk\x1b[0m|\x1b[36mskbinary.sk\x1b[0m)*\n"
			"  \x1b[2m*  : zero or more\x1b[0m\n"
			"  \x1b[2mx|y: x or y\x1b[0m\n"
			"\n"
			"\x1b[1mmodes:\x1b[0m\n"
			"  \x1b[32mexec\x1b[0m : default; compile source files to temporary bytecode and execute them with the VM\n"
			"  \x1b[32mrun \x1b[0m : load binary files and execute them with the VM\n"
			"  \x1b[32mout \x1b[0m : compile source files to bytecode and save them to files\n"
			"\n"
			"\x1b[1mflags:\x1b[0m\n"
			"  \x1b[33mweaver\x1b[0m     : output debugging information for the node weaving stage\n"
			"  \x1b[33mchecker\x1b[0m    : output debugging information for the checking stage\n"
			"  \x1b[33mbcgenerator\x1b[0m: output debugging information for the bytecode generation stage\n"
			"  \x1b[33mdebuger\x1b[0m    : run the debugger; accepts source files in exec mode or binary files in run mode\n"
			"  \x1b[33mrun\x1b[0m        : switch to run mode; accepts binary files\n"
			"  \x1b[33mout\x1b[0m        : switch to output mode; accepts source files\n"
		);
		return 0x45;
	}

	DYNBUFF* programs = DB_Create(256, sizeof(u1*));
	for (u4 i = 1; i < args; i++) {
		u4 cmdarglen = CstrLen(vargs[i]);
		if (vargs[i][0] == '-') {
			for (u4 dbmd = 1; dbmd < sk_dbmd_count; dbmd++) {
				if (CstrCmp(vargs[i] + 1, sk_dbmd2str[dbmd])) {
					debugmode |= ((u2)1) << (dbmd-1);
					goto nxtarg;
				}
			}
		}
		else {
			DB_Push(programs, &vargs[i]);
		}
	nxtarg:;
	}
	
	SKVM_PROG prog;
	void(*skvm[])(SKVM_PROG*) = { SKVM_Exe, SKVM_ExeDebug };
	
	for (u4 i = 0; i < DB_ElementCount(programs); i++) {
		u1* sourcefiledir = *(u1**)DB_Index(programs, i);
		Print("compiling: %s\n", sourcefiledir);
		SK_COMPUNIT* cu = SK_CompUnitCreate(sourcefiledir, "main");
		
		SK_WeaverWeaveProgram(cu, debugmode);
		SK_CheckerCheck(cu, debugmode);

		if (!cu->errorcount) {
			SK_BCGeneratorGenerateBytecode(&prog, cu, debugmode);
			if (debugmode & sk_dbmd_bytecode) {
				SKVM_PrintByteCodeWithSymbols(prog.bytecode->data, prog.bytecode->occ, prog.symbols);
			}
			skvm[debugmode & sk_dbmd_debugger? 1 : 0](&prog);
		}
		else SK_CompUnitFlushErrors(cu);
		SK_CompUnitFree(cu);
	}
	DB_Free(programs);
	return 0x45;
}