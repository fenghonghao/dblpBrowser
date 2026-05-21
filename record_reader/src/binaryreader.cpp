#include "binaryio.h"

BinaryReader::BinaryReader(const std::string &filename)
{
    ifs.open(filename, std::ios::binary);
    if (!ifs.is_open())
        throw std::runtime_error("[BinaryReader] 文件打开失败: " + filename);
}

BinaryReader::~BinaryReader()
{
    if (ifs.is_open())
        ifs.close();
}

std::string BinaryReader::readString()
{
    uint16_t length = read<uint16_t>();
    if (length == 0)
        return "";
    std::string str(length, '\0');
    ifs.read(str.data(), length);
    checkState();
    return str;
}

std::vector<std::string> BinaryReader::readStringVec()
{
    uint32_t count = read<uint32_t>();
    std::vector<std::string> vec;
    vec.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        vec.push_back(readString());
    return vec;
}

void BinaryReader::readRaw(char *buf, std::size_t size)
{
    ifs.read(buf, static_cast<std::streamsize>(size));
    checkState();
}

bool BinaryReader::isEof()
{
    ifs.peek();
    return ifs.eof();
}

void BinaryReader::checkState()
{
    if (!ifs.good())
        throw std::runtime_error("[BinaryReader] 读取错误！");
}
