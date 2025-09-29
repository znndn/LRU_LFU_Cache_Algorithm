#pragma once

namespace AlgorithmStandard
{

    template<typename Value,typename Key>
    class Algorithmstandard
    {
        virtual ~Algorithmstandard()=0;
    public:
        virtual bool get(Value val,Key key)=0;
        virtual Value get(Key key)=0;
        virtual void put(Value val,Key key)=0;
    };
}