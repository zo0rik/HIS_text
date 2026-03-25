#pragma once
#ifndef MODELS_H
#define MODELS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1. 患者档案链表节点
typedef struct Patient {
    char id[20];
    char password[50];
    char name[100];
    char gender[10];        // 限制为 "男性" 或 "女性"
    int age;
    char allergy[100];
    int isEmergency;
    double balance;         // 账户余额 (充值、扣费、住院押金均通过此变动)
    struct Patient* next;
} Patient, * PatientList;

// 2. 医护人员链表节点
typedef struct Staff {
    char id[20];
    char password[50];
    char name[100];
    char department[100];
    char level[100];
    struct Staff* next;
} Staff, * StaffList;

// 3. 药品库存链表节点
typedef struct Medicine {
    char id[20];
    char name[100];
    int stock;
    double price;
    char expiryDate[20];
    struct Medicine* next;
} Medicine, * MedicineList;

// 4. 全院业务流水记录链表 (已按5大类规范改造)
typedef struct Record {
    char recordId[30];
    int type;               // 1:挂号 2:看诊 3:开药 4:检查 5:住院
    char patientId[20];
    char staffId[20];
    double cost;            // 记录涉及的金额(含押金、单项费用)
    int isPaid;             // 0:待处理/待缴费 1:已缴费/进行中 2:已完结
    char description[300];  // 格式化详情 (不能有空格，用下划线替代)
    char createTime[30];    // 记录生成时间 (如 2026-03-24_10:00:00)
    struct Record* next;
} Record, * RecordList;

// 5. 病房与床位链表节点 (已按需求扩充属性)
typedef struct Bed {
    int bedNumber;          // 床位号
    int isOccupied;         // 0:空闲 1:占用
    char patientId[20];     // 关联的患者ID
    char wardType[50];      // 护理类别: 特殊病房 / 普通病房
    char bedType[50];       // 单人病房/双人病房/三人病房/单人陪护病房/单人陪护疗养病房
    double price;           // 每日床位费
    struct Bed* next;
} Bed, * BedList;

extern PatientList patientHead;
extern StaffList staffHead;
extern MedicineList medicineHead;
extern RecordList recordHead;
extern BedList bedHead;

#endif