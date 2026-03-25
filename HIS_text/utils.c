#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"

void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
    else { buffer[0] = '\0'; }
}

int safeGetInt() {
    char buffer[100]; safeGetString(buffer, sizeof(buffer));
    int value = 0; sscanf(buffer, "%d", &value);
    return value;
}

double safeGetDouble() {
    char buffer[100]; safeGetString(buffer, sizeof(buffer));
    double value = 0.0; sscanf(buffer, "%lf", &value);
    return value;
}

// 【防呆设计】强制要求输入大于0的正整数
int safeGetPositiveInt() {
    while (1) {
        int val = safeGetInt();
        if (val > 0) return val;
        printf("【输入错误】请输入大于0的有效整数: ");
    }
}

// 【防呆设计】强制限制性别输入
void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) {
            return;
        }
        printf("【输入错误】只能填入 男性 或 女性，请重新输入: ");
    }
}

// 获取格式化系统时间（无空格，防止文件读取断裂）
void getCurrentTimeStr(char* buffer, int size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}


// ... 前面部分保持不变 ...

void loadAllDataFromTxt() {
    // ... 加载 Patient, Medicine, Staff, Record 保持不变 ...
    // ...
    FILE* fpBeds = fopen("beds.txt", "r");
    if (fpBeds) {
        Bed* tail = bedHead; Bed temp;
        // 适配新的病床字典 (bedId 变为字符串，增加 isRoundsDone)
        while (fscanf(fpBeds, "%19s %d %19s %49s %49s %lf %d",
            temp.bedId, &temp.isOccupied, temp.patientId, temp.wardType, temp.bedType, &temp.price, &temp.isRoundsDone) == 7) {
            Bed* newNode = (Bed*)malloc(sizeof(Bed)); *newNode = temp; newNode->next = NULL;
            tail->next = newNode; tail = newNode;
        }
        fclose(fpBeds);
    }
}

void saveAllDataToTxt() {
    // ... 保存 Patient, Medicine, Staff, Record 保持不变 ...
    // ...
    FILE* fpBeds = fopen("beds.txt", "w");
    if (fpBeds) {
        Bed* b = bedHead->next;
        while (b) {
            fprintf(fpBeds, "%s %d %s %s %s %.2f %d\n",
                b->bedId, b->isOccupied, strlen(b->patientId) > 0 ? b->patientId : "none", b->wardType, b->bedType, b->price, b->isRoundsDone);
            b = b->next;
        }
        fclose(fpBeds);
    }
}