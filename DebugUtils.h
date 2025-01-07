#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#define DEBUG_LEVEL 2 // 0 = Uit, 1 = Foutmeldingen, 2 = Info, 3 = Gedetailleerd

#if DEBUG_LEVEL > 0
  #define DEBUG_ERROR(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_ERROR(...)
#endif

#if DEBUG_LEVEL > 1
  #define DEBUG_INFO(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_INFO(...)
#endif

#if DEBUG_LEVEL > 2
  #define DEBUG_VERBOSE(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_VERBOSE(...)
#endif

#endif // DEBUGUTILS_H