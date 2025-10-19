#include <iostream>
#include <chrono>
#include <iomanip>
#include <climits>
#include <random>
#include "LRUAlgorithm.h"
#include "AlgorithmStandard.h"
#include "LFUAlgorithm.h"

namespace TEST
{
    void printResult(int operations,int hits,long long duration,std::string description);

    class Timer
    {
    private:
        std::chrono::high_resolution_clock::time_point start;
        std::chrono::high_resolution_clock::time_point end;
        //  使用high_resolution_clock不收系统时钟影响
    public:
        void TimerStart()
        {
            start = std::chrono::high_resolution_clock ::now();
        }
        auto TimerEnd()
        {
            end = std::chrono::high_resolution_clock ::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            return duration;
        }
    };

    void TestAlgorithm()
    {
        LRU::LRUAlgorithm<std::string,int> lru;
        LFU::LFUAlgorithm<int,std::string> lfuNoReduction(INT_MAX);
        LFU::LFUAlgorithm<int,std::string> lfuWithReduction(9000); // 假设最大阈值是20000来触发衰减
        int hits_LRU=0;
        int hits_LFU=0;
        int hits_LFU_AGING=0;
        int operations=0;
        std::random_device seed;
        // 需要随机种子，每次调用seed生成一个随机数，seed是实例名
        std::mt19937 rng(seed());
        std::uniform_int_distribution<int> GetOrPut(1,10);
        std::uniform_int_distribution<int> HotOrCold(1,10);
        std::uniform_int_distribution<int> HotKeyGen(0,HOTKEY-1);
        std::uniform_int_distribution<int> ColdKeyGen(HOTKEY, HOTKEY + COLDKEYS - 1);
        // 注意冷数据的数量是5000个，而非和热数据一共5000个。

        // 预热
        for (int i=0;i<HOTKEY;i++)
        {
            std::string ToBeValue = "value" + std::to_string(i);
            lru.put(ToBeValue,i);
            lfuNoReduction.put(ToBeValue,i);
            lfuWithReduction.put(ToBeValue,i);
        }


        Timer t;
        t.TimerStart();
        for (int i=0;i<OPERATIONS;i++)
        {
            if (GetOrPut(rng)<=3)
            {
                if (HotOrCold(rng)<=3)
                {
                    int CurrentKey = ColdKeyGen(rng);
                    lru.put("value"+std::to_string(CurrentKey),CurrentKey);
                    lfuNoReduction.put("value"+std::to_string(CurrentKey),CurrentKey);
                    lfuWithReduction.put("value"+std::to_string(CurrentKey),CurrentKey);
                    // 注意格式一致，先写上value
                }
                else
                {
                    int CurrentKey = HotKeyGen(rng);
                    lru.put("value"+std::to_string(CurrentKey),CurrentKey);
                    lfuNoReduction.put("value"+std::to_string(CurrentKey),CurrentKey);
                    lfuWithReduction.put("value"+std::to_string(CurrentKey),CurrentKey);
                }
            }
            else
            {
                operations++;
                if (HotOrCold(rng)<=3)
                {
                    int CurrentKey = ColdKeyGen(rng);
                    std::string retrived_value;
                    // C++允许传入空的函数参数，get通过修改对应指针的值返回value
                    if (lru.get(CurrentKey,retrived_value)==true)
                    {
                        hits_LRU++;
                        // 缓存返回值正确性检查
                        if (retrived_value!="value"+std::to_string(CurrentKey))
                        {
                            std::cout<<"使用get方法时读取错误在"<<retrived_value<<std::endl;
                        }
                    }
                    if (lfuNoReduction.get(CurrentKey,retrived_value)==true)
                    {
                        hits_LFU++;
                        // 缓存返回值正确性检查
                        if (retrived_value!="value"+std::to_string(CurrentKey))
                        {
                            std::cout<<"LFU无衰减使用get方法时读取错误在"<<retrived_value<<std::endl;
                        }
                    }
                    if (lfuWithReduction.get(CurrentKey,retrived_value)==true)
                    {
                        hits_LFU_AGING++;
                        // 缓存返回值正确性检查
                        if (retrived_value!="value"+std::to_string(CurrentKey))
                        {
                            std::cout<<"LFU有衰减使用get方法时读取错误在"<<retrived_value<<std::endl;
                        }
                    }
                }
                else
                {
                    int CurrentKey = HotKeyGen(rng);
                    std::string retrived_value="value"+std::to_string(CurrentKey);
                    if (lru.get(CurrentKey,retrived_value)==true)
                    {
                        hits_LRU++;
                        if (retrived_value!="value"+std::to_string(CurrentKey))
                        {
                            std::cout<<"使用get方法时读取错误在"<<retrived_value<<std::endl;
                        }
                    }
                    if (lfuNoReduction.get(CurrentKey,retrived_value)==true)
                    {
                        hits_LFU++;
                        if (retrived_value!="value"+std::to_string(CurrentKey))
                        {
                            std::cout<<"LFU无衰减使用get方法时读取错误在"<<retrived_value<<std::endl;
                        }
                    }
                    if (lfuWithReduction.get(CurrentKey,retrived_value)==true)
                    {
                        hits_LFU_AGING++;
                        if (retrived_value!="value"+std::to_string(CurrentKey))
                        {
                            std::cout<<"LFU有衰减使用get方法时读取错误在"<<retrived_value<<std::endl;
                        }
                    }
                }
            }
        }
        printResult(operations,hits_LRU,t.TimerEnd().count(),"");
        printResult(operations,hits_LFU,t.TimerEnd().count(),"LFU无衰减");
        printResult(operations,hits_LFU_AGING,t.TimerEnd().count(),"LFU有衰减");
    }

    void printResult(int operations,int hits,long long duration,std::string description)
    {
        double hitRate = static_cast<double>(hits) / static_cast<double>(operations);
        // C++风格的强制类型转换应该使用static_cast
        std::cout<<description+" 热点数据访问测试结果:"<<std::endl;
        std::cout<<"操作数: "<<operations<<std::endl;
        std::cout<<"命中数: "<<hits<<std::endl;
        std::cout<<"命中率: "<<std::fixed<<std::setprecision(4)<<hitRate<<std::endl;
        std::cout<<"总用时: "<<duration<<"ms"<<std::endl;
    }
}

// main函数总是应该在命名空间外部
int main()
{
    TEST::TestAlgorithm();
    return 0;
}