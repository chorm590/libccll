#ifndef __KLCIPH_H__
#define __KLCIPH_H__

typedef enum {
	KLCR_SUCC,
	KLCR_FAIL, // you may try again
	KLCR_NOT_INIT, // just init it
	KLCR_TIME_INVAL, // impossible, don't try any more
	KLCR_ERR // don't try any more
} KLCP_RET;

/*
 * Encrypt with rule of klon-cipher.
 *
 * @param plain [in]
 * @param plen [in]
 * @param cipher [out]
 * 		  256 bytes in success.
 * @param clen [out]
 * 		  256 in success.
 * */
KLCP_RET klciph_enc(uint8_t *plain, int plen, uint8_t *cipher, int *clen);

/*
 * Decrypt with rule of klon-cipher.
 * 
 * @param cipher [in]
 * @param clen [in]
 * @param plain [out]
 * @param plen [out]
 * */
KLCP_RET klciph_dec(uint8_t *cipher, int clen, uint8_t *plain, int *plen);

#endif
