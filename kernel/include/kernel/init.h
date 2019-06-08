#ifndef _KERNEL_INIT_H
#define _KERNEL_INIT_H

#include <kernel/stdio.h>

//Ignore this next comment...

/*This header file links us to all our init functions.
 *That way we only have to include one thing (I made the mistake in my last OS attempt)*/

//GDT
void initialize_gdt();

#endif