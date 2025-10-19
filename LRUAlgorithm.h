#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
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
            prev.reset();
        };
        Key getKey() const
        {
            return key;
        }
        Value getValue() const
        {
            return value;
        }
        // const 成员函数的用法（放在函数声明末尾），表示该函数的所有操作只读
        void setValue(Value val)
        {
            value=val;
        }
        void setKey(Key key)
        {
            this->key=key;
        }
    };

    template<typename Value,typename Key>
    class LRUAlgorithm: public AlgorithmStandard::Algorithmstandard<Value,Key>
    {
    private:
        int capacity=DEFAULT_CACHE_CAPACITY; // 记录缓存的最大容量
        std::unordered_map<Key,std::shared_ptr<LRUNode<Value,Key> > > cache;
        // 对于unordered_map的operator[key]，如果找到会返回value（返回某变量的引用=返回某变量本身）。
        std::shared_ptr<LRUNode<Value,Key>> dummyhead;
        std::shared_ptr<LRUNode<Value,Key>> dummytail;
        std::mutex mutex;

        void addNodeToLast(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            if (auto dummytail_prev_shareptr=dummytail->prev.lock())
            {
                dummytail_prev_shareptr->next=node;
            }
            node->prev=dummytail->prev;
            node->next=dummytail;
            dummytail->prev=node;
        }

        void removeNode(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            node->next->prev=node->prev;
            if (auto node_prev_sharedptr=node->prev.lock())
                node_prev_sharedptr->next=node->next;
            node->next=nullptr;
            node->prev.reset();
        }

    public:
        ~LRUAlgorithm() override=default;

        explicit LRUAlgorithm(int capacity):capacity(capacity)
        {
            dummyhead=std::make_shared<LRUNode<Value,Key> >(Value{}, Key{});
            dummytail=std::make_shared<LRUNode<Value,Key> >(Value{}, Key{});
            // 这是一个函数，调用它并希望返回变量需要加括号，同时需要给出默认的value和key（LRUNode模版要求）
            // 使用makeshare而非new来构造sharedpointer更加合适
            dummyhead->next=dummytail;
            dummytail->prev=dummyhead;
            // 这里一定要记得先为dummytail和dummyhead分配内存，直接就后两行就是空指针
        }
        // 注意每次调用都会更新上次访问历史记录

        bool get(const Key& key, Value& value) override
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto iter=cache.find(key);
            if (iter!=cache.end())
            {
                auto node_ptr=cache[key];
                value=node_ptr->getValue();
                removeNode(node_ptr);
                addNodeToLast(node_ptr);
                return true;
            }
            else  return false;
        }

        void put(const Value& val,const Key& key) override
        {
            std::lock_guard lock(mutex);
            auto iter=cache.find(key);
            if (iter!=cache.end())
            {
                // 记得更新value值（刚被访问）
                auto node=cache[key];
                node->setValue(val);
                removeNode(cache[key]);
                addNodeToLast(cache[key]);
            }
            else
            {
                if (capacity<=cache.size())
                {
                    auto NodeToDelete=dummyhead->next;
                    cache.erase(NodeToDelete->getKey());
                    removeNode(NodeToDelete);
                    auto newNode=std::make_shared<LRUNode<Value,Key> >(val,key);
                    // 不要使用new关键字，使用make_shared确保创建在堆上（手动，否则会被直接释放）
                    cache[key]=newNode;
                    // 不需要使用insert，因为cache[key]无法找到时会自动创建一个新的键值对（0和nullptr）
                    // 找到时会返回std::shared_ptr的引用
                    addNodeToLast(newNode);
                }
                else
                {
                    auto NewNode=std::make_shared<LRUNode<Value,Key> >(val,key);
                    cache[key]=NewNode;
                    addNodeToLast(NewNode);
                }
            }
        }
    };
}