# SegmentTree-AdBidding-Engine

广告位分时段动态竞价引擎 —— 数据结构课程设计

---

## 项目简介

基于**线段树 + 手工链表**实现的广告位动态竞价系统。支持区间竞价投递、撤销、单点/区间查询，前端可视化看板实时展示全天1440分钟的出价归属。

---

## 核心数据结构

| 结构 | 说明 |
|------|------|
| **线段树** | 1440个叶子节点（全天每分钟），节点维护本地最高价 + 子树汇总最高价 |
| **链表** | 每个线段树节点独立维护，降序存储该区间所有竞价记录，头节点即为最高出价 |

---

## 核心功能

- **区间竞价投递**：更新 `[QL, QR]` 区间出价
- **单点/区间查询**：查询某分钟/某时段最高出价及赢家
- **全额撤价**：撤销指定订单，自动恢复次高有效出价
- **路径溯源**：实时打印线段树遍历路径

---

## 项目结构

```
├── common/
│   ├── class_AdEngine.cpp      # 广告引擎（多广告位管理）
│   ├── class_segment_tree.cpp  # 线段树核心实现
│   ├── class_my_list.cpp       # 手工链表
│   └── class_NaiveEngine.cpp   # 暴力对照组（用于对拍验证）
├── include/
│   ├── class_AdEngine.h
│   ├── class_segment_tree.h
│   ├── class_my_list.h
│   ├── class_NaiveEngine.h
│   └── common_types.h          # QueryResult 等公共类型
├── test_function/
│   └── test_function.cpp       # 5000条仿真数据对拍测试
├── test_http/
│   └── test_http.cpp           # HTTP 服务（连接前端可视化）
├── web_visual/
│   ├── index.html              # 可视化看板
│   ├── script.js               # 前端交互逻辑
│   └── styles.css              # 样式
└── README.md
```

---

## 运行

编译 `test_http` 项目 → 运行 → 访问 `http://localhost:8080`

---

## 技术栈

C++ | cpp-httplib | HTML + CSS + JavaScript

---

**Author**: 黄宇镐 | **Course**: 数据结构课程设计 | **Year**: 2026
