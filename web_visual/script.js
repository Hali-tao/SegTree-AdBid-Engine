const advColors = {
    'Apple': '#56d078', 'Xiaomi': '#ff9a52', 'Huawei': '#f25f5f',
    'Dyson': '#b87fd0', 'Sony': '#4a5c6c', 'Samsung': '#428bff', '': '#e9ecef'
};
const slotsList = ["SLOT_BANNER", "SLOT_DETAIL", "SLOT_SPLASH", "SLOT_FLOAT"];

/**
 * @brief 提取并严格校验前端输入的 时:分，并转换为绝对分钟数
 */
function parseAndValidateTime(hId, mId) {
    const hEl = document.getElementById(hId);
    const mEl = document.getElementById(mId);
    
    if (!hEl || !mEl || hEl.value.trim() === "" || mEl.value.trim() === "") {
        return null;
    }
    
    const h = parseInt(hEl.value, 10);
    const m = parseInt(mEl.value, 10);
    
    if (isNaN(h) || h < 0 || h > 23 || isNaN(m) || m < 0 || m > 59) {
        return null;
    }
    
    return h * 60 + m;
}

function renderLegend() {
    const box = document.getElementById('legendBox'); box.innerHTML = '';
    Object.keys(advColors).forEach(name => {
        if(!name) return;
        box.innerHTML += `<div class="legend-item"><div class="legend-color" style="background:${advColors[name]}"></div><span>${name}</span></div>`;
    });
}

function initTimelineSkeletons() {
    const container = document.getElementById('multiTimelines'); container.innerHTML = '';
    slotsList.forEach(slot => {
        container.innerHTML += `
            <div class="slot-row">
                <div class="slot-name">${slot}</div>
                <div class="timeline-wrapper">
                    <div class="timeline-container" id="timeline-${slot}"></div>
                    <div class="time-scale"><span>00:00</span><span>06:00</span><span>12:00</span><span>18:00</span><span>23:59</span></div>
                </div>
            </div>`;
    });
}

// 核心重构：控制表单行显隐，REVOKE 模式下同时隐藏网位选择器
function toggleOpMode() {
    const mode = document.getElementById('opType').value;
    
    const rowSlot = document.getElementById('rowSlot');
    const rowTime = document.getElementById('rowTime');
    const rowBid = document.getElementById('rowBid');
    const rowAdv = document.getElementById('rowAdv');
    const rowPrice = document.getElementById('rowPrice');
    const btn = document.getElementById('actionBtn');

    if (mode === "UPDATE") {
        if (rowSlot) rowSlot.style.display = "flex";   // 显示网位
        if (rowTime) rowTime.style.display = "flex";   // 显示时间
        if (rowBid) rowBid.style.display = "none";     // 隐藏订单号
        if (rowAdv) rowAdv.style.display = "flex";     
        if (rowPrice) rowPrice.style.display = "flex"; 
        if (btn) { btn.innerText = "🚀 投递区间出价 (Update)"; btn.className = "bg-update"; }
    } else {
        if (rowSlot) rowSlot.style.display = "none";   // 🚀 撤销不需要选网位，隐藏！
        if (rowTime) rowTime.style.display = "none";   // 隐藏时间
        if (rowBid) rowBid.style.display = "flex";     // 显示订单号填空
        if (rowAdv) rowAdv.style.display = "none";     
        if (rowPrice) rowPrice.style.display = "none"; 
        if (btn) { btn.innerText = "🗑️ 撤销区间出价 (Revoke)"; btn.className = "bg-revoke"; }
    }
}

function toggleQueryMode() {
    const type = document.getElementById('queryType').value;
    const endWrapper = document.getElementById('searchEndWrapper');
    if (endWrapper) {
        endWrapper.style.display = (type === "point") ? "none" : "flex";
    }
}

function refreshAllTimelines() {
    slotsList.forEach(slot => {
        fetch(`/api/get_timeline?slotId=${slot}`)
            .then(res => {
                if(!res.ok) throw new Error("Server Error " + res.status);
                return res.json();
            })
            .then(data => {
                const tContainer = document.getElementById(`timeline-${slot}`);
                if(!tContainer) return;
                tContainer.innerHTML = '';
                data.forEach(item => {
                    const block = document.createElement('div');
                    block.className = 'minute-block';
                    block.style.background = advColors[item.winner] || '#e9ecef';
                    
                    block.onclick = () => {
                        fetchTimelinePointToSnapshot(slot, item.time);
                    };
                    tContainer.appendChild(block);
                });
            })
            .catch(err => console.warn(`渲染看板通道[${slot}]发生网络滞后:`, err));
    });
}

