#include "PngWriter.h"

#include <cstdint>
#include <fstream>
#include <vector>

namespace
{

std::uint32_t Crc32(const std::uint8_t* data, size_t len)
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

void AppendU32Be(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
}

void AppendChunk(std::vector<std::uint8_t>& out, const char* type, const std::uint8_t* data, size_t len)
{
    AppendU32Be(out, static_cast<std::uint32_t>(len));
    const size_t typeStart = out.size();
    for (int i = 0; type[i] != '\0'; ++i)
    {
        out.push_back(static_cast<std::uint8_t>(type[i]));
    }
    if (len > 0)
    {
        out.insert(out.end(), data, data + len);
    }
    std::vector<std::uint8_t> typeAndData;
    typeAndData.insert(typeAndData.end(), out.begin() + static_cast<long>(typeStart), out.end());
    const std::uint32_t crc = Crc32(typeAndData.data(), typeAndData.size());
    AppendU32Be(out, crc);
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

std::vector<std::uint8_t> BuildZlibStored(const std::uint8_t* raw, size_t rawLen)
{
    std::vector<std::uint8_t> zlib;
    zlib.push_back(0x78);
    zlib.push_back(0x01);

    size_t offset = 0;
    bool first = true;
    while (offset < rawLen || first)
    {
        first = false;
        const size_t remain = rawLen - offset;
        const size_t blockLen = remain > 65535 ? 65535 : remain;
        const bool bfinal = (offset + blockLen) >= rawLen;
        zlib.push_back(bfinal ? 0x01 : 0x00);
        zlib.push_back(static_cast<std::uint8_t>(blockLen & 0xff));
        zlib.push_back(static_cast<std::uint8_t>((blockLen >> 8) & 0xff));
        const std::uint16_t nlen = static_cast<std::uint16_t>(~blockLen);
        zlib.push_back(static_cast<std::uint8_t>(nlen & 0xff));
        zlib.push_back(static_cast<std::uint8_t>((nlen >> 8) & 0xff));
        zlib.insert(zlib.end(), raw + offset, raw + offset + blockLen);
        offset += blockLen;
        if (blockLen == 0 && bfinal)
        {
            break;
        }
    }

    const std::uint32_t adler = Adler32(raw, rawLen);
    zlib.push_back(static_cast<std::uint8_t>((adler >> 24) & 0xff));
    zlib.push_back(static_cast<std::uint8_t>((adler >> 16) & 0xff));
    zlib.push_back(static_cast<std::uint8_t>((adler >> 8) & 0xff));
    zlib.push_back(static_cast<std::uint8_t>(adler & 0xff));
    return zlib;
}

} // namespace

bool WriteRgbaPng(const std::string& path, const Framebuffer& fb)
{
    if (fb.width <= 0 || fb.height <= 0
        || fb.rgba.size() < static_cast<size_t>(fb.width * fb.height * 4))
    {
        return false;
    }

    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<size_t>(fb.height) * (1 + static_cast<size_t>(fb.width) * 3));
    for (int y = 0; y < fb.height; ++y)
    {
        raw.push_back(0);
        for (int x = 0; x < fb.width; ++x)
        {
            const size_t i = static_cast<size_t>((y * fb.width + x) * 4);
            raw.push_back(fb.rgba[i]);
            raw.push_back(fb.rgba[i + 1]);
            raw.push_back(fb.rgba[i + 2]);
        }
    }

    const std::vector<std::uint8_t> idatData = BuildZlibStored(raw.data(), raw.size());

    std::vector<std::uint8_t> file;
    const std::uint8_t sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    file.insert(file.end(), sig, sig + 8);

    std::uint8_t ihdr[13];
    ihdr[0] = static_cast<std::uint8_t>((fb.width >> 24) & 0xff);
    ihdr[1] = static_cast<std::uint8_t>((fb.width >> 16) & 0xff);
    ihdr[2] = static_cast<std::uint8_t>((fb.width >> 8) & 0xff);
    ihdr[3] = static_cast<std::uint8_t>(fb.width & 0xff);
    ihdr[4] = static_cast<std::uint8_t>((fb.height >> 24) & 0xff);
    ihdr[5] = static_cast<std::uint8_t>((fb.height >> 16) & 0xff);
    ihdr[6] = static_cast<std::uint8_t>((fb.height >> 8) & 0xff);
    ihdr[7] = static_cast<std::uint8_t>(fb.height & 0xff);
    ihdr[8] = 8;
    ihdr[9] = 2;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    AppendChunk(file, "IHDR", ihdr, sizeof(ihdr));
    AppendChunk(file, "IDAT", idatData.data(), idatData.size());
    AppendChunk(file, "IEND", nullptr, 0);

    std::ofstream out(path, std::ios::binary);
    if (!out)
    {
        return false;
    }
    out.write(reinterpret_cast<const char*>(file.data()), static_cast<std::streamsize>(file.size()));
    return static_cast<bool>(out);
}
