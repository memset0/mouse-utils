#include "windows.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

#define ULL unsigned long long

#define MAX_QUEUE 100
#define SECOND ((ULL)10000000)

#define debug(...) ((void)0)
//#define debug(...) fprintf(stderr, __VA_ARGS__)

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

int CPS_L = 15;
int CPS_R = 30;
ULL MAINTAIN_PERIOD = SECOND >> 1;

enum SHORTCUT {
  CTRL = VK_LCONTROL,
  SHIFT = VK_LSHIFT,
  ALT = VK_MENU,
  ESC = VK_ESCAPE,
  WIN = VK_LWIN,
  BACK = VK_BACK,
  TAB = VK_TAB,
  ENTER = VK_RETURN,
  HOME = VK_HOME,
  END = VK_END,
  DEL = VK_DELETE,
  INS = VK_INSERT,
  UP = VK_UP,
  DOWN = VK_DOWN,
  RIGHT = VK_RIGHT,
  LEFT = VK_LEFT,
  XBTN1 = VK_XBUTTON1,
  XBTN2 = VK_XBUTTON2,
  F1 = VK_F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  PRTSC = VK_PRINT,
  A = 65,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z
};

void crashed(int exitcode = 1) {
  printf("输入回车键退出程序\n");
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

  inline ULL Last() {
    return l == r ? 0 : queue[r == 0 ? MAX_QUEUE - 1 : r - 1];
  }

  inline void Shrink(ULL current_time) {
    // 将监听周期之外的事件从队列中移除

    while (r != l && queue[l] < current_time - period) {
      l++;
      if (l == MAX_QUEUE) {
        l = 0;
      }
    }
  }

  inline void Push(ULL event_time) {
    // 将新的事件时间压入队列
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
      debug("鼠标左键被点击 %llu\n", GetTime());
      left_click_q.Push(GetTime());
      break;
    case WM_RBUTTONDOWN:
      debug("鼠标右键被点击 %llu\n", GetTime());
      right_click_q.Push(GetTime());
      break;
    }
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD WINAPI RegisterMouseListener(LPVOID lpParam) {
  // 安装鼠标事件钩子
  HHOOK mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
  if (mouse_hook == NULL) {
    printf("安装鼠标事件钩子失败！\n");
    return crashed(), 1;
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // 卸载鼠标事件钩子
  UnhookWindowsHookEx(mouse_hook);
  return 0;
}

void SendMouseClick(DWORD dwflags) { mouse_event(dwflags, 0, 0, 0, 0); }

int main() {
  HANDLE hThread = CreateThread(NULL, 0, RegisterMouseListener, NULL, 0, NULL);
  if (hThread == NULL) {
    printf("无法创建鼠标事件监听线程！\n");
    return crashed(), 1;
  }

  ULL last_left_t = 0;
  ULL last_right_t = 0;

  while (true) {
    bool state_x1 = (SHORT)GetAsyncKeyState(XBTN1) & 0x8000;
    bool state_x2 = (SHORT)GetAsyncKeyState(XBTN2) & 0x8000;

    if (state_x1 || state_x2) {
      ULL current_time = GetTime();
      left_click_q.Shrink(current_time);
      right_click_q.Shrink(current_time);

      debug("鼠标侧键被按下 %d %d %d %d %llu %lf\n", (int)state_x1,
            (int)state_x2, (int)left_click_q.Length(),
            (int)right_click_q.Length(), current_time,
            (double)current_time / SECOND);

      if (state_x2 && last_left_t < current_time - SECOND / CPS_L &&
          left_click_q.Length() < CPS_L * MAINTAIN_PERIOD / SECOND) {
        SendMouseClick(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);
        last_left_t = current_time;
      }

      if (state_x1 && last_right_t < current_time - SECOND / CPS_R &&
          right_click_q.Length() < CPS_R * MAINTAIN_PERIOD / SECOND) {
        SendMouseClick(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP);
        last_right_t = current_time;
      }

    } else {
      // debug("鼠标侧键没有被按下\n");
    }
  }
}
