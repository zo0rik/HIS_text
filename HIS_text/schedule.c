#define _CRT_SECURE_NO_WARNINGS
#include "schedule.h"
#include "doctor.h"     // 需要引用医生头文件以校验医生ID是否存在及获取姓名
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 独立维护的排班记录全局链表指针
Schedule* scheduleList = NULL;

// ---------------------------------------------------------
// 从本地文本加载排班数据到内存链表
// ---------------------------------------------------------
void loadSchedules() {
    FILE* fp = fopen("schedules.txt", "r");
    if (!fp) return; // 如果文件不存在，则跳过加载，链表保持为空

    char line[256];
    Schedule s;
    Schedule* tail = NULL;
    // 逐行读取排班数据
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0; // 消除行尾的换行符

        // 使用 strtok 按逗号切割字符串并赋值给结构体
        char* token = strtok(line, ",");
        if (token) s.doctor_id = atoi(token); else s.doctor_id = 0;
        token = strtok(NULL, ",");
        if (token) strcpy(s.date, token); else s.date[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(s.shift, token); else s.shift[0] = '\0';

        // 动态分配新节点
        Schedule* node = (Schedule*)malloc(sizeof(Schedule));
        *node = s;
        node->next = NULL;

        // 尾插法构建链表，保证加载后的顺序与文件一致
        if (!scheduleList) scheduleList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 将内存中的排班数据持久化保存到文本文件
// ---------------------------------------------------------
void saveSchedules() {
    FILE* fp = fopen("schedules.txt", "w");
    if (!fp) return;
    Schedule* p = scheduleList;
    while (p) {
        // 按照 医生ID,日期,班次 的格式写入文件
        fprintf(fp, "%d,%s,%s\n", p->doctor_id, p->date, p->shift);
        p = p->next;
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 内部工具：检查排班冲突 (同一医生同一天不能排多次班)
// 返回 1 表示有冲突，返回 0 表示无冲突
// ---------------------------------------------------------
static int checkConflict(int doctor_id, char* date) {
    Schedule* s = scheduleList;
    while (s) {
        if (s->doctor_id == doctor_id && strcmp(s->date, date) == 0)
            return 1;
        s = s->next;
    }
    return 0;
}

// ---------------------------------------------------------
// 业务一：查看特定时间段的排班表
// ---------------------------------------------------------
static void viewSchedule() {
    char start[11], end[11];
    printf("请输入起始日期 (YYYY-MM-DD): "); scanf("%s", start);
    printf("请输入结束日期 (YYYY-MM-DD): "); scanf("%s", end);
    printf("\n--- 排班表 (%s 至 %s) ---\n", start, end);
    printf("%-12s %-15s %-12s %-10s\n", "日期", "医生姓名", "科室", "班次");

    Schedule* s = scheduleList;
    if (!s) { printf("暂无排班记录。\n"); return; }

    while (s) {
        // 使用 strcmp 比较日期字符串的大小，实现范围过滤
        if (strcmp(s->date, start) >= 0 && strcmp(s->date, end) <= 0) {

            // 联动 doctorList 查找医生详细信息
            Doctor* d = doctorList;
            char docName[50] = "未知";
            char docDept[30] = "";
            while (d) {
                if (d->id == s->doctor_id) {
                    strcpy(docName, d->name);
                    strcpy(docDept, d->department);
                    break;
                }
                d = d->next;
            }
            printf("%-12s %-15s %-12s %-10s\n", s->date, docName, docDept, s->shift);
        }
        s = s->next;
    }
}

// ---------------------------------------------------------
// 业务二：手动添加医生排班
// ---------------------------------------------------------
static void addSchedule() {
    int doc_id;
    char date[11], shift[10];
    printf("请输入医生ID: "); scanf("%d", &doc_id);

    // 校验前置条件：医生必须在人事系统(doctorList)中存在
    Doctor* d = doctorList;
    int exists = 0;
    while (d) { if (d->id == doc_id) { exists = 1; break; } d = d->next; }
    if (!exists) { printf("医生ID不存在！\n"); return; }

    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);

    // 校验排班冲突
    if (checkConflict(doc_id, date)) { printf("该医生当天已有排班，不能重复添加。\n"); return; }

    printf("请输入班次 (早班/晚班/休息): "); scanf("%s", shift);
    // 限制班次输入规范
    if (strcmp(shift, "早班") != 0 && strcmp(shift, "晚班") != 0 && strcmp(shift, "休息") != 0) {
        printf("无效班次。\n"); return;
    }

    // 采用头插法将新排班节点插入链表
    Schedule* node = (Schedule*)malloc(sizeof(Schedule));
    node->doctor_id = doc_id;
    strcpy(node->date, date);
    strcpy(node->shift, shift);
    node->next = scheduleList;
    scheduleList = node;
    printf("排班添加成功。\n");
}

// ---------------------------------------------------------
// 业务三：删除指定医生在指定日期的排班
// ---------------------------------------------------------
static void deleteSchedule() {
    int doc_id;
    char date[11];
    printf("请输入医生ID: "); scanf("%d", &doc_id);
    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);

    // 双指针法遍历并删除链表节点
    Schedule* prev = NULL, * curr = scheduleList;
    while (curr) {
        if (curr->doctor_id == doc_id && strcmp(curr->date, date) == 0) {
            if (prev) prev->next = curr->next;
            else scheduleList = curr->next; // 若删除的是头节点
            free(curr); // 释放内存
            printf("排班删除成功。\n");
            return;
        }
        prev = curr; curr = curr->next;
    }
    printf("未找到该排班。\n");
}

// ---------------------------------------------------------
// 业务四：修改排班班次
// ---------------------------------------------------------
static void modifySchedule() {
    int doc_id;
    char date[11];
    printf("请输入医生ID: "); scanf("%d", &doc_id);
    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);

    Schedule* p = scheduleList;
    while (p) {
        // 定位到具体的排班记录
        if (p->doctor_id == doc_id && strcmp(p->date, date) == 0) {
            printf("当前班次: %s\n", p->shift);
            printf("请输入新班次 (早班/晚班/休息): "); scanf("%s", p->shift);
            // 校验新班次格式
            if (strcmp(p->shift, "早班") != 0 && strcmp(p->shift, "晚班") != 0 && strcmp(p->shift, "休息") != 0) {
                printf("无效班次，操作取消。\n");
                return;
            }
            printf("排班修改成功。\n");
            return;
        }
        p = p->next;
    }
    printf("未找到该排班。\n");
}

// ---------------------------------------------------------
// 排班管理模块子路由
// ---------------------------------------------------------
void scheduleMenu() {
    int choice;
    do {
        printf("\n========== 医生排班管理 ==========\n");
        printf("1. 排班表查看\n");
        printf("2. 手动排班（添加）\n");
        printf("3. 删除排班\n");
        printf("4. 修改排班\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1: viewSchedule(); break;
        case 2: addSchedule(); break;
        case 3: deleteSchedule(); break;
        case 4: modifySchedule(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}