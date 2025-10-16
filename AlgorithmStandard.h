#pragma once
inline const int CACHE_CAPACITY=20;
inline const int OPERATIONS=500000;
inline const int HOTKEY=20;
inline const int COLDKEYS=5000;
//  使用 inline 关键字，这明确告诉编译器这个变量可以有多个定义。

// C++中一般不使用c风格的define，而是const int

namespace AlgorithmStandard
{

    // 使用const保护的指针传递要好过值传递

    template<typename Value,typename Key>
    // 这是一个类模板声明
    class Algorithmstandard
    {
    public:
        virtual ~Algorithmstandard()=0;
        // 注意析构函数在大多数情况下都应该是public的
        virtual bool get(const Key& key, Value& value) = 0;
        // 使用bool确认是否查找成功，通过修改指针value传达key对应的value
        virtual void put(const Value& val,const Key& key)=0;
    };

    template<typename Value, typename Key> // 这是一个模板
    Algorithmstandard<Value, Key>::~Algorithmstandard() {}
    // 因为基类的析构函数是一个虚函数，纯虚函数无法被直接使用（子类析构函数执行完后会调用父类析构函数）
    // 所以需要写出定义，使连接器可以找到析构函数的地址。
}