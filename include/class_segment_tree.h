#pragma once
#include "../include/class_my_list.h"
#include "../include/common_types.h"
#include <string>
#include <iostream>
#include <algorithm>

// 线段树节点
struct SegmentTreeNode {
    int l = 0;
    int r = 0;

    // 1. 本地竞价链表（仅存储【完全覆盖】当前节点区间的竞价） 
    my_list bids;
    int localMaxPrice = 0;  // 当前节点自身的最高出价（即 bids 链表头部的价格）
    std::string localBidId = "";
    std::string localWinnerId = ""; // 当前节点自身的赢家 ID

    // 2. 子树汇总属性（用于加速区间查询）
    int treeMaxPrice = 0;   // 以当前节点为根的子树中，所有子区间及自身的最大出价
    std::string treeBidId = "";
    std::string treeWinnerId = "";  // 对应 treeMaxPrice 的赢家 ID
};

// 线段树类声明
class segment_tree {
private:
    static const int MAX_NODES = 5760; // 4 * 1440 的静态数组大小
    SegmentTreeNode tree[MAX_NODES];
    int totalLeaves;

    // 私有辅助递归函数
    void pushUp(int node);
    void build(int node, int l, int r);
    void update(int node, int L, int R, const BidRecord& record);
    QueryResult query(int node, int L, int R, int ancestorMax, const std::string& ancestorBid, const std::string& ancestorWinner);
    bool revoke(int node, int L, int R, const std::string& bidId);
public:
    // 构造函数
    segment_tree(int leaves = 1440);

    // 外部公共接口
    void update_bid(int L, int R, const BidRecord& record);
    QueryResult query_bid(int L, int R);
    bool revoke_bid(int L, int R, const std::string& bidId);
};

// 外部打印接口，用于返回遍历的路径
void trace_internal(int node, int l, int r, int L, int R, std::stringstream& ss);
std::string get_trace(int L, int R);