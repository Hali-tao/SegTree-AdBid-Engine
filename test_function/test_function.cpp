#include "../include/class_AdEngine.h"
#include "../include/class_NaiveEngine.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cassert>

// 仿真数据结构：用于记录随机生成的竞价单，方便后续随机发起撤销
struct ActiveBid {
    std::string slotId;
    int QL = 0;
    int QR = 0;
    std::string bidId;
};

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 1. 初始化日志输出流
    std::string logPath = "D:/DataStructureProject/log.txt";
    std::ofstream logFile(logPath);
    if (!logFile.is_open()) {
        std::cerr << " [警告] 无法打开日志文件路径: " << logPath << std::endl;
        std::cerr << "请确保该盘符与目录存在。程序将尝试在当前目录下生成 log.txt。" << std::endl;
        logPath = "log.txt";
        logFile.open(logPath);
        if (!logFile.is_open()) {
            std::cerr << " [错误] 无法创建任何日志文件，程序退出！" << std::endl;
            return 1;
        }
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "  BidTrack 5000条海量仿真数据高并发对拍测试开始    " << std::endl;
    std::cout << "  日志将实时写入: " << logPath << std::endl;
    std::cout << "==================================================" << std::endl;

    logFile << "==================================================\n";
    logFile << "           BidTrack 仿真对拍测试运行日志           \n";
    logFile << "==================================================\n\n";

    AdEngine segmentTreeEngine;  // 手写线段树引擎
    NaiveEngine naiveEngine;     // 暴力对照组引擎

    const int TOTAL_COMMANDS = 5000;

    // 仿真广告主池和广告位池
    const std::string slotsPool[] = { "SLOT_BANNER", "SLOT_DETAIL", "SLOT_SPLASH", "SLOT_FLOAT" };
    const std::string advertisers[] = { "Apple", "Xiaomi", "Huawei", "Dyson", "Sony", "Samsung" };

    // 维护一个动态数组来记录激活的竞价单（用于模拟撤销）
    const int MAX_ACTIVE_BIDS = 10000;
    ActiveBid* activeBids = new ActiveBid[MAX_ACTIVE_BIDS];
    int activeCount = 0;
    int bidCounter = 100000; // 用于生成自增的全局唯一 bidId

    int updateCount = 0;
    int cancelCount = 0;
    int queryCount = 0;
    int guaranteeQueryCount = 0;

    for (int step = 1; step <= TOTAL_COMMANDS; ++step) {
        // 随机决策当前指令类型：
        // 50% 区间竞价投递, 15% 区间竞价撤单, 25% 单点赢家查询, 10% 独占霸屏价查询
        int op = std::rand() % 100;
        std::string slot = slotsPool[std::rand() % 4];

        if (op < 50) {
            // 1. 产生竞价投递
            int start = std::rand() % 1440;
            int end = start + (std::rand() % (1440 - start)); // [start, 1439] 的随机区间
            int price = (std::rand() % 1000) + 1; // 1 ~ 1000 元
            std::string bidId = "BID_" + std::to_string(++bidCounter);
            std::string adv = advertisers[std::rand() % 6];

            // 写入日志
            logFile << "[Step " << step << "] [UPDATE] 槽位: " << slot
                << " | 区间: [" << start << ", " << end << "] | 竞价ID: " << bidId
                << " | 广告主: " << adv << " | 价格: " << price << " 元\n";

            // 两个引擎同步更新
            segmentTreeEngine.updateBid(slot, start, end, bidId, adv, price);
            naiveEngine.updateBid(slot, start, end, bidId, adv, price);

            // 记录下这个竞价单
            if (activeCount < MAX_ACTIVE_BIDS) {
                activeBids[activeCount++] = { slot, start, end, bidId };
            }
            updateCount++;

        }
        else if (op < 65) {
            // 2. 产生撤价撤单（仅当有已经投递的单子时）
            if (activeCount > 0) {
                int targetIdx = std::rand() % activeCount;
                ActiveBid target = activeBids[targetIdx];

                // 写入日志
                logFile << "[Step " << step << "] [CANCEL] 槽位: " << target.slotId
                    << " | 原始区间: [" << target.QL << ", " << target.QR
                    << "] | 撤销竞价ID: " << target.bidId << "\n";

                segmentTreeEngine.cancelBid(target.slotId, target.bidId);
                naiveEngine.cancelBid(target.slotId, target.QL, target.QR, target.bidId);

                // 从记录中移除
                activeBids[targetIdx] = activeBids[activeCount - 1];
                activeCount--;
                cancelCount++;
            }
            else {
                // 如果当前没有活跃订单，降级为单点查询，维持步数推进
                op = 70;
            }
        }

        // 重新判定（处理降级或原本的分支）
        if (op >= 65 && op < 90) {
            // 3. 产生单点赢家对拍查询
            int time = std::rand() % 1440;
            QueryResult st_res = segmentTreeEngine.query_point(slot, time);
            QueryResult na_res = naiveEngine.query_point(slot, time);

            int priceST = st_res.price;
            std::string winnerST = st_res.advertiserId;
            int priceNaive = na_res.price; 
            std::string winnerNaive = na_res.advertiserId;


            logFile << "[Step " << step << "] [QUERY_POINT] 槽位: " << slot
                << " | 时间点: " << time
                << " | ST结果: [" << winnerST << ", " << priceST
                << "] | Naive结果: [" << winnerNaive << ", " << priceNaive << "]\n";

            // 核心对拍断言
            if (priceST != priceNaive || winnerST != winnerNaive) {
                std::cerr << "\n [ERROR] 对拍失败！ 步骤: " << step << std::endl;
                std::cerr << "槽位: " << slot << " | 时间: " << time << std::endl;
                std::cerr << "线段树结果: " << winnerST << " (" << priceST << "元)" << std::endl;
                std::cerr << "暴力法结果: " << winnerNaive << " (" << priceNaive << "元)" << std::endl;

                // 写入崩溃日志
                logFile << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                logFile << "[FATAL ERROR] 对拍不一致中断！\n";
                logFile << "步骤: " << step << " | 槽位: " << slot << " | 时间点: " << time << "\n";
                logFile << "线段树结果: " << winnerST << " (" << priceST << "元)\n";
                logFile << "暴力法结果: " << winnerNaive << " (" << priceNaive << "元)\n";
                logFile << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                logFile.close();

                assert(false); // 直接抛出错误中断
            }
            queryCount++;

        }
        else if (op >= 90) {
            // 4. 产生区间独占保鲜价对拍查询
            int start = std::rand() % 1440;
            int end = start + (std::rand() % (1440 - start));

            QueryResult st_res = segmentTreeEngine.query_range(slot, start, end);
            int thresholdST = st_res.price;

            QueryResult na_res = naiveEngine.query_range(slot, start, end);
            int thresholdNaive = na_res.price;

            logFile << "[Step " << step << "] [QUERY_RANGE] 槽位: " << slot
                << " | 区间: [" << start << ", " << end
                << "] | ST保鲜价: " << thresholdST << " | Naive保鲜价: " << thresholdNaive << "\n";

            // 核心对拍断言
            if (thresholdST != thresholdNaive) {
                std::cerr << "\n [ERROR] 区间独占保鲜价对拍失败！ 步骤: " << step << std::endl;
                std::cerr << "槽位: " << slot << " | 区间: [" << start << ", " << end << "]" << std::endl;
                std::cerr << "线段树保鲜价: " << thresholdST << std::endl;
                std::cerr << "暴力法保鲜价: " << thresholdNaive << std::endl;

                // 写入崩溃日志
                logFile << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                logFile << "[FATAL ERROR] 保鲜价对拍不一致中断！\n";
                logFile << "步骤: " << step << " | 槽位: " << slot << " | 区间: [" << start << ", " << end << "]\n";
                logFile << "线段树保鲜价: " << thresholdST << " 元\n";
                logFile << "暴力法保鲜价: " << thresholdNaive << " 元\n";
                logFile << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                logFile.close();

                assert(false);
            }
            guaranteeQueryCount++;
        }

        // 刷新文件缓冲区，防止发生断言崩溃时部分日志未写入
        logFile.flush();

        // 阶段性输出
        if (step % 1000 == 0) {
            std::cout << "[成功] 已对拍验证 " << step << " / " << TOTAL_COMMANDS << " 条指令..." << std::endl;
        }
    }

    std::cout << "\n==================================================" << std::endl;
    std::cout << " 恭喜！5000条并发指令全部对拍通过，无任何断言失败！" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "-> 详情请查阅完整日志文件: " << logPath << std::endl;

    logFile << "\n==================================================\n";
    logFile << "            对拍成功结束！统计数据如下            \n";
    logFile << "==================================================\n";
    logFile << "-> 竞价更新指令（Update）执行: " << updateCount << " 次\n";
    logFile << "-> 竞价撤销指令（Cancel）执行: " << cancelCount << " 次\n";
    logFile << "-> 单点赢家查询（QueryPoint）对拍: " << queryCount << " 次\n";
    logFile << "-> 独占保鲜价查询（QueryRange）对拍: " << guaranteeQueryCount << " 次\n";
    logFile << "==================================================\n";

    logFile.close();
    delete[] activeBids;
    return 0;
}