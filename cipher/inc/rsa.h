#ifndef __CL_RSA_H__
#define __CL_RSA_H__

#include <openssl/evp.h>

#define CL_RSA_PKCS1  1
#define CL_RSA_PKCS8  2
#ifndef CL_RSA_PEM_FORMAT
#define CL_RSA_PEM_FORMAT CL_RSA_PKCS1
#endif

/*
 * Generate a RSA key-pair
 *
 * @param exponent [in]
 * 		  Eg: 65537
 * @param bits [in]
 * 		  Eg: 2048, 4096
 * @param pkey [out]
 * 		  Auto allocate the memory of EVP_PKEY, must free it with 'cl_rsa_destroy'
 * */
Ret cl_rsa_gen(const int exponent, const int bits, EVP_PKEY **pkey);

void cl_rsa_destroy(EVP_PKEY *pkey);

/*
 * Export the RSA key-pair to file.
 *
 * @param pub_key_fn [in]
 * 		  The file path to be export, if NULL, won't export.
 *
 * @param prv_key_fn [in]
 * 		  Same as 'pub_key_fn'.
 * */
Ret cl_rsa_to_file(EVP_PKEY *pkey, const char *pub_key_fn, const char *prv_key_fn);

/*
 * Export the RSA key-pair to byte stream.
 *
 * @param pub_key_buf [out]
 * 		  The buffer to storage, allocate by caller.
 *
 * @param pbk_len [out]
 * 		  The bytes of pub-key
 *
 * @param prv_key_buf [out]
 * 		  Same as 'pub_key_buf'.
 *
 * @param pvk_len [out]
 * 		  The bytes of prv-key
 * */
Ret cl_rsa_to_bytes(EVP_PKEY *pkey, uint8_t *pub_key_buf, int *pbk_len, uint8_t *prv_key_buf, int *pvk_len);

/*
 * Encrypt 'plain' to 'cipher' with RSA pub-key or prv-key.
 *
 * @param pkey [in]
 *
 * @param with_pbk [in]
 * 		  true  -- encrypt with pub-key.
 * 		  false -- encrypt with prv-key.
 *
 * @param plain [in]
 * 		  Byte stream that wait for encrypt.
 *
 * @param plen [in]
 * 		  The bytes of 'plain'
 *
 * @param cipher [out]
 * 		  Byte stream encrypted.
 *
 * @param clen [out]
 * 		  The bytes of 'cipher'
 * */
Ret cl_rsa_enc(EVP_PKEY *pkey, bool with_pbk, uint8_t *plain, int plen, uint8_t *cipher, int *clen);

/*
 * Decrypt 'cipher' to 'plain' with RSA pub-key or prv-key.
 *
 * @param pkey [in]
 *
 * @param with_pbk [in]
 * 		  true  -- decrypt with pub-key.
 * 		  false -- decrypt with prv-key.
 *
 * @param cipher [in]
 * 		  Byte stream that wait for decrypt.
 *
 * @param clen [in]
 * 		  The bytes of 'cipher'
 *
 * @param plain [out]
 * 		  Byte stream decrypted.
 *
 * @param plen [out]
 * 		  The bytes of 'plain'
 * */
Ret cl_rsa_dec(EVP_PKEY *pkey, bool with_pbk, uint8_t *cipher, int clen, uint8_t *plain, int *plen);

#endif
