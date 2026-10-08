#include <windows.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: child.exe <pipe_handle>\n";
    return 1;
  }

  HANDLE rd = NULL;
  try {
    rd = reinterpret_cast< HANDLE >(static_cast< DWORD_PTR >(std::stoull(argv[1])));
  } catch (...) {
    std::cerr << "Bad handle argument\n";
    return 1;
  }

  printf("Received string:\n");

  char buf[256];
  DWORD got = 0;
  for (;;) {
    if (!ReadFile(rd, buf, sizeof(buf), &got, NULL)) {
      DWORD err = GetLastError();
      if (err == ERROR_BROKEN_PIPE) break;
      std::cerr << "Read error: " << err << '\n';
      CloseHandle(rd);
      return 1;
    }
    if (got == 0) break;

    const char* z = static_cast< const char* >(memchr(buf, '\0', got));
    size_t n = z ? static_cast< size_t >(z - buf) : got;
    fwrite(buf, 1, n, stdout);
    if (z) break;
  }
  printf("\n");
  fflush(stdout);

  CloseHandle(rd);
  return 0;
}
