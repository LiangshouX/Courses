/*
* @file MemoryMonitoring.cpp
* @author 肖良寿 1120200563
* @time   2022/11/1
* @brief 设计一个内存监视器，能实时地显示当前系统中内存的使用情况，
* 包括系统地址空间的布局，物理内存的使用情况；能实时显示某个进程的虚拟地址空间布局和工作集信息等。
* 注：在Windows平台上做。相关的系统调用：
*
*     GetSystemInfo, VirtualQueryEx,VirtualAlloc, GetPerformanceInfo, GlobalMemoryStatusEx …
*
*/



#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <iostream>
#include <tchar.h>
#include <locale.h>

// 将 Byte 换算为 KB
#define DIV 1024

using namespace std;

int _tmain(int argc, TCHAR* argv[])
{
	// 解决中文字符显示
	setlocale(LC_ALL, "chs");

	// 获取系统信息
	SYSTEM_INFO systemInfo;
	ZeroMemory(&systemInfo, sizeof(systemInfo));
	GetSystemInfo(&systemInfo);

	cout << "进程最低内存地址:0x" << systemInfo.lpMinimumApplicationAddress << endl;
	cout << "进程最高内存地址:0x" << systemInfo.lpMaximumApplicationAddress << endl;
	printf("页大小：%dKB\n", systemInfo.dwPageSize / DIV);
	printf("分配粒度：%dKB\n", systemInfo.dwAllocationGranularity / DIV);

	printf("\n");

	// 获取内存状态
	MEMORYSTATUSEX memoryInfo;
	ZeroMemory(&memoryInfo, sizeof(memoryInfo));
	// 调用 GlobalMemoryStatusEx 前必须赋值
	memoryInfo.dwLength = sizeof(memoryInfo);
	GlobalMemoryStatusEx(&memoryInfo);

	printf("物理内存大小：%I64dKB\n", memoryInfo.ullTotalPhys / DIV);
	printf("可用物理内存：%I64dKB\n", memoryInfo.ullAvailPhys / DIV);
	printf("共%d%%的物理内存被使用\n\n", memoryInfo.dwMemoryLoad);
	printf("页面文件大小：%I64dKB\n", memoryInfo.ullTotalPageFile / DIV);
	printf("可用页面文件：%I64dKB\n", memoryInfo.ullAvailPageFile / DIV);
	printf("虚拟内存大小：%I64dKB\n", memoryInfo.ullTotalVirtual / DIV);
	printf("可用虚拟内存：%I64dKB\n\n", memoryInfo.ullAvailVirtual / DIV);

	// 显示所有进程的 PID
	PROCESSENTRY32 processEntry;
	ZeroMemory(&processEntry, sizeof(processEntry));
	// 调用 CreateToolhelp32Snapshot 前必须赋值
	processEntry.dwSize = sizeof(processEntry);
	// 创建系统进程快照
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	printf("进程名称        PID\n");
	// 第一个快照，成功返回 TRUE
	bool snapSucess = Process32First(snapshot, &processEntry);
	while (snapSucess)
	{
		// 输出进程名称
		printf("%ws    ", processEntry.szExeFile);
		// 输出 PID
		printf("%d\n", processEntry.th32ProcessID);
		// 下一个快照
		snapSucess = Process32Next(snapshot, &processEntry);
	}
	printf("\n");

	// 输入特定进程的 PID
	int pid;
pid_search:
	printf("输入要查询的PID：");
	scanf("%d", &pid);

	// 打开对应进程对象
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	// 错误处理
	if (process == NULL)
	{
		printf("对应的进程不存在!\n");
		goto pid_search;
	}

	// 初始化存储页面信息的结构体
	MEMORY_BASIC_INFORMATION membscInfo;
	ZeroMemory(&membscInfo, sizeof(membscInfo));

	// 条目介绍
	printf("内存区域                                页状态    内存保护    页类型\n");

	// block 为指针，从最低内存地址遍历至最高
	LPCVOID block = (LPVOID)systemInfo.lpMinimumApplicationAddress;
	while (block < systemInfo.lpMaximumApplicationAddress)
	{
		// VirtualQueryEx 错误时返回 0
		if (VirtualQueryEx(process, block, &membscInfo, sizeof(membscInfo)))
		{
			// 计算得到块的结尾
			LPCVOID end = (PBYTE)block + membscInfo.RegionSize;

			// 输出地址
			cout << "0x" << block << " - 0x" << end;

			// 输出页面状态
			switch (membscInfo.State)
			{
			case MEM_COMMIT: printf("  提交"); break;
			case MEM_FREE: printf("  空闲"); break;
			case MEM_RESERVE: printf("  预留"); break;
			}

			// 输出内存保护状态
			switch (membscInfo.AllocationProtect)
			{
			case PAGE_EXECUTE: printf("  EXECUTE"); break;
			case PAGE_EXECUTE_READ: printf("  EXECUTE_READ"); break;
			case PAGE_EXECUTE_READWRITE: printf("  EXECUTE_READWRITE"); break;
			case PAGE_EXECUTE_WRITECOPY: printf("  EXECUTE_WRITECOPY"); break;
			case PAGE_NOACCESS: printf("  NOACCESS"); break;
			case PAGE_READONLY: printf("  READONLY"); break;
			case PAGE_READWRITE: printf("  READWRITE"); break;
			case PAGE_WRITECOPY: printf("  WRITECOPY"); break;
			}

			// 输出页面类型
			switch (membscInfo.Type)
			{
			case MEM_IMAGE: printf("  IMAGE"); break;
			case MEM_MAPPED: printf("  MAPPED"); break;
			case MEM_PRIVATE: printf("  PRIVATE"); break;
			}

			printf("\n");

			// 将指针指向刚才页面的结尾（下一页开头）
			block = end;
		}
	}
	printf("\n按任意键退出");
	getchar(); getchar();
	exit(0);
}