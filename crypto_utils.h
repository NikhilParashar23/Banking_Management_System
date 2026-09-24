#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>

namespace CryptoUtils {

    // Generates a random hex salt of the given byte length
    // (default 16 bytes -> 32 hex characters).
    std::string generateSalt(int numBytes = 16);

    // Returns SHA-256(salt + pin) as a 64-character hex string.
    // We hash salt+pin together so two users with the same PIN
    // still get completely different stored hashes.
    std::string hashPin(const std::string& pin, const std::string& salt);

}

#endif
