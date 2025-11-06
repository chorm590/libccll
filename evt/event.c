#include <pthread.h>
#include <semaphore.h>

#include "def.h"
#include "log.h"
#include "event.h"
#include "_event.h"

TAG = "evt";

Ret cl_evt_init()
{
	return SUCC;
}
