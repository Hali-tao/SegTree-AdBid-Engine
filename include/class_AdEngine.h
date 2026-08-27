#pragma once
#include "../include/class_segment_tree.h"
#include <string>

// 竞价注册表项
// 用于维护订单号与广告位、时间区间的直接映射
struct bid {
    std::string slotId;       // 广告位 ID
    int QL = 0;               // 竞价起始时间点（分钟）
    int QR = 0;               // 竞价结束时间点（分钟）
    bool isValid = false;     // 该笔竞价当前是否有效
};

// 广告位结构体
struct AdSlot {
    std::string slotId;                // 广告位唯一标识
    segment_tree* biddedTree = nullptr; // 该广告位绑定的独立线段树指针
};

class AdEngine {
private:
    AdSlot* slots;            // 动态数组，存储广告位
    int capacity;             // 数组最大容量
    int size;                 // 当前广告位数量

    bid* bidRegistry;         // 动态分配的直接寻址数组 
    const int MAX_BID_LIMIT = 10000; // 规定容量为 10000
    const int START_BID_ID = 100001; // 订单号的起始基准线

    // 辅助私有函数：根据广告位 ID 查找对应的索引，若不存在且有空间则创建新树
    segment_tree* getOrCreateTree(const std::string& slotId);

public:
    AdEngine(int initCapacity = 10);
    ~AdEngine();

    // 1. 区间竞价投递
    void updateBid(const std::string& slotId, int QL, int QR,
        const std::string& bidId, const std::string& advId, int price);

    // 2. 区间竞价撤销
    void cancelBid(const std::string& slotId, const std::string& bidId);

    // 3. 单点赢家查询
    QueryResult query_point(const std::string& slotId, int time) const;

    // 4. 独占区间霸屏保鲜价查询
    QueryResult query_range(const std::string& slotId, int QL, int QR) const;

    // 5. 打印指定广告位的路径溯源（辅助 Debug）
    void printSlotTrace(const std::string& slotId, int QL, int QR, const std::string& bidId, int price);

    // 6. 返回开始、结束时间
    void get_time(const std::string slotId, int& QL, int& QR) const;

    // 7. 寻找订单，同时检查有没有越界的问题
    bool get_slotId(const std::string bidId, std::string& slotId);
};
