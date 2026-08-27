#include "../include/class_my_list.h"
#include <iostream>

my_list::my_list() : head(nullptr), tail(nullptr), size(0){}

my_list::~my_list() {
	clear();
}

int my_list::getSize() const{
	return size;
}

bool my_list::isEmpty() const {
	return size == 0;
}

ListNode* my_list::getHead() const {
	return head;
}

ListNode* my_list::getTail() const {
	return tail;
}

void my_list::clear() {
    ListNode* curr = head;
    while (curr != nullptr) {
        ListNode* nextNode = curr->next;
        delete curr;
        curr = nextNode;
    }
    head = nullptr;
    tail = nullptr;
    size = 0;
}


/***************************************************************************
  函数名称：insert_bid
  功    能：向链表中插入一条新的竞价记录（保持出价从大到小降序排列）
  输入参数：const BidRecord& record - 待插入的竞价记录数据
  返 回 值：无
  说    明：采用降序插入策略。头节点永远是当前区间的最高出价 。
            分为三种情况：1.链表为空；2.新出价最高插入头部；3.遍历寻找合适位置插入。
 ***************************************************************************/
void my_list::insert_bid(const BidRecord& record) {
    ListNode* newNode = new ListNode(record);

    // 情况 1：链表为空，直接作为头尾
    if (head == nullptr) {
        head = tail = newNode;
        size++;
        return;
    }

    // 情况 2：新出价大于当前最高价，插入到头部
    if (record.price > head->data.price) {
        newNode->next = head;
        head = newNode;
        size++;
        return;
    }

    // 情况 3：遍历链表，寻找合适的插入位置（降序）
    ListNode* curr = head;
    while (curr->next != nullptr && curr->next->data.price >= record.price) {
        curr = curr->next;
    }

    // 插入到 curr 之后
    newNode->next = curr->next;
    curr->next = newNode;

    // 如果插入到了最后，更新 tail 指针
    if (newNode->next == nullptr) {
        tail = newNode;
    }
    size++;
}


/***************************************************************************
  函数名称：remove_bid
  功    能：全额撤价——根据竞价 ID 从链表中删除对应的出价记录
  输入参数：const std::string& bidId - 准备撤销的竞价唯一标识 ID
  返 回 值：bool - true 表示撤销成功，false 表示未找到该 ID 对应的记录
  说    明：对应题目“全额撤价”需求。由于链表是有序的，如果删除了头节点（最高价），
            链表的新头部会自动变成“之前的次高有效出价”，从而完美配合线段树的数据恢复。
 ***************************************************************************/
bool my_list::remove_bid(const std::string& bidId) {
    if (head == nullptr) return false;

    // 情况 1：要删除的是头节点
    if (head->data.bidId == bidId) {
        ListNode* temp = head;
        head = head->next;
        if (head == nullptr) {
            tail = nullptr; // 链表删空了
        }
        delete temp;
        size--;
        return true;
    }

    // 情况 2：遍历查找后续节点
    ListNode* curr = head;
    while (curr->next != nullptr && curr->next->data.bidId != bidId) {
        curr = curr->next;
    }

    // 找到了目标节点 curr->next
    if (curr->next != nullptr) {
        ListNode* temp = curr->next;
        curr->next = temp->next;

        // 如果删除了尾节点，更新 tail
        if (curr->next == nullptr) {
            tail = curr;
        }
        delete temp;
        size--;
        return true;
    }

    return false; // 没找到该 bidId
}


/***************************************************************************
  函数名称：first_bid
  功    能：获取链表中当前的最高竞价记录（即赢家信息）
  输入参数：BidRecord& outRecord - [引用输出参数] 用于带回当前的最高出价记录
  返 回 值：bool - true 表示获取成功，false 表示链表为空（无任何出价）
  说    明：因为 insert_bid 保证了链表降序，所以直接将 head->data 赋值给输出参数即可 。
 ***************************************************************************/
bool my_list::first_bid(BidRecord& outRecord) const {
    if (head == nullptr) {
        return false;
    }
    outRecord = head->data; // 由于是有序链表，头节点就是最高价
    return true;
}

void my_list::print_list() const {
    ListNode* curr = head;
    std::cout << "List [Size=" << size << "]: ";
    while (curr != nullptr) {
        std::cout << "(" << curr->data.bidId << ", P:" << curr->data.price << ") -> ";
        curr = curr->next;
    }
    std::cout << "NULL" << std::endl;
}