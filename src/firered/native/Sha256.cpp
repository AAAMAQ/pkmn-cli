#include "Sha256.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <vector>

namespace firered {

namespace {
constexpr std::array<std::uint32_t, 64> k{
    0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,0x923F82A4,0xAB1C5ED5,
    0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,
    0xE49B69C1,0xEFBE4786,0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,
    0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,0x06CA6351,0x14292967,
    0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,
    0xA2BFE8A1,0xA81A664B,0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,
    0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,0x5B9CCA4F,0x682E6FF3,
    0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2
};

std::uint32_t RotateRight(std::uint32_t value, unsigned count) {
    return (value >> count) | (value << (32U - count));
}
} // namespace

std::string Sha256Hex(std::span<const std::uint8_t> bytes) {
    std::vector<std::uint8_t> message(bytes.begin(), bytes.end());
    const std::uint64_t bitLength = static_cast<std::uint64_t>(message.size()) * 8;
    message.push_back(0x80);
    while ((message.size() % 64) != 56) message.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<std::uint8_t>((bitLength >> shift) & 0xFFU));
    }

    std::array<std::uint32_t, 8> hash{
        0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,
        0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19
    };

    for (std::size_t chunk = 0; chunk < message.size(); chunk += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            const std::size_t offset = chunk + i * 4;
            words[i] = (static_cast<std::uint32_t>(message[offset]) << 24)
                | (static_cast<std::uint32_t>(message[offset + 1]) << 16)
                | (static_cast<std::uint32_t>(message[offset + 2]) << 8)
                | static_cast<std::uint32_t>(message[offset + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = RotateRight(words[i - 15], 7) ^ RotateRight(words[i - 15], 18)
                ^ (words[i - 15] >> 3);
            const auto s1 = RotateRight(words[i - 2], 17) ^ RotateRight(words[i - 2], 19)
                ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }

        auto a=hash[0], b=hash[1], c=hash[2], d=hash[3];
        auto e=hash[4], f=hash[5], g=hash[6], h=hash[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto sum1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const auto choose = (e & f) ^ (~e & g);
            const auto temp1 = h + sum1 + choose + k[i] + words[i];
            const auto sum0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = sum0 + majority;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        hash[0]+=a; hash[1]+=b; hash[2]+=c; hash[3]+=d;
        hash[4]+=e; hash[5]+=f; hash[6]+=g; hash[7]+=h;
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto value : hash) out << std::setw(8) << value;
    return out.str();
}

} // namespace firered
