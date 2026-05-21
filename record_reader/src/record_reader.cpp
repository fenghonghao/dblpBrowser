#include "record.h"
#include "binaryio.h"

#include <charconv>     // from_chars
#include <cstdint>
#include <cstring>      // memcmp
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>      // std::move
#include <vector>

#include <windows.h>

namespace fs = std::filesystem;

namespace {

// ================================================================
//  常量
// ================================================================
constexpr char     MAGIC[4]         = {'D','B','L','P'};
constexpr uint32_t FORMAT_VERSION   = 1;
constexpr uint64_t PROGRESS_EVERY   = 100'000;

// ================================================================
//  路径
// ================================================================
std::string get_exe_dir()
{
    wchar_t buf[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return fs::current_path().string();
    return fs::path(buf).parent_path().string();
}

// ================================================================
//  二进制缓存
// ================================================================
void save_records(const std::string& bin_path, const std::vector<Record>& recs)
{
    BinaryWriter w(bin_path);
    w.writeRaw(MAGIC, sizeof(MAGIC));
    w.write<uint32_t>(static_cast<uint32_t>(recs.size()));
    for (const auto& r : recs) {
        w.writeString(r.key);
        w.writeString(r.title);
        w.writeString(r.journal);
        w.writeString(r.volume);
        w.write<uint16_t>(r.year);
        w.writeStringVec(r.authors);
    }
}

std::vector<Record> load_records(const std::string& bin_path)
{
    BinaryReader r(bin_path);

    char magic[sizeof(MAGIC)];
    r.readRaw(magic, sizeof(magic));
    if (std::memcmp(magic, MAGIC, sizeof(MAGIC)) != 0)
        throw std::runtime_error("magic 不匹配");

    uint32_t count = r.read<uint32_t>();
    std::vector<Record> out(count);
    for (uint32_t i = 0; i < count; ++i) {
        out[i].id      = i;
        out[i].key     = r.readString();
        out[i].title   = r.readString();
        out[i].journal = r.readString();
        out[i].volume  = r.readString();
        out[i].year    = r.read<uint16_t>();
        out[i].authors = r.readStringVec();
    }
    return out;
}

// ================================================================
//  行扫描解析
// ================================================================

// 去掉首尾空白 + 末尾 '\r'（Windows CRLF）
std::string_view trim_view(std::string_view sv)
{
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t')) sv.remove_prefix(1);
    while (!sv.empty() && (sv.back()  == ' ' || sv.back()  == '\t' || sv.back() == '\r'))
        sv.remove_suffix(1);
    return sv;
}

// 形如 <tag attrs?>content</tag> → out = content；其它情况返回 false
bool extract_field(std::string_view line, std::string_view tag, std::string& out)
{
    if (line.empty() || line.front() != '<') return false;

    std::size_t open_end = line.find('>');
    if (open_end == std::string_view::npos) return false;

    // 自闭合 <tag/> 不算字段
    if (open_end > 0 && line[open_end - 1] == '/') return false;

    // 找标签名末尾（到第一个空格/tab 为止）
    std::size_t name_end = 1;
    while (name_end < open_end
        && line[name_end] != ' '
        && line[name_end] != '\t') {
        ++name_end;
    }
    if (line.substr(1, name_end - 1) != tag) return false;

    // 检查末尾是否是 </tag>（不构造临时 string，零分配）
    std::size_t close_len = tag.size() + 3;            // "</" + tag + ">"
    if (line.size() < open_end + 1 + close_len) return false;
    std::size_t close_start = line.size() - close_len;
    if (line[close_start] != '<' || line[close_start + 1] != '/') return false;
    if (line.compare(close_start + 2, tag.size(), tag) != 0) return false;
    if (line.back() != '>') return false;

    out.assign(line.data() + open_end + 1, close_start - open_end - 1);
    return true;
}

// 从 <article ... key="X" ...> 抽 key 属性
void parse_key_attr(std::string_view line, std::string& out)
{
    std::size_t k = line.find(" key=\"");
    if (k == std::string_view::npos) k = line.find("\tkey=\"");
    if (k == std::string_view::npos) return;
    std::size_t vstart = k + 6;
    std::size_t vend = line.find('"', vstart);
    if (vend == std::string_view::npos) return;
    out.assign(line.data() + vstart, vend - vstart);
}

uint16_t parse_year(std::string_view s)
{
    if (s.empty()) return 0;
    unsigned int v = 0;
    auto res = std::from_chars(s.data(), s.data() + s.size(), v);
    if (res.ec != std::errc{}) return 0;
    return v <= 65535 ? static_cast<uint16_t>(v) : 0;
}

struct Parser {
    Record current;
    std::vector<Record> out;
    bool recording = false;
    uint64_t records_parsed = 0;
};

void process_line(Parser& p, std::string_view line)
{
    line = trim_view(line);
    if (line.empty() || line.front() != '<') return;

    // 1) 记录开始
    if (line.substr(0, 9)  == "<article "        || line.substr(0, 9)  == "<article>"
     || line.substr(0, 15) == "<inproceedings "  || line.substr(0, 15) == "<inproceedings>") {
        p.current = Record{};
        p.recording = true;
        parse_key_attr(line, p.current.key);
        return;
    }

    // 2) 记录结束（后面可能挤着下一个开始标签）
    if (line.substr(0, 10) == "</article>" || line.substr(0, 16) == "</inproceedings>") {
        if (p.recording) {
            p.current.id = static_cast<uint32_t>(p.out.size());
            p.out.push_back(std::move(p.current));
            ++p.records_parsed;
            if (p.records_parsed % PROGRESS_EVERY == 0) {
                std::cerr << "[parse_xml] 已解析 " << p.records_parsed << " 条\n";
            }
        }
        p.recording = false;

        // 同一行还有剩余 → 递归处理
        std::size_t gt = line.find('>');
        if (gt + 1 < line.size()) {
            process_line(p, line.substr(gt + 1));
        }
        return;
    }

    if (!p.recording) return;

    // 3) 字段（一行一个 <tag>content</tag>）—— 字段值原样存，不做编码转换
    std::string content;
    if      (extract_field(line, "author",    content)) p.current.authors.push_back(std::move(content));
    else if (extract_field(line, "title",     content)) p.current.title   = std::move(content);
    else if (extract_field(line, "year",      content)) p.current.year    = parse_year(content);
    else if (extract_field(line, "journal",   content)) p.current.journal = std::move(content);
    else if (extract_field(line, "booktitle", content)) p.current.journal = std::move(content);
    else if (extract_field(line, "volume",    content)) p.current.volume  = std::move(content);
    // 其它（note/ee/cdrom/school/month/cite/crossref/...）静默忽略
}

// 主入口：流式扫描 XML
std::vector<Record> parse_xml(const std::string& xml_path)
{
    std::ifstream f(xml_path, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("[parse_xml] 文件打不开: " + xml_path);

    Parser p;
    std::string line;
    while (std::getline(f, line)) {
        process_line(p, line);
    }
    std::cerr << "[parse_xml] 完成，共 " << p.records_parsed << " 条\n";
    return std::move(p.out);
}

} // namespace

// ================================================================
//  公共入口
// ================================================================
std::vector<Record> read_record(const std::string& path)
{
    std::string xml = path.empty()
        ? (fs::path(get_exe_dir()) / "dblp.xml").string()
        : path;
    std::string bin = fs::path(xml).replace_extension(".bin").string();

    if (!fs::exists(xml))
        throw std::runtime_error("[read_record] 文件不存在: " + xml);
    std::vector<Record> result;

    // bin 优先
    if (fs::exists(bin)) {
        try {
            result = load_records(bin);
            std::cerr << "[read_record] 从 bin 加载 " << result.size() << " 条\n";
            return result;
        } catch (const std::runtime_error& e) {
            std::cerr << "[read_record] bin 加载失败，将重新解析 XML: " << e.what() << "\n";
            result.clear();
        }
    }

    // 解析 XML，然后写 bin 缓存
    result = parse_xml(xml);
    std::cerr << "[read_record] 写入 bin 缓存: " << bin << "\n";
    save_records(bin, result);
    return result;
}
