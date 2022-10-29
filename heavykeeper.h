#ifndef _heavykeeper_H
#define _heavykeeper_H

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <functional>
#include <string>
#include <optional>
#include <assert.h>
#include <unordered_map>
#include <cstring>
#define HEAVY_KEEPER_ARRAY (2)
#define HEAVY_KEEPER_B (1.08)
#define HEAVY_KEEPER_LENGTH (3079)
#define HEAVY_KEEPER_THRESHOLD (1000)
using namespace std;

struct DLinkedNode {
    std::string key;
    int value;
    DLinkedNode* prev;
    DLinkedNode* next;
    DLinkedNode(): value(0), prev(nullptr), next(nullptr) {}
    DLinkedNode(std::string _key, int _value): key(_key), value(_value), prev(nullptr), next(nullptr) {}
};

class LRUCache {
private:
    DLinkedNode* head;
    DLinkedNode* tail;
    int size;
    int capacity;

public:
    unordered_map<std::string, DLinkedNode*> cache;

    LRUCache(int _capacity): capacity(_capacity), size(0) {
        // 使用伪头部和伪尾部节点
        head = new DLinkedNode();
        tail = new DLinkedNode();
        head->next = tail;
        tail->prev = head;
    }
    
    int Length() const {
        return size;
    }

    std::optional<int> RawGet(std::string key) {
        if (!cache.count(key)) {
            return std::nullopt;
        }
        return cache[key]->value;
    }

    void Put(std::string key, int value) {
        if (!cache.count(key)) {
            // 如果 key 不存在，创建一个新的节点
            DLinkedNode* node = new DLinkedNode(key, value);
            // 添加进哈希表
            cache[key] = node;
            // 添加至双向链表的头部
            addToHead(node);
            ++size;
            if (size > capacity) {
                // 如果超出容量，删除双向链表的尾部节点
                DLinkedNode* removed = removeTail();
                // 删除哈希表中对应的项
                cache.erase(removed->key);
                // 防止内存泄漏
                delete removed;
                --size;
            }
        }
        else {
            // 如果 key 存在，先通过哈希表定位，再修改 value，并移到头部
            DLinkedNode* node = cache[key];
            node->value = value;
            moveToHead(node);
        }
    }

private:
    void addToHead(DLinkedNode* node) {
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    
    void removeNode(DLinkedNode* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(DLinkedNode* node) {
        removeNode(node);
        addToHead(node);
    }

    DLinkedNode* removeTail() {
        DLinkedNode* node = tail->prev;
        removeNode(node);
        return node;
    }
};

class HeavyKeeper {
    private:
        LRUCache* ssummary;
        struct hkNode {
            // 可以设置成更小的结构吗
            int count;
            unsigned long long fingerprint;
        } HK[HEAVY_KEEPER_ARRAY][HEAVY_KEEPER_LENGTH];
        int topK;

        struct Node {
            string x;
            int y;
        } q[HEAVY_KEEPER_LENGTH+10];

        static int cmp(Node i,Node j) {
            return i.y > j.y;
        }
    public:
        // TODO: HEAVY_KEEPER_LENGTH的长度需要是一个质数吗
        HeavyKeeper(int K) : topK(K) { 
            ssummary = new LRUCache(K);
        }

        void Clear() {
            for (int i = 0; i < HEAVY_KEEPER_ARRAY; i++) {
                for (int j = 0; j < HEAVY_KEEPER_LENGTH; j++) {
                    HK[i][j].count = 0;
                    HK[i][j].fingerprint = 0;
                }
            }
        }

        unsigned long long Hash(string ST) {
            return std::hash<std::string>{}(ST);
        }

        void Insert(string key) {
            // step1: 检查x是否存在在min-heap中
            bool exist = false;
            // RawGet不会对LRU中的优先级做修改
            std::optional<int> key_count = ssummary->RawGet(key);
            if (key_count) {
                exist = true;
            }
            // 优化2: key在LRU中没找到，但是哈希冲撞导致FP相同，此时不减count
            int estimate = 0;
            // 用long long表示FP，减少冲突的概率
            unsigned long long FP = Hash(key);
            for (int j = 0; j < HEAVY_KEEPER_ARRAY; j++)
            {
                // 基于hash的结果计算哈希项,可是为什么这样算，为了不同的hash函数哈希项不一样吗？
                int hash_code = FP%(HEAVY_KEEPER_LENGTH-2*(j+HEAVY_KEEPER_ARRAY)+3);
                // 获取此key的count和FP，可能出现哈希碰撞
                int count = HK[j][hash_code].count;
                if (HK[j][hash_code].fingerprint == FP)
                {   
                    // 不存在证明哈希冲撞，不增加count
                    // 此key存在在哈希表中，或者count低于阈值时递增count
                    if (exist || count <= HEAVY_KEEPER_THRESHOLD) {
                        //std::cout << "x [" << x << "] exist [" << exist << "] c [" << c << "]\n";
                        HK[j][hash_code].count++;
                    }
                    estimate = max(estimate, HK[j][hash_code].count);
                } else {   // 衰减概率，当C小于零的时候执行替换
                    if (!(rand()%int(std::pow(HEAVY_KEEPER_B, HK[j][hash_code].count)))) {
                        // rand可能成为性能瓶颈
                        HK[j][hash_code].count--;
                        if (HK[j][hash_code].count <= 0) {
                            HK[j][hash_code].fingerprint = FP;
                            HK[j][hash_code].count = 1;
                            estimate = max(estimate, 1);
                        }
                    }
                }
            }

            //std::cout << "[" << x << "] -> estimate[" << estimate << "]\n";
            // estimate用于记录此次key对应的最大count，即预估值; 我们接下来判断是否插入数组
            // 等于零的时候说明既FP不相同，也没有发生FP的更替，即大象流依旧存在,此时需要更新min-heap
            // !exist时x不存在在min-heap中
            if (!exist) {
                // 如果此次key不在LRU中，且estimate的值大于HEAVY_KEEPER_THRESHOLD,就插入LRU中
                // 并淘汰最老的热key，在LRU中总是最新的热key
                if (estimate - HEAVY_KEEPER_THRESHOLD == 1) {
                    //|| ssummary->length() < topK 这里不需要添加这个条件，否则热点会出现误差
                    ssummary->Put(key, estimate);
                }
                // 优化1: estimate大于HEAVY_KEEPER_THRESHOLD但是key不在LRU不添加此key；
                //std::cout << "not exist: ssummary size[" << ssummary->length() << "] > topK[" << topK << "]\n";
            } else if (estimate > key_count) {
                //std::cout << "exist estimate[" << estimate << "] > key_count[" << key_count << "]\n";
                // 将此key放到LRU的最前面
                ssummary->Put(key, estimate);
            }
        }

        void Work() {   
            int CNT=0;
            // key数据存多份可能是一个问题
            for (const auto& item : ssummary->cache) {
                q[CNT].x = item.second->key;
                q[CNT].y = item.second->value;
                CNT++; 
            }
            sort(q, q+CNT , cmp);
        }

        pair<string ,int> Query(int k) {
            return make_pair(q[k].x,q[k].y);
        }
};

#endif