#ifndef CRYPTO_ENGINE_H
#define CRYPTO_ENGINE_H

#include <string>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>

class CryptoEngine {
public:
    using ProgressCallback = std::function<void(int)>;

    static bool encryptFile(const std::string& inputPath, 
                           const std::string& outputPath, 
                           const std::string& password,
                           ProgressCallback callback = nullptr);
    
    static bool decryptFile(const std::string& inputPath, 
                           const std::string& outputPath, 
                           const std::string& password,
                           ProgressCallback callback = nullptr);
    
    static int passwordStrength(const std::string& password);

    static bool isEncryptedFile(const std::string& path);

private:
    static void deriveKey(const std::string& password, 
                         CryptoPP::byte* key, size_t keySize, 
                         CryptoPP::byte* salt, size_t saltSize);
    
    static void secureWipe(void* ptr, size_t size);
};

#endif // CRYPTO_ENGINE_H

