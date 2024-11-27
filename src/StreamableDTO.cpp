#include "StreamableDTO.h"

void StreamableDTO::setProperty(const String& key, const String& value) {
  _table.put(key, value);
}

String StreamableDTO::getProperty(const String& key, const String& defaultValue) {
  String* valuePtr = _table.get(key);
  if (!valuePtr) {
    return defaultValue;
  }
  return *valuePtr;
}

void StreamableDTO::removeProperty(const String& key) {
  _table.remove(key);
}

bool StreamableDTO::containsKey(const String& key) {
  return _table.get(key) != nullptr;
}

SimpleVector<String> StreamableDTO::getKeys() {
  SimpleVector<String> keys = _table.keys();

  return keys;
}

void StreamableDTO::parseLine(uint16_t lineNumber, const String &line) {
  if (lineNumber == 0 && parseMetaLine(line)) return;
  String key = line;
  String value;
  int separatorIndex = line.indexOf('=');
  if (separatorIndex != -1) {
    key = line.substring(0, separatorIndex);
    key.trim();
    value = line.substring(separatorIndex + 1);
    value.trim();
  }
  parseValue(lineNumber, key, value);
}

bool StreamableDTO::parseMetaLine(const String &metaLine) {
  String typeIdKey(pgmToString(StreamableDTOStrings::META_KEY));
  String minVerIdKey(pgmToString(StreamableDTOStrings::META_SEP));
  int typeIdIdx = metaLine.indexOf(typeIdKey);
  int minVerIdIdx = metaLine.indexOf(minVerIdKey);
  if (typeIdIdx != -1) {
    uint16_t typeId = metaLine.substring(typeIdIdx + typeIdKey.length()).toInt();
    uint8_t minCompatVersionId = metaLine.substring(minVerIdIdx + minVerIdKey.length()).toInt();
    checkTypeAndVersion(typeId, minCompatVersionId);
    return true;
  }
  // not a meta line
  return false;
}

void StreamableDTO::parseValue(uint16_t lineNumber, const String& key, const String& value) {
  setProperty(key, value);
}

String StreamableDTO::toLine(const String& key, const String& value) {
  return key + "=" + value;
}

String StreamableDTO::pgmToString(const char* pgmString) {
  uint8_t len = strlen_P(pgmString);
  String result;
  result.reserve(len);  
  for (uint8_t i = 0; i < len; i++) {
    result += (char)pgm_read_byte(pgmString + i);
  }
  return result;
}

bool StreamableDTO::stringToBool(const String& str) {
  String lowerStr = str;
  lowerStr.trim();
  lowerStr.toLowerCase();
  return ( lowerStr == "1" || lowerStr == "true" || lowerStr == "yes" );
}

bool StreamableDTO::checkTypeAndVersion(uint16_t streamTypeId, uint8_t minCompatVersionId) {
  if (getTypeId() != streamTypeId) {
#if defined(DEBUG)
    Serial.println(String(F("ERROR: Type mismatch! Can't load DTO typeId=")) 
          + streamTypeId + " into typeId=" + getTypeId());
#endif
    return false;
  }
  if (getSerialVersion() < minCompatVersionId) {
#if defined(DEBUG)
    Serial.println(String(F("ERROR: Incompatible version for DTO typeId=")) 
          + getTypeId() + String(F(", have DTO v")) + getSerialVersion() 
          + String(F(" but stream requires >=v"))
          + minCompatVersionId);
#endif
    return false;
  }
  return true;
}