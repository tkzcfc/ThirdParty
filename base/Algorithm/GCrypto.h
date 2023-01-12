#pragma once

#include <stdint.h>
#include <string>

class GCrypto
{
public:

	static uint32_t CRC32(const std::string& str);

	static std::string encodeBase64(const std::string& plaintext);

	static std::string decodedBase64(const std::string& ciphertext);

	static std::string SHA1(const std::string& str);

	static std::string SHA1_from_file(const std::string& filename);
};


