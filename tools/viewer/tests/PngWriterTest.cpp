#include "PngWriter.h"

#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <vector>

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
    ASSERT_GE(bytes.size(), 24u);
    EXPECT_EQ(bytes[0], 0x89);
    EXPECT_EQ(bytes[1], static_cast<unsigned char>('P'));
    auto be32 = [&](size_t i) {
        return (std::uint32_t(bytes[i]) << 24) | (std::uint32_t(bytes[i + 1]) << 16)
            | (std::uint32_t(bytes[i + 2]) << 8) | std::uint32_t(bytes[i + 3]);
    };
    EXPECT_EQ(be32(16), 2u);
    EXPECT_EQ(be32(20), 2u);
}
