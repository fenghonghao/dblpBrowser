#include "binaryio.h"

BinaryWriter::BinaryWriter(const std::string &filename)
{
    ofs.open(filename, std::ios::binary);
    if (!ofs.is_open())
        throw std::runtime_error("[BinaryWriter] 文件打开失败: " + filename);
}

BinaryWriter::~BinaryWriter()
{
    if (ofs.is_open())
        ofs.close();
}

void BinaryWriter::writeString(const std::string &s)
{
    if (s.size() > UINT16_MAX)
        throw std::runtime_error("[BinaryWriter] 字符串长度超过 uint16 上限");
    write<uint16_t>(static_cast<uint16_t>(s.size()));
    if (!s.empty()) {
        ofs.write(s.data(), static_cast<std::streamsize>(s.size()));
        checkState();
    }
}

void BinaryWriter::writeStringVec(const std::vector<std::string> &v)
{
    write<uint32_t>(static_cast<uint32_t>(v.size()));
    for (const auto &s : v)
        writeString(s);
}

void BinaryWriter::writeRaw(const char *data, std::size_t size)
{
    ofs.write(data, static_cast<std::streamsize>(size));
    checkState();
}

void BinaryWriter::checkState()
{
    if (!ofs.good())
        throw std::runtime_error("[BinaryWriter] 写入错误！");
}
