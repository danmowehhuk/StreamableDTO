#ifndef STREAMABLEDTO_HAL_STREAMABLEDTOHAL_H
#define STREAMABLEDTO_HAL_STREAMABLEDTOHAL_H

#include "FlashStr.h"

#ifndef NO_ARDUINO
#include <Arduino.h>
#else
#include <BareMetalHAL.h>
#endif

namespace StreamableDTOHal {

#ifndef NO_ARDUINO

inline void print(const FlashStr* s) { Serial.print(s); }
inline void print(int i) { Serial.print(i); }
inline void println(const FlashStr* s) { Serial.println(s); }
inline void println(int i) { Serial.println(i); }

#else

inline void print(const FlashStr* s) { BareMetalHAL::Uart0::print(s); }
inline void print(int i) { BareMetalHAL::Uart0::print(i); }
inline void println(const FlashStr* s) { BareMetalHAL::Uart0::println(s); }
inline void println(int i) { BareMetalHAL::Uart0::println(i); }

#endif

}  // namespace StreamableDTOHal

#endif
