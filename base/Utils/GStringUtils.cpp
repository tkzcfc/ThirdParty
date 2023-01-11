#include "GStringUtils.h"
#include <stdarg.h>
#include <assert.h>
#include "../Platform/GPlatformMacros.h"

namespace StringUtils
{
    std::string format(const char* format, ...)
    {
#define CC_VSNPRINTF_BUFFER_LENGTH 512
        va_list args;
        std::string buffer(CC_VSNPRINTF_BUFFER_LENGTH, '\0');

        va_start(args, format);
        int nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
        va_end(args);

        if (nret >= 0) {
            if ((unsigned int)nret < buffer.length()) {
                buffer.resize(nret);
            }
            else if ((unsigned int)nret > buffer.length()) { // VS2015/2017 or later Visual Studio Version
                buffer.resize(nret);

                va_start(args, format);
                nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

                assert(nret == buffer.length());
            }
            // else equals, do nothing.
        }
        else { // less or equal VS2013 and Unix System glibc implement.
            do {
                buffer.resize(buffer.length() * 3 / 2);

                va_start(args, format);
                nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

            } while (nret < 0);

            buffer.resize(nret);
        }

        return buffer;
    }

    std::string msgKey(uint32_t msgID)
    {
        char szbuf[32];
        sprintf(szbuf, "%u", msgID);
        return szbuf;
    }

    void split(const std::string& str, const std::string& delimiter, std::vector<std::string>& result)
    {
        std::string strTmp = str;
        size_t cutAt;
        while ((cutAt = strTmp.find(delimiter)) != strTmp.npos)
        {
            if (cutAt > 0)
            {
                result.push_back(strTmp.substr(0, cutAt));
            }
            strTmp = strTmp.substr(cutAt + 1);
        }

        if (!strTmp.empty())
        {
            result.push_back(strTmp);
        }
    }


#if UNICODE
    static std::wstring stringUtf8ToWideChar(const std::string& strUtf8)
    {
#if G_TARGET_PLATFORM == G_PLATFORM_WIN32
        std::wstring ret;
        if (!strUtf8.empty())
        {
            int nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, nullptr, 0);
            if (nNum)
            {
                WCHAR* wideCharString = new WCHAR[nNum + 1];
                wideCharString[0] = 0;

                nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wideCharString, nNum + 1);

                ret = wideCharString;
                delete[] wideCharString;
            }
            else
            {
                printf("Wrong convert to WideChar code:0x%x", GetLastError());
            }
        }
        return ret;
#else
        return std::wstring();
#endif
    }

    static std::string stringWideCharToUtf8(const std::wstring& strWideChar)
    {
#if G_TARGET_PLATFORM == G_PLATFORM_WIN32
        std::string ret;
        if (!strWideChar.empty())
        {
            int nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, nullptr, 0, nullptr, FALSE);
            if (nNum)
            {
                char* utf8String = new char[nNum + 1];
                utf8String[0] = 0;

                nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, utf8String, nNum + 1, nullptr, FALSE);

                ret = utf8String;
                delete[] utf8String;
            }
            else
            {
                printf("Wrong convert to Utf8 code:0x%x", GetLastError());
            }
        }

        return ret;
#else
        return std::string();
#endif
    }
#else

