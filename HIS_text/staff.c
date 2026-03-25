#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "staff.h"
#include "utils.h"
#include "schedule.h"
#include "inpatient_department.h"

// ---------------------------------------------------------
// 全局状态保持：记录当前医生正在接诊的患者ID
// ---------------------------------------------------------
char currentCallingPatientId[20] = "";

void generateRecordID(char* buffer) {
    static int recCount = 2000;
    sprintf(buffer, "R2025%04d", recCount++);
}

// ---------------------------------------------------------
// 门诊：接诊叫号 (直接预览该医生的所有排班日期)
// ---------------------------------------------------------
void callPatient(const char* docId) {
    system("cls");
    printf("\n--- 您的排班日期预览 ---\n");

    // 找出该医生未来及今天的排班日期，供其直接选择
    Schedule* s = scheduleList;
    int sCount = 0;
    char availableDates[20][15]; // 假设最多存20个班次

    while (s) {
        if (s->doctor_id == atoi(docId + 1)) { // D101 取数字部分 101
            int duplicate = 0;
            for (int i = 0; i < sCount; i++) {
                if (strcmp(availableDates[i], s->date) == 0) duplicate = 1;
            }
            if (!duplicate) {
                strcpy(availableDates[sCount], s->date);
                sCount++;
            }
        }
        s = s->next;
    }

    if (sCount == 0) {
        printf("您近期暂无排班安排。\n");
        return;
    }

    for (int i = 0; i < sCount; i++) {
        printf("[%d] 日期: %s\n", i + 1, availableDates[i]);
    }
    printf("[0] 返回上一级\n请选择要查看的排班日期编号: ");

    int dChoice = safeGetInt();
    if (dChoice == 0 || dChoice > sCount) return;

    char* targetDate = availableDates[dChoice - 1];

    printf("\n--- [%s] 预约患者名单概览 ---\n", targetDate);
    printf("%-10s %-15s %-15s %-15s %-10s\n", "顺序号", "挂号单流水", "患者ID", "患者姓名", "状态");
    int count = 0;
    Record* r = recordHead->next;

    while (r) {
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && strstr(r->description, targetDate)) {
            Patient* p = patientHead->next;
            char pName[100] = "未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

            char status[30];
            if (r->isPaid == 0) strcpy(status, "待缴费");
            else if (r->isPaid == 1) strcpy(status, "已缴费");
            else strcpy(status, "已接诊");

            // 解析存储在 description 中的排号
            int seqNum = 0;
            char* seqPtr = strstr(r->description, "排号:");
            if (seqPtr) sscanf(seqPtr, "排号:%d", &seqNum);

            printf("%-10d %-15s %-15s %-15s %-10s\n", seqNum, r->recordId, r->patientId, pName, status);
            count++;
        }
        r = r->next;
    }

    if (count == 0) {
        printf("当日暂无患者预约。\n");
        return;
    }

    printf("\n是否呼叫下一位排队患者进入诊室？(1.是 0.否): ");
    if (safeGetInt() == 1) {
        r = recordHead->next;
        int found = 0;
        while (r) {
            // 【核心修改】将判断条件放宽为 r->isPaid != 2，无需缴费即可直接叫号
            if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid != 2 && strstr(r->description, targetDate)) {
                Patient* p = patientHead->next;
                char pName[100] = "未知";
                while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                printf("\n【呼叫系统】请 [%s - %s] 进入诊室就诊!\n", r->patientId, pName);
                r->isPaid = 2; // 闭环挂号单，置为已接诊完结
                found = 1;

                // 【核心】状态保持！记住当前接诊的病人ID
                extern char currentCallingPatientId[20];
                strcpy(currentCallingPatientId, r->patientId);
                printf(">> 系统已自动锁定该患者身份，后续看诊/开药无需重复输入ID。 <<\n");
                break;
            }
            r = r->next;
        }
        if (!found) printf("当前排队患者已全部接诊完毕！\n");
    }
}
// ---------------------------------------------------------
// 门诊：看诊与检查 (自动带入就诊患者)
// ---------------------------------------------------------
void diagnoseAndTest(const char* docId) {
    char pId[20];

    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n当前接诊患者: %s\n", pId);
    }
    else {
        printf("请输入就诊患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    char symptoms[100], diag[100];
    printf("录入临床症状: "); safeGetString(symptoms, 100);
    printf("录入临床诊断: "); safeGetString(diag, 100);

    Record* r2 = (Record*)malloc(sizeof(Record));
    generateRecordID(r2->recordId);
    r2->type = 2; strcpy(r2->patientId, pId); strcpy(r2->staffId, docId);
    r2->cost = 20.0; r2->isPaid = 0;
    sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag);
    getCurrentTimeStr(r2->createTime, 30);
    r2->next = recordHead->next; recordHead->next = r2;
    printf("【通知】已生成包含症状与诊断的 ②看诊记录。\n");

    printf("是否需要开具辅助检查单？(1.是 0.否): ");
    int needTest;
    while (1) { needTest = safeGetInt(); if (needTest == 0 || needTest == 1) break; }

    if (needTest == 1) {
        char testName[50];
        printf("请输入检查名称(如:血常规/X光): "); safeGetString(testName, 50);
        printf("请输入该项检查费用: "); double tCost = safeGetDouble();

        Record* r4 = (Record*)malloc(sizeof(Record));
        generateRecordID(r4->recordId);
        r4->type = 4; strcpy(r4->patientId, pId); strcpy(r4->staffId, docId);
        r4->cost = tCost; r4->isPaid = 0;
        sprintf(r4->description, "检查名称:%s", testName);
        getCurrentTimeStr(r4->createTime, 30);
        r4->next = recordHead->next; recordHead->next = r4;
        printf("【通知】已生成专属 ④检查单记录，已同步至患者待缴费。\n");
    }
}

