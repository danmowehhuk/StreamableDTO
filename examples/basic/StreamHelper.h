#ifndef StreamHelper_h
#define StreamHelper_h

#include <Arduino.h>

class StreamHelper : public Stream {
public:
  StreamHelper(String& str) : _str(str), _pos(0) {}

  size_t write(uint8_t byte) override {
    _str += (char)byte;
    return 1;
  }

  int available() override {
    return _pos < _str.length();
  }

  int read() override {
    if (_pos >= _str.length()) return -1;
    return _str[_pos++];
  }

  int peek() override {
    if (_pos >= _str.length()) return -1;
    return _str[_pos];
  }

  void flush() override {
    _pos = _str.length();  // Move to the end of the string
  }

  void reset() {
    _pos = 0;
  }

private:
    String& _str;
    size_t _pos;
};

#endif
