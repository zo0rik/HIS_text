#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "models.h"
#include "utils.h"
#include "inpatient_department.h"

// 内部工具：生成病房数据字典
void initBedsIfEmpty() {
    if (bedHead->next) return;

    // 初始化20张固定的病床，按要求区分房型与护理级别
    char types[5][50] = { "单人病房", "双人病房", "三人病房", "单人陪护病房", "单人陪护疗养病房" };
    double prices[5] = { 200.0, 150.0, 100.0, 300.0, 500.0 };
    char wards[2][50] = { "普通病房", "特殊病房" };

    Bed* tail = bedHead;
    int bedNum = 101;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) { // 每种类型生成4张床
            Bed* b = (Bed*)malloc(sizeof(Bed));
            b->bedNumber = bedNum++;
            b->isOccupied = 0;
            strcpy(b->patientId, "");
            strcpy(b->bedType, types[i]);
            b->price = prices[i];
            strcpy(b->wardType, (i >= 3) ? wards[1] : wards[0]); // 后两种为特殊病房
            b->next = NULL;
            tail->next = b;
            tail = b;
        }
    }
}

// 获取全院流水外部工具
extern void generateRecordID(char* buffer);

// ---------------------------------------------------------
// 动态检测床位紧张度并执行调配转换
// ---------------------------------------------------------
void checkAndAdjustBedTension() {
    int total = 0, empty = 0;
    Bed* b = bedHead->next;
    while (b) {
        total++;
        if (b->isOccupied == 0) empty++;
        b = b->next;
    }

    // 如果空床率不到 20%
    if (total > 0 && ((float)empty / total) < 0.2f) {
        printf("\n【系统智能告警】当前空余床位已不足 20%%，触发病床动态转换机制！\n");
        b = bedHead->next;
        int converted = 0;
        while (b) {
            // 将空闲的 单人陪护疗养病房 降维转为 双人病房
            if (b->isOccupied == 0 && strcmp(b->bedType, "单人陪护疗养病房") == 0) {
                strcpy(b->bedType, "双人病房");
                b->price = 150.0;
                printf(" -> 已将房号 %d (原疗养病房) 调整为 [双人病房]\n", b->bedNumber);

                // 虚拟增加一张同房的附属双人床 (编号+1000)
                Bed* extra = (Bed*)malloc(sizeof(Bed));
                *extra = *b;
                extra->bedNumber = b->bedNumber + 1000;
                extra->next = bedHead->next;
                bedHead->next = extra;
                printf(" -> 自动释放扩充附属双人床位 %d，以缓解病房压力\n", extra->bedNumber);
                converted++;
            }
            b = b->next;
        }
        if (converted == 0) printf(" -> 目前无可供转换的空闲疗养病房。\n");
    }
}

// ---------------------------------------------------------
// 办理入院 (生成 ⑤住院记录)
// ---------------------------------------------------------
void admitPatient(const char* docId) {
    initBedsIfEmpty();
    checkAndAdjustBedTension();

    char pId[20];
    printf("\n请输入办理入院的患者ID: "); safeGetString(pId, 20);

    // 校验患者并获取可用资金
    Patient* p = patientHead->next; Patient* targetPat = NULL;
    while (p) { if (strcmp(p->id, pId) == 0) { targetPat = p; break; } p = p->next; }
    if (!targetPat) { printf("患者不存在。\n"); return; }

    printf("请输入拟住院天数: ");
    int days = safeGetPositiveInt();

    // 【算法规则】押金计算：100整数倍，200*天数，最低1000
    int baseDeposit = 200 * days;
    if (baseDeposit < 1000) baseDeposit = 1000;
    // 取整逻辑 (向上百位取整)
    int finalDeposit = ((baseDeposit + 99) / 100) * 100;

    printf("按规则系统核算，该患者需缴纳住院押金: %d 元。\n", finalDeposit);

    // 强制模拟直接从患者余额抵扣作为押金 (方便跑通流程)
    if (targetPat->balance < finalDeposit) {
        printf("患者余额不足以支付押金，请先前往财务中心充值！\n"); return;
    }
    targetPat->balance -= finalDeposit;
    printf("已成功从患者账户划扣押金。\n");

    // 寻找空床展示
    printf("\n可用空闲病床列表:\n");
    Bed* b = bedHead->next; int hasEmpty = 0;
    while (b) {
        if (!b->isOccupied) {
            printf("[%d] %s - %s (每日 %.2f)\n", b->bedNumber, b->wardType, b->bedType, b->price);
            hasEmpty = 1;
        }
        b = b->next;
    }
    if (!hasEmpty) { printf("全院床位已满！\n"); targetPat->balance += finalDeposit; return; }

    printf("请输入分配的床位号: ");
    int selectBed = safeGetInt();
    b = bedHead->next; Bed* finalBed = NULL;
    while (b) { if (b->bedNumber == selectBed && !b->isOccupied) { finalBed = b; break; } b = b->next; }
    if (!finalBed) { printf("无效的床位号或床位被占。\n"); targetPat->balance += finalDeposit; return; }

    finalBed->isOccupied = 1; strcpy(finalBed->patientId, pId);

    // 【规范生成 ⑤ 住院记录】
    Record* r5 = (Record*)malloc(sizeof(Record));
    extern void generateRecordID(char* buffer);
    generateRecordID(r5->recordId);
    r5->type = 5; strcpy(r5->patientId, pId); strcpy(r5->staffId, docId);
    r5->cost = finalDeposit; // 住院记录初期 cost 暂存押金金额
    r5->isPaid = 1; // 押金已支付

    char adminTime[30]; getCurrentTimeStr(adminTime, 30);
    sprintf(r5->description, "病房:%s_床位:%d_入院日期:%s_预期天数:%d_押金:%d_出院日期:无_总费用:0",
        finalBed->wardType, finalBed->bedNumber, adminTime, days, finalDeposit);
    getCurrentTimeStr(r5->createTime, 30);

    r5->next = recordHead->next; recordHead->next = r5;
    printf("【成功】已成功办理入院，关联床位 %d。\n", finalBed->bedNumber);
}

