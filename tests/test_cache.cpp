#include "Cache.h"
#include "test_helpers.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

// ============================================================================
// CacheFileHeader Tests
// ============================================================================

static void testCacheHeaderDefaultsAndValidation() {
    std::cout << "  [TEST] CacheFileHeader defaults and validation... ";

    // Verify constants
    assert(Cache::CACHE_MAGIC == 0x57415643);
    assert(Cache::CACHE_VERSION == 2);

    // Verify default construction
    Cache::CacheFileHeader header;
    assert(header.magic == Cache::CACHE_MAGIC);
    assert(header.version == Cache::CACHE_VERSION);
    assert(header.numChunks == 0);

    // Validation predicate
    auto isValidHeader = [](const Cache::CacheFileHeader& h) -> bool {
        return h.magic == Cache::CACHE_MAGIC && h.version == Cache::CACHE_VERSION;
    };

    assert(isValidHeader(header) == true);

    // Corrupted magic
    Cache::CacheFileHeader invalidMagic = header;
    invalidMagic.magic = 0xDEADBEEF;
    assert(isValidHeader(invalidMagic) == false);

    // Unsupported version
    Cache::CacheFileHeader invalidVersion = header;
    invalidVersion.version = 999;
    assert(isValidHeader(invalidVersion) == false);

    Cache::CacheFileHeader zeroVersion = header;
    zeroVersion.version = 0;
    assert(isValidHeader(zeroVersion) == false);

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// ChunkDescriptor Serialization / Deserialization Tests
// ============================================================================

static void testChunkDescriptorRoundTrip() {
    std::cout << "  [TEST] ChunkDescriptor fields and buffer round-trip... ";

    Cache::ChunkDescriptor original{};
    original.type = Cache::ChunkType::VERTEX_DATA;
    original.version = 1;
    original.offset = 4096;
    original.size = 16384;

    // Serialize to raw buffer
    std::vector<uint8_t> buffer(sizeof(Cache::ChunkDescriptor));
    std::memcpy(buffer.data(), &original, sizeof(original));

    // Deserialize
    Cache::ChunkDescriptor deserialized{};
    std::memcpy(&deserialized, buffer.data(), sizeof(deserialized));

    assert(deserialized.type == original.type);
    assert(deserialized.version == original.version);
    assert(deserialized.offset == original.offset);
    assert(deserialized.size == original.size);

    // Test multiple chunks representing different chunk types
    const Cache::ChunkType chunkTypes[] = {
        Cache::ChunkType::MATERIALS,
        Cache::ChunkType::SCENE_NODES,
        Cache::ChunkType::VERTEX_DATA,
        Cache::ChunkType::TEXTURE_INVENTORY,
        Cache::ChunkType::TEXTURE_ATLAS_PIXELS,
        Cache::ChunkType::TEXTURE_ATLAS_RECTS
    };

    std::vector<Cache::ChunkDescriptor> chunkList;
    uint64_t currentOffset = sizeof(Cache::CacheFileHeader);
    for (size_t i = 0; i < sizeof(chunkTypes) / sizeof(chunkTypes[0]); ++i) {
        Cache::ChunkDescriptor desc{};
        desc.type = chunkTypes[i];
        desc.version = static_cast<uint32_t>(i + 1);
        desc.offset = currentOffset;
        desc.size = static_cast<uint64_t>((i + 1) * 1024);
        currentOffset += desc.size;
        chunkList.push_back(desc);
    }

    std::vector<uint8_t> chunkTableBuffer(chunkList.size() * sizeof(Cache::ChunkDescriptor));
    std::memcpy(chunkTableBuffer.data(), chunkList.data(), chunkTableBuffer.size());

    std::vector<Cache::ChunkDescriptor> readChunkList(chunkList.size());
    std::memcpy(readChunkList.data(), chunkTableBuffer.data(), chunkTableBuffer.size());

    for (size_t i = 0; i < chunkList.size(); ++i) {
        assert(readChunkList[i].type == chunkList[i].type);
        assert(readChunkList[i].version == chunkList[i].version);
        assert(readChunkList[i].offset == chunkList[i].offset);
        assert(readChunkList[i].size == chunkList[i].size);
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TextureInventoryEntry Tests
// ============================================================================

static void testTextureInventoryEntry() {
    std::cout << "  [TEST] TextureInventoryEntry layout, name copy, and dimensions... ";

    Cache::TextureInventoryEntry entry{};
    const char* testPath = "textures/diffuse_bricks.png";
    std::strncpy(entry.normalizedPath, testPath, sizeof(entry.normalizedPath) - 1);
    entry.normalizedPath[sizeof(entry.normalizedPath) - 1] = '\0';
    entry.width = 1920;
    entry.height = 1080;
    entry.bpp = 4;

    assert(std::strcmp(entry.normalizedPath, testPath) == 0);
    assert(entry.width == 1920);
    assert(entry.height == 1080);
    assert(entry.bpp == 4);

    // Serialization round-trip
    std::vector<uint8_t> buffer(sizeof(Cache::TextureInventoryEntry));
    std::memcpy(buffer.data(), &entry, sizeof(entry));

    Cache::TextureInventoryEntry restored{};
    std::memcpy(&restored, buffer.data(), sizeof(restored));

    assert(std::strcmp(restored.normalizedPath, testPath) == 0);
    assert(restored.width == 1920);
    assert(restored.height == 1080);
    assert(restored.bpp == 4);

    // Test max length / truncation safety
    std::string longPath(300, 'a');
    std::strncpy(entry.normalizedPath, longPath.c_str(), sizeof(entry.normalizedPath) - 1);
    entry.normalizedPath[sizeof(entry.normalizedPath) - 1] = '\0';
    assert(std::strlen(entry.normalizedPath) == sizeof(entry.normalizedPath) - 1);

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TextureAtlasRectEntry Tests
// ============================================================================

static void testTextureAtlasRectEntry() {
    std::cout << "  [TEST] TextureAtlasRectEntry fields and packing... ";

    Cache::TextureAtlasRectEntry rect{};
    const char* key = "textures/terrain_rock";
    std::strncpy(rect.textureKey, key, sizeof(rect.textureKey) - 1);
    rect.textureKey[sizeof(rect.textureKey) - 1] = '\0';
    rect.x = 128;
    rect.y = 256;
    rect.width = 512;
    rect.height = 512;
    rect.atlasWidth = 2048;
    rect.atlasHeight = 2048;

    assert(std::strcmp(rect.textureKey, key) == 0);
    assert(rect.x == 128);
    assert(rect.y == 256);
    assert(rect.width == 512);
    assert(rect.height == 512);
    assert(rect.atlasWidth == 2048);
    assert(rect.atlasHeight == 2048);

    // Buffer round-trip
    std::vector<uint8_t> buffer(sizeof(Cache::TextureAtlasRectEntry));
    std::memcpy(buffer.data(), &rect, sizeof(rect));

    Cache::TextureAtlasRectEntry restored{};
    std::memcpy(&restored, buffer.data(), sizeof(restored));

    assert(std::strcmp(restored.textureKey, key) == 0);
    assert(restored.x == 128);
    assert(restored.y == 256);
    assert(restored.width == 512);
    assert(restored.height == 512);
    assert(restored.atlasWidth == 2048);
    assert(restored.atlasHeight == 2048);

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Test Suite Runners
// ============================================================================

int run_cache_tests() {
    std::cout << "\n=== Cache Round-Trip & Schema Tests ===" << std::endl;
    testCacheHeaderDefaultsAndValidation();
    testChunkDescriptorRoundTrip();
    testTextureInventoryEntry();
    testTextureAtlasRectEntry();
    std::cout << "All Cache Round-Trip & Schema tests PASSED!" << std::endl;
    return 0;
}

int runCacheTests() {
    return run_cache_tests();
}