// ---------------------------------------------------------
// 门诊：处方开药 (支持药品模糊搜索与最大10盒限制)
// ---------------------------------------------------------
void prescribeMedicine(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n当前接诊患者: %s\n", pId);
    }
    else {
        printf("请输入患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    while (1) {
        char key[50];
        printf("\n搜索药品名称或编号关键字 (输入0结束开药): "); safeGetString(key, 50);
        if (strcmp(key, "0") == 0) break;

        // 【模糊搜索并列出所有匹配项】
        Medicine* m = medicineHead->next;
        Medicine* matched[50]; // 最多展示50个结果
        int mCount = 0;

        printf("\n--- 匹配到的药品库 ---\n");
        while (m) {
            if (strstr(m->name, key) || strstr(m->id, key)) {
                matched[mCount] = m;
                printf("[%d] 编号:%s | 名称:%-15s | 单价:%.2f | 库存:%d\n", mCount + 1, m->id, m->name, m->price, m->stock);
                mCount++;
            }
            m = m->next;
        }

        if (mCount == 0) {
            printf("未找到包含该关键字的药品，请重新输入。\n");
            continue;
        }

        printf("请选择要开具的药品编号 (输入0重新搜索): ");
        int mChoice = safeGetInt();
        if (mChoice == 0 || mChoice > mCount) continue;

        Medicine* selectedMed = matched[mChoice - 1];

        // 【开药数量与库存、限购限制】
        int qty;
        while (1) {
            printf("请输入开具数量 (单次最多开10盒): ");
            qty = safeGetPositiveInt();
            if (qty > 10) {
                printf("【警告】医保规定单种药品一次最多开具 10 盒！\n");
            }
            else if (qty > selectedMed->stock) {
                printf("【警告】药房库存不足！当前剩余库存为: %d\n", selectedMed->stock);
            }
            else {
                break; // 满足条件跳出循环
            }
        }

        double totalCost = qty * selectedMed->price;

        Record* r3 = (Record*)malloc(sizeof(Record));
        generateRecordID(r3->recordId);
        r3->type = 3; strcpy(r3->patientId, pId); strcpy(r3->staffId, docId);
        r3->cost = totalCost; r3->isPaid = 0;
        sprintf(r3->description, "药品:%s_单价:%.2f_数量:%d_总价:%.2f", selectedMed->name, selectedMed->price, qty, totalCost);
        getCurrentTimeStr(r3->createTime, 30);
        r3->next = recordHead->next; recordHead->next = r3;
        printf("【成功】③处方记录已生成下发，该药总计 %.2f 元。您可以继续搜索开药。\n", totalCost);
    }
}

