#ifndef __CL_MAC_H__
#define __CL_MAC_H__

/*
 * Supported formats:
 * 	1. 00:11:22:33:44:55
 * 	2. 00-11-22-33-44-55
 * 	3. 001122334455
 * */
bool cl_is_mac_valid(const char *mac_str);

#endif
