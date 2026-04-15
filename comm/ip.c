#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "def.h"
#include "_log.h"
#include "log.h"

TAG = TAG_PREFIX "alloc";

bool cl_chk_ipv4(const char *ip) {
	(void) cltag;
    if (ip == NULL || *ip == '\0') {
        return false;
    }

    int dots = 0;
    int num = 0;
    int digit_count = 0;
    
    while (*ip) {
        if (*ip == '.') {
            if (digit_count == 0 || num > 255) {
                return false;
            }
            dots++;
            num = 0;
            digit_count = 0;
        } 
        else if (isdigit(*ip)) {
            num = num * 10 + (*ip - '0');
            digit_count++;
            if (digit_count > 3 || num > 255) {
                return false;
            }
        } 
        else {
            return false;
        }
        ip++;
    }
    
    if (digit_count == 0 || num > 255 || dots != 3) {
        return false;
    }
    
    return true;
}

