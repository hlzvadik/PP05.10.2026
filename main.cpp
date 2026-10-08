#include <windows.h>
#include <cstdio>
#include <iostream>
#include <string>

DWORD send(DWORD& err, HANDLE wr, const char* b, DWORD k)
{
  DWORD r = 0;
  DWORD h = 0;
  while (r < k) {
    if (!WriteFile(wr, b + r, k - r, &h, NULL)) {
      err = GetLastError();
      break;
    }
    r += h;
  }
  return r;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: main.exe <path_to_child.exe>\n";
    return 1;
  }

  HANDLE read = NULL, write = NULL;
  DWORD err = 0;

  SECURITY_ATTRIBUTES sa = {};
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = FALSE;

  if (!CreatePipe(&read, &write, &sa, 256)) {
    std::cerr << GetLastError() << std::endl;
    return 1;
  }

  if (!SetHandleInformation(read, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
    std::cerr << GetLastError() << std::endl;
    CloseHandle(read);
    CloseHandle(write);
    return 1;
  }

  PROCESS_INFORMATION pi = {};
  STARTUPINFOA si = {};
  si.cb = sizeof(si);

  std::string cmd = '"' + std::string(argv[1]) + "\" " + std::to_string(reinterpret_cast< DWORD_PTR >(read));

  if (!CreateProcessA(argv[1], cmd.data(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
    std::cerr << GetLastError() << std::endl;
    CloseHandle(read);
    CloseHandle(write);
    return 1;
  }
  CloseHandle(read);

  printf("Type string, complete with EOF (Enter, Ctrl+Z, Enter):\n");

  bool ok = true;
  char msg[255];
  size_t scans = fread(msg, 1, sizeof(msg), stdin);
  while (scans == sizeof(msg)) {
    if (send(err, write, msg, (DWORD)scans) != scans) {
      std::cerr << "Write error: " << err << '\n';
      ok = false;
      break;
    }
    scans = fread(msg, 1, sizeof(msg), stdin);
  }
  if (ok) {
    msg[scans] = '\0';
    if (send(err, write, msg, (DWORD)scans + 1) != scans + 1) {
      std::cerr << "Write error: " << err << '\n';
      ok = false;
    }
  }

  CloseHandle(write);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  return ok ? 0 : 1;
}
