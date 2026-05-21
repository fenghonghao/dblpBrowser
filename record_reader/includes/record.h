#ifndef _RECORD_H_
#define _RECORD_H_

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 一篇论文的核心字段
 *
 * id 语义："该记录在文件中的序号"（从 0 开始）。
 * 不写入文件，loadRecords 按读取顺序自动赋值。
 * 上层索引（作者→论文、年份→论文）只存 id，按 id 反查 Record。
 */
struct Record {
    uint32_t    id = 0;
    std::string key;        // dblp唯一标识
    std::string title;      // F1/F4/F5
    std::string journal;    // F1 展示
    std::string volume;     // F1 展示
    uint16_t    year = 0;   // F4 按年分组
    std::vector<std::string> authors;  // F1/F2/F3/F6 全靠它
};

std::vector<Record> read_record(const std::string& path = "");

#endif // _RECORD_H_
