
#ifndef STRING_HELPER_H_
#define STRING_HELPER_H_

#ifdef _WIN32
// Win
#include <Windows.h>
#include <string>
#else
// Linux
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <errno.h>
#endif

namespace
{
#ifdef _WIN32
  template <class T>
  std::string wstringToString(T ws)
  {
    // wstring to UTF8
    int iBufferSize = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, (char*)NULL, 0, NULL, NULL);
    char* cpMultiByte = new char[iBufferSize];
    // wstring to UTF8
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, cpMultiByte, iBufferSize, NULL, NULL);
    std::string oRet(cpMultiByte, cpMultiByte + iBufferSize - 1);
    delete[] cpMultiByte;
    return oRet;
  }

  template <class T>
  std::wstring UTF8ToUnicode(T str)
  {
    int unicodeLen = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t* pUnicode;
    pUnicode = new wchar_t[unicodeLen + 1];
    memset(pUnicode, 0, (unicodeLen + 1) * sizeof(wchar_t));
    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, (LPWSTR)pUnicode, unicodeLen);
    std::wstring rt;
    rt = (wchar_t*)pUnicode;
    delete[] pUnicode;
    return rt;
  }
#else
  const int UNICODELEN_MAX_SIZE = 256;

  template <class T>
  std::string wstringToString(T ws)
  {
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    size_t len = wcsrtombs(NULL, &ws.c_str(), 0, &state);
    char* cpMultiByte = new char[len + 1];
    memset(cpMultiByte, 0, (len + 1) * sizeof(char));
    wcsrtombs(cpMultiByte, &ws.c_str(), len, &state);
    std::string oRet;
    oRet = (char*)cpMultiByte;
    delete[] cpMultiByte;
    return oRet;
  }

  template <class T>
  std::wstring UTF8ToUnicode(T str)
  {
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    size_t len = mbsrtowcs(NULL, &str.c_str(), 0, &state);
    wchar_t* pUnicode = new wchar_t[len + 1];
    memset(pUnicode, 0, (len + 1) * sizeof(wchar_t));
    mbsrtowcs(pUnicode, &str.c_str(), len, &state);
    std::wstring rt;
    rt = (wchar_t*)pUnicode;
    delete[] pUnicode;
    return rt;
  }
#endif
}

#endif // STRING_HELPER_H_
