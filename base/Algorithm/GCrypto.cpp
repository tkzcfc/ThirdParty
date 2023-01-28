#include "GCrypto.h"
#include "crc32.hpp"
#include "base64.hpp"
#include "sha1.hpp"
#include "md5.h"
#include "../Platform/GPlatformMacros.h"

uint32_t GCrypto::CRC32(const std::string& str)
{
	return NFrame::CRC32(str);
}

std::string GCrypto::encodeBase64(const std::string& plaintext)
{
	auto len = base64::encoded_size(plaintext.size());

	std::string ciphertext;
	ciphertext.reserve(len);
	
	if (base64::encode((void*)ciphertext.data(), (const void*)plaintext.data(), plaintext.size()) != len)
	{
		LOG(ERROR) << "encode base64 error:" << plaintext;
		G_ASSERT(false);
	}
	return ciphertext;
}

std::string GCrypto::decodedBase64(const std::string& ciphertext)
{
	auto len = base64::decoded_size(ciphertext.size());

	std::string plaintext;
	plaintext.reserve(len);

	auto result = base64::decode((void*)plaintext.data(), ciphertext.data(), ciphertext.size());
	if (result.first <= 0)
	{
		LOG(ERROR) << "decode base64 error:" << ciphertext;
	}
	return plaintext;
}

std::string GCrypto::SHA1(const std::string& str)
{
	::SHA1 checksum;
	checksum.update(str);
	return checksum.final();
}

std::string GCrypto::SHA1_from_file(const std::string& filename)
{
	return SHA1::from_file(filename);
}

std::string GCrypto::md5(const std::string& str)
{
	return md5Data((uint8_t*)str.c_str(), (int32_t)str.size());
}

std::string GCrypto::md5Data(uint8_t* data, int32_t len)
{
	static const unsigned int MD5_DIGEST_LENGTH = 16;

	md5_state_t state;
	md5_byte_t digest[MD5_DIGEST_LENGTH];
	char hexOutput[(MD5_DIGEST_LENGTH << 1) + 1] = { 0 };

	md5_init(&state);
	md5_append(&state, (const md5_byte_t*)data, len);
	md5_finish(&state, digest);

	for (int di = 0; di < 16; ++di)
		sprintf(hexOutput + di * 2, "%02x", digest[di]);
	return hexOutput;
}

