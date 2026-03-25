#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"
#include "inpatient_department.h"

// ---------------------------------------------------------
// 内部工具：生成病房数据字典 (病房号-床位号)
// ---------------------------------------------------------
void initBedsIfEmpty() {
    if (bedHead->next) return;
    char types[5][50] = { "单人病房", "双人病房", "三人病房", "单人陪护病房", "单人陪护疗养病房" };
    double prices[5] = { 200.0, 150.0, 100.0, 300.0, 500.0 };
    char wards[2][50] = { "普通病房", "特殊病房" };

    Bed* tail = bedHead;
    int roomNum = 1;
    for (int i = 0; i < 5; i++) {
        for (int j = 1; j <= 4; j++) { // 假设每种类型占据一个病房，房内有4张床
            Bed* b = (Bed*)malloc(sizeof(Bed));
            sprintf(b->bedId, "%d-%d", roomNum, j); // 生成如 1-1, 1-2
            b->isOccupied = 0; strcpy(b->patientId, ""); strcpy(b->bedType, types[i]);
            b->price = prices[i]; strcpy(b->wardType, (i >= 3) ? wards[1] : wards[0]);
            b->isRoundsDone = 0; // 默认未查房
            b->next = NULL; tail->next = b; tail = b;
        }
        roomNum++;
    }
}

void checkAndAdjustBedTension() {
    int total = 0, empty = 0;
    Bed* b = bedHead->next;
    while (b) { total++; if (b->isOccupied == 0) empty++; b = b->next; }

    if (total > 0 && ((float)empty / total) < 0.2f) {
        printf("\n【系统告警】当前空床不足 20%%，自动将单人疗养病房转为双人病房！\n");
        b = bedHead->next; int converted = 0;
        while (b) {
            if (b->isOccupied == 0 && strcmp(b->bedType, "单人陪护疗养病房") == 0) {
                strcpy(b->bedType, "双人病房"); b->price = 150.0;

                Bed* extra = (Bed*)malloc(sizeof(Bed)); *extra = *b;
                // 为附属床位生成新编号 (如 5-1 转为 5-1A)
                sprintf(extra->bedId, "%sA", b->bedId);
                extra->next = bedHead->next; bedHead->next = extra;
                converted++;
            }
            b = b->next;
        }
    }
}

void viewAllBeds() {
    printf("\n========== 全院病房与床位实时使用图谱 ==========\n");
    printf("%-10s %-12s %-18s %-8s %-15s %-15s\n", "房号-床位", "病房区域", "房型", "价格", "状态", "入住患者");

    initBedsIfEmpty();
    Bed* b = bedHead->next;
    while (b) {
        char patName[100] = "无";
        if (b->isOccupied) {
            Patient* p = patientHead->next;
            while (p) { if (strcmp(p->id, b->patientId) == 0) { strcpy(patName, p->name); break; } p = p->next; }
        }
        printf("%-10s %-12s %-18s %-8.2f %-15s %s(%s)\n",
            b->bedId, b->wardType, b->bedType, b->price,
            b->isOccupied ? "[占用]" : "[空闲]", patName, b->patientId);
        b = b->next;
    }
    printf("===============================================\n");
}

