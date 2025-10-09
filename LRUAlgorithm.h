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
            this->key=key;
        }
    };

    template<typename Value,typename Key>
    class LRUAlgorithm: public AlgorithmStandard::Algorithmstandard<Value,Key>
    {
    private:
        int capacity=CAPACITY; // 记录缓存的最大容量
        std::unordered_map<Key,std::shared_ptr<LRUNode<Value,Key> > > cache;
        // 对于unordered_map的operator[key]，如果找到会返回value（返回某变量的引用=返回某变量本身）。
        std::shared_ptr<LRUNode<Value,Key>> dummyhead;
        std::shared_ptr<LRUNode<Value,Key>> dummytail;
        std::mutex mutex;

        void removeNodeFromList(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            // 这里牢记对于任意node的prev都是弱指针，只能通过lock方法变成sharedptr，不能直接使用
            if (auto node_prev_sharedptr=node->prev.lock())
                node_prev_sharedptr->next=node->next;
            node->next->prev=node->prev;
            // 为什么node->prev->next需要lock解锁，但是node->next->prev不用？
            // 因为c++允许weakptrA=weakptrB的操作
            // 第一行访问weakptr->next非法
            // 第二行next->prev是weakptr（左边），同时node->prev也是weakptr（右边），所以可以直接赋值
            node->next=nullptr;
            node->prev=nullptr;
        }
        void moveNodeToLast(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            if (auto node_prev_sharedptr=node->prev.lock())
            {
                node_prev_sharedptr->next=node->next;
            }
            node->next->prev=node->prev;
            node->next=dummytail;
            node->prev=dummytail->prev;
            if (auto dummytail_prev_sharedptr=dummytail->prev.lock())
            {
                dummytail_prev_sharedptr->next=node;
                dummytail_prev_sharedptr=node;
            }
        }

        void removeNode(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            node->next->prev=node->prev;
            if (auto node_prev_sharedptr=node->prev.lock())
                node_prev_sharedptr->next=node->next;
            node->next=nullptr;
            node->prev=nullptr;
            node=nullptr;
        }

    public:
        ~LRUAlgorithm() override=default;

        LRUAlgorithm(int capacity):capacity(capacity)
        {
            dummyhead=std::make_shared<LRUNode<Value,Key> >;
            dummytail=std::make_shared<LRUNode<Value,Key> >;
            // 使用makeshare而非new来构造sharedpointer更加合适
            dummyhead->next=dummytail;
            dummytail->prev=dummyhead;
            // 这里一定要记得先为dummytail和dummyhead分配内存，直接就后两行就是空指针
        }
        // 注意每次调用都会更新上次访问历史记录
        Value get(const Key& key) override
        {
            if (cache.find(key)!=cache.end())
            {
                // HashMap不能访问特定下标，但是可以访问特定的键
                removeNodeFromList(cache[key]);
                // unordered_map[key]返回的是key对应的value的引用（也就是value本身），无法使用.first,.second等
                moveNodeToLast(cache[key]);
                return cache[key]->getValue();
            }
            else
            {
                return Value{};
                // 未命中，返回一个默认构造的 Value
            }
            // 记住对于unordered_map，使用MapName[Key]访问不存在的key会自动创建
            // 这里需要考虑没有找到的情况，不能直接返回对应的key，否则可能会创建新的键值对
        }
        bool get(const Value& val,const Key& key) override
        {
            if (cache.find(key)!=cache.end())
            {
                removeNodeFromList(cache[key]);
                moveNodeToLast(cache[key]);
                return true;
            }
            return false;
        }
        void put(const Value& val,const Key& key) override
        {
            std::lock_guard lock(mutex);
            if (cache.find(key)!=cache.end())
            {
                // 记得更新value值（刚被访问）
                auto node=cache[key];
                node->setValue(val);
                removeNodeFromList(cache[key]);
                moveNodeToLast(cache[key]);
            }
            else
            {
                if (capacity<=cache.size())
                {
                    auto NodeToDelete=dummyhead->next;
                    cache.erase(NodeToDelete->getKey());
                    removeNodeFromList(NodeToDelete);
                    auto newNode=std::make_shared<LRUNode<Value,Key> >(val,key);
                    // 不要使用new关键字，使用make_shared确保创建在堆上（手动，否则会被直接释放）
                    cache[key]=newNode;
                    // 不需要使用insert，因为cache[key]无法找到时会自动创建一个新的键值对（0和nullptr）
                    // 找到时会返回std::shared_ptr的引用
                    moveNodeToLast(newNode);
                }
                else
                {
                    auto NewNode=std::make_shared<LRUNode<Value,Key> >(val,key);
                    cache[key]=NewNode;
                    moveNodeToLast(NewNode);
                }
            }
        }
    };
}