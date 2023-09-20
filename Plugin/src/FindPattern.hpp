#pragma once

#include <cstring>
#include <psapi.h>

inline bool MatchPattern(uintptr_t start, const unsigned char* pattern, const char* mask)
{
  for (size_t i = 0; mask[i]; i++)
  {
	  if (mask[i] != '?' && reinterpret_cast<const unsigned char*>(start)[i] != pattern[i]) return false;
  }

  return true;
}

inline uintptr_t FindPattern(uintptr_t start, size_t length, const unsigned char* pattern, const char* mask)
{
  const auto last = start + length - std::strlen(mask);

  for (auto it = start; it <= last; ++it)
  {
	  if (MatchPattern(it, pattern, mask)) return it;
  }

  return -1;
}

inline uintptr_t FindPattern(HMODULE module, const unsigned char* pattern, const char* mask)
{
  MODULEINFO info = { };
  GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

  return FindPattern(reinterpret_cast<uintptr_t>(module), info.SizeOfImage, pattern, mask);
}