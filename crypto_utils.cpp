#include "crypto_utils.h"
#include <openssl/sha.h>
#include <random>
#include <sstream>
#include <iomanip>

namespace CryptoUtils {

std::string generateSalt(int numBytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    std::ostringstream oss;
    for (int i = 0; i < numBytes; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << dist(gen);
    }
    return oss.str();
}

std::string hashPin(const std::string& pin, const std::string& salt) {
    std::string combined = salt + pin;

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()),
           combined.size(), digest);

    std::ostringstream oss;
    for (unsigned char c : digest) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

}
