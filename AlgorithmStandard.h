#pragma once
#define CAPACITY 20
#define OPERATIONS 500000
#define HOTKEYS 20
#define COLDKEYS 5000

namespace AlgorithmStandard
{

    template<typename Value,typename Key>
    // 这是一个类模板声明
    class Algorithmstandard
    {
    public:
        virtual ~Algorithmstandard()=0;
        // 注意析构函数在大多数情况下都应该是public的
        virtual bool get(Value val,Key key)=0;
        // 通常习惯将 Key放在前面，Value 放在后面
        // 即 template<typename Key, typename Value>。
        // 这只是一个编码约定，不影响功能，但能让代码更符合常规。
        virtual Value get(Key key)=0;
        virtual void put(Value val,Key key)=0;
    };

    template<typename Value, typename Key> // 这是一个模板
    AlgorithmStandard::Algorithmstandard<Value, Key>::~Algorithmstandard() {}
    // 因为基类的析构函数是一个虚函数，纯虚函数无法被直接使用（子类析构函数执行完后会调用父类析构函数）
    // 所以需要写出定义，使连接器可以找到析构函数的地址。
}