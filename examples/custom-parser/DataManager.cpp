#include <Arduino.h>
#include "Book.h"
#include "DataManager.h"
#include "StreamableBook.h"
#include "StreamHelper.h"


Book* DataManager::getBook(const String& name) {
  if (_book != nullptr) {
    delete _book; // Clean up old book memory
  }
  _book = new StreamableBook();
  Stream* s = getStream(name);
  load(s, static_cast<StreamableBook*>(_book));
  return _book;
}

void DataManager::streamToSerial(Book* book) {
  // Note that &Serial could be replaced with an SD file, a UART
  // connection, bluetooth, etc.
  send(&Serial, static_cast<StreamableBook*>(book));
}

Stream* DataManager::getStream(const String &name) {
  if (name.equals("Catcher In The Rye")) {
    _citr.reset();
    return &_citr;
  } else if (name.equals("Les Miserables")) {
    _lesMis.reset();
    return &_lesMis;
  } else {
    _unknown.reset();
    return &_unknown;
  }
}
