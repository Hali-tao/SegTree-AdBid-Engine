#pragma once
#include <string>

/**
 * @brief 竞价记录结构体
 * 存储广告主在某一时间段内的单次出价详细信息
 */
struct BidRecord {
	std::string bidId;          // 竞价记录的唯一标识 ID（用于全额撤价时精准定位）
	std::string advertiserId;   // 广告主唯一标识 ID
	int price = 0;              // 广告主的出价金额
};

/**
 * @brief 链表节点结构体
 * 用于构建手工链表，存储线段树节点内部的广告主出价历史
 */
struct ListNode {
	BidRecord data;				// 节点存储的竞价记录数据
	ListNode* next;				// 指向下一个节点的指针

	/**
	 * @brief 构造函数
	 * @param record 初始化的竞价记录数据
	 */
	ListNode(const BidRecord& record) : data(record), next(nullptr) {}
};


/**
 * @brief 自定义单向链表类 (my_list)
 * 1. 严格遵循“纯手工构建底层核心数据结构”原则，严禁调用 STL 或第三方集合库。
 * 2. 存储于线段树的每个节点中，用于维护当前区间的“胜出广告主 ID 链表”及历史出价。
 * 3. 方便在“全额撤价”时，通过遍历或维护链表来快速恢复次高有效出价 。
 */
class my_list {
private:
	ListNode* head;				// 链表头节点指针
	ListNode* tail;				// 链表尾节点指针
	int size;					// 链表当前包含的有效节点数量

public:
	my_list();
	~my_list();

	// 基础状态接口
	int getSize() const;
	bool isEmpty() const;
	ListNode* getHead() const;
	ListNode* getTail() const;

	void clear();
	
	// 功能接口
	void insert_bid(const BidRecord& record);
	bool remove_bid(const std::string& bidId);
	bool first_bid(BidRecord& outRecord) const;

	// 调试接口
	void print_list() const;
};