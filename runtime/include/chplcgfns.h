/*
 * Copyright 2020-2021 Hewlett Packard Enterprise Development LP
 * Copyright 2004-2019 Cray Inc.
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// chplcgfns.h
//
// Declarations for variables and prototypes for functions which are
// generated by the chapel compiler.
// This header makes those symbols available to client code in the runtime.
//
// TODO: Arguably, since it contains both function and variable declarations,
//  it should be renamed as chplcgdecl.h (or so).
//
#ifndef _CHPL_GEN_INTERFACE_H_
#define _CHPL_GEN_INTERFACE_H_

#include <stddef.h>
#include <stdlib.h>

#include "chpltypes.h"

/* This header file is for routines that are in the generated code */

#ifdef __cplusplus
extern "C" {
#endif

/* defined in chpl_compilation_config.c: */
extern const char* chpl_compileCommand;
extern const char* chpl_compileVersion;
extern const char* chpl_compileDirectory;
extern const char* chpl_saveCDir;

extern const char* CHPL_HOME;
extern const char* CHPL_HOST_PLATFORM;
extern const char* CHPL_HOST_COMPILER;
extern const char* CHPL_TARGET_PLATFORM;
extern const char* CHPL_TARGET_COMPILER;
extern const char* CHPL_TARGET_CPU;
extern const char* CHPL_LOCALE_MODEL;
extern const char* CHPL_COMM;
extern const char* CHPL_COMM_SUBSTRATE;
extern const char* CHPL_GASNET_SEGMENT;
extern const char* CHPL_LIBFABRIC;
extern const char* CHPL_TASKS;
extern const char* CHPL_LAUNCHER;
extern const char* CHPL_TIMERS;
extern const char* CHPL_MEM;
extern const char* CHPL_MAKE;
extern const char* CHPL_ATOMICS;
extern const char* CHPL_NETWORK_ATOMICS;
extern const char* CHPL_GMP;
extern const char* CHPL_HWLOC;
extern const char* CHPL_RE2;
extern const char* CHPL_LLVM;
extern const char* CHPL_AUX_FILESYS;
extern const char* CHPL_UNWIND;
extern const char* CHPL_RUNTIME_LIB;
extern const char* CHPL_RUNTIME_INCL;
extern const char* CHPL_THIRD_PARTY;
extern const int CHPL_STACK_CHECKS;
extern const int CHPL_CACHE_REMOTE;
extern const int CHPL_INTERLEAVE_MEM;

// Sorted lookup table of filenames used with insertLineNumbers for error
// messages and logging. Defined in chpl_compilation_config.c (needed by launchers)
extern const c_string chpl_filenameTable[];
extern const int32_t chpl_filenameTableSize;

// Lookup tables used as a symbol table by the stack unwinder for translating
// C symbols into Chapel symbols. Defined in chpl_compilation_config.c
extern const c_string chpl_funSymTable[];
extern const int chpl_filenumSymTable[];
extern const int32_t chpl_sizeSymTable;

extern char* chpl_executionCommand;

// Stores embedded code for GPU kernels.
extern const char *chpl_gpuBinary;

/* generated */
extern const chpl_fn_p chpl_ftable[];
extern const chpl_fn_info chpl_finfo[];

extern void chpl__initStringLiterals(void);


void chpl__init_preInit(int64_t _ln, int32_t _fn);
void chpl__init_PrintModuleInitOrder(int64_t _ln, int32_t _fn);
void chpl__init_ChapelStandard(int64_t _ln, int32_t _fn);

/* used for entry point: */
extern int64_t chpl_gen_main(chpl_main_argument* const _arg);

/* used for config vars: */
extern void CreateConfigVarTable(void);

#ifdef __cplusplus
}
#endif

#endif
