#pragma once

#include <array>
#include <string>

using namespace std;

namespace hide_string
{
    constexpr char hide_str_key = 111;

    template <size_t N, int K>
    class hide_string_impl final
    {
        array<char, N + 1> encrypted_;

        constexpr char enc(const char c) const
        {
            return c ^ hide_str_key;
        }

        char dec(const char c) const
        {
            return c ^ hide_str_key;
        }

    public:
        template <size_t... Is>
        constexpr hide_string_impl(const char* str, index_sequence<Is...>)
            : encrypted_
        {
            enc(str[Is])...
        }
        {

        }

        std::string decrypt()
        {
            std::string str;
            str.resize(encrypted_.size());

            for (size_t i = 0; i < N; ++i)
            {
                str[i] = dec(encrypted_[i]);
            }
            return str;
        }
    };
}

#define HIDE_STR(s) (hide_string::hide_string_impl<sizeof(s) - 1, __COUNTER__ >(s, std::make_index_sequence<sizeof(s) - 1>()).decrypt())
