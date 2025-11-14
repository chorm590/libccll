#ifndef __CL_TXT_H__
#define __CL_TXT_H__

/*
 * copy a line to 'dst' from 'src'.
 * 
 * @param dst [out]
 * @param max [in]
 * 			access only at most 'max - 1' bytes per line.
 * @param src [in]
 *
 * @return The offset, if 0, the last line occur
 * */
int cl_txt_pos_line(char *dst, const size_t max, char *src);

#endif
