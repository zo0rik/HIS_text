#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "patient.h"
#include "utils.h"
#include "doctor.h"
#include "schedule.h"

void generatePatientID(char* idBuffer) {
    static int counter = 1000;
    sprintf(idBuffer, "P2025%04d", counter++);
}

Patient* findPatientById(const char* pid) {
    Patient* p = patientHead->next;
    while (p != NULL) { if (strcmp(p->id, pid) == 0) return p; p = p->next; }
    return NULL;
}

void registerPatient() {
    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    newPatient->next = NULL;

    printf("\n--- 账户注册与建档 ---\n");
    printf("请选择就诊类型 (1.普通 2.急诊 0.返回): ");
    int type;
    while (1) {
        type = safeGetInt();
        if (type == 0) { free(newPatient); return; }
        if (type == 1 || type == 2) break;
        printf("【输入错误】只能输入 1 或 2，请重新选择: ");
    }
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    printf("请输入姓名: "); safeGetString(newPatient->name, 100);
    printf("请设置登录密码: "); safeGetString(newPatient->password, 50);
    printf("请输入性别 (男性/女性): "); safeGetGender(newPatient->gender, 10);

    if (!newPatient->isEmergency) {
        printf("请输入年龄: "); newPatient->age = safeGetPositiveInt();
        printf("请输入过敏史(无则填无): "); safeGetString(newPatient->allergy, 100);
    }
    else {
        newPatient->age = -1; strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id); newPatient->balance = 0.0;
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;
    printf("【成功】档案建立成功！您的登录账号为: %s\n", newPatient->id);
}