void admitPatient(const char* docId) {
    initBedsIfEmpty(); checkAndAdjustBedTension();

    printf("\n--- 门诊下发《待入院通知单》队列 ---\n");
    Record* r = recordHead->next; int noticeCount = 0;

    printf("【重症优先通道】\n");
    while (r) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "重症")) {
            Patient* p = patientHead->next; char pName[100] = "未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }
            printf(" -> [紧急] 患者ID: %s | 姓名: %s | 说明: %s\n", r->patientId, pName, r->description);
            noticeCount++;
        }
        r = r->next;
    }

    printf("\n【普通入院队列】\n");
    r = recordHead->next;
    while (r) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "普通")) {
            Patient* p = patientHead->next; char pName[100] = "未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }
            printf(" -> [常规] 患者ID: %s | 姓名: %s | 说明: %s\n", r->patientId, pName, r->description);
            noticeCount++;
        }
        r = r->next;
    }

    if (noticeCount == 0) printf("当前暂无门诊下发的住院通知单。\n");

    char pId[20];
    printf("\n请输入需办理入院的患者ID (0返回): "); safeGetString(pId, 20);
    if (strcmp(pId, "0") == 0) return;

    Record* targetNotice = NULL; r = recordHead->next;
    while (r) { if (r->type == 6 && strcmp(r->patientId, pId) == 0 && r->isPaid == 0) { targetNotice = r; break; } r = r->next; }

    if (!targetNotice) {
        Bed* b = bedHead->next; int alreadyIn = 0;
        while (b) { if (b->isOccupied && strcmp(b->patientId, pId) == 0) { alreadyIn = 1; break; } b = b->next; }
        if (alreadyIn) printf("【拦截警告】该患者已经处于住院状态中，请勿重复办理！\n");
        else printf("【拦截警告】缺乏门诊开具的住院通知，无法强行收治入院！\n");
        return;
    }

    Patient* p = patientHead->next; Patient* targetPat = NULL;
    while (p) { if (strcmp(p->id, pId) == 0) { targetPat = p; break; } p = p->next; }
    if (!targetPat) { printf("患者不存在。\n"); return; }

    printf("请输入拟住院天数: "); int days = safeGetPositiveInt();

    int baseDeposit = 200 * days;
    if (baseDeposit < 1000) baseDeposit = 1000;
    int finalDeposit = ((baseDeposit + 99) / 100) * 100;

    printf("按规则系统核算，需缴纳住院押金: %d 元。\n", finalDeposit);

    int isPaid = 0;
    if (targetPat->balance >= finalDeposit) {
        targetPat->balance -= finalDeposit; isPaid = 1;
        printf("已成功从患者账户划扣押金，住院立即生效。\n");
    }
    else {
        printf("【提示】患者余额不足！将生成待缴费住院押金账单，请提醒前往财务缴费。\n");
    }

    printf("\n可用空闲病床列表:\n");
    Bed* b = bedHead->next; int hasEmpty = 0;
    while (b) {
        if (!b->isOccupied) { printf("[%s] %s - %s (每日 %.2f)\n", b->bedId, b->wardType, b->bedType, b->price); hasEmpty = 1; }
        b = b->next;
    }
    if (!hasEmpty) { printf("全院床位已满！\n"); if (isPaid) targetPat->balance += finalDeposit; return; }

    char selectBed[20];
    printf("请输入分配的床位号 (如 1-3): "); safeGetString(selectBed, 20);
    b = bedHead->next; Bed* finalBed = NULL;
    while (b) { if (strcmp(b->bedId, selectBed) == 0 && !b->isOccupied) { finalBed = b; break; } b = b->next; }
    if (!finalBed) { printf("无效床位。\n"); if (isPaid) targetPat->balance += finalDeposit; return; }

    finalBed->isOccupied = 1; strcpy(finalBed->patientId, pId);
    finalBed->isRoundsDone = 0; // 新入院重置查房状态
    targetNotice->isPaid = 1;

    Record* r5 = (Record*)malloc(sizeof(Record));
    extern void generateRecordID(char* buffer);
    generateRecordID(r5->recordId);
    r5->type = 5; strcpy(r5->patientId, pId); strcpy(r5->staffId, docId);
    r5->cost = finalDeposit; r5->isPaid = isPaid;

    char adminTime[30]; getCurrentTimeStr(adminTime, 30);
    sprintf(r5->description, "病房:%s_床位:%s_入院日期:%s_预期天数:%d_押金:%d_出院日期:无_总费用:0",
        finalBed->wardType, finalBed->bedId, adminTime, days, finalDeposit);
    getCurrentTimeStr(r5->createTime, 30);

    r5->next = recordHead->next; recordHead->next = r5;

    if (isPaid) printf("【成功】已成功办理入院，关联床位 %s。\n", finalBed->bedId);
    else printf("【待缴费】已预留床位 %s，并生成⑤住院账单，同步至患者端。\n", finalBed->bedId);
}

