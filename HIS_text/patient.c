#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "patient.h"
#include "utils.h"

// 内部工具：生成自增的患者ID
void generatePatientID(char* idBuffer) {
    static int counter = 1000;
    sprintf(idBuffer, "P2025%04d", counter++);
}

// 内部工具：通过ID查找患者实体
Patient* findPatientById(const char* pid) {
    Patient* p = patientHead->next;
    while (p != NULL) {
        if (strcmp(p->id, pid) == 0) return p;
        p = p->next;
    }
    return NULL;
}

// ---------------------------------------------------------
// 业务一：患者注册建档 (增加密码设置)
// ---------------------------------------------------------
void registerPatient() {
    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    newPatient->next = NULL;

    printf("\n--- 账户注册与建档 ---\n");
    printf("请选择就诊类型 (1.普通 2.急诊): ");
    int type = safeGetInt();
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    printf("请输入姓名: "); safeGetString(newPatient->name, 100);
    printf("请设置登录密码: "); safeGetString(newPatient->password, 50); // 新增的密码录入
    printf("请输入性别: "); safeGetString(newPatient->gender, 10);

    // 急诊跳过繁琐信息
    if (!newPatient->isEmergency) {
        printf("请输入年龄: "); newPatient->age = safeGetInt();
        printf("请输入过敏史(无则填无): "); safeGetString(newPatient->allergy, 100);
    }
    else {
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id); // 分配ID
    newPatient->balance = 0.0;         // 初始余额为0

    // 挂载到链表尾部
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("【成功】档案建立成功！您的登录账号为: %s\n", newPatient->id);
}

// ---------------------------------------------------------
// 业务二：自助挂号 (利用透传的 currentPatientId，无需再输ID)
// ---------------------------------------------------------
void bookAppointment(const char* currentPatientId) {
    printf("\n--- 自助挂号 (科室与医生大屏) ---\n");
    Staff* doctor = staffHead->next;
    while (doctor) {
        printf("工号:%-10s 姓名:%-10s 科室:%-10s 级别:%s\n", doctor->id, doctor->name, doctor->department, doctor->level);
        doctor = doctor->next;
    }

    char targetDoctorId[20];
    printf("输入要预约的医生工号 (0退出): "); safeGetString(targetDoctorId, 20);
    if (strcmp(targetDoctorId, "0") == 0) return;

    // 查找目标医生
    doctor = staffHead->next;
    Staff* selectedDoctor = NULL;
    while (doctor) {
        if (strcmp(doctor->id, targetDoctorId) == 0) { selectedDoctor = doctor; break; }
        doctor = doctor->next;
    }
    if (!selectedDoctor) { printf("医生不存在！\n"); system("pause"); return; }

    // 【满员检测机制】统计医生名下未处理的挂号单
    int pendingCount = 0;
    Record* rec = recordHead->next;
    while (rec) {
        if (strcmp(rec->staffId, selectedDoctor->id) == 0 && rec->type == 1 && rec->isPaid == 0) pendingCount++;
        rec = rec->next;
    }

    // 超过3人触发推荐机制
    if (pendingCount >= 3) {
        printf("\n【警告】%s 医生排队已满！正在推荐...\n", selectedDoctor->name);
        Staff* alt = staffHead->next;
        Staff* recDoc = NULL;
        while (alt) {
            // 找同科室的另外一名医生
            if (strcmp(alt->department, selectedDoctor->department) == 0 && strcmp(alt->id, selectedDoctor->id) != 0) {
                recDoc = alt; break;
            }
            alt = alt->next;
        }
        if (recDoc) {
            printf("推荐同科室医生: %s，确认预约? (1.是 0.否): ", recDoc->name);
            if (safeGetInt() == 1) selectedDoctor = recDoc; // 接受推荐
            else return; // 拒绝则退出
        }
    }

    // 根据医生级别动态定价
    double regFee = strstr(selectedDoctor->level, "主任") != NULL ? 50.0 : 15.0;

    // 生成流水账单
    Record* newRecord = (Record*)malloc(sizeof(Record));
    static int regCount = 5000;
    sprintf(newRecord->recordId, "REG2025%04d", regCount++);
    newRecord->type = 1;
    strcpy(newRecord->patientId, currentPatientId); // 直接使用当前登录ID
    strcpy(newRecord->staffId, selectedDoctor->id);
    newRecord->cost = regFee;
    newRecord->isPaid = 0;
    sprintf(newRecord->description, "挂号费:%s", selectedDoctor->name);
    newRecord->next = NULL;

    Record* temp = recordHead;
    while (temp->next) temp = temp->next;
    temp->next = newRecord;

    printf("【挂号成功】预约 %s 医师，费用 %.2f 元，请前往财务中心缴费。\n", selectedDoctor->name, regFee);
    system("pause");
}

