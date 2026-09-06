#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace Capture
{
    struct Buffer
    {
        std::vector<uint8_t> data;
        size_t size = 0;
    };

    struct ShaderProgram
    {
        std::string vertexShaderSource;
        std::string fragmentShaderSource;
    };

    struct DrawCall
    {
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        ShaderProgram program;
    };

    struct Frame
    {
        std::vector<DrawCall> drawCalls;
        Buffer vertexBuffer;
        Buffer indexBuffer;
    };

} // namespace Capture
