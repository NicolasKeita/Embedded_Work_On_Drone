/*
Filename: Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Crypto.cpp
Description: SHA-1 digest computation required by the WebSocket opening handshake.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module TwinWebSocketPublisher;

import std;

namespace sim::hil {

namespace {
    /* Rotates one SHA-1 working word left by the requested bit count. */
    constexpr std::uint32_t rotate_left(std::uint32_t value, std::uint32_t bits) noexcept
    {
        return (value << bits) | (value >> (32U - bits));
    }

    /* Expands one 64-byte message block into the 80-word SHA-1 schedule. */
    std::array<std::uint32_t, 80> sha1_message_words(const std::array<std::uint8_t, 128>& message,
                                                     std::size_t offset)
    {
        std::array<std::uint32_t, 80> words{};

        for (std::size_t index = 0; index < 16U; ++index) {
            const std::size_t base = offset + index * 4U;
            words[index] = (static_cast<std::uint32_t>(message[base]) << 24U)
                           | (static_cast<std::uint32_t>(message[base + 1U]) << 16U)
                           | (static_cast<std::uint32_t>(message[base + 2U]) << 8U)
                           | static_cast<std::uint32_t>(message[base + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index) {
            words[index] = rotate_left(words[index - 3U] ^ words[index - 8U]
                                           ^ words[index - 14U] ^ words[index - 16U], 1U);
        }
        return words;
    }

    /* Mixing function of the SHA-1 round index. */
    std::uint32_t sha1_round_function(std::uint32_t b, std::uint32_t c, std::uint32_t d, std::size_t index)
    {
        if (index < 20U) {
            return (b & c) | ((~b) & d);
        }
        if (index >= 40U && index < 60U) {
            return (b & c) | (b & d) | (c & d);
        }
        return b ^ c ^ d;
    }

    /* Additive round constant of the SHA-1 round index. */
    constexpr std::uint32_t sha1_round_constant(std::size_t index) noexcept
    {
        constexpr std::array<std::uint32_t, 4> kRoundConstants{
            0x5A827999U, 0x6ED9EBA1U, 0x8F1BBCDCU, 0xCA62C1D6U};

        return kRoundConstants[std::min(index / 20U, std::size_t{3})];
    }

    /* Compresses one 64-byte block into the five-word SHA-1 state. */
    void sha1_compress_block(const std::array<std::uint8_t, 128>& message,
                             std::size_t                          offset,
                             std::array<std::uint32_t, 5>&        state) noexcept
    {
        const std::array<std::uint32_t, 80> words = sha1_message_words(message, offset);
        std::uint32_t                       a = state[0];
        std::uint32_t                       b = state[1];
        std::uint32_t                       c = state[2];
        std::uint32_t                       d = state[3];
        std::uint32_t                       e = state[4];

        for (std::size_t index = 0; index < words.size(); ++index) {
            const std::uint32_t temporary = rotate_left(a, 5U) + sha1_round_function(b, c, d, index)
                                            + e + sha1_round_constant(index) + words[index];
            e = d;
            d = c;
            c = rotate_left(b, 30U);
            b = a;
            a = temporary;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
    }
}

/* Computes the SHA-1 digest required by the WebSocket opening handshake. */
std::array<std::uint8_t, 20> sha1(std::string_view input) noexcept
{
    std::array<std::uint8_t, 128> message{};
    std::array<std::uint32_t, 5>  state{0x67452301U, 0xEFCDAB89U, 0x98BADCFEU, 0x10325476U, 0xC3D2E1F0U};
    const std::size_t             message_size = input.size();
    const std::size_t             padded_size = ((message_size + 9U + 63U) / 64U) * 64U;

    std::copy(input.begin(), input.end(), message.begin());
    message[message_size] = 0x80U;
    const std::uint64_t bit_length = static_cast<std::uint64_t>(message_size) * 8U;
    for (std::size_t index = 0; index < 8U; ++index) {
        message[padded_size - 1U - index] = static_cast<std::uint8_t>(bit_length >> (index * 8U));
    }
    for (std::size_t offset = 0; offset < padded_size; offset += 64U) {
        sha1_compress_block(message, offset, state);
    }

    std::array<std::uint8_t, 20> digest{};
    for (std::size_t index = 0; index < digest.size(); ++index) {
        digest[index] = static_cast<std::uint8_t>(state[index / 4U] >> (24U - (index % 4U) * 8U));
    }
    return digest;
}

}
