#ifndef STREAMABLEDTO_HAL_STREAM_H
#define STREAMABLEDTO_HAL_STREAM_H

#ifndef NO_ARDUINO

#include <Arduino.h>
using Stream = ::Stream;

#else

#include <stddef.h>
#include <stdint.h>

// Minimal interface covering exactly the methods StreamableManager
// actually calls on a Stream* - not Arduino's full Print/Stream surface.
class Stream {
  public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual size_t write(uint8_t byte) = 0;
    virtual int availableForWrite() = 0;
    virtual ~Stream() {}
};

#endif

#endif
