#pragma once

#include <climits>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "AlgorithmStandard.h"

namespace LFU
{
    template<typename Value,typename Key>
    class LFUAlgorithm;

    template <typename Key,typename Value>
    struct Node
    {
        Key key;
        Value value;
        int NodeFrequency; // 表示节点访问频率
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;

        // std::weakptr不能直接由nullptr构造,默认构造即为空
        Node():NodeFrequency(1),next(nullptr){};
        Node(Key key,Value value):key(key),value(value),NodeFrequency(1),next(nullptr){};
    };

    template <typename Key,typename Value>
    class FreqList
    {
        private:
        int ListFrequency; // 表示这条链表所对应的访问频率
        std::shared_ptr<Node<Key,Value>> head;
        std::shared_ptr<Node<Key,Value>> tail;
        // 不需要专门的capacityUsage，检查KeyValue索引的HashMap的大小即可
        friend class LFUAlgorithm<Key,Value>;

        public:
        explicit FreqList(const int freq)
        {
            ListFrequency=freq;
            head=std::make_shared<Node<Key,Value>>();
            tail=std::make_shared<Node<Key,Value>>();
            head->next=tail;
            tail->prev=std::weak_ptr<Node<Key,Value>>(head);
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
        std::shared_ptr<Node<Key,Value>> getCurrFirstNode()
        {
            if (isEmpty()==false)
            {
                return head->next;
            }
            return nullptr;
        }
    };

    template <typename Key,typename Value>
    class LFUAlgorithm final :public AlgorithmStandard::Algorithmstandard<Value,Key>
    // 小心模版参数的历史遗留问题
    {
        private:
        // 索引出现次数对应的双向链表,注意这里的第二个参数是指针，指向一个新的Freqlist模板类实例，指针可以提升效率
        std::unordered_map<Key,FreqList<Key,Value>*> FreqToNode;
        // 用来查找某个key是否存在以及对应的node在哪的主索引
        std::unordered_map<Key,std::shared_ptr<Node<Key,Value>>> cache;
        int minFrequency;
        std::mutex mutex;
        int capacity=DEFAULT_CACHE_CAPACITY;

        int threshold;
        int currentAverageNumber;
        int currentTotalNumber;
        // 当平均值大于最大平均值限制时将所有结点的访问次数减去最大平均值限制的一半或者一个固定值。
        // 相当于热点数据“老化”了，这样可以避免频次计数溢出，也可以缓解缓存污染。

        public:
        explicit LFUAlgorithm(const int threshold):
            minFrequency(INT_MAX),
            threshold(threshold),currentAverageNumber(0),currentTotalNumber(0)
        {
        }
        ~LFUAlgorithm() override
        {
            for (auto node:FreqToNode)
            {
                delete(node.second);
            }
        }

        void AddNodeToNewFrequencyList(std::shared_ptr<Node<Key,Value>> node)
        {
            if (FreqToNode.contains(node->NodeFrequency)==false)
            {
                FreqList<Key,Value>(node->NodeFrequency);
                FreqToNode[node->NodeFrequency]->addNodeToCurrTail(node);
            }
            else
            {
                FreqToNode[node->NodeFrequency]->addNodeToCurrTail(node);
            }
        }
        void DeleteOldNode()
        {
            FreqToNode[minFrequency]->head->next=FreqToNode[minFrequency]->head->next->next;
            FreqToNode[minFrequency]->head->next->prev=
                std::weak_ptr<Node<Key,Value>> (FreqToNode[minFrequency]->head);
            currentTotalNumber-=minFrequency;
            if (cache.size()==0) currentAverageNumber=currentTotalNumber;
            else currentAverageNumber+=currentTotalNumber/cache.size();
        }
        void NodeFreqUpgrade(std::shared_ptr<Node<Key,Value>> node)
        {
            FreqToNode[node->NodeFrequency]->removeNodeFromCurrList(node);
            node->NodeFrequency+=1;
            addFrequencyCount();
            AddNodeToNewFrequencyList(node);
        }
        void NodeFreqDowngrade(std::shared_ptr<Node<Key,Value>> node)
        {
            FreqToNode[node->NodeFrequency]->removeNodeFromCurrList(node);
            node->NodeFrequency-=1;
            reduceFrequencyCount();
            AddNodeToNewFrequencyList(node);
        }
        void NewNodeInsert(Key key,Value value)
        {
            auto NewNode=std::make_shared<Node<Key,Value>>(key,value);
            if (cache.size()<capacity)
            {
                AddNodeToNewFrequencyList(NewNode);
                addFrequencyCount();
            }
            else
            {
                DeleteOldNode();
                AddNodeToNewFrequencyList(NewNode);
                addFrequencyCount();
            }
        }
        bool get(const Key& key, Value& value) override
        {
            std::lock_guard<std::mutex> lock(mutex);
            // 迭代器可以直接使用->访问哈希表的键和值
            if (cache.contains(key))
            {
                auto Nodeptr=cache.find(key)->second;
                if (Nodeptr->NodeFrequency==minFrequency)
                {
                    minFrequency++;
                }
                NodeFreqUpgrade(Nodeptr);
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
                if (CacheIter->second->NodeFrequency==minFrequency)
                {
                    minFrequency++;
                }
                NodeFreqUpgrade(CacheIter->second);
            }
            else
            {
                minFrequency=1;
                NewNodeInsert(key,val);
            }
        }
        /*
        LFU-Aging 对于LFU的改进版
        LFU对于新数据不友好，因为新插入的数据访问次数count=1,放在尾部
        如果需要淘汰就会将新数据淘汰。
        所以LFU-Aging加入了平均访问次数的概念
        如果节点的平均访问次数大于某个固定值x时，则将所有节点的count值减去x/2
        这样可解决“缓存污染”
        */
        void UpdateMinfrequency()
        {
            minFrequency=INT_MAX;
            for (auto map:FreqToNode)
            {
                if (map.second->isEmpty()==false && map.first<minFrequency)
                {
                    minFrequency=map.first;
                }
            }
        }
        void addFrequencyCount()
        {
            currentTotalNumber++;
            if (cache.size()==0) currentAverageNumber=1;
            else currentAverageNumber=currentTotalNumber/cache.size();
            if (currentAverageNumber>threshold) ReduceAllNodeFrequency();
        }
        void reduceFrequencyCount()
        {
            currentTotalNumber--;
            if (cache.size()==0) currentAverageNumber=currentTotalNumber;
            else currentAverageNumber+=currentTotalNumber/cache.size();
        }
        void ReduceAllNodeFrequency()
        {
            int ValueToReduce=currentAverageNumber/2;
            for (auto map:cache)
            {
                int NodeFrequency=map.second->NodeFrequency;
                if (NodeFrequency==1) continue;
                FreqToNode[map.first]->removeNodeFromCurrList(map.second);
                if (NodeFrequency-ValueToReduce<1)
                {
                    map.second->NodeFrequency=1;
                }
                else map.second->NodeFrequency-=ValueToReduce;
                FreqToNode[map.first]->addNodeToCurrTail(map.second);
            }
            UpdateMinfrequency();
        }
    };
};
