#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

class CompressionEngine {
public:
    enum class CompressionLevel {
        NONE = 0,
        FASTEST = 1,
        FAST = 3,
        NORMAL = 6,
        MAXIMUM = 9,
        BEST = 9
    };

    enum class CompressionMethod {
        STORED = 0,
        DEFLATED = 8
    };

    struct CompressionResult {
        bool success;
        std::vector<uint8_t> compressedData;
        uint32_t crc32;
        std::string errorMessage;
    };

    struct DecompressionResult {
        bool success;
        std::vector<uint8_t> decompressedData;
        std::string errorMessage;
    };

public:
    CompressionEngine();
    explicit CompressionEngine(CompressionLevel level);
    ~CompressionEngine();

    // Configuration
    void setCompressionLevel(CompressionLevel level);
    CompressionLevel getCompressionLevel() const { return compressionLevel_; }
    void setCompressionMethod(CompressionMethod method);
    CompressionMethod getCompressionMethod() const { return compressionMethod_; }

    // Compression operations
    CompressionResult compress(const std::vector<uint8_t>& data);
    CompressionResult compress(const uint8_t* data, size_t size);
    
    // Decompression operations
    DecompressionResult decompress(const std::vector<uint8_t>& compressedData, size_t expectedSize = 0);
    DecompressionResult decompress(const uint8_t* compressedData, size_t compressedSize, size_t expectedSize = 0);

    // Utility methods
    static uint32_t calculateCRC32(const std::vector<uint8_t>& data);
    static uint32_t calculateCRC32(const uint8_t* data, size_t size);
    static uint32_t updateCRC32(uint32_t crc, const uint8_t* data, size_t size);
    
    // Validation
    static bool isValidCompressionMethod(CompressionMethod method);
    static bool isValidCompressionLevel(CompressionLevel level);

    // Error handling
    static std::string getZlibError(int errorCode);

private:
    CompressionLevel compressionLevel_;
    CompressionMethod compressionMethod_;

    // Internal helper methods
    CompressionResult compressDeflate(const std::vector<uint8_t>& data);
    CompressionResult compressStored(const std::vector<uint8_t>& data);
    DecompressionResult decompressDeflate(const std::vector<uint8_t>& compressedData, size_t expectedSize);
    DecompressionResult decompressStored(const std::vector<uint8_t>& compressedData, size_t expectedSize);

    // Constants
    static constexpr uint32_t CRC32_INITIAL = 0xFFFFFFFF;
    static constexpr size_t DEFAULT_BUFFER_SIZE = 8192;
};