// ---------------------------------------------------------
// 门诊：下发住院通知 (新增，标记为 Type 6，作为内部通信不收费)
// ---------------------------------------------------------
void issueAdmissionNotice(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n当前接诊患者: %s\n", pId);
    }
    else {
        printf("请输入需下达住院指令的患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    printf("请判定该患者是否为重症？(1.重症优先 0.普通): ");
    int isSevere;
    while (1) { isSevere = safeGetInt(); if (isSevere == 0 || isSevere == 1) break; }

    printf("请填写简单入院诊断/说明: ");
    char note[100]; safeGetString(note, 100);

    // 下发 Type 6 代表内部的《住院通知单》，无需缴费 (isPaid=0 代表尚未办理入院, 1代表已办理)
    Record* r6 = (Record*)malloc(sizeof(Record));
    generateRecordID(r6->recordId);
    r6->type = 6; strcpy(r6->patientId, pId); strcpy(r6->staffId, docId);
    r6->cost = 0.0; r6->isPaid = 0;
    sprintf(r6->description, "优先级:%s_说明:%s", isSevere ? "重症" : "普通", note);
    getCurrentTimeStr(r6->createTime, 30);
    r6->next = recordHead->next; recordHead->next = r6;

    printf("【成功】住院通知已下发至住院部！(已标记为%s)。\n", isSevere ? "重症" : "普通");
}

// ---------------------------------------------------------
// 工作管理 (修复删除BUG)
// ---------------------------------------------------------
void workManagementMenu(const char* docId) {
    while (1) {
        system("cls");
        printf("\n===== 工作管理 =====\n");

        typedef struct { char id[20]; char name[100]; } PatInfo;
        PatInfo pats[100]; int pCount = 0;

        Record* r = recordHead->next;
        while (r) {
            if (strcmp(r->staffId, docId) == 0) {
                int exists = 0;
                for (int i = 0; i < pCount; i++) {
                    if (strcmp(pats[i].id, r->patientId) == 0) { exists = 1; break; }
                }
                if (!exists) {
                    strcpy(pats[pCount].id, r->patientId);
                    Patient* p = patientHead->next;
                    strcpy(pats[pCount].name, "未知");
                    while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pats[pCount].name, p->name); break; } p = p->next; }
                    pCount++;
                }
            }
            r = r->next;
        }

        if (pCount == 0) { printf("您当前暂无任何接诊记录。\n"); system("pause"); return; }

        printf("--- 您曾接诊过的患者列表 ---\n");
        for (int i = 0; i < pCount; i++) printf("[%d] 患者ID: %s | 姓名: %s\n", i + 1, pats[i].id, pats[i].name);
        printf("[0] 返回上级菜单\n");
        printf("请输入患者对应编号进行操作: ");
        int pChoice = safeGetInt();
        if (pChoice == 0) return;
        if (pChoice < 1 || pChoice > pCount) continue;

        char* targetPid = pats[pChoice - 1].id;

        while (1) {
            system("cls");
            printf("\n--- 患者 %s (%s) 名下由您创建的诊疗记录 ---\n", pats[pChoice - 1].name, targetPid);

            typedef struct { char recId[30]; int type; char desc[300]; } RecInfo;
            RecInfo recs[100]; int rCount = 0;

            r = recordHead->next;
            while (r) {
                if (strcmp(r->staffId, docId) == 0 && strcmp(r->patientId, targetPid) == 0) {
                    strcpy(recs[rCount].recId, r->recordId);
                    recs[rCount].type = r->type;
                    strcpy(recs[rCount].desc, r->description);
                    rCount++;
                }
                r = r->next;
            }

            if (rCount == 0) { printf("该患者名下暂无您的记录。\n"); system("pause"); break; }

            for (int i = 0; i < rCount; i++) {
                char tName[30];
                switch (recs[i].type) {
                case 1: strcpy(tName, "①挂号记录"); break;
                case 2: strcpy(tName, "②看诊记录"); break;
                case 3: strcpy(tName, "③开药记录"); break;
                case 4: strcpy(tName, "④检查记录"); break;
                case 5: strcpy(tName, "⑤住院记录"); break;
                case 6: strcpy(tName, "⑥住院通知单"); break;
                }
                printf("[%d] %s | 流水号: %s | 详情: %s\n", i + 1, tName, recs[i].recId, recs[i].desc);
            }

            printf("\n操作: 1.修改记录 2.删除记录 0.返回重选患者\n请选择: ");
            int op = safeGetInt();
            if (op == 0) break;
            if (op == 1 || op == 2) {
                printf("请输入要操作的【记录编号】: ");
                int rChoice = safeGetInt();
                if (rChoice < 1 || rChoice > rCount) continue;

                char* targetRecId = recs[rChoice - 1].recId;

                // 【核心修复】防止头节点删除导致链表断裂的BUG
                Record* prev = recordHead;
                Record* curr = recordHead->next;
                while (curr) {
                    if (strcmp(curr->recordId, targetRecId) == 0) {
                        if (op == 1) {
                            printf("\n原详情: %s\n", curr->description);
                            printf("请输入新的描述信息(注意不包含空格): ");
                            safeGetString(curr->description, 300);
                            printf("修改成功！(内存级实时同步)\n");
                        }
                        else {
                            prev->next = curr->next; free(curr);
                            printf("删除成功！(内存级实时同步)\n");
                        }
                        break;
                    }
                    prev = curr; curr = curr->next;
                }
                system("pause");
            }
        }
    }
}

