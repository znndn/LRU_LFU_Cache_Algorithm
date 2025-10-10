#include <iostream>
#include <chrono>
#include <iomanip>
#include <random>
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
        LRU::LRUAlgorithm<std::string,int> lru(CAPACITY);
        int hits=0;
        int operations=0;
        std::mt19937 rng;
        std::uniform_int_distribution<int> ReadOrWrite(1,10);
        std::uniform_int_distribution<int> HotOrCold(1,10);
        std::uniform_int_distribution<int> HotKeyGen(0,HOTKEY-1);
        std::uniform_int_distribution<int> ColdKeyGen(HOTKEY,COLDKEYS);

        // 预热
        for (int i=0;i<HOTKEY;i++)
        {
            std::string ToBeValue = "value" + std::to_string(i);
            lru.put(ToBeValue,i);
        }

        Timer t;
        t.TimerStart();
        for (int i=0;i<OPERATIONS;i++)
        {
            
        }
    }

    void printResult(int operations,int hits)
    {
        double hitRate=((double)hits)/((double)operations);
        std::cout<<"热点数据访问测试结果:"<<std::endl;
        std::cout<<"操作数: "<<operations<<std::endl;
        std::cout<<"命中数: "<<hits<<std::endl;
        std::cout<<"命中率: "<<std::fixed<<std::setprecision(4)<<hitRate<<std::endl;
    }

    int main()
    {
        TestAlgorithm();
        return 0;
    }
}