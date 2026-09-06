#pragma once

#include <cstdint>
#include <vector>

namespace Cache {

// ============================================================================
// E2.B01: Cache vNext schema and chunk registry
// ============================================================================

// Magic number to identify the cache file format (e.g., "WAV")
constexpr uint32_t CACHE_MAGIC = 0x57415643; // 'CVAW'
// Current version of the cache file format. Increment on breaking changes.
// Version 1: Initial chunked schema
// Version 2: Adds TEXTURE_ATLAS_RECTS metadata chunk for atlas UV remap reuse
constexpr uint32_t CACHE_VERSION = 2;

enum class ChunkType : uint32_t {
    UNKNOWN = 0,
    MATERIALS,
    SCENE_NODES,
    VERTEX_DATA,
    TEXTURE_INVENTORY,
    TEXTURE_ATLAS_PIXELS,
    TEXTURE_ATLAS_RECTS,
    // Add other chunk types as needed
};

// Describes a contiguous block of data within the cache file
struct ChunkDescriptor {
    ChunkType type;
    uint32_t version;
    uint64_t offset; // Offset from the beginning of the file to the chunk's data
    uint64_t size;   // Size of the chunk's data in bytes
};

// Main cache file header
struct CacheFileHeader {
    uint32_t magic = CACHE_MAGIC;
    uint32_t version = CACHE_VERSION;
    uint32_t numChunks = 0; // Number of ChunkDescriptor entries that follow the header
};

// ============================================================================
// E2.B02: Deterministic texture inventory and normalization
// ============================================================================

// Entry for the texture inventory chunk
// This helps ensure deterministic texture loading and identification,
// regardless of load order.
struct TextureInventoryEntry {
    char normalizedPath[256]; // Normalized, unique path to the texture
    uint32_t width;
    uint32_t height;
    uint32_t bpp; // bytes per pixel
};

struct TextureAtlasRectEntry {
    char textureKey[256];
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t atlasWidth;
    uint32_t atlasHeight;
};

} // namespace Cache