// ---------------------------------------------------------
// 业务二：自助挂号 (极限规则校验、动态科室抓取与排队号抢占)
// ---------------------------------------------------------
void bookAppointment(const char* currentPatientId) {
    char today[11], nextWeek[11];
    time_t t = time(NULL); struct tm* tm_info = localtime(&t);
    strftime(today, sizeof(today), "%Y-%m-%d", tm_info);
    t += 7 * 24 * 60 * 60; tm_info = localtime(&t);
    strftime(nextWeek, sizeof(nextWeek), "%Y-%m-%d", tm_info);

    while (1) {
        system("cls");
        printf("\n--- 自助预约挂号 ---\n");
        printf("1. 按科室查找未来一周可预约排班\n");
        printf("2. 搜索医生姓名/工号查找未来一周排班\n");
        printf("0. 返回上级菜单\n");
        printf("请选择您的查号方式: ");
        int choice = safeGetInt();
        if (choice == 0) return;

        char keyword[50];
        if (choice == 1) {
            // 动态抓取当前存在的科室 (遍历医生链表去重)
            char depts[20][50]; int dCount = 0;
            Staff* stf = staffHead->next;
            while (stf) {
                int exists = 0;
                for (int i = 0; i < dCount; i++) {
                    if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; }
                }
                if (!exists && strlen(stf->department) > 0) {
                    strcpy(depts[dCount], stf->department);
                    dCount++;
                }
                stf = stf->next;
            }

            // 动态展示抓取到的科室供患者参考
            printf("\n当前存在科室: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);
            printf("\n请输入目标科室名称: ");
            safeGetString(keyword, 50);
        }
        else if (choice == 2) {
            printf("请输入医生姓名或工号 (如: 李四 或 1001): ");
            safeGetString(keyword, 50);
        }
        else continue;

        // 排班表展示 (增加医生工号显示)
        printf("\n--- 未来一周可预约排班表 (%s 至 %s) ---\n", today, nextWeek);
        printf("%-8s %-12s %-10s %-18s %-12s %-10s\n", "排班ID", "日期", "班次", "医生(工号)", "科室", "级别");

        int found = 0; Schedule* s = scheduleList;
        while (s) {
            if (strcmp(s->date, today) < 0 || strcmp(s->date, nextWeek) > 0) { s = s->next; continue; }
            Doctor* d = doctorList; Doctor* matchedDoc = NULL;
            while (d) { if (d->id == s->doctor_id) { matchedDoc = d; break; } d = d->next; }
            if (!matchedDoc) { s = s->next; continue; }

            int match = 0;
            if (choice == 1 && strstr(matchedDoc->department, keyword)) match = 1;
            if (choice == 2) {
                char docIdStr[20]; sprintf(docIdStr, "%d", matchedDoc->id);
                if (strstr(matchedDoc->name, keyword) || strcmp(docIdStr, keyword) == 0) match = 1;
            }

            if (match && strcmp(s->shift, "休息") != 0) {
                // 拼接医生姓名与工号
                char docDisp[50];
                sprintf(docDisp, "%s(%d)", matchedDoc->name, matchedDoc->id);
                printf("%-8d %-12s %-10s %-18s %-12s %-10s\n",
                    s->schedule_id, s->date, s->shift, docDisp, matchedDoc->department, matchedDoc->title);
                found++;
            }
            s = s->next;
        }

        if (found == 0) { printf("未找到匹配的排班信息。\n"); system("pause"); continue; }

        printf("\n请输入要预约的【排班ID】 (输入0返回): ");
        int targetSchId = safeGetInt();
        if (targetSchId == 0) continue;

        Schedule* targetSch = NULL; s = scheduleList;
        while (s) { if (s->schedule_id == targetSchId) { targetSch = s; break; } s = s->next; }
        if (!targetSch) { printf("无效的排班ID！\n"); system("pause"); continue; }

        Doctor* targetDoc = NULL; Doctor* d = doctorList;
        while (d) { if (d->id == targetSch->doctor_id) { targetDoc = d; break; } d = d->next; }
        if (!targetDoc) { printf("【数据异常】该医生不存在！\n"); system("pause"); continue; }

        char staffIdStr[20]; sprintf(staffIdStr, "D%d", targetDoc->id);

        // ==========================================
        // 核心统筹：将所有约束精准限定到“同一日期”
        // ==========================================
        int patientDailyActive = 0;   // 规则：同日个人最多挂5个号
        int patientDeptDailyActive = 0; // 规则：同日同科室最多1个
        int sameDocSameDay = 0;       // 规则：同日不能重复挂同一医生
        int docDailyCount = 0;        // 规则：医生单日最多50人
        int hospitalDailyCount = 0;   // 规则：全院单日最多1000号

        Record* rec = recordHead->next;
        while (rec) {
            // 只统计目标日期的挂号记录
            if (rec->type == 1 && strstr(rec->description, targetSch->date)) {
                hospitalDailyCount++;
                if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++;

                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                    patientDailyActive++;

                    // 追溯看这个单子对应哪个科室，防止同科室同日挂多个
                    Doctor* recDoc = doctorList;
                    while (recDoc) {
                        char tempDId[20]; sprintf(tempDId, "D%d", recDoc->id);
                        if (strcmp(tempDId, rec->staffId) == 0) {
                            if (strcmp(recDoc->department, targetDoc->department) == 0) patientDeptDailyActive++;
                            if (strcmp(tempDId, staffIdStr) == 0) sameDocSameDay = 1;
                            break;
                        }
                        recDoc = recDoc->next;
                    }
                }
            }
            rec = rec->next;
        }

        // 拦截机制
        if (hospitalDailyCount >= 1000) { printf("【超载】该日全院挂号总数已达1000上限！\n"); system("pause"); continue; }
        if (patientDailyActive >= 5) { printf("【限制】您在 %s 已预约 %d 个号，已达单日最高上限（5个）！\n", targetSch->date, patientDailyActive); system("pause"); continue; }
        if (patientDeptDailyActive >= 1) { printf("【限制】您在 %s 已挂过 [%s] 的号，同日同科室最多挂一个！\n", targetSch->date, targetDoc->department); system("pause"); continue; }
        if (sameDocSameDay) { printf("【限制】您已挂过 %s 医生在 %s 的号，不允许同日重复挂号！\n", targetDoc->name, targetSch->date); system("pause"); continue; }

        if (docDailyCount >= 50) {
            printf("\n【排队已满】%s 医生在 %s 候诊队列已满 (50人)！\n", targetDoc->name, targetSch->date);
            printf(">>> 智能AI推荐系统为您匹配以下备选方案 <<<\n");

            int recCount = 0; Schedule* altS = scheduleList;
            printf("\n[推荐一：该医生 %s 的其他出诊时间]\n", targetDoc->name);
            while (altS) {
                if (altS->doctor_id == targetDoc->id && strcmp(altS->date, targetSch->date) != 0 && strcmp(altS->shift, "休息") != 0) {
                    if (strcmp(altS->date, today) >= 0 && strcmp(altS->date, nextWeek) <= 0) {
                        printf(" - 排班ID [%d] 日期: %s 班次: %s\n", altS->schedule_id, altS->date, altS->shift); recCount++;
                    }
                }
                altS = altS->next;
            }

            printf("\n[推荐二：同科室 [%s] 其他医生 (%s当日)]\n", targetDoc->department, targetSch->date);
            altS = scheduleList;
            while (altS) {
                if (strcmp(altS->date, targetSch->date) == 0 && altS->doctor_id != targetDoc->id && strcmp(altS->shift, "休息") != 0) {
                    Doctor* altD = doctorList;
                    while (altD) {
                        if (altD->id == altS->doctor_id && strcmp(altD->department, targetDoc->department) == 0) {
                            printf(" - 排班ID [%d] 医生: %s 班次: %s\n", altS->schedule_id, altD->name, altS->shift); recCount++; break;
                        }
                        altD = altD->next;
                    }
                }
                altS = altS->next;
            }
            if (recCount == 0) printf("暂无其他可推荐排班，请改日再试。\n");
            system("pause"); continue;
        }

        // 生成单据部分 (无需缴费，直接占用排队号)
        int seqNum = docDailyCount + 1;
        double regFee = strstr(targetDoc->title, "主任") != NULL ? 50.0 : 15.0;

        Record* newRecord = (Record*)malloc(sizeof(Record));
        static int regCount = 5000; sprintf(newRecord->recordId, "REG2025%04d", regCount++);
        newRecord->type = 1;
        strcpy(newRecord->patientId, currentPatientId); strcpy(newRecord->staffId, staffIdStr);
        newRecord->cost = regFee;
        newRecord->isPaid = 0; // 0 是待缴费，但不影响其顺序号的下发

        // 排号内嵌到描述中，供医生端解析
        sprintf(newRecord->description, "挂号:%s(%s)_排号:%d", targetDoc->name, targetSch->date, seqNum);
        getCurrentTimeStr(newRecord->createTime, 30);
        newRecord->next = NULL;

        Record* temp = recordHead;
        while (temp->next) temp = temp->next;
        temp->next = newRecord;

        printf("\n【挂号成功】您已成功抢占 %s 医师 的号源！费用 %.2f 元。\n", targetDoc->name, regFee);
        printf(">>> 您在当日的专属候诊顺序为：【第 %d 号】 <<<\n", seqNum);
        printf("注：请在就诊前前往财务中心完成缴费，否则医生端将无法呼叫。\n");
        system("pause"); return;
    }
}
void financeCenter(const char* currentPatientId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentPatientId);
        if (!p) return;

        printf("\n--- 财务中心 (当前余额: %.2f) ---\n", p->balance);
        Record* rec = recordHead->next; int hasUnpaid = 0;
        while (rec) {
            if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                // 住院单据(Type 5) 也能在这里缴费了！
                char typeName[20];
                switch (rec->type) {
                case 1: strcpy(typeName, "挂号费"); break;
                case 2: strcpy(typeName, "诊疗费"); break;
                case 3: strcpy(typeName, "药费"); break;
                case 4: strcpy(typeName, "检查费"); break;
                case 5: strcpy(typeName, "住院押金"); break;
                }
                printf("单号: %s | [%s] %s | 金额: %.2f元\n", rec->recordId, typeName, rec->description, rec->cost);
                hasUnpaid = 1;
            }
            rec = rec->next;
        }
        if (!hasUnpaid) { printf("无待缴费账单。\n"); system("pause"); return; }

        char target[30];
        printf("\n输入要缴费的单号 (输入0返回): "); safeGetString(target, 30);
        if (strcmp(target, "0") == 0) return;

        Record* tRec = NULL; rec = recordHead->next;
        while (rec) {
            if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { tRec = rec; break; }
            rec = rec->next;
        }
        if (!tRec) { printf("单号错误。\n"); system("pause"); continue; }

        while (p->balance < tRec->cost) {
            printf("余额不足！差额 %.2f，请输入充值金额 (0取消): ", tRec->cost - p->balance);
            double money = safeGetDouble();
            if (money == 0) break;
            if (money > 0) p->balance += money;
        }

        if (p->balance >= tRec->cost) {
            p->balance -= tRec->cost; tRec->isPaid = 1;
            printf("【缴费成功】已扣款。当前账户余额: %.2f\n", p->balance);

            // 内存实时同步药品扣减
            char mName[50]; int qty;
            if (sscanf(tRec->description, "药品:%[^_]_单价:%*f_数量:%d", mName, &qty) == 2) {
                Medicine* m = medicineHead->next;
                while (m) {
                    if (strcmp(m->name, mName) == 0) { m->stock -= qty; break; }
                    m = m->next;
                }
            }
        }
        system("pause");
    }
}

void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n--- 医疗档案 ---\n1. 挂号记录\n2. 看诊与检查记录\n3. 住院记录\n0. 返回上级菜单\n选择: ");
        int c = safeGetInt();
        if (c == 0) return;

        Record* rec = recordHead->next; int printed = 0;
        while (rec) {
            if (strcmp(rec->patientId, currentPatientId) == 0) {
                if ((c == 1 && rec->type == 1) || (c == 2 && (rec->type == 2 || rec->type == 3 || rec->type == 4)) || (c == 3 && rec->type == 5)) {
                    printf("%s | %s | %.2f | %s\n", rec->recordId, rec->description, rec->cost, rec->isPaid ? "已处理/已缴费" : "待缴费");
                    printed = 1;
                }
            }
            rec = rec->next;
        }
        if (!printed) printf("无相关记录。\n");
        system("pause");
    }
}

void userTerminal(const char* currentId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentId);
        printf("\n--- 患者自助终端 (当前登录患者: %s - %s) ---\n", p->name, p->id);
        printf("1. 自助预约挂号\n2. 财务中心缴费\n3. 医疗档案查询\n0. 注销并返回大厅\n选择: ");
        switch (safeGetInt()) {
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 0: return;
        }
    }
}