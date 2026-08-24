#ifndef _StringStream_h
#define _StringStream_h

#include "hal/Stream.h"
#include "hal/FlashStr.h"

#ifndef NO_ARDUINO
#include <Arduino.h>
#endif

/*
 * A Stream backed by an in-memory string
 */
class StringStream : public Stream {
  public:

#ifndef NO_ARDUINO
    // Construct an input stream (source) from an Arduino String
    StringStream(const String& str);
#endif

    // Construct an input stream (source)
    StringStream(const char* str);
    StringStream(const FlashStr* fstr);

    // Construct an output stream (sink)
    // Default output stream is 128 bytes
    StringStream();

    // Construct an output stream (sink) with a specified capacity
    StringStream(size_t capacity);

    ~StringStream();

    size_t write(uint8_t byte) override;
    int available() override;
    int availableForWrite() override;
    int read() override;
    int peek();
    void flush();
#ifndef NO_ARDUINO
    String getString();
#endif
    char* get();
    void reset();
    void toInStream();

  private:
    char* _buffer = nullptr;
    size_t _pos = 0;
    size_t _length = 0;
    size_t _capacity = 0;
    bool _outStream = false;

    void initFromCString(const char* str, size_t len);
};

#endif
