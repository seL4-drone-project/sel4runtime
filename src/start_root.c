/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <sel4/sel4.h>
#include <stdint.h>

#include "auxv.h"
#include "start.h"

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

/*
 * As this file is only included when we are running a root server,
 * these symbols must exist and be provided for this file to function
 * properly.
 *
 * This will generate a link time error if this function is used outside
 * of a root server.
 */

extern unsigned int _tdata_start[];
extern unsigned int _tdata_end[];
extern unsigned int _tbss_end[];

/*
 * The entrypoint into a root task is somewhat different to the
 * entrypoint into a regular process. The kernel does not provide a
 * stack to the root task nor does it conform to System-V ABI; instead
 * it simply starts execution at the entrypoint with the first argument
 * being the pointer to the seL4_BootInfo.
 *
 * This is invoked by _sel4_start, which simply sets up a static stack
 * and passes the argument to us.
 */
void __sel4_start_root(seL4_BootInfo *boot_info) {

	uintptr_t tdata_start = (uintptr_t) &_tdata_start[0];
	uintptr_t tdata_end = (uintptr_t) &_tdata_end[0];
	uintptr_t tbss_end = (uintptr_t) &_tbss_end[0];

	Elf_Phdr tls_header = {
		.p_type   = PT_TLS,
		.p_offset = 0,
		.p_vaddr  = (Elf_Addr) tdata_start,
		.p_paddr  = 0,
		.p_filesz = tdata_end - tdata_start,
		.p_memsz  = tbss_end - tdata_start,
		.p_align = 16,
	};

	auxv_t auxv[] = {
		{
			.a_type = AT_PHENT,
			.a_un.a_val = sizeof(Elf32_Phdr),
		}, {
			.a_type = AT_PHNUM,
			.a_un.a_val = 1,
		}, {
			.a_type = AT_PHDR,
			.a_un.a_ptr = &tls_header,
		}, {
			.a_type = AT_SEL4_BOOT_INFO,
			.a_un.a_ptr = boot_info,
		}, {
			.a_type = AT_SEL4_TCB,
			.a_un.a_val = seL4_CapInitThreadTCB,
		}, {
			// Null terminating entry
			.a_type = AT_NULL,
			.a_un.a_val = 0
		},
	};

	char *envp[] = {
		"seL4=1",
		NULL,
	};

	char *argv[] = {
		"rootserver",
		NULL,
	};

	__sel4runtime_start_main(main, ARRAY_LENGTH(argv), argv, envp, auxv);
}
