#include <iostream>
#include <memory>
#include <chrono>
#include <string>
#include <unordered_map>
#include "AlgorithmStandard.h"

namespace LRU
{
    template<typename Value,typename Key>
    class LRUAlgorithm;

    template<typename Value,typename Key>
    class LRUNode
    {
        friend class LRUAlgorithm<Value,Key>;
        private:
        Key key;
        Value value;
        std::shared_ptr<LRUNode<Value,Key> > next;
        std::weak_ptr<LRUNode<Value,Key> > prev;

        public:
        LRUNode(Value val,Key key):value(val),key(key)
        {
            next=nullptr;
            prev=nullptr;
        };
        Key getKey()
        {
            return key;
        }
        Value getValue()
        {
            return value;
        }
        void setValue(Value val)
        {
            value=val;
        }
        void setKey(Key key)
        {
            key=key;
        }
    };

    template<typename Value,typename Key>
    class LRUAlgorithm: public AlgorithmStandard::Algorithmstandard<Value,Key>
    {
        private:
        int capacity=0; // 记录缓存的最大容量
        std::unordered_map<Key,std::shared_ptr<LRUNode<Value,Key> > > cache;
        LRUNode<Value,Key> dummyhead;
        LRUNode<Value,Key> dummytail;

        ~LRUAlgorithm() override=default;

        void removeNodeFromList(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            node->prev->next=node->next;
            node->next->prev=node->prev;
            node->next=nullptr;
            node->prev=nullptr;
        }
        void moveNodeToLast(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            node->next=dummytail;
            node->prev=dummytail->prev;
            dummytail->prev->next=node;
            dummytail->prev=node;
        }

        public:
        LRUAlgorithm(int capacity):capacity(capacity)
        {
            LRUNode<Value,Key> *dummyhead;
            LRUNode<Value,Key> *dummytail;
            dummyhead->next=&dummytail;
            dummytail->prev=&dummyhead;
        }
        bool get(Key key)
        {
            if (cache.find(key)!=cache.end())
            {
                removeNodeFromList(cache[key].second);
                moveNodeToLast(cache[key].second);
                return true;
            }
            else
            {
                return false;
            }
        }
    };
}