function fetchTimelinePointToSnapshot(slotId, timeMin) {
    fetch(`/api/query_engine?slotId=${slotId}&type=point&QL=${timeMin}`)
        .then(res => res.json())
        .then(data => {
            const hours = Math.floor(timeMin / 60).toString().padStart(2, '0');
            const mins = (timeMin % 60).toString().padStart(2, '0');

            document.getElementById('snapshotPlaceholder').style.display = 'none';
            document.getElementById('snapshotContent').style.display = 'block';
            
            document.getElementById('ssSlot').innerText = slotId;
            document.getElementById('ssTime').innerText = `${hours}:${mins} (第 ${timeMin} 分钟)`;
            
            if (data && data.price > 0 && data.bidId) {
                document.getElementById('ssBid').innerText = data.bidId;
                document.getElementById('ssBid').className = "clickable-id";
                // 🚀 联动修改：点击时只将订单ID传过去
                document.getElementById('ssBid').onclick = () => autoFillRevoke(data.bidId);
                document.getElementById('ssAdv').innerText = data.winner || "未知广告主";
                document.getElementById('ssAdv').style.color = advColors[data.winner] || "#888";
                document.getElementById('ssPrice').innerText = `¥ ${data.price}`;
            } else {
                document.getElementById('ssBid').innerText = "暂无有效订单";
                document.getElementById('ssBid').className = "";
                document.getElementById('ssBid').onclick = null;
                document.getElementById('ssAdv').innerText = "系统保留底价";
                document.getElementById('ssAdv').style.color = "#888";
                document.getElementById('ssPrice').innerText = "¥ 0";
            }
        }).catch(err => console.error("抓取即时状态失败:", err));
}

function executeSearch() {
    const slotId = document.getElementById('searchSlotId').value;
    const type = document.getElementById('queryType').value;
    
    const QL = parseAndValidateTime('searchStartH', 'searchStartM');
    if (QL === null) {
        alert("请输入有效的起始时间！(小时: 0-23, 分钟: 0-59)");
        return;
    }

    let url = `/api/query_engine?slotId=${slotId}&type=${type}&QL=${QL}`;
    let title = `📍 精准单点检索 [网位: ${slotId} | 绝对时间轴:${QL}min]`;

    if (type === "range") {
        const QR = parseAndValidateTime('searchEndH', 'searchEndM');
        if (QR === null) {
            alert("请输入有效的结束时间！(小时: 0-23, 分钟: 0-59)");
            return;
        }
        if (QL > QR) {
            alert("起始时间不能晚于结束时间！");
            return;
        }
        url += `&QR=${QR}`;
        title = `📊 区间最高出价检索 [网位: ${slotId} | 绝对区间:${QL}-${QR}min]`;
    }

    fetch(url)
        .then(res => res.json())
        .then(data => {
            const div = document.getElementById('detailContent');
            div.style.display = 'block';
            if (data && data.price > 0) {
                // 🚀 联动修改：检索结果中的订单号点击，也去掉多余参数只传 data.bidId
                div.innerHTML = `
                    <strong>检索网位：</strong> <span style="color:#2980b9">${slotId}</span><br>
                    <strong>有效订单：</strong> <span class="clickable-id" onclick="autoFillRevoke('${data.bidId}')">${data.bidId}</span><br>
                    <strong>当前赢家：</strong> <span style="color:${advColors[data.winner] || '#333'}; font-weight:bold;">${data.winner}</span><br>
                    <strong>胜出高价：</strong> <span style="color:#2ecc71; font-weight:bold;">¥ ${data.price}</span>
                `;
            } else {
                div.innerHTML = `<strong>检索网位：</strong> ${slotId}<br><span style="color:#95a5a6">当前所搜时段无任何广告有效订单</span>`;
            }

            appendTraceLog(title, data.trace || "无执行追踪链路", 'trace-query');
        })
        .catch(err => console.error("深度检索出现通信异常:", err));
}

// 核心重构：快照点击回填时自动剔除 "BID_" 前缀，只在格子里填入 6 位数字
function autoFillRevoke(fullBidId) {
    document.getElementById('opType').value = "REVOKE";
    toggleOpMode(); 
    
    const bidInput = document.getElementById('bidId');
    if (fullBidId && fullBidId.startsWith("BID_")) {
        bidInput.value = fullBidId.substring(4); // 截取掉前4个字符，留下6位数字
    } else {
        bidInput.value = fullBidId;
    }
    
    window.scrollTo({ top: 400, behavior: 'smooth' });
}

