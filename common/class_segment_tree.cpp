#include "../include/class_segment_tree.h"
#include <iostream>
#include <sstream>

// ==================== segment_tree 实现 ====================
static int lc(int node) { return node << 1; }
static int rc(int node) { return node << 1 | 1; }

// 构造函数实现
segment_tree::segment_tree(int leaves) : totalLeaves(leaves) {
    build(1, 0, totalLeaves - 1); // 1号节点作为整棵树的根
}

// 自底向上汇总最大值
void segment_tree::pushUp(int node) {
    // 1. 首先将本地最高出价包装为 QueryResult
    QueryResult bestRes(tree[node].localMaxPrice, tree[node].localBidId, tree[node].localWinnerId);

    // 2. 若不是叶子节点，则结合左右子树的汇总最大值进行三方比较
    if (tree[node].l < tree[node].r) {
        int leftChild = lc(node);
        int rightChild = rc(node);

        QueryResult leftRes(tree[leftChild].treeMaxPrice, tree[leftChild].treeBidId, tree[leftChild].treeWinnerId);
        QueryResult rightRes(tree[rightChild].treeMaxPrice, tree[rightChild].treeBidId, tree[rightChild].treeWinnerId);

        if (leftRes > bestRes) {
            bestRes = leftRes;
        }
        if (rightRes > bestRes) {
            bestRes = rightRes;
        }
    }

    // 3. 将最终获胜的三个字段同步到树的 tree 属性中
    tree[node].treeMaxPrice = bestRes.price;
    tree[node].treeBidId = bestRes.bidId;
    tree[node].treeWinnerId = bestRes.advertiserId;
}

// 递归构建静态线段树
void segment_tree::build(int node, int l, int r) {
    tree[node].l = l;
    tree[node].r = r;
    tree[node].bids.clear();
    tree[node].localMaxPrice = 0;
    tree[node].localBidId = "";
    tree[node].localWinnerId = "";
    tree[node].treeMaxPrice = 0;
    tree[node].treeBidId = "";
    tree[node].treeWinnerId = "";

    if (l == r) {
        return; // 叶子节点
    }
    int mid = l + (r - l) / 2;
    build(lc(node), l, mid);
    build(rc(node), mid + 1, r);
}

// 内部计算函数

// 区间竞价更新（内部递归）
void segment_tree::update(int node, int L, int R, const BidRecord& record) {
    int l = tree[node].l;
    int r = tree[node].r;

    if (l >= L && r <= R) {
        
        // 插入手写链表
        tree[node].bids.insert_bid(record);

        // 刷新本地最大值（链表头部应是当前本地最优的 BidRecord）
        BidRecord topBid;
        if (tree[node].bids.first_bid(topBid)) {
            tree[node].localMaxPrice = topBid.price;
            tree[node].localBidId = topBid.bidId;
            tree[node].localWinnerId = topBid.advertiserId;
        }
        pushUp(node);
        return;
    }

    int mid = l + (r - l) / 2;
    if (L <= mid) {
        update(lc(node), L, R, record);
    }
    if (R > mid) {
        update(rc(node), L, R, record);
    }
    pushUp(node); // 回溯更新
}

