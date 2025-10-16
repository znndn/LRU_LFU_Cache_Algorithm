#pragma once

#include <climits>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "AlgorithmStandard.h"

namespace LFU
{

    template <typename Key,typename Value>
    struct Node
    {
        Key key;
        Value value;
        int NodeFrequency; // 表示节点访问频率
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;

        Node():NodeFrequency(1),next(nullptr),prev(nullptr){};
        Node(Key key,Value value):key(key),value(value),NodeFrequency(1),next(nullptr),prev(nullptr){};
    };

    template <typename Key,typename Value>
    class FreqList
    {
        private:
        int ListFrequency; // 表示这条链表所对应的访问频率
        std::shared_ptr<Node<Key,Value>> head;
        std::weak_ptr<Node<Key,Value>> tail;
        // 不需要专门的capacityUsage，检查KeyValue索引的HashMap的大小即可

        public:
        FreqList(int freq)
        {
            ListFrequency=freq;
            head=std::make_shared<Node<Key,Value>>();
            tail=std::make_shared<Node<Key,Value>>();
            head->next=tail;
            if (auto tailPrevPtr=tail->prev.lock())
            {
                tailPrevPtr=head;
            }
        }

        bool isEmpty()
        {
            if (head->next==tail)
            {
                return true;
            }
            return false;
        }
        void addNodeToCurrTail(std::shared_ptr<Node<Key,Value>> node)
        {
            node->next=tail;
            node->prev=tail->prev;
            if (auto tailPrevPtr=tail->prev.lock())
            {
                tailPrevPtr->next=node;
                tailPrevPtr=node;
            }
        }
        void removeNodeFromCurrList(std::shared_ptr<Node<Key,Value> > node)
        {
            node->next->prev=node->prev;
            if (auto node_prev_sharedptr=node->prev.lock())
                node_prev_sharedptr->next=node->next;
            node->next=nullptr;
            node->prev.reset();
        }
        std::shared_ptr<Node<Key,Value>> getFirstNode()
        {
            if (isEmpty()==false)
            {
                return head->next;
            }
        }
    };

    template <typename Key,typename Value>
    class LFUAlgorithm:public AlgorithmStandard::Algorithmstandard<Value,Key>
    // 小心模版参数的历史遗留问题
    {
        private:
        // 索引出现次数对应的双向链表
        std::unordered_map<Key,FreqList<Key,Value>> FreqToNode;
        // 用来查找某个key是否存在以及对应的node在哪的主索引
        std::unordered_map<Key,std::shared_ptr<Node<Key,Value>>> cache;
        int minFrequency;
        std::mutex mutex;

        const int MAX_AVR_NUM=1000000;
        int currentAverageNumber;
        int currentTotalNumber;
        // 当平均值大于最大平均值限制时将所有结点的访问次数减去最大平均值限制的一半或者一个固定值。
        // 相当于热点数据“老化”了，这样可以避免频次计数溢出，也可以缓解缓存污染。

        public:
        LFUAlgorithm():minFrequency(INT_MAX),currentAverageNumber(0),currentTotalNumber(0)
        {
            FreqToNode=std::make_shared<Node<Key,Value>>();
            cache=std::make_shared<Node<Key,Value>>();
        }
        ~LFUAlgorithm()
        {
            for (auto node:FreqToNode)
            {
                delete(node);
            }
        }

        void DeleteOldNode()
        {

        }
        void NodeFreqUpgrade(std::shared_ptr<Node<Key,Value>> node)
        {
            FreqList<Key,Value>::removeNode(node);
            node->NodeFrequency++;
            FreqList<Key,Value>::addNode(node);
        }
        void NewNodeInsert(Key key,Value value)
        {
            auto NewNode=std::make_shared<Node<Key,Value>>();
            Node(key,value);
            if (cache.size()<CACHE_CAPACITY)
            {
                cache[key]=NewNode;

            }
        }
        bool get(const Key& key, Value& value) override
        {
            std::lock_guard<std::mutex> lock(mutex);
            // 迭代器可以直接使用->访问哈希表的键和值
            if (cache.find(key)!=cache.end())
            {
                NodeFreqUpgrade(FreqToNode.find(key)->second);
                return true;
            }
            return false;
        }
        void put(const Value& val,const Key& key) override
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto CacheIter=cache.find(key);
            if (CacheIter!=cache.end())
            {
                CacheIter->second->value=val;
                auto FreqToNodeIter=FreqToNode.find(key);
                NodeFreqUpgrade(FreqToNodeIter->second);
            }
            else
            {
            }
        }
    };
};
