#include "../include/class_NaiveEngine.h"
#include <iostream>

// 构造函数：初始化动态数组
NaiveEngine::NaiveEngine(int initCapacity) {
    capacity = initCapacity > 0 ? initCapacity : 10;
    size = 0;
    slots = new NaiveSlot[capacity];
}

// 析构函数：释放广告位数组及其内部为 1440 分钟分配的链表数组
NaiveEngine::~NaiveEngine() {
    if (slots != nullptr) {
        for (int i = 0; i < size; ++i) {
            // 释放 1440 个 my_list 组成的数组
            if (slots[i].timeline != nullptr) {
                delete[] slots[i].timeline;
                slots[i].timeline = nullptr;
            }
        }
        delete[] slots;
        slots = nullptr;
    }
}

// 辅助私有函数：查找或创建时间轴（带动态扩容逻辑）
my_list* NaiveEngine::getOrCreateTimeline(const std::string& slotId) {
    // 1. 遍历当前已有广告位
    for (int i = 0; i < size; ++i) {
        if (slots[i].slotId == slotId) {
            return slots[i].timeline;
        }
    }

    // 2. 若不存在且数组已满，进行动态扩容
    if (size >= capacity) {
        int newCapacity = capacity * 2;
        NaiveSlot* newSlots = new NaiveSlot[newCapacity];
        for (int i = 0; i < size; ++i) {
            newSlots[i] = slots[i];
        }
        delete[] slots;
        slots = newSlots;
        capacity = newCapacity;
    }

    // 3. 创建新广告位，并为其分配 1440 个独立的链表空间（代表全天 1440 分钟）
    slots[size].slotId = slotId;
    slots[size].timeline = new my_list[1440];

    return slots[size++].timeline;
}

// 1. 区间竞价投递（暴力遍历区间）
void NaiveEngine::updateBid(const std::string& slotId, int QL, int QR,
    const std::string& bidId, const std::string& advId, int price) {
    my_list* timeline = getOrCreateTimeline(slotId);
    BidRecord record{ bidId, advId, price };

    // 确保时间合法性边界检查
    int start = (QL < 0) ? 0 : QL;
    int end = (QR > 1439) ? 1439 : QR;

    // 逐分钟将出价记录插入对应的链表
    for (int t = start; t <= end; ++t) {
        timeline[t].insert_bid(record);
    }
}

// 2. 区间竞价撤销（暴力遍历区间）
void NaiveEngine::cancelBid(const std::string& slotId, int QL, int QR, const std::string& bidId) {
    for (int i = 0; i < size; ++i) {
        if (slots[i].slotId == slotId) {
            int start = (QL < 0) ? 0 : QL;
            int end = (QR > 1439) ? 1439 : QR;

            // 逐分钟在对应链表中查找并删除该竞价
            for (int t = start; t <= end; ++t) {
                slots[i].timeline[t].remove_bid(bidId);
            }
            return;
        }
    }
}

// 3. 单点查询
QueryResult NaiveEngine::query_point(const std::string slotId, int time) const {
    QueryResult res;
    for (int i = 0; i < size; ++i) {
        if (slots[i].slotId == slotId) {
            BidRecord topBid;
            // 直接获取该分钟链表头部的最高有效出价
            if (slots[i].timeline[time].first_bid(topBid)) {
                res.price = topBid.price;
                res.advertiserId = topBid.advertiserId;
                res.bidId = topBid.bidId;
            }
            break;
        }
    }
    return res;
}
// 4. 区间查询
QueryResult NaiveEngine::query_range(const std::string& slotId, int QL, int QR) const {
    QueryResult res;
    int maxPrice = 0;
    for (int i = 0; i < size; ++i) {
        if (slots[i].slotId == slotId) {
            int start = (QL < 0) ? 0 : QL;
            int end = (QR > 1439) ? 1439 : QR;

            // 逐分钟查询并汇总全区间的最高历史
            for (int t = start; t <= end; ++t) {
                BidRecord topBid;
                if (slots[i].timeline[t].first_bid(topBid)) {
                    if (topBid.price > res.price || (topBid.price == res.price && topBid.bidId < res.bidId)) {
                        res.price = topBid.price;
                        res.advertiserId = topBid.advertiserId;
                        res.bidId = topBid.bidId;
                    }
                }
            }
            return res;
        }
    }
    return res; // 暂无竞价返回 0
}
