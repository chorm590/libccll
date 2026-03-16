#ifndef __CL_CONVT_H__
#define __CL_CONVT_H__

void cl_print_bytes(uint8_t *bytes, int size);

/*
 *  < 1KB -> nB
 *  < 1MB -> nKB
 *  < 1GB -> nMB
 * others -> nGB
 *
 * @param readable_str [out]
 *        Include the unit.
 * */
Ret cl_bytes_to_readable_str(size_t bytes, char *readable_str);

#endif
