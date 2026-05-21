#include "record.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <windows.h>

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);   // Windows 终端显示中文不乱码

    try {
        using clock = std::chrono::steady_clock;
        auto t0 = clock::now();

        std::string path = (argc >= 2) ? argv[1] : "";
        auto records = read_record(path);

        auto secs = std::chrono::duration<double>(clock::now() - t0).count();
        std::cout << "\n=== 加载完成 ===\n";
        std::cout << "记录数: " << records.size() << "\n";
        std::cout << "耗时:   " << secs << " 秒\n";

        if (!records.empty()) {
            const auto& f = records.front();
            std::cout << "\n首条样例:\n";
            std::cout << "  key:     " << f.key << "\n";
            std::cout << "  title:   " << f.title.substr(0, 80) << "\n";
            std::cout << "  year:    " << f.year << "\n";
            std::cout << "  journal: " << f.journal << "\n";
            std::cout << "  volume:  " << f.volume << "\n";
            std::cout << "  authors: ";
            for (std::size_t i = 0; i < f.authors.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << f.authors[i];
            }
            std::cout << "\n";
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[错误] " << e.what() << "\n";
        return 1;
    }
}
