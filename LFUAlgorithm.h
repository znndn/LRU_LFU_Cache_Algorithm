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
        Node():NodeFrequency(1),next(nullptr){}
        Node(Key key,Value value):key(key),value(value),NodeFrequency(1),next(nullptr){}
    };

    template <typename Key,typename Value>
    class FreqList
    {
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
            if (head==nullptr||head->next==tail)
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
            }
            tail->prev=node;
        }
        void removeNodeFromCurrList(std::shared_ptr<Node<Key,Value> > node)
        {
            if (node->next == nullptr)
            {
                return;
            }
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
        // 索引出现次数对应的双向链表,注意这里的第二个参数是指针，指向一个新的Freqlist模板类实例，指针可以提升效率
        std::unordered_map<int,FreqList<Key,Value>*> FreqToList;
        // 这里其实应该使用智能指针，裸指针太容易出bug了
        // 用来查找某个key是否存在以及对应的node在哪的主索引.
        // LFU 算法的核心是按访问频率 (Frequency) 分组。这个 map 的键必须是 int，代表访问频率。
        std::unordered_map<Key,std::shared_ptr<Node<Key,Value>>> cache;
        int minFrequency;
        std::mutex mutex;

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
            for (auto node:FreqToList)
            {
                delete node.second;
            }
        }

        void AddNodeToNewFrequencyList(std::shared_ptr<Node<Key,Value>> node)
        {
            int freq = node->NodeFrequency;
            if (FreqToList.contains(freq) == false || FreqToList[freq] == nullptr)
            {
                FreqToList[freq] = new FreqList<Key,Value>(freq);
            }
            FreqToList[freq]->addNodeToCurrTail(node);
        }
        void DeleteOldNode()
        {
            // 要考虑不存在的情况
            if (FreqToList.contains(minFrequency)==false)
            {
                UpdateMinfrequency();
                if (FreqToList.contains(minFrequency)==false) return;
                // 那就是全空直接删完了
            }
            auto list = FreqToList[minFrequency];
            if (list==nullptr || list->isEmpty()) return;
            // 说明自动创建了
            auto NodeToDelete = list->getCurrFirstNode();
            if (NodeToDelete==nullptr)
            {
                delete list;
                FreqToList.erase(minFrequency);
                UpdateMinfrequency();
                return;
            }
            list->removeNodeFromCurrList(NodeToDelete);
            cache.erase(NodeToDelete->key);
            currentTotalNumber -= NodeToDelete->NodeFrequency;
            if (cache.size() == 0)
                currentAverageNumber = 0;
            else
                currentAverageNumber = currentTotalNumber / cache.size();
        }
        void NodeFreqUpgrade(std::shared_ptr<Node<Key,Value>> node)
        {
            int OldFrequency = node->NodeFrequency;
            if (FreqToList.contains(OldFrequency))
            {
                auto oldList = FreqToList[OldFrequency];
                if (oldList!=nullptr)
                {
                   oldList->removeNodeFromCurrList(node);
                }
            }
            node->NodeFrequency+=1;
            addFrequencyCount();
            AddNodeToNewFrequencyList(node);
        }
        void NodeFreqDowngrade(std::shared_ptr<Node<Key,Value>> node)
        {
            FreqToList[node->NodeFrequency]->removeNodeFromCurrList(node);
            node->NodeFrequency-=1;
            reduceFrequencyCount();
            AddNodeToNewFrequencyList(node);
        }
        void NewNodeInsert(Key key,Value value)
        {
            if (cache.size() >= DEFAULT_CACHE_CAPACITY)
            {
                DeleteOldNode();
            }
            auto NewNode=std::make_shared<Node<Key,Value>>(key,value);
            minFrequency = 1;
            AddNodeToNewFrequencyList(NewNode);
            cache[key]=NewNode;
            addFrequencyCount();
        }
        bool get(const Key& key, Value& value) override
        {
            std::lock_guard lock(mutex);
            // 迭代器可以直接使用->访问哈希表的键和值
            if (cache.contains(key))
            {
                auto Nodeptr=cache.find(key)->second;
                // 你必须先保存旧频率，然后执行升级，最后再检查旧频率对应的列表是否为空。
                int OldFrequency=Nodeptr->NodeFrequency;
                NodeFreqUpgrade(Nodeptr);
                if (FreqToList.contains(OldFrequency))
                {
                    auto oldList=FreqToList[OldFrequency];
                    if (oldList!=nullptr && oldList->isEmpty())
                    {
                        delete oldList;
                        FreqToList.erase(OldFrequency);
                        if (OldFrequency == minFrequency)
                        {
                            UpdateMinfrequency();
                        }
                    }
                }
                value=Nodeptr->value;
                return true;
            }
            return false;
        }
        void put(const Value& val,const Key& key) override
        {
            std::lock_guard lock(mutex);
            auto CacheIter=cache.find(key);
            if (CacheIter!=cache.end())
            {
                CacheIter->second->value=val;
                int OldFrequency=CacheIter->second->NodeFrequency;
                NodeFreqUpgrade(CacheIter->second);
                if (FreqToList.contains(OldFrequency))
                {
                    auto oldList=FreqToList[OldFrequency];
                    if (oldList!=nullptr && oldList->isEmpty())
                    {
                        delete oldList;
                        FreqToList.erase(OldFrequency);

                        if (OldFrequency == minFrequency)
                        {
                            UpdateMinfrequency();
                        }
                    }
                }
            }
            else
            {
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
            for (auto map_pair : FreqToList)
            {
                if (map_pair.second != nullptr && map_pair.second->isEmpty()==false && map_pair.first < minFrequency)
                {
                    minFrequency=map_pair.first;
                }
            }

            if (minFrequency == INT_MAX)
            {
                minFrequency = 1;
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
            int ValueToReduce = currentAverageNumber / 2;
            int ReducedFrequency = 0;

            if (ValueToReduce == 0 && currentAverageNumber > 0)
            {
                ValueToReduce = 1;
            }
            else if (ValueToReduce == 0)
            {
                return;
            }

            for (auto map_pair : cache)
            {
                auto node = map_pair.second;
                int NodeOldFrequency = node->NodeFrequency;

                if (NodeOldFrequency == 1) continue;

                if (FreqToList.contains(NodeOldFrequency))
                {
                    auto oldList = FreqToList[NodeOldFrequency];
                    if (oldList != nullptr)
                    {
                        oldList->removeNodeFromCurrList(node);
                    }
                }

                if (NodeOldFrequency - ValueToReduce < 1)
                {
                    ReducedFrequency +=  NodeOldFrequency - 1;
                    node->NodeFrequency = 1;
                }
                else
                {
                    ReducedFrequency += ValueToReduce;
                    node->NodeFrequency -= ValueToReduce;
                }

                AddNodeToNewFrequencyList(node);
            }

            for (auto it = FreqToList.begin(); it != FreqToList.end();)
            {
                FreqList<Key, Value>* list = it->second;
                if (list == nullptr || list->isEmpty())
                {
                    if (list != nullptr)
                    {
                        delete list;
                    }
                    it = FreqToList.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            UpdateMinfrequency();
            currentTotalNumber -= ReducedFrequency;

            if (cache.size() == 0) currentAverageNumber = 0;
            else currentAverageNumber = currentTotalNumber / cache.size();
        }
    };
}