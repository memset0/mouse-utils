// @Copyright: Copyright(c) 2023 mem.ac
// @License: MIT
// @FileName: autoclicker.cpp
// @Author: memset0
// @Version: 2.2.0
// @Create-Date: 2023-04-14
// @Update-Date: 2023-07-17
// @Description: yet another auto clicker for personal use

#include "windows.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#define ULL unsigned long long

#define MAX_QUEUE 100
#define SECOND ((ULL)10000000)

#define debug(...) ((void)0)
// #define debug(...) fprintf(stderr, __VA_ARGS__)

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

const ULL MAINTAIN_PERIOD = SECOND >> 1;
unsigned int CPS_L = 10, CPS_L_MIN = -1, CPS_L_MAX = -1;
unsigned int CPS_R = 10, CPS_R_MIN = -1, CPS_R_MAX = -1;
bool SWAP_X_CLICK = false;

enum SHORTCUT { CTRL = VK_LCONTROL, SHIFT = VK_LSHIFT, ALT = VK_MENU, ESC = VK_ESCAPE, WIN = VK_LWIN, BACK = VK_BACK, TAB = VK_TAB, ENTER = VK_RETURN, HOME = VK_HOME, END = VK_END, DEL = VK_DELETE, INS = VK_INSERT, UP = VK_UP, DOWN = VK_DOWN, RIGHT = VK_RIGHT, LEFT = VK_LEFT, XBTN1 = VK_XBUTTON1, XBTN2 = VK_XBUTTON2, F1 = VK_F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, PRTSC = VK_PRINT, A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z };

void Exit(int exitcode = 1, bool crashed = true) {
  if (crashed) {
    printf("程序遇到问题退出，按回车键关闭窗口\n");
  } else {
    printf("程序结束，按回车键关闭窗口\n");
  }
  string buffer;
  cin >> buffer;
  exit(exitcode);
}

