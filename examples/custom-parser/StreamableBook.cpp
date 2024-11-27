#include "StreamableBook.h"

void StreamableBook::parseValue(uint16_t lineNumber, const String& key, const String& value) {
  if (key.equals("meta")) {
    int sepIdx = value.indexOf('|');
    String publisher = value.substring(0, sepIdx);
    String year = value.substring(sepIdx + 1);
    publisher.trim();
    _publisher = publisher;
    year.trim();
    _year = year;

    // Need to add the key with an empty value so send(...) includes it
    setProperty(String(F("meta")), String()); 
  } else {
    // Send to the default value parser
    StreamableDTO::parseValue(lineNumber, key, value);
  }
}

String StreamableBook::toLine(const String& key, const String& value) {
  if (key.equals("meta")) {
    // ignore the value, it's empty anyway
    return key + "=" + _publisher + "|" + _year;
  }
  
  // Send to the default serializer
  return StreamableDTO::toLine(key, value);
}
