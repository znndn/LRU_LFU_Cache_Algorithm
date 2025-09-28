#pragma once

#include <iostream>

namespace AlgorithmStandard
{

    template<typename Value,typename Key>
    class AlgorithmStandard
    {
        virtual ~AlgorithmStandard()=0;
    public:
        virtual Value Get(Value val,Key key)=0;
        virtual bool Get(Value val)=0;
        
    };
}