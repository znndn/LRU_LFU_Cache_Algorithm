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
        int capacity=0; // 记录缓存的最大容量
        std::unordered_map<Key,std::shared_ptr<LRUNode<Value,Key> > > cache;
        LRUNode<Value,Key> dummyhead;
        LRUNode<Value,Key> dummytail;
        std::mutex mutex;

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
        void removeNode(std::shared_ptr<LRUNode<Value,Key> > node)
        {
            node->next->prev=node->prev;
            node->prev->next=node->next;
            node->next=nullptr;
            node->prev=nullptr;
            node=nullptr;
        }

    public:
        LRUAlgorithm(int capacity):capacity(capacity)
        {
            LRUNode<Value,Key> *dummyhead;
            LRUNode<Value,Key> *dummytail;
            dummyhead->next=&dummytail;
            dummytail->prev=&dummyhead;
        }
        // 注意每次调用都会更新上次访问历史记录
        Value get(Key key) override
        {
            if (cache.find(key)!=cache.end())
            {
                // HashMap不能访问特定下标，但是可以访问特定的键
                removeNodeFromList(cache[key].second);
                moveNodeToLast(cache[key].second);
            }
            return cache[key].second;
        }
        bool get(Value val,Key key) override
        {
            if (cache.find(key)!=cache.end())
            {
                removeNodeFromList(cache[key].second);
                moveNodeToLast(cache[key].second);
                return true;
            }
            return false;
        }
        void put(Key key,Value val) override
        {
            std::lock_guard lock(mutex);
            if (cache.find(key)!=cache.end())
            {
                removeNodeFromList(cache[key].second);
                moveNodeToLast(cache[key].second);
            }
            else
            {
                if (capacity<=cache.size())
                {
                    Key toDeleteKey=dummyhead.next->getKey();
                    removeNodeFromList(cache[toDeleteKey]);
                    cache.erase(toDeleteKey);
                    LRUNode<Value,Key> newNode = LRUNode<Value,Key>(val,key);
                    std::shared_ptr<LRUNode<Value,Key> > *newNodePtr=&newNode;
                    cache.insert(std::pair<Key,std::shared_ptr<LRUNode<Value,Key> > >(key,newNode));
                    moveNodeToLast(newNode);
                }
                else
                {
                    LRUNode<Value,Key> newNode = LRUNode<Value,Key>(val,key);
                    std::shared_ptr<LRUNode<Value,Key> > *newNodePtr=&newNode;
                    cache.insert(std::pair<Key,std::shared_ptr<LRUNode<Value,Key> > >(key,newNode));
                    moveNodeToLast(newNode);
                }
            }
        }
    };
}