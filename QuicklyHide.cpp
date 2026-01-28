
#include <Windows.h>
#include<stdlib.h>
#include <thread>
#include <mutex>
#include <stdexcept>
#include "rbslib/Storage.h"
#include "rbslib/FileIO.h"
#include "rbslib/Buffer.h"
#include "json/CJsonObject.h"
constexpr auto ID_EXIT = 0;
constexpr auto ID_SHOW = 1;
constexpr auto ID_HIDE = 2;
constexpr auto ID_GETWINDOW = 3;


const char* config_file_path = "config.json";
std::mutex global_mutex;

std::string window_name;
std::string class_name;
HWND hWnd;

void Init();
void MessageLoop();


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HANDLE mutex = CreateMutex(NULL, 0, TEXT("HideProcess_QuicklyHide"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(NULL, TEXT("不能重复运行"), TEXT("警告"), MB_OK);
        exit(0);
    }
    try {
        Init();
    }
    catch (const std::exception& ex) {
        MessageBoxA(NULL, ex.what(), "初始化失败", MB_OK);
        return 1;
    }
    MessageBox(NULL, TEXT("初始化成功\nCtrl+HOME\t将当前焦点所在窗口设为目标窗口\nCtrl+Space\t隐藏窗口\nCtrl+UP\t\t取消隐藏\nCtrl+E\t\t退出本程序"), TEXT("提示"), MB_OK);
    // 消息循环
    MessageLoop();
    CloseHandle(mutex);
    return 0;
}


void Init() {
    neb::CJsonObject json;
    if (!RbsLib::Storage::StorageFile(config_file_path).IsExist()) {
        //创建配置文件
        RbsLib::Storage::FileIO::File file{ config_file_path, RbsLib::Storage::FileIO::OpenMode::Write };
        json.Add("default_window_class", "");
        json.Add("default_window_name", "");
        file.Write(RbsLib::Buffer(json.ToFormattedString()));
    }
    json.Clear();
    json.Parse(RbsLib::Storage::FileIO::File(config_file_path).Read(RbsLib::Storage::StorageFile(config_file_path).GetFileSize()).ToString().c_str());
    window_name = json("default_window_name");
    class_name = json("default_window_class");
    std::string error_msg;
    if (0 == RegisterHotKey(NULL, ID_HIDE, MOD_CONTROL, ' '))
    {
        error_msg += "Ctrl+Space热键注册失败";
    }
    if (0 == RegisterHotKey(NULL, ID_SHOW, MOD_CONTROL, VK_UP)) {
        error_msg += "Ctrl+UP热键注册失败";
    }
    if (0 == RegisterHotKey(NULL, ID_EXIT, MOD_CONTROL, 'E')) {
        error_msg += "Ctrl+E热键注册失败";
    }
    if (0 == RegisterHotKey(NULL, ID_GETWINDOW, MOD_CONTROL, VK_HOME)) {
        error_msg += "Ctrl+HOME热键注册失败";
    }
    if (!error_msg.empty()) throw std::runtime_error(error_msg);

    if (0 == SetTimer(NULL, 0, 1000, NULL)) {
        throw std::runtime_error("无法创建定时器");
    }

}

void MessageLoop() {
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        switch (msg.message) {
        case WM_HOTKEY:
        {

            if (ID_HIDE == msg.wParam) {
                if (IsWindow(hWnd) == NULL)
                {
                    if (IDNO == MessageBox(NULL, TEXT("窗口句柄丢失，是否继续等待目标程序启动"), TEXT("警告"), MB_YESNO)) exit(0);
                    hWnd = NULL;
                }
                ShowWindow(hWnd, SW_HIDE);
            }

            else if (ID_SHOW == msg.wParam) {
                if (IsWindow(hWnd) == NULL)
                {
                    if (IDNO == MessageBox(NULL, TEXT("窗口句柄丢失，是否继续等待目标程序启动"), TEXT("警告"), MB_YESNO)) exit(0);
                    hWnd = NULL;
                }
                ShowWindow(hWnd, SW_SHOW);
            }
            else if (ID_EXIT == msg.wParam) {

                if (IDYES == MessageBox(NULL, TEXT("是否退出程序"), TEXT("警告"), MB_YESNO))
                {
                    UnregisterHotKey(NULL, 1); UnregisterHotKey(NULL, 2); UnregisterHotKey(NULL, 3);
                    
                    MessageBox(NULL, TEXT("退出后如果窗口依旧处于隐藏状态，请重新打开本程序并使用恢复显示快捷键或在任务管理器中终止程序"), TEXT("提示"), NULL);
                    return;
                }
            }
            else if (ID_GETWINDOW == msg.wParam) {
                hWnd = GetForegroundWindow();
                if (hWnd) {
                    char buffer[1024];
                    if (GetWindowTextA(hWnd, buffer, 1024)) {
                        window_name = buffer;
                    }
                    else {
                        window_name.clear();
                    }
                    if (GetClassNameA(hWnd, buffer, 1024)) {
                        class_name = buffer;
                    }
                    else {
                        class_name.clear();
                    }
                    try
                    {
                        RbsLib::Storage::FileIO::File file{ config_file_path, RbsLib::Storage::FileIO::OpenMode::Write|RbsLib::Storage::FileIO::OpenMode::Replace };
                        neb::CJsonObject json;
                        json.Add("default_window_class", class_name);
                        json.Add("default_window_name", window_name);
                        file.Write(RbsLib::Buffer(json.ToFormattedString()));
                    }
                    catch (const std::exception& ex) {
                        MessageBoxA(NULL, ex.what(), "配置保存失败", MB_OK);
                    }
                    std::string txt = "窗口设置成功: " + class_name+" " + window_name;
                    MessageBoxA(NULL, txt.c_str(), "成功", MB_OK);
                }
            }
        }
        break;
        case WM_TIMER:
        {
            if (hWnd == NULL) {
                if (!(window_name.empty() && class_name.empty())) {
                    hWnd = FindWindowA(class_name.c_str(), window_name.c_str());
                    if (hWnd) {
                        MessageBoxA(NULL, "窗口已找到", "成功", MB_OK);
                    }
                }
            }
            else {
                if (IsWindow(hWnd) == NULL)
                {
                    if (IDNO == MessageBox(NULL, TEXT("窗口句柄丢失，是否继续等待目标程序启动"), TEXT("警告"), MB_YESNO)) exit(0);
                    hWnd = NULL;
                }
            }
        }
        break;
        default:
            break;
        }

    }
}
