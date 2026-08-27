#pragma once

#include <string>

// 查询结果结构体
struct QueryResult {
    int price;                  // 当前查询区间的最高出价
    std::string bidId;          // 对应最高出价的竞价记录 ID
    std::string advertiserId;   // 当前胜出的广告主 ID
    QueryResult(int p = 0, std::string bid = "", std::string aid = "")
        : price(p), bidId(bid), advertiserId(aid) {}
    // 重载 > 运算符，方便进行结果的大小比较（价格优先；价格相同时可自定义 ID 排序逻辑）
    bool operator>(const QueryResult& other) const {
        if (price != other.price) {
            return price > other.price;
        }
        return bidId < other.bidId;
    }
};