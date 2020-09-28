#ifndef _KERNEL_MUTEX_H
#define _KERNEL_MUTEX_H

#include <stdbool.h>

#define mutex bool

#define CREATE_MUTEX(m) mutex __mutex_##m = MUTEX_STATE_UNLOCKED; mutex * m = &__mutex_##m

#define MUTEX_STATE_UNLOCKED 	0
#define MUTEX_STATE_LOCKED 		1

void MUTEX_LOCK(mutex *m);
bool MUTEX_TRYLOCK(mutex *m);
void MUTEX_UNLOCK(mutex *m);
bool MUTEX_CHECK(mutex *m);

#endif