inline ULL GetTime() {
  FILETIME ft;
  GetSystemTimePreciseAsFileTime(&ft);
  return ((ULL)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

class ClickEventHelper {
private:
  size_t l = 0, r = 0;
  ULL period;
  ULL queue[MAX_QUEUE];

public:
  ClickEventHelper(ULL _period) { period = _period; }

  inline size_t Length() {
    size_t t_len = r + MAX_QUEUE - l;
    return t_len >= MAX_QUEUE ? t_len - MAX_QUEUE : t_len;
  }

  inline ULL Last() { return l == r ? 0 : queue[r == 0 ? MAX_QUEUE - 1 : r - 1]; }

  inline void Shrink(ULL current_time) {
    while (r != l && queue[l] < current_time - period) {
      l++;
      if (l == MAX_QUEUE) {
        l = 0;
      }
    }
  }

  inline void Push(ULL event_time) {
    queue[r++] = event_time;
    if (r == MAX_QUEUE) {
      r = 0;
    }
  }
} left_click_q(MAINTAIN_PERIOD), right_click_q(MAINTAIN_PERIOD);

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {
    switch (wParam) {
    case WM_LBUTTONDOWN:
      debug("Left button clicked %llu\n", GetTime());
      left_click_q.Push(GetTime());
      break;
    case WM_RBUTTONDOWN:
      debug("Right button clicked %llu\n", GetTime());
      right_click_q.Push(GetTime());
      break;
    }
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD WINAPI RegisterMouseListener(LPVOID lpParam) {
  HHOOK mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
  if (mouse_hook == NULL) {
    printf("挂载监听钩子失败");
    return Exit(), 1;
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  UnhookWindowsHookEx(mouse_hook);
  return 0;
}

void SendMouseClick(DWORD dwflags) {
  // mouse_event(dwflags, 0, 0, 0, 0);

  INPUT input = {0};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = dwflags;

  SendInput(1, &input, sizeof(INPUT));
}

void SendMouseClick(std::vector<DWORD> dwflags) {
  for (size_t i = 0; i < dwflags.size(); i++) {
    if (i) {
      Sleep(3 + rand() % 3);
    }
    SendMouseClick(dwflags[i]);
  }
}

bool IsIntegerString(string str) {
  for (int i = 0; i < str.length(); i++) {
    if (str[i] < '0' || str[i] > '9') {
      return false;
    }
  }
  return true;
}

int readIntegerFromIniFile(const char *section, const char *key, int defaultValue, const char *iniFilePath) {
  char buffer[256];
  char defaultBuffer[256];

  sprintf(defaultBuffer, "%d", defaultValue);
  GetPrivateProfileString(section, key, defaultBuffer, buffer, 256, iniFilePath);

  if (!IsIntegerString(string(buffer))) {
    printf("输入不是整数\n");
    return Exit(), 0;
  } else {
    int value = std::stoi(buffer);
    return value;
  }
}

void LoadConfig() {
  const char *iniFileName = "config.ini";
  char exePath[MAX_PATH];
  GetModuleFileNameA(NULL, exePath, MAX_PATH);
  char *exeDir = strrchr(exePath, '\\');
  if (exeDir != NULL) {
    *(exeDir + 1) = '\0';
  }
  const string iniFilePathString = string(exePath) + iniFileName;
  const char *iniFilePath = iniFilePathString.c_str();

  DWORD fileAttributes = GetFileAttributes(iniFilePath);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES || (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    printf("不合法的配置文件");

    DWORD writeError = 0;
    char buffer[16];

    sprintf(buffer, "%d", CPS_L);
    WritePrivateProfileString("CPS", "L", buffer, iniFilePath);
    writeError = writeError | GetLastError();

    sprintf(buffer, "%d", CPS_R);
    WritePrivateProfileString("CPS", "R", buffer, iniFilePath);
    writeError = writeError | GetLastError();

    sprintf(buffer, "%d", SWAP_X_CLICK);
    WritePrivateProfileString("General", "SwapXClick", buffer, iniFilePath);
    writeError = writeError | GetLastError();

    // if (writeError) {
    //   printf("保存配置文件失败：%lu\n", writeError);
    //   return Exit();
    // }
  }

  CPS_L = readIntegerFromIniFile("CPS", "L", CPS_L, iniFilePath);
  CPS_L_MIN = readIntegerFromIniFile("CPS", "LMin", CPS_L_MIN, iniFilePath);
  CPS_L_MAX = readIntegerFromIniFile("CPS", "LMax", CPS_L_MAX, iniFilePath);
  CPS_R = readIntegerFromIniFile("CPS", "R", CPS_R, iniFilePath);
  CPS_R_MIN = readIntegerFromIniFile("CPS", "RMin", CPS_R_MIN, iniFilePath);
  CPS_R_MAX = readIntegerFromIniFile("CPS", "RMax", CPS_R_MAX, iniFilePath);
  SWAP_X_CLICK = readIntegerFromIniFile("General", "SwapXClick", SWAP_X_CLICK, iniFilePath);

  if (CPS_L_MIN == -1) CPS_L_MIN = CPS_L;
  if (CPS_L_MAX == -1) CPS_L_MAX = CPS_L;
  if (CPS_R_MIN == -1) CPS_R_MIN = CPS_R;
  if (CPS_R_MAX == -1) CPS_R_MAX = CPS_R;

  printf("当前配置：\n");
  printf("左键CPS：%d ~ %d\n", CPS_L_MIN, CPS_R_MAX);
  printf("右键CPS：%d ~ %d\n", CPS_R_MIN, CPS_R_MAX);
  printf("侧键检测交换（是=1，否=0）：%d\n", SWAP_X_CLICK);
}

int main() {
  srand(time(NULL));

  LoadConfig();

  HANDLE hThread = CreateThread(NULL, 0, RegisterMouseListener, NULL, 0, NULL);
  if (hThread == NULL) {
    printf("注册鼠标事件监听器失败");
    return Exit(), 1;
  }

  ULL last_left_t = 0;
  ULL last_right_t = 0;

  ULL last_refresh = -1;
  while (true) {
    bool state_x1 = (SHORT)GetAsyncKeyState(XBTN1) & 0x8000;
    bool state_x2 = (SHORT)GetAsyncKeyState(XBTN2) & 0x8000;

    if (SWAP_X_CLICK) {
      std::swap(state_x1, state_x2);
    }

    if (state_x1 || state_x2) {
      ULL current_time = GetTime();
      if (current_time / SECOND != last_refresh) {
        CPS_L = rand() % (CPS_L_MAX - CPS_L_MIN + 1) + CPS_L_MIN;
        CPS_R = rand() % (CPS_R_MAX - CPS_R_MIN + 1) + CPS_R_MIN;
        last_refresh = current_time / SECOND;
      }

      left_click_q.Shrink(current_time);
      right_click_q.Shrink(current_time);

      debug("X-button Down %d %d : %d %d %llu %lf\n", (int)state_x1, (int)state_x2, (int)left_click_q.Length(), (int)right_click_q.Length(), current_time, (double)current_time / SECOND);

      if (state_x2 && last_left_t < current_time - SECOND / CPS_L && left_click_q.Length() < CPS_L * MAINTAIN_PERIOD / SECOND) {
        SendMouseClick({MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP});
        last_left_t = current_time;
      }

      if (state_x1 && last_right_t < current_time - SECOND / CPS_R && right_click_q.Length() < CPS_R * MAINTAIN_PERIOD / SECOND) {
        SendMouseClick({MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP});

        last_right_t = current_time;
      }

    } else {
    }
  }
}
