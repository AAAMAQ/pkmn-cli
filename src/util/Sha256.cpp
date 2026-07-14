#include "util/Sha256.hpp"

#include <array>
#include <iomanip>
#include <sstream>

namespace pkmn::cli::util {
namespace {
constexpr std::array<std::uint32_t, 64> k = {
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
std::uint32_t R(std::uint32_t v, unsigned n) { return (v >> n) | (v << (32U - n)); }
}  // namespace

std::string Sha256Hex(const std::vector<std::uint8_t>& bytes) {
    auto message = bytes;
    const std::uint64_t bitLength = static_cast<std::uint64_t>(message.size()) * 8U;
    message.push_back(0x80);
    while (message.size() % 64U != 56U) message.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8)
        message.push_back(static_cast<std::uint8_t>(bitLength >> shift));
    std::array<std::uint32_t, 8> hash = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    for (std::size_t chunk = 0; chunk < message.size(); chunk += 64U) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            const auto p = chunk + i * 4U;
            w[i] = (static_cast<std::uint32_t>(message[p]) << 24U) |
                   (static_cast<std::uint32_t>(message[p+1]) << 16U) |
                   (static_cast<std::uint32_t>(message[p+2]) << 8U) | message[p+3];
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0=R(w[i-15],7)^R(w[i-15],18)^(w[i-15]>>3U);
            const auto s1=R(w[i-2],17)^R(w[i-2],19)^(w[i-2]>>10U);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        auto a=hash[0],b=hash[1],c=hash[2],d=hash[3],e=hash[4],f=hash[5],g=hash[6],h=hash[7];
        for (std::size_t i=0;i<64;++i) {
            const auto t1=h+(R(e,6)^R(e,11)^R(e,25))+((e&f)^((~e)&g))+k[i]+w[i];
            const auto t2=(R(a,2)^R(a,13)^R(a,22))+((a&b)^(a&c)^(b&c));
            h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        hash[0]+=a;hash[1]+=b;hash[2]+=c;hash[3]+=d;hash[4]+=e;hash[5]+=f;hash[6]+=g;hash[7]+=h;
    }
    std::ostringstream output; output << std::hex << std::setfill('0');
    for (const auto word : hash) output << std::setw(8) << word;
    return output.str();
}
}  // namespace pkmn::cli::util
