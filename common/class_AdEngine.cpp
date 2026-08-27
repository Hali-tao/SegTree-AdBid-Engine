#include "../include/class_AdEngine.h"
#include <iostream>

// 构造函数：初始化动态数组容量及大小
AdEngine::AdEngine(int initCapacity) {
    capacity = initCapacity > 0 ? initCapacity : 10;
    size = 0;
    slots = new AdSlot[capacity];

    // 初始化订单表
    bidRegistry = new bid[MAX_BID_LIMIT];
}

// 析构函数：释放所有手工构建的线段树内存，防止内存泄漏
AdEngine::~AdEngine() {
    if (slots != nullptr) {
        for (int i = 0; i < size; ++i) {
            if (slots[i].biddedTree != nullptr) {
                delete slots[i].biddedTree;
                slots[i].biddedTree = nullptr;
            }
        }
        delete[] slots;
        slots = nullptr;
    }
    if (bidRegistry != nullptr) {
        delete[] bidRegistry;
    }
}

// 辅助私有函数：查找或创建线段树（带动态扩容逻辑）
segment_tree* AdEngine::getOrCreateTree(const std::string& slotId) {
    // 1. 遍历当前已有的广告位
    for (int i = 0; i < size; ++i) {
        if (slots[i].slotId == slotId) {
            return slots[i].biddedTree;
        }
    }

    // 2. 若不存在且数组已满，进行动态扩容（体现高质量工程系统化思维）
    if (size >= capacity) {
        int newCapacity = capacity * 2;
        AdSlot* newSlots = new AdSlot[newCapacity];
        for (int i = 0; i < size; ++i) {
            newSlots[i] = slots[i];
        }
        delete[] slots;
        slots = newSlots;
        capacity = newCapacity;
    }

    // 3. 创建全天 1440 分钟的黄金标准线段树
    slots[size].slotId = slotId;
    slots[size].biddedTree = new segment_tree(1440);

    return slots[size++].biddedTree;
}

// 1. 区间竞价投递
void AdEngine::updateBid(const std::string& slotId, int QL, int QR,
    const std::string& bidId, const std::string& advId, int price) {
    segment_tree* tree = getOrCreateTree(slotId);
    if (tree != nullptr) {
        BidRecord record{ bidId, advId, price };
        tree->update_bid(QL, QR, record);
    }
    int index = std::stoi(bidId.substr(4)) - START_BID_ID;
    if (index >= 0 && index < MAX_BID_LIMIT) {
        bidRegistry[index].slotId = slotId;
        bidRegistry[index].QL = QL;
        bidRegistry[index].QR = QR;
        bidRegistry[index].isValid = true;
    }
}

// 2. 区间竞价撤销
void AdEngine::cancelBid(const std::string& slotId, const std::string& bidId) {
    int index = std::stoi(bidId.substr(4)) - START_BID_ID;
    // 边界与有效性检查
    if (index < 0 || index >= MAX_BID_LIMIT || !bidRegistry[index].isValid) {
        return;
    }
    int QL = bidRegistry[index].QL;
    int QR = bidRegistry[index].QR;

    // 撤销操作首先寻找既有广告位，若不存在则无需处理
    for (int i = 0; i < size; ++i) {
        if (slots[i].slotId == slotId) {
            slots[i].biddedTree->revoke_bid(QL, QR, bidId);
            return;
        }
    }
    // 撤销完毕，该席位释放，恢复至不可用
    bidRegistry[index].isValid = false;
}

// 3. 单点赢家查询
QueryResult AdEngine::query_point(const std::string& slotId, int time) const {
    QueryResult res;
    for (int i = 0; i < size; i++) {
        if (slots[i].slotId == slotId) {
            res =  slots[i].biddedTree->query_bid(time, time);
        }
    }
    return res;
}

// 4. 独占区间霸屏保鲜价查询
QueryResult AdEngine::query_range(const std::string& slotId, int QL, int QR) const {
    QueryResult res;
    for (int i = 0; i < size; i++) {
        if (slots[i].slotId == slotId) {
            res = slots[i].biddedTree->query_bid(QL, QR);
        }
    }
    return res;
}

// 5. 打印指定广告位的路径溯源（辅助 Debug 演示）
void AdEngine::printSlotTrace(const std::string& slotId, int QL, int QR, const std::string& bidId, int price) {
    std::cout << "\n================== [线段树路径溯源开始] ==================\n";
    std::cout << "[Engine Log] 正在向广告位 " << slotId << " 投递区间竞价...\n";
    std::cout << "[Engine Log] 目标时段区间: [" << QL << " - " << QR << "], 出价: " << price << "\n";

    // 调用 updateBid，其底层 segment_tree::update 会实时在控制台打印被遍历的节点路径
    updateBid(slotId, QL, QR, bidId, "TraceAdvertiser", price);

    std::cout << "================== [线段树路径溯源结束] ==================\n\n";
}

// 6. 返回开始、结束时间
void AdEngine::get_time(const std::string bidId, int& QL, int& QR) const{
    int index = std::stoi(bidId.substr(4)) - START_BID_ID;
    // 边界与有效性检查
    if (index < 0 || index >= MAX_BID_LIMIT || !bidRegistry[index].isValid) {
        return;
    }
    QL = bidRegistry[index].QL;
    QR = bidRegistry[index].QR;
}

// 7. 寻找订单，同时检查有没有越界的问题
bool AdEngine::get_slotId(const std::string bidId, std::string& slotId) {
    int index = std::stoi(bidId.substr(4)) - START_BID_ID;
    // 边界与有效性检查
    if (index < 0 || index >= MAX_BID_LIMIT || !bidRegistry[index].isValid) {
        return false;
    }
    slotId = bidRegistry[index].slotId;
    return true;
}