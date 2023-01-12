#include "GCrypto.h"
#include "crc32.hpp"
#include "base64.hpp"
#include "sha1.hpp"
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