function appendTraceLog(title, traceText, typeClass) {
    const box = document.getElementById('logBox');
    if(traceText) {
        const body = document.createElement('div');
        body.className = `log-line`; 
        body.innerText = traceText; 
        box.insertBefore(body, box.firstChild); 
    }
    const head = document.createElement('div');
    head.className = `log-line ${typeClass}`;
    head.innerText = `>>> ${title}`;
    box.insertBefore(head, box.firstChild); 
    box.scrollTop = 0;
}

// 核心重构：处理投递与撤销操作，对撤销订单号进行纯数字与长度的强拦截，并通过前端拼接送出
function handleEngineAction() {
    const mode = document.getElementById('opType').value;
    const p = new URLSearchParams();

    if (mode === "UPDATE") {
        const slotId = document.getElementById('slotId').value;
        p.append('slotId', slotId); 

        const QL = parseAndValidateTime('startH', 'startM');
        const QR = parseAndValidateTime('endH', 'endM');
        const advId = document.getElementById('advId').value;
        const price = document.getElementById('price').value;

        if (QL === null || QR === null) {
            alert("请输入合法的时间数值！(小时范围0-23 分钟范围0-59)");
            return;
        }
        if (QL > QR) { alert("起始时间不能晚于结束时间！"); return; }
        if (!price || price <= 0) { alert("请输入合法的出价金额！"); return; }
        // ====== 针对 price 的防越界及溢出强拦截 ======
        const priceNum = parseFloat(price);

        if (isNaN(priceNum) || priceNum <= 0) {
            alert("【安全拦截】出价金额必须是大于 0 的有效数字！");
            return;
        }

        // 假设底层 C++ 引擎接收的最大金额为 1 亿 (可根据 C++ 数据类型调整，如 int32 最大约为 21 亿)
        const MAX_PRICE = 100000000; 
        if (priceNum > MAX_PRICE) {
            alert(`【安全拦截】出价金额过大！单次出价最高不能超过 ¥${MAX_PRICE.toLocaleString()}，防止引擎数据溢出。`);
            return;
        }

        // 检查是否包含过多的小数位（如果系统只支持整数出价）
        if (!Number.isInteger(priceNum)) {
            alert("【安全拦截】出价金额仅支持整数，请勿输入小数！");
            return;
        }
        
        p.append('QL', QL); 
        p.append('QR', QR); 
        p.append('advId', advId); 
        p.append('price', price);

        fetch('/api/update_bid', { method: 'POST', body: p })
        .then(res => res.json())
        .then(data => {
            appendTraceLog(`[UPDATE 成功] 广告位:${slotId}, 绝对区间:[${QL}-${QR}], 分配订单号:${data.bidId || 'OK'}`, data.trace, 'trace-update');
            refreshAllTimelines();
        }).catch(err => console.error("投递异常:", err));
        
    } else {
        // 🚀 REVOKE 分支：完全剥离对 slotId 的依赖与传输
        const rawInput = document.getElementById('bidId').value.trim();
        
        if (!rawInput) {
            alert("请填写需要撤销的订单编号！");
            return;
        }
        
        // 使用正则表达式严格检验：必须是纯数字且刚好满足 6 位长度
        const isPureSixDigits = /^\d{6}$/.test(rawInput);
        if (!isPureSixDigits) {
            alert("【安全拦截】订单号输入不合规！必须为 6 位纯整数数字（例如: 100025) 严禁携带字母、符号或空格。");
            return; // 熔断阻止，直接拒绝发送包给底层 C++
        }

        // 前端安全拼装完整前缀送后端
        const finalBidId = "BID_" + rawInput;
        p.append('bidId', finalBidId);

        fetch('/api/revoke_bid', { method: 'POST', body: p })
        .then(res => res.json())
        .then(data => {
            appendTraceLog(`[REVOKE 成功] 撤销指令已下发，目标订单:${finalBidId}`, data.trace, 'trace-revoke');
            document.getElementById('snapshotPlaceholder').style.display = 'block';
            document.getElementById('snapshotContent').style.display = 'none';
            refreshAllTimelines();
        }).catch(err => console.error("撤销异常:", err));
    }
}

window.onload = () => {
    renderLegend();
    initTimelineSkeletons();
    refreshAllTimelines();
    toggleOpMode(); 
    fetchTimelinePointToSnapshot("SLOT_BANNER", 0);
};