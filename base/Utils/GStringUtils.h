#pragma once

#include <string>
#include <vector>

namespace StringUtils
{
	std::string format(const char* format, ...);

	std::string msgKey(uint32_t msgID);

	void split(const std::string& str, const std::string& delimiter, std::vector<std::string>& result);

#if UNICODE
	static std::wstring stringUtf8ToWideChar(const std::string& strUtf8);

	static std::string stringWideCharToUtf8(const std::wstring& strWideChar);
#else
	static std::string stringUtf8ToWideChar(const std::string& strUtf8)
	{
		return strUtf8;
	}

	static std::string stringWideCharToUtf8(const std::string& strWideChar)
	{
		return strWideChar;
	}
#endif

	bool endswith(const std::string& str, const std::string& end);

	std::string trim(const std::string& str);

	bool startswith(const std::string& str, const std::string& start);

	std::string replaceString(const std::string& str, const std::string& targetstr, const std::string& replacestr);


	// https://github.com/BinomialLLC/crunch

	char* strcpy_safe(char* pDst, uint32_t dst_len, const char* pSrc);

	bool int_to_string(int value, char* pDst, uint32_t len);
	bool uint_to_string(uint32_t value, char* pDst, uint32_t len);

	bool string_to_int(const char*& pBuf, int& value);

	bool string_to_uint(const char*& pBuf, uint32_t& value);

	bool string_to_int64(const char*& pBuf, int64_t& value);
	bool string_to_uint64(const char*& pBuf, uint64_t& value);

	bool string_to_bool(const char* p, bool& value);

	bool string_to_float(const char*& p, float& value, uint32_t round_digit = 512U);

	bool string_to_double(const char*& p, double& value, uint32_t round_digit = 512U);
	bool string_to_double(const char*& p, const char* pEnd, double& value, uint32_t round_digit = 512U);
}
