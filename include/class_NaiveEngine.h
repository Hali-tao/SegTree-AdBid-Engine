#pragma once
#include "../include/class_my_list.h"
#include "../include/common_types.h"
#include <string>

// 暴力单广告位结构：用 1440 个链表组成的数组
struct NaiveSlot {
    std::string slotId;
    my_list* timeline = nullptr; // 1440长度的数组，每个元素是一个链表
};

class NaiveEngine {
private:
    NaiveSlot* slots;
    int capacity;
    int size;

    my_list* getOrCreateTimeline(const std::string& slotId);

public:
    NaiveEngine(int initCapacity = 10);
    ~NaiveEngine();

    // 更新操作
    void updateBid(const std::string& slotId, int QL, int QR, const std::string& bidId, const std::string& advId, int price);
    // 删除操作
    void cancelBid(const std::string& slotId, int QL, int QR, const std::string& bidId);
    // 单点查询
    QueryResult query_point(const std::string slotId, int time) const;
    // 区间查询
    QueryResult query_range(const std::string& slotId, int QL, int QR) const;

};