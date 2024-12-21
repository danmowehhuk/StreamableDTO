#ifndef _StringStream_h
#define _StringStream_h


#include <Arduino.h>

/*
 * A Stream backed by an in-memory String object
 */
class StringStream : public Stream {

  public:
    StringStream(String& str) : _str(str), _pos(0), _outStream(false) {}
    StringStream() : _str(*new String()), _pos(0), _outStream(true) {}
    size_t write(uint8_t byte) override {
      _str += (char)byte;
      return 1;
    };
    int available() override {
      return _pos < _str.length();
    };
    int availableForWrite() override {
      return _outStream ? INT8_MAX : 0;
    };
    int read() override {
      if (_pos >= _str.length()) return -1;
      return _str[_pos++];
    };
    int peek() override {
      if (_pos >= _str.length()) return -1;
      return _str[_pos];
    };
    void flush() override {
      _pos = _str.length();  // Move to the end of the string
    };
    String getString() {
      return _str;
    };
    void reset() {
      if (_outStream) {
        _str = *new String();
      }
      _pos = 0;
    };

  private:
    String& _str;
    size_t _pos;
    bool _outStream;

};


#endif
