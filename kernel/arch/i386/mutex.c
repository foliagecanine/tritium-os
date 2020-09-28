#include <kernel/mutex.h>

void MUTEX_LOCK(mutex *m) {
	while (*m)
		;
	*m = 1;
}

bool MUTEX_TRYLOCK(mutex *m) {
	if (*m)
		return false;
	*m = 1;
	return true;
}

void MUTEX_UNLOCK(mutex *m) {
	*m = 0;
}

bool MUTEX_CHECK(mutex *m) {
	return (bool)*m;
}