// ---------------------------------------------------------
// 医护端总路由
// ---------------------------------------------------------
void staffTerminal(Staff* me) {
    while (1) {
        system("cls");

        // 进大厅自动清空接诊状态
        strcpy(currentCallingPatientId, "");

        printf("\n--- 医生工作台 (%s科: %s医师) ---\n", me->department, me->name);
        printf("1. 门诊业务中心\n2. 住院业务中心 (住院部专网)\n3. 工作管理模块 (患者追踪与记录修改)\n0. 注销退出\n选择: ");
        int c = safeGetInt();

        if (c == 1) {
            while (1) {
                system("cls");
                printf("\n>> 门诊业务中心 <<\n");
                printf("当前接诊锁定患者: %s\n", strlen(currentCallingPatientId) > 0 ? currentCallingPatientId : "【无】");
                printf("1. 接诊预览与叫号\n2. 录入看诊与辅助检查\n3. 开具电子处方\n4. 下发住院通知\n0. 返回上级菜单\n请选择: ");
                int sc = safeGetInt();
                if (sc == 1) { callPatient(me->id); system("pause"); }
                else if (sc == 2) { diagnoseAndTest(me->id); system("pause"); }
                else if (sc == 3) { prescribeMedicine(me->id); system("pause"); }
                else if (sc == 4) { issueAdmissionNotice(me->id); system("pause"); }
                else if (sc == 0) break;
            }
        }
        else if (c == 2) { inpatientMenu(me->id); }
        else if (c == 3) { workManagementMenu(me->id); }
        else if (c == 0) return;
    }
}