#include "../include/httplib.h" // 引入单头文件 Web 库
#include "../include/class_AdEngine.h"
#include <sstream>
#include <random>
#include <ctime>
#include <iostream>
#include <string>
#include <chrono>


int main() {
    AdEngine engine(10);

    // 仿真广告主池和广告位池
    const std::string slotsPool[] = { "SLOT_BANNER", "SLOT_DETAIL", "SLOT_SPLASH", "SLOT_FLOAT" };
    const std::string advertisers[] = { "Apple", "Xiaomi", "Huawei", "Dyson", "Sony", "Samsung" };

    std::random_device rd;
    std::mt19937 rng(rd());

    // 依然保持均匀随机的字段
    std::uniform_int_distribution<int> slotDist(0, 3);
    std::uniform_int_distribution<int> advDist(0, 5);
    std::uniform_int_distribution<int> lenDist(15, 180);   // 区间长度一般在 15分钟 到 3小时

    // 【文献佐证 1：对数正态分布】模拟商业拍卖长尾效应，绝大部分中小出价，极少数头部高价
    // 参数 6.5 (均值) 和 1.0 (标准差) 生成的值经取整截断后能完美落在题目要求的 [100, 9900] 范围内
    std::lognormal_distribution<double> priceDist(6.5, 1.0);

    // 【文献佐证 2：指数分布】模拟泊松流中的“请求到达时间间隔”
    // 全天 1440 分钟总共涌入 5000 条高并发请求，平均每分钟到达率 Lambda = 5000.0 / 1440.0 ≈ 3.4722
    std::exponential_distribution<double> intervalDist(3.4722);

    std::cout << "[Simulation] 开始注入 5000 条大促高并发竞价数据..." << std::endl;

    double cumulativeTime = 0.0; // 动态累加的时间戳（单位：分钟）
    int i;
    for (i = 1; i <= 5000; ++i) {
        std::string slotId = slotsPool[slotDist(rng)];
        std::string advId = advertisers[advDist(rng)];
        std::string bidId = "BID_" + std::to_string(100000 + i);

        // 1. 基于泊松流的时间间隔累加，得到当前请求的发生时间点
        cumulativeTime += intervalDist(rng);
        int startMin = static_cast<int>(cumulativeTime) % 1440; // 滚动映射在全天 1440 分钟内

        // 2. 生成区间长度并计算结束时间
        int len = lenDist(rng);
        int endMin = startMin + len;
        if (endMin > 1439) endMin = 1439; // 防止越界

        // 3. 基于对数正态分布生成出价金额，并进行 [100, 9900] 边界截断
        int price = static_cast<int>(priceDist(rng));
        price = std::max(100, std::min(9900, price));

        // 4. 送入线段树引擎
        engine.updateBid(slotId, startMin, endMin, bidId, advId, price);
    }
    std::cout << "[Simulation] 5000 条数据注入完毕！线段树状态已就绪。" << std::endl;

    

    httplib::Server svr;

    // 接口 1：静态文件路由
    svr.set_mount_point("/", "./web_visual");

    // 接口 2：处理竞价投递并捕获 std::cout 的线段树路径溯源
    svr.Post("/api/update_bid", [&](const httplib::Request& req, httplib::Response& res) {
        std::string slotId = req.get_param_value("slotId");
        std::string ql_str = req.get_param_value("QL");
        std::string qr_str = req.get_param_value("QR");
        std::string advId = req.get_param_value("advId");
        std::string price_str = req.get_param_value("price");

        if (slotId.empty() || ql_str.empty() || qr_str.empty() || price_str.empty()) {
            res.status = 200;
            res.set_content("{\"status\":\"error\",\"trace\":\"参数不完整\"}", "application/json");
            return;
        }

        int QL = std::stoi(ql_str);
        int QR = std::stoi(qr_str);
        int price = std::stoi(price_str);
        std::string bidId = "BID_" + std::to_string(100000 + i);
        i++;
        engine.updateBid(slotId, QL, QR, bidId, advId, price);

        std::string trace = "[UPDATE]\\n" + get_trace(QL, QR);

        res.status = 200;
        res.set_content("{\"status\":\"success\", \"trace\":\"" + trace + "\"}", "application/json");
        });

    // 接口 3：处理竞价撤销并捕获线段树的路径溯源
    svr.Post("/api/revoke_bid", [&](const httplib::Request& req, httplib::Response& res) {
        std::string bidId = req.get_param_value("bidId");
        int QL, QR;
        std::string slotId;

        if (bidId.empty()) {
            res.status = 200;
            res.set_content("{\"status\":\"error\",\"trace\":\"[REVOKE 拒绝] 参数不完整，订单号不能为空\"}", "application/json; charset=utf-8");
            return;
        }

        if (!engine.get_slotId(bidId, slotId)) {
            res.status = 200;
            std::string errTrace = "[REVOKE REJECTED] Revocation failed: Order ID " + bidId + " not found in the system or the order has already been revoked.";
            res.set_content("{\"status\":\"error\", \"trace\":\"" + errTrace + "\"}", "application/json; charset=utf-8");
            return;
        }

        engine.cancelBid(slotId, bidId);
        engine.get_time(bidId, QL, QR);
        std::string trace = "[REVOKE]\\n" + get_trace(QL, QR);

        res.status = 200;
        res.set_content("{\"status\":\"success\", \"trace\":\"" + trace + "\"}", "application/json");
        });

    // 接口 4：支持单点查询并输出线段树路径溯源
    svr.Get("/api/query_engine", [&](const httplib::Request& req, httplib::Response& res) {
        std::string slotId = req.get_param_value("slotId");
        std::string type = req.get_param_value("type");
        std::string ql_str = req.get_param_value("QL");

        if (slotId.empty() || type.empty() || ql_str.empty()) {
            res.status = 200;
            res.set_content("{\"bidId\":\"\",\"winner\":\"\",\"price\":0,\"trace\":\"参数缺失\"}", "application/json");
            return;
        }

        int QL = std::stoi(ql_str);
        QueryResult qr;
        std::string trace;
        if (type == "point") {
            qr = engine.query_point(slotId, QL);
            trace = "[QUERY_POINT]\\n" + get_trace(QL, QL);
        }
        else {
            std::string qr_str = req.get_param_value("QR");
            int QR = qr_str.empty() ? QL : std::stoi(qr_str);
            qr = engine.query_range(slotId, QL, QR);
            trace = "[QUERY_RANGE]\\n" + get_trace(QL, QR);
        }

        std::stringstream json;
        json << "{\"bidId\":\"" << qr.bidId << "\",\"winner\":\"" << qr.advertiserId
            << "\",\"price\":" << qr.price << ",\"trace\":\"" << trace << "\"}";

        res.status = 200;
        res.set_content(json.str(), "application/json");
        });

    // 接口 5：全天看板渲染数据拉取
    svr.Get("/api/get_timeline", [&](const httplib::Request& req, httplib::Response& res) {
        std::string slotId = req.get_param_value("slotId");
        if (slotId.empty()) {
            res.status = 200;
            res.set_content("[]", "application/json");
            return;
        }

        std::stringstream json;
        json << "[";
        for (int t = 0; t < 1440; ++t) {
            QueryResult qr = engine.query_point(slotId, t);
            json << "{\"time\":" << t << ",\"price\":" << qr.price << ",\"winner\":\"" << qr.advertiserId << "\"}";
            if (t < 1439) json << ",";
        }
        json << "]";

        res.status = 200;
        res.set_content(json.str(), "application/json");
        });

    std::cout << "[Visual Engine] 可视化服务器已启动：http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}