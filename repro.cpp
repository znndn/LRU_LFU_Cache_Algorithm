// repro.cpp
#include "LFUAlgorithm.h"
#include <string>

int main()
{
    for (int i = 0; i < 100; ++i)   // 循环 100 次放大泄漏
    {
        LFU::LFUAlgorithm<int, std::string> c(1);  // 阈值 = 1
        c.put("a", 1);                             // value="a", key=1
        std::string out;
        c.get(1, out);
    }
    return 0;
}