#endif



    bool endswith(const std::string& str, const std::string& end)
    {
        int srclen = str.size();
        int endlen = end.size();
        if (srclen >= endlen)
        {
            std::string temp = str.substr(srclen - endlen, endlen);
            if (temp == end)
                return true;
        }

        return false;
    }
    std::string trim(const std::string& str)
    {
        std::string ret;
        //find the first position of not start with space or '\t'
        std::string::size_type pos_begin = str.find_first_not_of(" \t");
        if (pos_begin == std::string::npos)
            return str;

        //find the last position of end with space or '\t'
        std::string::size_type pos_end = str.find_last_not_of(" \t");
        if (pos_end == std::string::npos)
            return str;

        ret = str.substr(pos_begin, pos_end - pos_begin + 1);

        return ret;
    }

    bool startswith(const std::string& str, const std::string& start)
    {
        int srclen = str.size();
        int startlen = start.size();
        if (srclen >= startlen)
        {
            std::string temp = str.substr(0, startlen);
            if (temp == start)
                return true;
        }

        return false;
    }

    std::string replaceString(const std::string& str, const std::string& targetstr, const std::string& replacestr)
    {
        std::string outstr(str);
        while (true)
        {
            std::size_t pos = outstr.find(targetstr.c_str());
            if (pos == std::string::npos)
            {
                break;
            }
            else
            {
                outstr.replace(outstr.begin() + pos, outstr.begin() + pos + targetstr.size(), replacestr.c_str());
            }
        }
        return outstr;
    }


    char* strcpy_safe(char* pDst, uint32_t dst_len, const char* pSrc)
    {
        G_ASSERT(pDst && pSrc && dst_len);
        if (!dst_len)
            return pDst;

        char* q = pDst;
        char c;

        do
        {
            if (dst_len == 1)
            {
                *q++ = '\0';
                break;
            }

            c = *pSrc++;
            *q++ = c;

            dst_len--;

        } while (c);

        G_ASSERT((q - pDst) <= (int)dst_len);

        return pDst;
    }

    bool int_to_string(int value, char* pDst, uint32_t len)
    {
        G_ASSERT(pDst);

        const uint32_t cBufSize = 16;
        char buf[cBufSize];

        uint32_t j = static_cast<uint32_t>((value < 0) ? -value : value);

        char* p = buf + cBufSize - 1;

        *p-- = '\0';

        do
        {
            *p-- = static_cast<uint8_t>('0' + (j % 10));
            j /= 10;
        } while (j);

        if (value < 0)
            *p-- = '-';

        const size_t total_bytes = (buf + cBufSize - 1) - p;
        if (total_bytes > len)
            return false;

        for (size_t i = 0; i < total_bytes; i++)
            pDst[i] = p[1 + i];

        return true;
    }

    bool uint_to_string(uint32_t value, char* pDst, uint32_t len)
    {
        G_ASSERT(pDst);

        const uint32_t cBufSize = 16;
        char buf[cBufSize];

        char* p = buf + cBufSize - 1;

        *p-- = '\0';

        do
        {
            *p-- = static_cast<uint8_t>('0' + (value % 10));
            value /= 10;
        } while (value);

        const size_t total_bytes = (buf + cBufSize - 1) - p;
        if (total_bytes > len)
            return false;

        for (size_t i = 0; i < total_bytes; i++)
            pDst[i] = p[1 + i];

        return true;
    }

    bool string_to_int(const char*& pBuf, int& value)
    {
        value = 0;

        G_ASSERT(pBuf);
        const char* p = pBuf;

        while (*p && isspace(*p))
            p++;

        uint32_t result = 0;
        bool negative = false;

        if (!isdigit(*p))
        {
            if (p[0] == '-')
            {
                negative = true;
                p++;
            }
            else
                return false;
        }

        while (*p && isdigit(*p))
        {
            if (result & 0xE0000000U)
                return false;

            const uint32_t result8 = result << 3U;
            const uint32_t result2 = result << 1U;

            if (result2 > (0xFFFFFFFFU - result8))
                return false;

            result = result8 + result2;

            uint32_t c = p[0] - '0';
            if (c > (0xFFFFFFFFU - result))
                return false;

            result += c;

            p++;
        }

        if (negative)
        {
            if (result > 0x80000000U)
            {
                value = 0;
                return false;
            }
            value = -static_cast<int>(result);
        }
        else
        {
            if (result > 0x7FFFFFFFU)
            {
                value = 0;
                return false;
            }
            value = static_cast<int>(result);
        }

        pBuf = p;

        return true;
    }

    bool string_to_int64(const char*& pBuf, int64_t& value)
    {
        value = 0;

        G_ASSERT(pBuf);
        const char* p = pBuf;

        while (*p && isspace(*p))
            p++;

        uint64_t result = 0;
        bool negative = false;

        if (!isdigit(*p))
        {
            if (p[0] == '-')
            {
                negative = true;
                p++;
            }
            else
                return false;
        }

        while (*p && isdigit(*p))
        {
            if (result & 0xE000000000000000ULL)
                return false;

            const uint64_t result8 = result << 3U;
            const uint64_t result2 = result << 1U;

            if (result2 > (0xFFFFFFFFFFFFFFFFULL - result8))
                return false;

            result = result8 + result2;

            uint32_t c = p[0] - '0';
            if (c > (0xFFFFFFFFFFFFFFFFULL - result))
                return false;

            result += c;

            p++;
        }

        if (negative)
        {
            if (result > 0x8000000000000000ULL)
            {
                value = 0;
                return false;
            }
            value = -static_cast<int64_t>(result);
        }
        else
        {
            if (result > 0x7FFFFFFFFFFFFFFFULL)
            {
                value = 0;
                return false;
            }
            value = static_cast<int64_t>(result);
        }

        pBuf = p;

        return true;
    }

    bool string_to_uint(const char*& pBuf, uint32_t& value)
    {
        value = 0;

        G_ASSERT(pBuf);
        const char* p = pBuf;

        while (*p && isspace(*p))
            p++;

        uint32_t result = 0;

        if (!isdigit(*p))
            return false;

        while (*p && isdigit(*p))
        {
            if (result & 0xE0000000U)
                return false;

            const uint32_t result8 = result << 3U;
            const uint32_t result2 = result << 1U;

            if (result2 > (0xFFFFFFFFU - result8))
                return false;

            result = result8 + result2;

            uint32_t c = p[0] - '0';
            if (c > (0xFFFFFFFFU - result))
                return false;

            result += c;

            p++;
        }

        value = result;

        pBuf = p;

        return true;
    }

    bool string_to_uint64(const char*& pBuf, uint64_t& value)
    {
        value = 0;

        G_ASSERT(pBuf);
        const char* p = pBuf;

        while (*p && isspace(*p))
            p++;

        uint64_t result = 0;

        if (!isdigit(*p))
            return false;

        while (*p && isdigit(*p))
        {
            if (result & 0xE000000000000000ULL)
                return false;

            const uint64_t result8 = result << 3U;
            const uint64_t result2 = result << 1U;

            if (result2 > (0xFFFFFFFFFFFFFFFFULL - result8))
                return false;

            result = result8 + result2;

            uint32_t c = p[0] - '0';
            if (c > (0xFFFFFFFFFFFFFFFFULL - result))
                return false;

            result += c;

            p++;
        }

        value = result;

        pBuf = p;

        return true;
    }

    bool string_to_bool(const char* p, bool& value)
    {
        G_ASSERT(p);

        value = false;

        if (stricmp(p, "false") == 0)
            return true;

        if (stricmp(p, "true") == 0)
        {
            value = true;
            return true;
        }

        const char* q = p;
        uint32_t v;
        if (string_to_uint(q, v))
        {
            if (!v)
                return true;
            else if (v == 1)
            {
                value = true;
                return true;
            }
        }

        return false;
    }

    bool string_to_float(const char*& p, float& value, uint32_t round_digit)
    {
        double d;
        if (!string_to_double(p, d, round_digit))
        {
            value = 0;
            return false;
        }
        value = static_cast<float>(d);
        return true;
    }

    bool string_to_double(const char*& p, double& value, uint32_t round_digit)
    {
        return string_to_double(p, p + 128, value, round_digit);
    }

    // I wrote this approx. 20 years ago in C/assembly using a limited FP emulator package, so it's a bit crude.
    bool string_to_double(const char*& p, const char* pEnd, double& value, uint32_t round_digit)
    {
        G_ASSERT(p);

        value = 0;

        enum { AF_BLANK = 1, AF_SIGN = 2, AF_DPOINT = 3, AF_BADCHAR = 4, AF_OVRFLOW = 5, AF_EXPONENT = 6, AF_NODIGITS = 7 };
        int status = 0;

        const char* buf = p;

        int got_sign_flag = 0, got_dp_flag = 0, got_num_flag = 0;
        int got_e_flag = 0, got_e_sign_flag = 0, e_sign = 0;
        uint32_t whole_count = 0, frac_count = 0;

        double whole = 0, frac = 0, scale = 1, exponent = 1;

        if (p >= pEnd)
        {
            status = AF_NODIGITS;
            goto af_exit;
        }

        while (*buf)
        {
            if (!isspace(*buf))
                break;
            if (++buf >= pEnd)
            {
                status = AF_NODIGITS;
                goto af_exit;
            }
        }

        p = buf;

        while (*buf)
        {
            p = buf;
            if (buf >= pEnd)
                break;

            int i = *buf++;

            switch (i)
            {
            case 'e':
            case 'E':
            {
                got_e_flag = 1;
                goto exit_while;
            }
            case '+':
            {
                if ((got_num_flag) || (got_sign_flag))
                {
                    status = AF_SIGN;
                    goto af_exit;
                }

                got_sign_flag = 1;

                break;
            }
            case '-':
            {
                if ((got_num_flag) || (got_sign_flag))
                {
                    status = AF_SIGN;
                    goto af_exit;
                }

                got_sign_flag = -1;

                break;
            }
            case '.':
            {
                if (got_dp_flag)
                {
                    status = AF_DPOINT;
                    goto af_exit;
                }

                got_dp_flag = 1;

                break;
            }
            default:
            {
                if ((i < '0') || (i > '9'))
                    goto exit_while;
                else
                {
                    i -= '0';

                    got_num_flag = 1;

                    if (got_dp_flag)
                    {
                        if (frac_count < round_digit)
                        {
                            frac = frac * 10.0f + i;

                            scale = scale * 10.0f;
                        }
                        else if (frac_count == round_digit)
                        {
                            if (i >= 5)               /* check for round */
                                frac = frac + 1.0f;
                        }

                        frac_count++;
                    }
                    else
                    {
                        whole = whole * 10.0f + i;

                        whole_count++;

                        if (whole > 1e+100)
                        {
                            status = AF_OVRFLOW;
                            goto af_exit;
                        }
                    }
                }

                break;
            }
            }
        }

    exit_while:

        if (got_e_flag)
        {
            if ((got_num_flag == 0) && (got_dp_flag))
            {
                status = AF_EXPONENT;
                goto af_exit;
            }

            int e = 0;
            e_sign = 1;
            got_num_flag = 0;
            got_e_sign_flag = 0;

            while (*buf)
            {
                p = buf;
                if (buf >= pEnd)
                    break;

                int i = *buf++;

                if (i == '+')
                {
                    if ((got_num_flag) || (got_e_sign_flag))
                    {
                        status = AF_EXPONENT;
                        goto af_exit;
                    }

                    e_sign = 1;
                    got_e_sign_flag = 1;
                }
                else if (i == '-')
                {
                    if ((got_num_flag) || (got_e_sign_flag))
                    {
                        status = AF_EXPONENT;
                        goto af_exit;
                    }

                    e_sign = -1;
                    got_e_sign_flag = 1;
                }
                else if ((i >= '0') && (i <= '9'))
                {
                    got_num_flag = 1;

                    if ((e = (e * 10) + (i - 48)) > 100)
                    {
                        status = AF_EXPONENT;
                        goto af_exit;
                    }
                }
                else
                    break;
            }

            for (int i = 1; i <= e; i++)   /* compute 10^e */
                exponent = exponent * 10.0f;
        }

        if (((whole_count + frac_count) == 0) && (got_e_flag == 0))
        {
            status = AF_NODIGITS;
            goto af_exit;
        }

        if (frac)
            whole = whole + (frac / scale);

        if (got_e_flag)
        {
            if (e_sign > 0)
                whole = whole * exponent;
            else
                whole = whole / exponent;
        }

        if (got_sign_flag < 0)
            whole = -whole;

        value = whole;

    af_exit:
        return (status == 0);
    }

    int stricmp(const char* s1, const char* s2)
    {
        while (*s1 != 0 && *s2 != 0)
        {
            if (*s1 != *s2 && ::toupper(*s1) != ::toupper(*s2))
            {
                return -1;
            }
            s1++;
            s2++;
        }
        return (*s1 == 0 && *s2 == 0) ? 0 : -1;
    }
}

