#include <chrono>
#include <iostream>
#include "LRUAlgorithm.h"
#include "AlgorithmStandard.h"

namespace TEST
{
    class Timer
    {
    private:
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    public:
        void TimerStart()
        {
            start = std::chrono::system_clock::now();
        }
        auto TimerEnd()
        {
            end = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            return duration;
        }
    };

    void TestAlgorithm()
    {
        LRU::LRUAlgorithm<int,std::string> lru(CAPACITY);

    }
}