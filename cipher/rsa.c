#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bn.h>

#include "_def.h"
#include "def.h"
#include "log.h"
#include "_rsa.h"
#include "rsa.h"

TAG = "rsa";

Ret cl_rsa_init()
{
	TRACE();
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	return SUCC;
}

void cl_rsa_deinit()
{
	TRACE();
}

Ret cl_rsa_gen(const int exponent, const int bits, RSA **rsa)
{
	TRACE();
	if(rsa == NULL) return FAIL;
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

	BIGNUM *bn = BN_new();
	if(bn == NULL)
	{
		CLOGE("new BN failed, err: %d", errno);
		return FAIL;
	}

	if(!BN_set_word(bn, exponent))
	{
		CLOGE("set BN failed, errno: %d", errno);
		BN_free(bn);
		return FAIL;
	}

	RSA *_rsa = RSA_new();
	if(_rsa == NULL)
	{
		CLOGE("new RSA failed, err: %d", errno);
		BN_free(bn);
		return FAIL;
	}

	if(RSA_generate_key_ex(_rsa, bits, bn, NULL) != 1)
	{
		CLOGE("generate the RSA failed, err: %d", errno);
		RSA_free(_rsa);
		BN_free(bn);
		return FAIL;
	}
	BN_free(bn);
	CLOGI("RSA gened, bits: %d", bits);

	*rsa = _rsa;

	return SUCC;
}

void cl_rsa_destroy(RSA *rsa)
{
	TRACE();
	if(rsa == NULL)
	{
		CLOGE(NP);
		return;
	}
	RSA_free(rsa);
}

Ret cl_rsa_to_file(RSA *rsa, const char *pub_key_fn, const char *prv_key_fn)
{
	TRACE();
	if(rsa == NULL) return FAIL;

	if(pub_key_fn)
	{
		BIO *bio_pbk = BIO_new_file(pub_key_fn, "w");
		if(bio_pbk == NULL)
		{
			CLOGE("cre the pub-key file failed, err: %d", errno);
			return FAIL;
		}

		if(PEM_write_bio_RSAPublicKey(bio_pbk, rsa) != 1)
		{
			CLOGE("write pub-key to file failed, err: %d", errno);
			BIO_free_all(bio_pbk);
			remove(pub_key_fn);
			return FAIL;
		}
		BIO_free_all(bio_pbk);
		CLOGI("pub-key wrote to %s", pub_key_fn);
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

		if(PEM_write_bio_RSAPrivateKey(bio_pvk, rsa, NULL, NULL, 0, NULL, NULL) != 1)
		{
			CLOGE("write prv-key to file failed, err: %d", errno);
			BIO_free_all(bio_pvk);
			remove(prv_key_fn);
			if(pub_key_fn) remove(pub_key_fn);
			return FAIL;
		}
		BIO_free_all(bio_pvk);
		CLOGI("prv-key wrote to %s", prv_key_fn);
	}

	return SUCC;
}

Ret cl_rsa_to_bytes(RSA *rsa, uint8_t *pub_key_buf, size_t *pbk_len, uint8_t *prv_key_buf, size_t *pvk_len)
{
	TRACE();
	if(rsa == NULL) return FAIL;

	if(pub_key_buf)
	{
		if(pbk_len == NULL) return FAIL;

		BIO *bio_pbk = BIO_new(BIO_s_mem());
		if(bio_pbk == NULL)
		{
			CLOGE("allocate BIO mem for pbk failed, err: %d", errno);
			return FAIL;
		}

		if(PEM_write_bio_RSAPublicKey(bio_pbk, rsa) != 1)
		{
			CLOGE("convert pub-key to byte stream failed, err: %d", errno);
			BIO_free(bio_pbk);
			return FAIL;
		}
		const size_t _pbk_len = BIO_ctrl_pending(bio_pbk);
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

		if(PEM_write_bio_RSAPrivateKey(bio_pvk, rsa, NULL, NULL, 0, NULL, NULL) != 1)
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