// ---------------------------------------------------------
// 业务三：财务费用中心 (一键扣款与药房库存联动)
// ---------------------------------------------------------
void financeCenter(const char* currentPatientId) {
    Patient* p = findPatientById(currentPatientId);
    if (!p) return;

    printf("\n--- 财务中心 (当前余额: %.2f) ---\n", p->balance);

    // 检索该患者所有未缴费账单
    Record* rec = recordHead->next;
    int hasUnpaid = 0;
    while (rec) {
        if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
            printf("单号: %s | 描述: %s | 金额: %.2f元\n", rec->recordId, rec->description, rec->cost);
            hasUnpaid = 1;
        }
        rec = rec->next;
    }
    if (!hasUnpaid) { printf("无待缴费账单。\n"); system("pause"); return; }

    char target[30];
    printf("输入要缴费的单号 (0退出): "); safeGetString(target, 30);
    if (strcmp(target, "0") == 0) return;

    // 定位目标账单
    Record* tRec = NULL;
    rec = recordHead->next;
    while (rec) {
        if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { tRec = rec; break; }
        rec = rec->next;
    }
    if (!tRec) return;

    // 余额不足处理
    while (p->balance < tRec->cost) {
        printf("余额不足！差额 %.2f，请输入充值金额 (0取消): ", tRec->cost - p->balance);
        double money = safeGetDouble();
        if (money > 0) p->balance += money;
        else break;
    }

    // 满足条件则扣款
    if (p->balance >= tRec->cost) {
        p->balance -= tRec->cost;
        tRec->isPaid = 1; // 标记已缴费
        printf("【缴费成功】已扣款。当前余额: %.2f\n", p->balance);

        // 【核心联动】如果是处方药，缴费成功后同步扣减药房物理库存
        char mName[50]; int qty;
        if (sscanf(tRec->description, "处方药:%s x %d", mName, &qty) == 2) {
            Medicine* m = medicineHead->next;
            while (m) {
                if (strcmp(m->name, mName) == 0) {
                    m->stock -= qty; // 扣减库存
                    printf("[系统联动] 缴费成功，已通知药房扣减库存: %s\n", mName);
                    break;
                }
                m = m->next;
            }
        }
    }
    system("pause");
}

// ---------------------------------------------------------
// 业务四：医疗档案查询
// ---------------------------------------------------------
void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n--- 医疗档案 ---\n1. 挂号记录\n2. 看诊与检查记录\n3. 住院记录\n4. 数据清洗与修复\n0. 返回\n选择: ");
        int c = safeGetInt();
        if (c == 0) return;

        // 模拟大数据清洗系统
        if (c == 4) {
            printf("【系统】清洗完成！已自动修复冲突并补全档案缺失项。\n");
            system("pause");
            continue;
        }

        Record* rec = recordHead->next;
        while (rec) {
            // 仅输出当前登录者的账单
            if (strcmp(rec->patientId, currentPatientId) == 0) {
                // 根据选项通过 Type 过滤记录
                if ((c == 1 && rec->type == 1) ||
                    (c == 2 && (rec->type == 2 || rec->type == 3)) ||
                    (c == 3 && rec->type == 4)) {
                    printf("%s | %s | %.2f | %s\n",
                        rec->recordId, rec->description, rec->cost,
                        rec->isPaid ? "已处理/已缴费" : "待缴费");
                }
            }
            rec = rec->next;
        }
        system("pause");
    }
}

// ---------------------------------------------------------
// 患者端总路由 (消除了重复登录，接收透传的已登录 ID)
// ---------------------------------------------------------
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
        case 0: return; // 安全注销退回网关
        }
    }
}