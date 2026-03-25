#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "staff.h"
#include "utils.h"

// 引入全新的住院管理模块
#include "inpatient_department.h"

void generateRecordID(char* buffer) {
    static int recCount = 2000;
    sprintf(buffer, "R2025%04d", recCount++);
}

// ---------------------------------------------------------
// 门诊：接诊叫号
// ---------------------------------------------------------
void callPatient(const char* docId) {
    Record* r = recordHead->next;
    int found = 0;
    while (r) {
        // 挂号类型为 1 且已缴费的
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid == 1) {
            printf("【呼叫系统】请患者 %s 进入诊室就诊!\n", r->patientId);
            r->isPaid = 2; // 标记结束
            found = 1;
            break;
        }
        r = r->next;
    }
    if (!found) printf("当前无人排队，或候诊患者尚未在财务端完成挂号缴费。\n");
}

// ---------------------------------------------------------
// 门诊：看诊与检查 (生成规范化的 ②看诊记录 和 ④检查记录)
// ---------------------------------------------------------
void diagnoseAndTest(const char* docId) {
    char pId[20], symptoms[100], diag[100];
    printf("请输入就诊患者ID: "); safeGetString(pId, 20);
    printf("录入临床症状: "); safeGetString(symptoms, 100);
    printf("录入临床诊断: "); safeGetString(diag, 100);

    // 【规范生成 ② 看诊记录】
    Record* r2 = (Record*)malloc(sizeof(Record));
    generateRecordID(r2->recordId);
    r2->type = 2; strcpy(r2->patientId, pId); strcpy(r2->staffId, docId);
    r2->cost = 20.0; r2->isPaid = 0;
    sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag); // 下划线替代空格
    getCurrentTimeStr(r2->createTime, 30);
    r2->next = recordHead->next; recordHead->next = r2;
    printf("【通知】已生成包含症状与诊断的看诊记录。\n");

    printf("是否需要开具辅助检查单？(1.是 0.否): ");
    int needTest;
    while (1) {
        needTest = safeGetInt();
        if (needTest == 0 || needTest == 1) break;
        printf("只能输入 0 或 1，请重新选择: ");
    }

    if (needTest == 1) {
        char testName[50];
        printf("请输入检查名称(如:血常规/X光): "); safeGetString(testName, 50);
        printf("请输入该项检查费用: "); double tCost = safeGetDouble();

        // 【规范生成 ④ 检查记录】
        Record* r4 = (Record*)malloc(sizeof(Record));
        generateRecordID(r4->recordId);
        r4->type = 4; strcpy(r4->patientId, pId); strcpy(r4->staffId, docId);
        r4->cost = tCost; r4->isPaid = 0;
        sprintf(r4->description, "检查名称:%s", testName);
        getCurrentTimeStr(r4->createTime, 30);
        r4->next = recordHead->next; recordHead->next = r4;
        printf("【通知】已生成专属检查记录单，待患者缴费。\n");
    }
}

// ---------------------------------------------------------
// 门诊：处方开药 (生成规范化的 ③开药记录)
// ---------------------------------------------------------
void prescribeMedicine(const char* docId) {
    char pId[20], key[50];
    printf("请输入患者ID: "); safeGetString(pId, 20);
    printf("搜索药品名称或编号: "); safeGetString(key, 50);

    Medicine* m = medicineHead->next;
    while (m) {
        if (strcmp(m->name, key) == 0 || strcmp(m->id, key) == 0) break;
        m = m->next;
    }
    if (!m) { printf("未找到该药品，请核对药房系统!\n"); return; }

    printf("药品单价: %.2f | 当前药房库存: %d\n", m->price, m->stock);
    printf("请输入开具数量: ");
    int qty = safeGetPositiveInt(); // 防呆

    if (qty > m->stock) {
        printf("【拦截警告】药房库存不足!\n"); return;
    }

    double totalCost = qty * m->price;

    // 【规范生成 ③ 开药记录】包含名称、单价、数量、总价
    Record* r3 = (Record*)malloc(sizeof(Record));
    generateRecordID(r3->recordId);
    r3->type = 3; strcpy(r3->patientId, pId); strcpy(r3->staffId, docId);
    r3->cost = totalCost; r3->isPaid = 0;
    sprintf(r3->description, "药品:%s_单价:%.2f_数量:%d_总价:%.2f", m->name, m->price, qty, totalCost);
    getCurrentTimeStr(r3->createTime, 30);
    r3->next = recordHead->next; recordHead->next = r3;
    printf("【成功】处方记录已生成下发，总计 %.2f 元!\n", totalCost);
}

// ---------------------------------------------------------
// 医护端总路由 (整合新建的住院模块)
// ---------------------------------------------------------
void staffTerminal(Staff* me) {
    while (1) {
        system("cls");
        printf("\n--- 医生工作台 (%s科: %s医师) ---\n", me->department, me->name);
        printf("1. 门诊业务中心\n2. 住院业务中心 (住院部专网)\n3. 个人中心\n0. 注销退出\n选择: ");
        int c = safeGetInt();

        if (c == 1) {
            printf(">> 门诊：1.接诊叫号 2.看诊与检查 3.开具处方\n请选择: ");
            int sc = safeGetInt();
            if (sc == 1) callPatient(me->id);
            else if (sc == 2) diagnoseAndTest(me->id);
            else if (sc == 3) prescribeMedicine(me->id);
            system("pause");
        }
        else if (c == 2) {
            // 调用单独的住院部管理模块
            inpatientMenu(me->id);
        }
        else if (c == 3) {
            printf("\n> 个人档案 <\n工号: %s\n科室: %s\n职称: %s\n", me->id, me->department, me->level);
            system("pause");
        }
        else if (c == 0) return;
    }
}