// ---------------------------------------------------------
// 日常查房与开医嘱 (支持查房状态标记与重症显示)
// ---------------------------------------------------------
void wardRounds(const char* docId) {
    while (1) {
        system("cls");
        printf("\n--- 住院部查房名单 ---\n");
        printf("%-10s %-15s %-10s %-10s %-10s\n", "房-床号", "患者ID", "姓名", "类型", "查房状态");

        Bed* b = bedHead->next;
        int pCount = 0;
        while (b) {
            if (b->isOccupied) {
                Patient* p = patientHead->next; char pName[100] = "未知";
                while (p) { if (strcmp(p->id, b->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                // 判断是否重症 (通过追溯最后一次 Type 6 通知单)
                char severity[10] = "普通";
                Record* r = recordHead->next;
                while (r) {
                    if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                        if (strstr(r->description, "重症")) strcpy(severity, "重症");
                    }
                    r = r->next;
                }

                printf("%-10s %-15s %-10s %-10s %-10s\n",
                    b->bedId, b->patientId, pName, severity, b->isRoundsDone ? "[已查房]" : "[未查房]");
                pCount++;
            }
            b = b->next;
        }

        if (pCount == 0) { printf("当前无住院患者。\n"); system("pause"); return; }

        char pId[20];
        printf("\n请输入需查房的住院患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;

        b = bedHead->next; Bed* targetBed = NULL;
        while (b) { if (b->isOccupied && strcmp(b->patientId, pId) == 0) { targetBed = b; break; } b = b->next; }
        if (!targetBed) { printf("该患者未办理住院或输入ID有误。\n"); system("pause"); continue; }

        printf("\n--- 对患者 %s 的查房选项 ---\n1. 下达日常医嘱笔记\n2. 开具住院药品并计费\n0. 返回\n请选择: ");
        int choice = safeGetInt();

        if (choice == 1) {
            printf("请输入查房医嘱记录(无空格): ");
            char note[200]; safeGetString(note, 200);
            Record* rx = (Record*)malloc(sizeof(Record));
            extern void generateRecordID(char* buffer);
            generateRecordID(rx->recordId);
            rx->type = 2; strcpy(rx->patientId, pId); strcpy(rx->staffId, docId);
            rx->cost = 0; rx->isPaid = 1;
            sprintf(rx->description, "住院查房医嘱:%s", note);
            getCurrentTimeStr(rx->createTime, 30);
            rx->next = recordHead->next; recordHead->next = rx;
            printf("查房医嘱记录已保存。\n");
            targetBed->isRoundsDone = 1; // 标记为已查房
            system("pause");
        }
        else if (choice == 2) {
            extern char currentCallingPatientId[20];
            strcpy(currentCallingPatientId, pId);
            extern void prescribeMedicine(const char* docId);
            prescribeMedicine(docId);
            strcpy(currentCallingPatientId, "");
            targetBed->isRoundsDone = 1;
            system("pause");
        }
    }
}

// ... 每日查床扣费 (dailyDeductionSimulation) 此时也应顺便将所有 isRoundsDone 设为 0 ...
void dailyDeductionSimulation() {
    printf("\n--- 模拟执行每日 08:00 床位费划扣 ---\n");
    Bed* b = bedHead->next; int count = 0;
    while (b) {
        if (b->isOccupied) {
            Patient* p = patientHead->next;
            while (p) {
                if (strcmp(p->id, b->patientId) == 0) {
                    if (p->balance < 1000) printf("【警告】患者 %s 余额 (%.2f) 低于1000元，请立刻停药并催缴！\n", p->name, p->balance);
                    printf("患者 %s (%s床) 已记账当日床位费 %.2f 元。\n", p->name, b->bedId, b->price);
                    count++; break;
                }
                p = p->next;
            }
            b->isRoundsDone = 0; // 【新增】新的一天重置查房状态
        }
        b = b->next;
    }
    printf("执行完毕，共扫描处理 %d 名在院患者，查房状态已重置。\n", count);
}

// ... 出院逻辑与之前基本一致，注意替换 description 里的 bedId 为字符串格式 ...
void dischargePatient() {
    // ... 省略重复的输入获取和校验 ...
    char pId[20];
    printf("请输入要办理出院的患者ID (0返回): "); safeGetString(pId, 20);
    if (strcmp(pId, "0") == 0) return;

    Bed* b = bedHead->next; Bed* targetBed = NULL;
    while (b) { if (b->isOccupied && strcmp(b->patientId, pId) == 0) { targetBed = b; break; } b = b->next; }
    if (!targetBed) { printf("该患者当前并未住院。\n"); return; }

    time_t t = time(NULL); struct tm* tm_info = localtime(&t);
    int currentHour = tm_info->tm_hour;

    printf("\n由于系统无法跨天流转，请输入实际发生住院天数用于结算: ");
    int actualDays = safeGetPositiveInt();
    int billableDays = actualDays;

    if (currentHour >= 0 && currentHour < 8) {
        printf("【特惠】当前办理时间 %02d:00，符合早8点前免收规定，减免最后一日床位费！\n", currentHour);
        billableDays = (actualDays > 1) ? actualDays - 1 : 0;
    }

    double totalBedFee = billableDays * targetBed->price;
    double totalDrugFee = 0;
    Record* r = recordHead->next;
    while (r) { if (strcmp(r->patientId, pId) == 0 && r->type == 3 && r->isPaid == 0) { totalDrugFee += r->cost; r->isPaid = 1; } r = r->next; }

    double totalHospitalCost = totalBedFee + totalDrugFee;
    r = recordHead->next; Record* r5 = NULL;
    while (r) { if (strcmp(r->patientId, pId) == 0 && r->type == 5 && r->isPaid == 1) { r5 = r; break; } r = r->next; }

    if (r5) {
        char outTime[30]; getCurrentTimeStr(outTime, 30);
        sprintf(r5->description, "病房:%s_床位:%s_入院日期:%s_实际天数:%d_押金:%.0f_出院日期:%s_总费用:%.2f",
            targetBed->wardType, targetBed->bedId, r5->createTime, actualDays, r5->cost, outTime, totalHospitalCost); // 这里的 bedId 是 %s
        r5->isPaid = 2;
        if (r5->cost > totalHospitalCost) {
            double refund = r5->cost - totalHospitalCost;
            Patient* p = patientHead->next;
            while (p) { if (strcmp(p->id, pId) == 0) { p->balance += refund; break; } p = p->next; }
            printf("\n【退款通知】经系统清算，押金结余 %.2f 元。已实时原路退回至患者账户余额！\n", refund);
        }
        else if (r5->cost < totalHospitalCost) {
            printf("\n【补缴通知】经系统清算，押金透支 %.2f 元。请通知患者前往财务中心补缴差额！\n", totalHospitalCost - r5->cost);
        }
    }

    targetBed->isOccupied = 0; strcpy(targetBed->patientId, "");
    printf("\n========== 出院结算单 ==========\n患者: %s | 床位总费: %.2f | 期间药费: %.2f | 总费用: %.2f\n病床 %s 已释放空闲。\n", pId, totalBedFee, totalDrugFee, totalHospitalCost, targetBed->bedId);
}

void inpatientMenu(const char* docId) {
    while (1) {
        system("cls");
        printf("\n===== 住院管理调度中心 =====\n");
        printf("1. 查看全院病房实时图谱\n");
        printf("2. 办理患者入院 (核算押金/床位分配)\n");
        printf("3. 执行每日早8点查床扣费\n");
        printf("4. 日常查房与下达医嘱记录\n");
        printf("5. 办理患者出院 (特惠与统账流转)\n");
        printf("0. 返回上级医生大厅\n选择: ");
        switch (safeGetInt()) {
        case 1: viewAllBeds(); system("pause"); break;
        case 2: admitPatient(docId); system("pause"); break;
        case 3: dailyDeductionSimulation(); system("pause"); break;
        case 4: wardRounds(docId); break; // wardRounds 里有自己的循环
        case 5: dischargePatient(); system("pause"); break;
        case 0: return;
        }
    }
}