// 区间/单点查询（内部递归，携带祖先最大值，完全适配三字段 QueryResult）
QueryResult segment_tree::query(int node, int L, int R, int ancestorMax, const std::string& ancestorBid, const std::string& ancestorWinner) {
    int l = tree[node].l;
    int r = tree[node].r;

    // 将传递的祖先状态和当前节点状态包装为 QueryResult 对象
    QueryResult currAncestor(ancestorMax, ancestorBid, ancestorWinner);
    QueryResult nodeLocal(tree[node].localMaxPrice, tree[node].localBidId, tree[node].localWinnerId);

    // 1. 更新沿途路径上的祖先最大值
    if (nodeLocal > currAncestor) {
        currAncestor = nodeLocal;
    }

    // 2. 命中目标区间
    if (l >= L && r <= R) {
        QueryResult treeMax(tree[node].treeMaxPrice, tree[node].treeBidId, tree[node].treeWinnerId);

        // 标记永久化核心逻辑：祖先最大值与当前子树汇总值的较大者
        return (treeMax > currAncestor) ? treeMax : currAncestor;
    }

    int mid = l + (r - l) / 2;
    QueryResult res;

    bool hasLeft = (L <= mid);
    bool hasRight = (R > mid);

    // 3. 递归查询子树，传递更新后的 currAncestor 的三个属性
    if (hasLeft && hasRight) {
        QueryResult leftRes = query(lc(node), L, R, currAncestor.price, currAncestor.bidId, currAncestor.advertiserId);
        QueryResult rightRes = query(rc(node), L, R, currAncestor.price, currAncestor.bidId, currAncestor.advertiserId);

        // 完美的重载运算符极简合并
        res = (leftRes > rightRes) ? leftRes : rightRes;
    }
    else if (hasLeft) {
        res = query(lc(node), L, R, currAncestor.price, currAncestor.bidId, currAncestor.advertiserId);
    }
    else if (hasRight) {
        res = query(rc(node), L, R, currAncestor.price, currAncestor.bidId, currAncestor.advertiserId);
    }

    return res;
}

// 全额撤价并恢复次高（内部递归）
bool segment_tree::revoke(int node, int L, int R, const std::string& bidId) {
    int l = tree[node].l;
    int r = tree[node].r;

    if (l >= L && r <= R) {
        bool removed = tree[node].bids.remove_bid(bidId);
        if (removed) {  


            // 链表内部已处理好排序，删除旧最高价后，first_bid 会直接返回新的次高记录
            BidRecord topBid;
            if (tree[node].bids.first_bid(topBid)) {
                tree[node].localMaxPrice = topBid.price;
                tree[node].localBidId = topBid.bidId;
                tree[node].localWinnerId = topBid.advertiserId;
            }
            else {
                tree[node].localMaxPrice = 0;
                tree[node].localBidId = "";
                tree[node].localWinnerId = "";
            }
            pushUp(node);
        }
        return removed;
    }

    int mid = l + (r - l) / 2;
    bool removedInLeft = false;
    bool removedInRight = false;

    if (L <= mid) {
        removedInLeft = revoke(lc(node), L, R, bidId);
    }
    if (R > mid) {
        removedInRight = revoke(rc(node), L, R, bidId);
    }
    pushUp(node);
    return removedInLeft || removedInRight;
}

// 外部接口函数

// 区间竞价更新（外部接口）
void segment_tree::update_bid(int L, int R, const BidRecord& record) {
    update(1, L, R, record);
}

// 区间/单点查询（外部接口）
QueryResult segment_tree::query_bid(int L, int R) {
    return query(1, L, R, 0, "", "");
}

// 全额撤价并恢复次高（外部接口）
bool segment_tree::revoke_bid(int L, int R, const std::string& bidId) {
    return revoke(1, L, R, bidId);
}


// 路径打印
void trace_internal(int node, int l, int r, int L, int R, std::stringstream& ss) {
    // 1. 记录当前访问节点所代表的实际时间区间
    ss << " visit [" << l << "-" << r << "]";

    // 2. 基准情形：目标区间完全覆盖当前节点区间（命中终止递归）
    if (l >= L && r <= R) {
        ss << " -> match [" << l << "-" << r << "]\\n";
        return;
    }

    // 3. 分治情形：分裂向下
    ss << " -> split\\n"; // 换行便于前端观察路径
    int mid = l + (r - l) / 2;

    if (L <= mid) {
        trace_internal(lc(node), l, mid, L, R, ss);
    }
    if (R > mid) {
        trace_internal(rc(node), mid + 1, r, L, R, ss);
    }
}

// 外部调用函数
std::string get_trace(int L, int R) {
    std::stringstream ss;
    // 假设整棵线段树的总区间是全天 0 ~ 1439 分钟，根节点编号为 1
    trace_internal(1, 0, 1439, L, R, ss);
    return ss.str();
}
