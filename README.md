# C++ 缓存淘汰算法实现练习

本项目是一个C++练习项目，旨在复现两种核心的缓存淘汰算法：LRU（最近最少使用）和 LFU（最不经常使用），并为 LFU 算法实现了一个“老化”（Aging）机制。

项目通过一个统一的接口 (`AlgorithmStandard.h`) 和一个模拟“热点数据访问”的测试程序 (`TestAlgorithm.cpp`) 来对比不同算法的性能。

## 核心组件与实现

### 1. 算法标准接口 (`AlgorithmStandard.h`)

为了统一所有缓存算法的调用方式，我们定义了一个模板化的纯虚基类 `AlgorithmStandard`。

* 它提供了两个核心的纯虚函数：`virtual bool get(const Key& key, Value& value) = 0;` 和 `virtual void put(const Value& val,const Key& key)=0;`。
* 所有具体的算法类（如 `LRUAlgorithm` 和 `LFUAlgorithm`）都公有继承自这个接口，确保了它们可以被测试程序一致地调用。

### 2. LRU (最近最少使用) 算法 (`LRUAlgorithm.h`)

LRU 算法淘汰的是“最近最久没有被访问过”的数据。

* **数据结构**：
    * `std::unordered_map`：用于实现 O(1) 复杂度的键值查找。
    * **双向链表**：用于维护所有数据的访问顺序。链表头部是“最久未访问”的，尾部是“最近刚访问”的。
* **实现细节**：
    * `get` / `put` 命中：当一个节点被访问时，它会从链表的当前位置被移除，然后重新添加到**链表尾部**。
    * `put` 未命中（缓存已满）：
        1.  淘汰链表**头部**（`dummyhead->next`）的节点（即最久未访问的节点）。
        2.  创建新节点，并将其添加到**链表尾部**。
* **内存管理**：
    * 此实现使用了现代C++的**智能指针**。
    * `std::shared_ptr` 用于构成链表的 `next` 指针和 `unordered_map` 对节点的引用。
    * `std::weak_ptr` 用于链表的 `prev` 指针，以打破 `shared_ptr` 的循环引用，确保节点可以在没有外部引用的情况下被自动销毁。

### 3. LFU (最不经常使用) 与 LFU-Aging 算法 (`LFUAlgorithm.h`)

LFU 算法淘汰的是“访问频率最低”的数据。

* **数据结构**：
    * `cache` (Map 1)：`std::unordered_map<Key, std::shared_ptr<Node<Key,Value>>>`，用于 O(1) 的键值查找。
    * `FreqToList` (Map 2)：`std::unordered_map<int, FreqList<Key,Value>*>`，这是 LFU 的核心。它将一个**访问频率 (int)** 映射到一个**存储着所有该频率节点的双向链表** (`FreqList*`)。
* **实现细节**：
    * `get` / `put` 命中：
        1.  节点的访问频率（`NodeFrequency`）+1。
        2.  节点从旧频率对应的链表（例如 `FreqToList[5]`）中移除。
        3.  节点被添加到新频率对应的链表（例如 `FreqToList[6]`）中。
    * 淘汰：
        1.  通过 `minFrequency` 变量追踪全局最小频率。
        2.  从 `FreqToList[minFrequency]` 对应的链表中移除第一个节点（即该频率下最久未访问的节点）。
* **LFU-Aging (老化机制)**：
    * **目的**：解决传统 LFU 中，旧的热点数据（访问频率极高）可能永远不会被淘汰，“污染”缓存的问题。
    * **实现**：
        1.  通过一个 `threshold`（阈值）变量来控制。
        2.  每次访问（`addFrequencyCount`）后，都会重新计算 `currentAverageNumber`（当前平均访问频率）。
        3.  如果 `currentAverageNumber > threshold`，则触发 `ReduceAllNodeFrequency` 函数。
        4.  此函数会遍历 `cache` 中的**所有节点**，将其 `NodeFrequency` 按比例降低（例如减去平均值的一半），从而使旧热点数据“降频”，有机会被新数据淘汰。
* **内存管理 (C++练习的重点)**：
    * 此实现故意使用了**原始指针 (`FreqList*`)** 来管理频率链表。
    * 这导致需要手动进行内存管理（`new FreqList` 和 `delete list`），是本项目中**非法内存访问**和**内存泄漏** bug 的主要来源。
    * 它暴露了在使用原始指针时，必须极其小心地处理 `nullptr` 检查、use-after-free（释放后使用）和指针生命周期管理等问题。

## 测试场景

本项目的测试程序模拟了一个经典的“热点数据访问”负载。

1.  **初始化**：同时创建 LRU、LFU（无衰减）和 LFU-Aging（有衰减阈值）三个算法实例。
2.  **缓存预热**：首先循环 `put` 固定的 `HOTKEY` 数量（例如20个）的数据，填满初始缓存。
3.  **模拟访问**：
    * 执行 `OPERATIONS` 次（例如500,000次）操作。
    * 每次操作有 30% 概率是 `put`（写），70% 概率是 `get`（读）。
    * 每次访问有 70% 概率访问**热点数据**（`HotKeyGen`），30% 概率访问**冷数据**（`ColdKeyGen`）。
4.  **结果统计**：
    * 在 `get` 操作时，如果返回 `true`，则对应算法的 `hits`（命中数）+1。
    * 循环结束后，调用 `printResult` 函数，计算并打印每种算法在热点数据场景下的总命中率。