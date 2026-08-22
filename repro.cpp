#include "LFUAlgorithm.h"
#include <string>

int main()
{
    for (int i = 0; i < 100; ++i)  
    {
        LFU::LFUAlgorithm<int, std::string> c(1);  
        c.put("a", 1);                           
        std::string out;
        c.get(1, out);
    }
    return 0;
}
