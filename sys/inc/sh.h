#ifndef __CL_SH_H__
#define __CL_SH_H__

/*
 * Execute a shell command and fetch the result.
 *
 * @param cmd [in]
 *        The shell.
 * @param result [out]
 *        The result returned by this shell.
 * @param size [in]
 *        The maximum result length expected. also, the bytes of 'result'.
 * @param rlen [out]
 *        The actual result-bytes in 'result'.
 * */
Ret cl_sh_exec(const char *cmd, char *result, int size, int *rlen);

#endif
