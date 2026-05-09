#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/param_build.h>
#include <openssl/core_names.h>
#endif

#include "_def.h"
#include "def.h"
#include "_log.h"
#define DISABLE_TRACE
#include "log.h"
#include "_rsa.h"
#include "rsa.h"

TAG = TAG_PREFIX "rsa";

Ret cl_rsa_gen(const int exponent, const int bits, EVP_PKEY **pkey)
{
	TRACE();
	if(pkey == NULL) return FAIL;
	const int allowed_expn[] = {
		65537,
	};
	const int ae_cnt = sizeof(allowed_expn) / sizeof(int);
	const int allowed_bits[] = {
		2048,
		4096,
	};
	const int ab_cnt = sizeof(allowed_bits) / sizeof(int);
	{
		int i, fod = false;
		for(i = 0; i < ae_cnt; i++)
			if(allowed_expn[i] == exponent)
			{
				fod = true;
				break;
			}
		if(!fod)
		{
			CLOGE("unsupported exponent: %d", exponent);
			return FAIL;
		}
	}
	{
		int i, fod = false;
		for(i = 0; i < ab_cnt; i++)
			if(allowed_bits[i] == bits)
			{
				fod = true;
				break;
			}
		if(!fod)
		{
			CLOGE("unsupported bits: %d", bits);
			return FAIL;
		}
	}

	EVP_PKEY *pk = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
#else
	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
#endif
	if(ctx == NULL)
	{
		CLOGE("new PKEY_CTX failed, err: %d", errno);
		return FAIL;
	}

	if(EVP_PKEY_keygen_init(ctx) != 1)
	{
		CLOGE("keygen init failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) != 1)
	{
		CLOGE("set keygen bits failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	{
		BIGNUM *bn_exp = BN_new();
		if(bn_exp == NULL)
		{
			CLOGE("new BN for pubexp failed, err: %d", errno);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
		if(!BN_set_word(bn_exp, exponent))
		{
			CLOGE("set BN pubexp failed, err: %d", errno);
			BN_free(bn_exp);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
		OSSL_PARAM_BLD *bld = OSSL_PARAM_BLD_new();
		if(bld == NULL)
		{
			CLOGE("new param BLD failed, err: %d", errno);
			BN_free(bn_exp);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
		if(!OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, bn_exp))
		{
			CLOGE("push RSA pubexp to param BLD failed, err: %d", errno);
			BN_free(bn_exp);
			OSSL_PARAM_BLD_free(bld);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
		OSSL_PARAM *params = OSSL_PARAM_BLD_to_param(bld);
		BN_free(bn_exp);
		OSSL_PARAM_BLD_free(bld);
		if(params == NULL)
		{
			CLOGE("build params failed, err: %d", errno);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
		if(EVP_PKEY_CTX_set_params(ctx, params) != 1)
		{
			CLOGE("set keygen pubexp failed, err: %d", errno);
			OSSL_PARAM_free(params);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
		OSSL_PARAM_free(params);
#else
		if(EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, bn_exp) != 1)
		{
			CLOGE("set keygen pubexp failed, err: %d", errno);
			BN_free(bn_exp);
			EVP_PKEY_CTX_free(ctx);
			return FAIL;
		}
		BN_free(bn_exp);
#endif
	}

	if(EVP_PKEY_keygen(ctx, &pk) != 1)
	{
		CLOGE("generate the RSA failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	EVP_PKEY_CTX_free(ctx);
	*pkey = pk;

	return SUCC;
}

void cl_rsa_destroy(EVP_PKEY *pkey)
{
	TRACE();
	if(pkey == NULL)
	{
		CLOGE(NP);
		return;
	}
	EVP_PKEY_free(pkey);
}

Ret cl_rsa_to_file(EVP_PKEY *pkey, const char *pub_key_fn, const char *prv_key_fn)
{
	TRACE();
	if(pkey == NULL) return FAIL;

	if(pub_key_fn)
	{
		BIO *bio_pbk = BIO_new_file(pub_key_fn, "w");
		if(bio_pbk == NULL)
		{
			CLOGE("cre the pub-key file failed, err: %d", errno);
			return FAIL;
		}

		if(PEM_write_bio_PUBKEY(bio_pbk, pkey) != 1)
		{
			CLOGE("write pub-key to file failed, err: %d", errno);
			BIO_free_all(bio_pbk);
			remove(pub_key_fn);
			return FAIL;
		}
		BIO_free_all(bio_pbk);
	}

	if(prv_key_fn)
	{
		BIO *bio_pvk = BIO_new_file(prv_key_fn, "w");
		if(bio_pvk == NULL)
		{
			CLOGE("cre the prv-key file failed, err: %d", errno);
			if(pub_key_fn) remove(pub_key_fn);
			return FAIL;
		}

		if(PEM_write_bio_PrivateKey(bio_pvk, pkey, NULL, NULL, 0, NULL, NULL) != 1)
		{
			CLOGE("write prv-key to file failed, err: %d", errno);
			BIO_free_all(bio_pvk);
			remove(prv_key_fn);
			if(pub_key_fn) remove(pub_key_fn);
			return FAIL;
		}
		BIO_free_all(bio_pvk);
	}

	return SUCC;
}

Ret cl_rsa_to_bytes(EVP_PKEY *pkey, uint8_t *pub_key_buf, int *pbk_len, uint8_t *prv_key_buf, int *pvk_len)
{
	TRACE();
	if(pkey == NULL) return FAIL;

	if(pub_key_buf)
	{
		if(pbk_len == NULL) return FAIL;

		BIO *bio_pbk = BIO_new(BIO_s_mem());
		if(bio_pbk == NULL)
		{
			CLOGE("allocate BIO mem for pbk failed, err: %d", errno);
			return FAIL;
		}

		if(PEM_write_bio_PUBKEY(bio_pbk, pkey) != 1)
		{
			CLOGE("convert pub-key to byte stream failed, err: %d", errno);
			BIO_free(bio_pbk);
			return FAIL;
		}
		const int _pbk_len = BIO_ctrl_pending(bio_pbk);
		BIO_read(bio_pbk, pub_key_buf, _pbk_len);
		*pbk_len = _pbk_len;
		BIO_free(bio_pbk);
	}

	if(prv_key_buf)
	{
		if(pvk_len == NULL) return FAIL;

		BIO *bio_pvk = BIO_new(BIO_s_mem());
		if(bio_pvk == NULL)
		{
			CLOGE("allocate BIO mem for pvk failed, err: %d", errno);
			return FAIL;
		}

		if(PEM_write_bio_PrivateKey(bio_pvk, pkey, NULL, NULL, 0, NULL, NULL) != 1)
		{
			CLOGE("convert prv-key to byte stream failed, err: %d", errno);
			BIO_free(bio_pvk);
			return FAIL;
		}
		const size_t _pvk_len = BIO_ctrl_pending(bio_pvk);
		BIO_read(bio_pvk, prv_key_buf, _pvk_len);
		*pvk_len = _pvk_len;
		BIO_free(bio_pvk);
	}

	return SUCC;
}

Ret cl_rsa_enc(EVP_PKEY *pkey, bool with_pbk, uint8_t *plain, int plen, uint8_t *cipher, int *clen)
{
	TRACE();
	if(pkey == NULL || plain == NULL || cipher == NULL || clen == NULL) return FAIL;

	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pkey, NULL);
	if(ctx == NULL)
	{
		CLOGE("new PKEY_CTX for enc failed, err: %d", errno);
		return FAIL;
	}

	size_t out_len = 0;
	int ret;

	if(with_pbk)
	{
		ret = EVP_PKEY_encrypt_init(ctx);
	}
	else
	{
		ret = EVP_PKEY_sign_init(ctx);
	}

	if(ret != 1)
	{
		CLOGE("enc init failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) != 1)
	{
		CLOGE("set enc padding failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(with_pbk)
	{
		ret = EVP_PKEY_encrypt(ctx, NULL, &out_len, plain, plen);
	}
	else
	{
		ret = EVP_PKEY_sign(ctx, NULL, &out_len, plain, plen);
	}

	if(ret != 1)
	{
		CLOGE("enc query size with %s failed, err: %d", with_pbk ? "pub-key" : "prv-key", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(with_pbk)
	{
		ret = EVP_PKEY_encrypt(ctx, cipher, &out_len, plain, plen);
	}
	else
	{
		ret = EVP_PKEY_sign(ctx, cipher, &out_len, plain, plen);
	}

	if(ret != 1)
	{
		CLOGE("enc with %s failed, err: %d", with_pbk ? "pub-key" : "prv-key", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	*clen = out_len;
	EVP_PKEY_CTX_free(ctx);

	return SUCC;
}

Ret cl_rsa_dec(EVP_PKEY *pkey, bool with_pbk, uint8_t *cipher, int clen, uint8_t *plain, int *plen)
{
	TRACE();
	if(pkey == NULL || cipher == NULL || plain == NULL || plen == NULL) return FAIL;

	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pkey, NULL);
	if(ctx == NULL)
	{
		CLOGE("new PKEY_CTX for dec failed, err: %d", errno);
		return FAIL;
	}

	size_t out_len = 0;
	int ret;

	if(with_pbk)
	{
		ret = EVP_PKEY_verify_recover_init(ctx);
	}
	else
	{
		ret = EVP_PKEY_decrypt_init(ctx);
	}

	if(ret != 1)
	{
		CLOGE("dec init failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) != 1)
	{
		CLOGE("set dec padding failed, err: %d", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(with_pbk)
	{
		ret = EVP_PKEY_verify_recover(ctx, NULL, &out_len, cipher, clen);
	}
	else
	{
		ret = EVP_PKEY_decrypt(ctx, NULL, &out_len, cipher, clen);
	}

	if(ret != 1)
	{
		CLOGE("dec query size with %s failed, err: %d", with_pbk ? "pub-key" : "prv-key", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	if(with_pbk)
	{
		ret = EVP_PKEY_verify_recover(ctx, plain, &out_len, cipher, clen);
	}
	else
	{
		ret = EVP_PKEY_decrypt(ctx, plain, &out_len, cipher, clen);
	}

	if(ret != 1)
	{
		CLOGE("dec with %s failed, err: %d", with_pbk ? "pub-key" : "prv-key", errno);
		EVP_PKEY_CTX_free(ctx);
		return FAIL;
	}

	*plen = out_len;
	EVP_PKEY_CTX_free(ctx);

	return SUCC;
}
