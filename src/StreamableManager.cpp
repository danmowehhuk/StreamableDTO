#include <Arduino.h>
#include "StreamableDTO.h"
#include "StreamableManager.h"

void StreamableManager::load(Stream* src, StreamableDTO* dto, uint16_t lineNumStart = 0) {
  if (dto->hasCustomLoader()) {
    dto->customLoad(src);
  } else {
    uint16_t lineNumber = lineNumStart;
    while (src->available()) {
      String line = readStringLine(src);
      dto->parseLine(lineNumber++, line);
    }
  }
}

StreamableDTO* StreamableManager::load(Stream* src, TypeMapper typeMapper) {
  String metaLine;
  if (src->available()) {
    metaLine = readStringLine(src);
  }
  String typeIdKey(StreamableDTO::pgmToString(StreamableDTOStrings::META_KEY));
  String minVerIdKey(StreamableDTO::pgmToString(StreamableDTOStrings::META_SEP));
  int typeIdIdx = metaLine.indexOf(typeIdKey);
  int minVerIdIdx = metaLine.indexOf(minVerIdKey);
  if (typeIdIdx == -1) {
#if defined(DEBUG)
    Serial.println(F("ERROR: Could not determine type from stream"));
#endif
    return nullptr;
  }
  uint16_t typeId = metaLine.substring(typeIdIdx + typeIdKey.length() + 1).toInt();
  uint8_t minCompatVersionId = metaLine.substring(minVerIdIdx + minVerIdKey.length()).toInt();
  StreamableDTO* dto = typeMapper(typeId);
  if (dto) {
    if (dto->checkTypeAndVersion(typeId, minCompatVersionId)) {
      load(src, dto, 1);
    } else {
      // Incorrect type or incompatible version
      delete dto;
      return nullptr;
    }
  }
  return dto;
}

void StreamableManager::send(Stream* dest, StreamableDTO* dto) {
  if (dto->getTypeId() != -1) {
    String metaLine(StreamableDTO::pgmToString(StreamableDTOStrings::META_KEY));
    metaLine += String(F("="));
    metaLine += String(dto->getTypeId());
    metaLine += String(StreamableDTO::pgmToString(StreamableDTOStrings::META_SEP));
    metaLine += String(dto->getMinCompatVersion());
    sendWithFlowControl(metaLine, dest);
  }
  if (dto->hasCustomSender()) {
    dto->customSend(dest);
  } else {
    for (StreamableDTO::KVIterator it = dto->begin(); it != dto->end(); ++it) {
      String key = it.key();
      key.trim();
      if (key.length() == 0) continue;
      String line = dto->toLine(key, it.value());
      sendWithFlowControl(line, dest);
    }
  }
}

void StreamableManager::pipe(Stream* src, Stream* dest, FilterFunction filter = nullptr, void* state = nullptr) {
  DestinationStream out(dest);
  while (src->available()) {
    String line = readStringLine(src);
    if (filter == nullptr) {
      out.println(line);
    } else {
      if (!filter(line, &out, state)) break;
    }
  }
}

String StreamableManager::readStringLine(Stream* s, char terminator = '\n') {
  char buffer[_bufferBytes];
  uint8_t i = 0;
  while (s->available()) {
    char c = s->read();
    if (c == terminator || i >= _bufferBytes - 1) {
#if defined(DEBUG)
      if (i >= _bufferBytes - 1) {
        Serial.println(String(F("readStringLine: line truncated to ")) + _bufferBytes + String(F(" chars")));
      }
#endif
      break;
    }
    buffer[i++] = c;
  }
  buffer[i] = '\0';
  String str(buffer);
  str.trim();
  return str;
}

void StreamableManager::sendWithFlowControl(const String &line, Stream* dest) {
  for (size_t i = 0; i < line.length(); i++) {
    while (dest->availableForWrite() == 0) {} // wait
    dest->write(line[i]);
  }
  while (dest->availableForWrite() == 0) {}
  dest->write('\n');
}
