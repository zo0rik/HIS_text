#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"
#include "patient.h"
#include "staff.h"
#include "admin.h"
#include "drug.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"

// ==========================================
// 全局链表头指针实例化分配区
// ==========================================
PatientList patientHead = NULL;
StaffList staffHead = NULL;
MedicineList medicineHead = NULL;
RecordList recordHead = NULL;
BedList bedHead = NULL;

// 初始化创建虚拟头节点
void initLists() {
    patientHead = (PatientList)malloc(sizeof(Patient)); patientHead->next = NULL;
    staffHead = (StaffList)malloc(sizeof(Staff)); staffHead->next = NULL;
    medicineHead = (MedicineList)malloc(sizeof(Medicine)); medicineHead->next = NULL;
    recordHead = (RecordList)malloc(sizeof(Record)); recordHead->next = NULL;
    bedHead = (BedList)malloc(sizeof(Bed)); bedHead->next = NULL;
}

// ==========================================
// 主函数入口：HIS 系统总调度枢纽
// ==========================================
int main() {
    initLists();

    // 【阶段一】系统点火：一次性将所有独立文件的数据读取到内存中
    loadAllDataFromTxt(); // 载入你的四个底层链表
    loadDrugs();          // 载入队友模块
    loadDrugHistory();
    loadDoctors();
    loadSchedules();
    loadTransactions();
    loadAdminData();      // 载入管理端超管账号配置

    // 【阶段二】主界面循环：对应流程图《主界面》的端口选择与统一登录分流
    while (1) {
        system("cls");
        printf("=========================================\n");
        printf("   现代大型综合医院 HIS 系统控制台大厅   \n");
        printf("=========================================\n");
        printf("  [1] 管理端登录口 (高管后台)\n");
        printf("  [2] 医护端登录口 (临床工作站)\n");
        printf("  [3] 患者端登录口 (含自助挂号注册)\n");
        printf("  [0] 断开连接并保存所有数据退出\n");
        printf("=========================================\n");
        printf("请选择您的访问端口: ");

        int port = safeGetInt();
        char acc[50], pwd[50];

        // ------------------ 端口 1: 管理端 ------------------
        if (port == 1) {
            printf("\n>>> 管理端身份验证 <<<\n请输入管理账号: "); safeGetString(acc, 50);
            printf("请输入口令密码: "); safeGetString(pwd, 50);

            // 校验 admin.txt 中的凭据
            if (strcmp(acc, admin.username) == 0 && strcmp(pwd, admin.password) == 0) {
                adminMenu(); // 验证通过
            }
            else {
                printf("【拦截】账号或密码错误！\n"); system("pause");
            }
        }
        // ------------------ 端口 2: 医护端 ------------------
        else if (port == 2) {
            printf("\n>>> 医护端身份验证 <<<\n请输入工号: "); safeGetString(acc, 50);
            printf("请输入密码: "); safeGetString(pwd, 50);

            // 遍历医护链表进行凭据匹配
            Staff* s = staffHead->next;
            Staff* me = NULL;
            while (s) {
                if (strcmp(s->id, acc) == 0 && strcmp(s->password, pwd) == 0) {
                    me = s; break;
                }
                s = s->next;
            }
            if (me) {
                staffTerminal(me); // 验证通过，将医生实体透传给工作台，避免二次登录
            }
            else {
                printf("【拦截】工号不存在或密码错误！\n"); system("pause");
            }
        }
        // ------------------ 端口 3: 患者端 ------------------
        else if (port == 3) {
            printf("\n-- 患者服务端 --\n1. 账号密码登录\n2. 首次就诊建档(注册)\n请选择: ");
            int pChoice = safeGetInt();

            if (pChoice == 1) {
                printf("请输入患者ID (如P20251000): "); safeGetString(acc, 50);
                printf("请输入登录密码: "); safeGetString(pwd, 50);

                // 遍历患者链表进行凭据匹配
                Patient* p = patientHead->next;
                Patient* me = NULL;
                while (p) {
                    if (strcmp(p->id, acc) == 0 && strcmp(p->password, pwd) == 0) {
                        me = p; break;
                    }
                    p = p->next;
                }
                if (me) {
                    userTerminal(me->id); // 验证通过，将唯一ID透传给患者端，挂号缴费无须重输
                }
                else {
                    printf("【拦截】患者ID或密码错误！\n"); system("pause");
                }
            }
            else if (pChoice == 2) {
                registerPatient(); // 引导进入注册流程
                system("pause");
            }
        }
        // ------------------ 端口 0: 安全退出并保存 ------------------
        else if (port == 0) {
            printf("\n正在将三端数据封存至物理磁盘...\n");
            // 【阶段三】系统停机：将发生变更的所有内存链表写回物理文件
            saveAllDataToTxt(); // 你的 4 个核心链表保存
            saveDrugs();        // 队友的数据保存
            saveDrugHistory();
            saveDoctors();
            saveSchedules();
            saveTransactions();
            saveAdminData();
            printf("所有数据已安全持久化保存，系统成功退出！\n");
            break;
        }
    }
    return 0;
}