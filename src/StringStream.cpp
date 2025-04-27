#include "StringStream.h"

StringStream::StringStream(const String& str) : _pos(0), _outStream(false) {
  initFromCString(str.c_str(), str.length());
}

StringStream::StringStream(const char* str) : _pos(0), _outStream(false) {
  size_t len = strlen(str);
  initFromCString(str, len);
}

void StringStream::initFromCString(const char* str, size_t len) {
  _capacity = len;
  _length = len;
  _buffer = new char[len + 1]();
  memcpy(_buffer, str, len);
  _buffer[len] = '\0';
}

StringStream::StringStream(const __FlashStringHelper* fstr) : _pos(0), _outStream(false) {
  const char* progmemStr = reinterpret_cast<const char*>(fstr);
  size_t len = strlen_P(progmemStr);
  _capacity = len;
  _length = len;
  _buffer = new char[len + 1]();
  strncpy_P(_buffer, progmemStr, len);
  _buffer[len] = '\0';
}

StringStream::StringStream() : _pos(0), _outStream(true) {
  _capacity = 128; // default output buffer size
  _buffer = new char[_capacity + 1]();
  _length = 0;
}

StringStream::~StringStream() {
  delete[] _buffer;
}

void StringStream::toInStream() {
  if (_outStream) {
    _outStream = false;
    reset();
  }
}

size_t StringStream::write(uint8_t byte) {
  if (!_outStream || _length >= _capacity) return 0;
  _buffer[_length++] = static_cast<char>(byte);
  _buffer[_length] = '\0';
  return 1;
}

int StringStream::available() {
  return (_outStream || !_buffer) ? 0 : (_pos < _length);
}

int StringStream::availableForWrite() {
  return _outStream ? (_capacity - _length) : 0;
}

int StringStream::read() {
  if (_outStream || _pos >= _length) return -1;
  return _buffer[_pos++];
}

int StringStream::peek() {
  if (_outStream || _pos >= _length) return -1;
  return _buffer[_pos];
}

void StringStream::flush() {
  _pos = _length;
}

String StringStream::getString() {
  return String(_buffer);
}

char* StringStream::get() {
  return _buffer;
}

void StringStream::reset() {
  _pos = 0;
  if (_outStream) {
    if (_buffer) delete[] _buffer;
    _capacity = 128; // default output buffer size
    _buffer = new char[_capacity + 1]();
    _length = 0;
    _buffer[0] = '\0';
  }
}
