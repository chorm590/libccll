#ifndef __CL_INI_H__
#define __CL_INI_H__

/*
 *	Get value fron specify ini-file.
 *	Lightweight parser, one key can be parse at a time.
 *
 *	@param fn [in]
 *			The ini-filename.
 *
 *	@param section [in]
 *			The section name, nullable.
 *
 *	@param key [in]
 *			The key name, can't null.
 *
 *	@param value [out]
 *			The value buffer, can't null.
 *			Caller must ensure the size for value, or it may cause segment-fault.
 * */
Ret cl_ini_get(const char *fn, const char *section, const char *key, char *value);

/*
 *  Same as 'cl_ini_get'.
 *	Get it from cfg-text in memory
 * */
Ret cl_ini_get2(char *cfg_txt, const char *section, const char *key, char *value);

#endif
