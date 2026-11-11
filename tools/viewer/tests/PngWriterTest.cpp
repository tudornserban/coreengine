#include "PngWriter.h"

#include <cstdint>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <vector>

namespace
{

std::uint32_t PngCrc32(const std::uint8_t* data, size_t len)
{
    static std::uint32_t table[256];
    static bool init = false;
    if (!init)
    {
        for (std::uint32_t n = 0; n < 256; ++n)
        {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k)
            {
                if (c & 1)
                {
                    c = 0xedb88320u ^ (c >> 1);
                }
                else
                {
                    c >>= 1;
                }
            }
            table[n] = c;
        }
        init = true;
    }
    std::uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < len; ++i)
    {
        crc = table[(crc ^ data[i]) & 0xffu] ^ (crc >> 8);
    }
    return crc ^ 0xffffffffu;
}

std::uint32_t Adler32(const std::uint8_t* data, size_t len)
{
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (size_t i = 0; i < len; ++i)
    {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

} // namespace

TEST(PngWriterTest, IhdrIsWidthThenHeight)
{
    Framebuffer fb;
    fb.width = 2;
    fb.height = 2;
    fb.rgba.assign(16, 0);
    fb.rgba[0] = 255;
    ASSERT_TRUE(WriteRgbaPng("/tmp/viewer-png-test.png", fb));
    std::ifstream in("/tmp/viewer-png-test.png", std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
    ASSERT_GE(bytes.size(), 33u);
    EXPECT_EQ(bytes[0], 0x89);
    EXPECT_EQ(bytes[1], static_cast<unsigned char>('P'));
    auto be32 = [&](size_t i) {
        return (std::uint32_t(bytes[i]) << 24) | (std::uint32_t(bytes[i + 1]) << 16)
            | (std::uint32_t(bytes[i + 2]) << 8) | std::uint32_t(bytes[i + 3]);
    };
    EXPECT_EQ(be32(8), 13u);
    EXPECT_EQ(be32(16), 2u);
    EXPECT_EQ(be32(20), 2u);
    EXPECT_EQ(be32(29), PngCrc32(bytes.data() + 12, 17u));
}

TEST(PngWriterTest, IhdrChunkCrcMismatchWouldFailThisCheck)
{
    Framebuffer fb;
    fb.width = 1;
    fb.height = 1;
    fb.rgba = {10, 20, 30, 255};
    ASSERT_TRUE(WriteRgbaPng("/tmp/viewer-png-ihdr-crc.png", fb));
    std::ifstream in("/tmp/viewer-png-ihdr-crc.png", std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
    const std::uint32_t expected = PngCrc32(bytes.data() + 12, 17);
    EXPECT_EQ((std::uint32_t(bytes[29]) << 24) | (std::uint32_t(bytes[30]) << 16)
            | (std::uint32_t(bytes[31]) << 8) | std::uint32_t(bytes[32]),
        expected);
    EXPECT_NE(expected, 0u);
}

TEST(PngWriterTest, TinyImageZlibTrailerAdler32MatchesReference)
{
    Framebuffer fb;
    fb.width = 1;
    fb.height = 1;
    fb.rgba = {1, 2, 3, 255};
    ASSERT_TRUE(WriteRgbaPng("/tmp/viewer-png-adler.png", fb));
    std::ifstream in("/tmp/viewer-png-adler.png", std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
    size_t pos = 8;
    ASSERT_GE(bytes.size(), pos + 12);
    const std::uint32_t ihdrLen = (std::uint32_t(bytes[pos]) << 24)
        | (std::uint32_t(bytes[pos + 1]) << 16) | (std::uint32_t(bytes[pos + 2]) << 8)
        | std::uint32_t(bytes[pos + 3]);
    pos += 4 + 4 + ihdrLen + 4;

    const std::uint32_t idatLen = (std::uint32_t(bytes[pos]) << 24)
        | (std::uint32_t(bytes[pos + 1]) << 16) | (std::uint32_t(bytes[pos + 2]) << 8)
        | std::uint32_t(bytes[pos + 3]);
    pos += 8;
    ASSERT_GE(bytes.size(), pos + idatLen);
    const std::uint8_t* zlib = bytes.data() + pos;
    const size_t zlibLen = idatLen;
    ASSERT_GE(zlibLen, 4u);
    const std::uint32_t storedAdler = (std::uint32_t(zlib[zlibLen - 4]) << 24)
        | (std::uint32_t(zlib[zlibLen - 3]) << 16) | (std::uint32_t(zlib[zlibLen - 2]) << 8)
        | std::uint32_t(zlib[zlibLen - 1]);

    std::vector<std::uint8_t> raw;
    raw.push_back(0);
    raw.push_back(1);
    raw.push_back(2);
    raw.push_back(3);
    const std::uint32_t expectedAdler = Adler32(raw.data(), raw.size());
    EXPECT_EQ(storedAdler, expectedAdler);
    EXPECT_EQ(expectedAdler, 0x000E0007u);
}
