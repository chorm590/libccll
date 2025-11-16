#ifndef __KLCIPH_H__
#define __KLCIPH_H__

/*
 * Encrypt with rule of klon-cipher.
 *
 * @param plain [in]
 * 		  Allowed range is [1, 120]
 * @param plen [in]
 * @param cipher [out]
 * 		  256 bytes in success.
 * @param clen [out]
 * 		  256 in success.
 * */
Ret klciph_enc(uint8_t *plain, int plen, uint8_t *cipher, int *clen);

/*
 * Decrypt with rule of klon-cipher.
 * 
 * @param cipher [in]
 * @param clen [in]
 * @param plain [out]
 * @param plen [out]
 * */
Ret klciph_dec(uint8_t *cipher, int clen, uint8_t *plain, int *plen);

#endif
