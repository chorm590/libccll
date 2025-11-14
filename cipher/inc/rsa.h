#ifndef __CL_RSA_H__
#define __CL_RSA_H__

/*
 * Generate a RSA key-pair
 *
 * @param exponent [in]
 * 		  Eg: 65537
 * @param bits [in]
 * 		  Eg: 2048, 4096
 * @param rsa [out]
 * 		  Auto allocate the memory of RSA, must free it with 'cl_rsa_destroy'
 * */
Ret cl_rsa_gen(const int exponent, const int bits, RSA **rsa);

void cl_rsa_destroy(RSA *rsa);

/*
 * Export the RSA object to extra file.
 *
 * @param pub_key_fn [in]
 * 		  The file path to be export, if NULL, won't export.
 *
 * @param prv_key_fn [in]
 * 		  Same as 'pub_key_fn'.
 * */
Ret cl_rsa_to_file(RSA *rsa, const char *pub_key_fn, const char *prv_key_fn);

/*
 * Export the RSA object to byte stream.
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
Ret cl_rsa_to_bytes(RSA *rsa, uint8_t *pub_key_buf, size_t *pbk_len, uint8_t *prv_key_buf, size_t *pvk_len);

#endif
