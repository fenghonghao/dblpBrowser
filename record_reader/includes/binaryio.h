#ifndef _BINARYIO_H_
#define _BINARYIO_H_

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>


// ================================================================
//  BinaryReader —— RAII 二进制读取器
// ================================================================

class BinaryReader
{
public:
    explicit BinaryReader(const std::string &filename);
    ~BinaryReader();

    // 禁止拷贝, 允许移动
    BinaryReader(const BinaryReader &)            = delete;
    BinaryReader &operator=(const BinaryReader &) = delete;
    BinaryReader(BinaryReader &&)                 = default;
    BinaryReader &operator=(BinaryReader &&)      = default;

    /**
     * @brief 读取 POD 类型数据
     * @tparam T POD 类型
     * @return 读取到的数据
     */
    template <typename T>
    T read();

    /**
     * @brief 读取字符串
     * @return 读取到的字符串
     *
     * 字符串格式：[uint16_t 长度][N 字节内容]
     */
    std::string readString();

    /**
     * @brief 读取 POD vector
     * @tparam T POD 类型
     * @return 读取到的 vector
     *
     * 格式：[uint32_t 个数][T_0][T_1]...
     */
    template <typename T>
    std::vector<T> readVector();

    /**
     * @brief 读取字符串数组（个数嵌入流中）
     * @return 字符串列表
     *
     * 格式：[uint32_t 个数][string_0][string_1]...
     * string 不是 POD，不能用 readVector 模板，所以单独提供
     */
    std::vector<std::string> readStringVec();

    /**
     * @brief 读取固定长度的原始字节到缓冲区
     * @param buf  目标缓冲区
     * @param size 字节数
     *
     * 用途：校验 magic number
     *   char magic[4];
     *   r.readRaw(magic, 4);
     *   if (std::memcmp(magic, "DBLP", 4) != 0) throw ...;
     */
    void readRaw(char *buf, std::size_t size);

    /**
     * @brief 是否已到文件末尾
     */
    bool isEof();

private:
    std::ifstream ifs;

    void checkState();
};


// ---- BinaryReader 模板成员定义（必须可见于头文件） ----

template <typename T>
T BinaryReader::read()
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "read<T>: T 必须是 POD 类型");
    T data{};
    ifs.read(reinterpret_cast<char *>(&data), sizeof(T));
    checkState();
    return data;
}

template <typename T>
std::vector<T> BinaryReader::readVector()
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "readVector<T>: T 必须是 POD  类型");
    uint32_t count = read<uint32_t>();
    std::vector<T> vec(count);
    if (count > 0) {
        ifs.read(reinterpret_cast<char *>(vec.data()),
                 static_cast<std::streamsize>(count * sizeof(T)));
        checkState();
    }
    return vec;
}


// ================================================================
//  BinaryWriter —— RAII 二进制写入器
// ================================================================

class BinaryWriter
{
public:
    explicit BinaryWriter(const std::string &filename);
    ~BinaryWriter();

    BinaryWriter(const BinaryWriter &)            = delete;
    BinaryWriter &operator=(const BinaryWriter &) = delete;
    BinaryWriter(BinaryWriter &&)                 = default;
    BinaryWriter &operator=(BinaryWriter &&)      = default;

    /**
     * @brief 写入一个 POD 类型的值
     * @tparam T POD 类型
     */
    template <typename T>
    void write(T value);

    /**
     * @brief 写入字符串（长度前缀 + 内容）
     * @throws std::runtime_error 长度超 uint16 上限时抛出
     */
    void writeString(const std::string &s);

    /**
     * @brief 批量写入 POD vector
     * @tparam T POD 类型
     *
     * 格式：[uint32_t 个数][T_0][T_1]...
     *
     */
    template <typename T>
    void writeVector(const std::vector<T> &v);

    /**
     * @brief 写入字符串数组
     *
     * 格式：[uint32_t 个数][string_0][string_1]...
     * 每个 string 自带 uint16_t 长度
     */
    void writeStringVec(const std::vector<std::string> &v);

    /**
     * @brief 写入固定长度的原始字节
     *
     * 用途：写文件头 magic
     *   w.writeRaw("DBLP", 4);
     */
    void writeRaw(const char *data, std::size_t size);

private:
    std::ofstream ofs;

    void checkState();
};


// ---- BinaryWriter 模板成员定义（必须可见于头文件） ----

template <typename T>
void BinaryWriter::write(T value)
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "write<T>: T 必须是 POD 类型");
    ofs.write(reinterpret_cast<const char *>(&value), sizeof(T));
    checkState();
}

template <typename T>
void BinaryWriter::writeVector(const std::vector<T> &v)
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "writeVector<T>: T 必须是 POD 类型");
    write<uint32_t>(static_cast<uint32_t>(v.size()));
    if (!v.empty()) {
        ofs.write(reinterpret_cast<const char *>(v.data()),
                  static_cast<std::streamsize>(v.size() * sizeof(T)));
        checkState();
    }
}

#endif // _BINARYIO_H_