// ---------------------------------------------------------
// 每日扣费模拟机制 (防呆：余额小于1000报警)
// ---------------------------------------------------------
void dailyDeductionSimulation() {
    printf("\n--- 模拟执行每日 08:00 床位费划扣 ---\n");
    Bed* b = bedHead->next; int count = 0;
    while (b) {
        if (b->isOccupied) {
            Patient* p = patientHead->next;
            while (p) {
                if (strcmp(p->id, b->patientId) == 0) {
                    // 患者在院期间押金余额检查
                    if (p->balance < 1000) {
                        printf("【警告】患者 %s 余额 (%.2f) 低于1000元，请立刻停药并催缴！\n", p->name, p->balance);
                    }
                    printf("患者 %s (%d床) 已记账当日床位费 %.2f 元。\n", p->name, b->bedNumber, b->price);
                    count++;
                    break;
                }
                p = p->next;
            }
        }
        b = b->next;
    }
    printf("执行完毕，共扫描处理 %d 名在院患者。\n", count);
}

// ---------------------------------------------------------
// 办理出院 (更新 ⑤住院记录并统账)
// ---------------------------------------------------------
void dischargePatient() {
    char pId[20];
    printf("请输入要办理出院的患者ID: "); safeGetString(pId, 20);

    // 1. 释放床位并获取每日单价
    Bed* b = bedHead->next; Bed* targetBed = NULL;
    while (b) {
        if (b->isOccupied && strcmp(b->patientId, pId) == 0) { targetBed = b; break; }
        b = b->next;
    }
    if (!targetBed) { printf("该患者当前并未住院。\n"); return; }

    // 2. 核算时间免单规则 (00:00-08:00出院免当天费)
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    int currentHour = tm_info->tm_hour;

    printf("\n由于系统无法随意跨天流转，请输入实际发生住院天数用于结算: ");
    int actualDays = safeGetPositiveInt();
    int billableDays = actualDays;

    if (currentHour >= 0 && currentHour < 8) {
        printf("【特惠检测】当前办理时间 %02d:00，符合早8点前免收费规定，减免最后一日床位费！\n", currentHour);
        billableDays = (actualDays > 1) ? actualDays - 1 : 0;
    }

    double totalBedFee = billableDays * targetBed->price;

    // 3. 抓取期间所有的处方药花费 (寻找挂在名下Type=3且isPaid=0的账单计入侵入住院总账)
    double totalDrugFee = 0;
    Record* r = recordHead->next;
    while (r) {
        if (strcmp(r->patientId, pId) == 0 && r->type == 3 && r->isPaid == 0) {
            totalDrugFee += r->cost;
            r->isPaid = 1; // 并入总账后标记消除
        }
        r = r->next;
    }

    double totalHospitalCost = totalBedFee + totalDrugFee;

    // 4. 定位Type=5记录进行终极更新
    r = recordHead->next; Record* r5 = NULL;
    while (r) {
        // 找到该患者已付过押金的住院单
        if (strcmp(r->patientId, pId) == 0 && r->type == 5 && r->isPaid == 1) { r5 = r; break; }
        r = r->next;
    }
    if (r5) {
        char outTime[30]; getCurrentTimeStr(outTime, 30);
        // 使用 strstr 和字符串截取替换原有的结尾 (原结尾为 _出院日期:无_总费用:0)
        // 为保持纯C简单安全，我们直接重新组装 description
        sprintf(r5->description, "病房:%s_床位:%d_入院日期:%s_实际天数:%d_押金:%.0f_出院日期:%s_总费用:%.2f",
            targetBed->wardType, targetBed->bedNumber, r5->createTime, actualDays, r5->cost, outTime, totalHospitalCost);
        r5->isPaid = 2; // 标记住院业务大闭环完结
    }

    targetBed->isOccupied = 0;
    strcpy(targetBed->patientId, "");

    printf("\n========== 出院结算单 ==========\n");
    printf("患者: %s | 计费天数: %d 天 | 每日单价: %.2f\n", pId, billableDays, targetBed->price);
    printf("床位总费: %.2f 元\n", totalBedFee);
    printf("期间药费: %.2f 元\n", totalDrugFee);
    printf("住院总计总费用: %.2f 元\n", totalHospitalCost);
    printf("病床 %d 已释放空闲。\n", targetBed->bedNumber);
}

// ---------------------------------------------------------
// 住院部网关路由
// ---------------------------------------------------------
void inpatientMenu(const char* docId) {
    while (1) {
        system("cls");
        printf("\n===== 住院管理调度中心 =====\n");
        printf("1. 办理患者入院 (核算押金/床位分配)\n");
        printf("2. 模拟执行每日早8点查床扣费\n");
        printf("3. 办理患者出院 (特惠与统账流转)\n");
        printf("0. 返回上级医生大厅\n选择: ");
        switch (safeGetInt()) {
        case 1: admitPatient(docId); system("pause"); break;
        case 2: dailyDeductionSimulation(); system("pause"); break;
        case 3: dischargePatient(); system("pause"); break;
        case 0: return;
        }
    }
}