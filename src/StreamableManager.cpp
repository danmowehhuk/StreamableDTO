#include "StreamableManager.h"

char* StreamableManager::readLine(Stream* s, char terminator = '\n') {
  char* buffer = new char[_bufferBytes]();
  uint8_t i = 0;
  while (s->available()) {
    char c = s->read();
    if (c == terminator || i >= _bufferBytes - 1) {
#if defined(DEBUG)
      if (i >= _bufferBytes - 1) {
        Serial.print(F("readLine: line truncated to "));
        Serial.print(_bufferBytes);
        Serial.println(F(" chars"));
      }
#endif
      break;
    }
    buffer[i++] = c;
  }
  buffer[i] = '\0';

  // Trim leading and trailing whitespace
  char* start = buffer;
  while (isspace(*start)) start++;
  char* end = start + strlen(start) - 1;
  while (end > start && isspace(*end)) *end-- = '\0';
  if (start != buffer) {
    // Allocate a new trimmed string
    size_t len = strlen(start);
    char* trimmed = new char[len + 1];
    strncpy(trimmed, start, len);
    trimmed[len] = '\0';
    delete[] buffer;
    return trimmed;
  }
  return buffer;
}

void StreamableManager::sendWithFlowControl(const char* line, Stream* dest) {
  for (size_t i = 0; i < strlen(line); i++) {
    while (dest->availableForWrite() == 0) {} // wait
    dest->write(line[i]);
  }
  while (dest->availableForWrite() == 0) {}
  dest->write('\n');
}

void StreamableManager::sendWithoutFlowControl(const char* line, Stream* dest) {
  for (size_t i = 0; i < strlen(line); i++) {
    dest->write(line[i]);
  }
  dest->write('\n');
}

void StreamableManager::sendMetaLine(StreamableDTO* dto, Stream* dest, bool flowControl = false) {
  constexpr size_t keyLen = 6;
  char key[keyLen + 1];
  strcpy_P(key, PSTR("__tvid"));
  constexpr size_t totalLen = keyLen + 1 + 5 + 1 + 3 + 1; // "__tvid=65535|255\0"
  char metaLine[totalLen];
  const uint16_t typeId = dto->getTypeId();
  const uint8_t serialVer = dto->getSerialVersion();
  static const char format[] PROGMEM = "%s=%u|%u";
  snprintf_P(metaLine, totalLen, format, key, typeId, serialVer);
  if (flowControl) {
    sendWithFlowControl(metaLine, dest);
  } else {
    sendWithoutFlowControl(metaLine, dest);
  }
}

bool StreamableManager::load(Stream* src, StreamableDTO* dto, uint16_t lineNumStart = 0) {
  uint16_t lineNumber = lineNumStart;
  while (src->available()) {
    char* line = readLine(src);
    if (lineNumber == 0) {
      StreamableDTO::MetaInfo* meta = dto->parseMetaLine(line);
      if (meta) {
        if (dto->isCompatibleTypeAndVersion(meta)) {
          delete meta;
          lineNumber++;
          continue;
        } else {
          if (line) delete[] line;
          return false; // incompatible type or version
        }
      }
    }
    if (!dto->parseLine(lineNumber++, line)) {
      if (line) delete[] line;
      return false;
    }
    if (line) delete[] line;
  }
  return true;
}

StreamableDTO* StreamableManager::load(Stream* src, TypeMapper typeMapper) {
  char* metaLine;
  if (src->available()) {
    metaLine = readLine(src);
  }
  StreamableDTO::MetaInfo* meta = StreamableDTO::parseMetaLine(metaLine);
  if (metaLine) delete[] metaLine;
  if (!meta) {
#if defined(DEBUG)
    Serial.println(F("ERROR: Could not determine type from stream"));
#endif
    return nullptr;        
  }
  StreamableDTO* dto = typeMapper(meta->typeId);
  if (!dto) {
#if defined(DEBUG)
    Serial.print(F("ERROR: Unknown typeId: "));
    Serial.println(meta->typeId);
#endif
    delete meta;      
    return nullptr;        
  }
  if (dto->isCompatibleTypeAndVersion(meta)) {
    load(src, dto, 1);
    dto->_deserializedVer = meta->serialVersion;
  } else {
    // Incorrect type or incompatible version
    delete dto;
    delete meta;      
    return nullptr;
  }
  delete meta;      
  return dto;
}

void StreamableManager::send(Stream* dest, StreamableDTO* dto, bool flowControl = false) {
  if (dto->getTypeId() != -1) {
    sendMetaLine(dto, dest, flowControl);
  }
  struct Capture {
    Stream* dest;
    StreamableDTO* dto;
    size_t bufferSize;
    bool flowControl;
    Capture(Stream* dest, StreamableDTO* dto, size_t bufferSize, bool flowControl):
        dest(dest), dto(dto), bufferSize(bufferSize), flowControl(flowControl) {};
  };
  auto entryProcessor = [](const char* key, const char* value, bool keyPmem, bool valPmem, void* capture) -> bool {
    Capture* c = static_cast<Capture*>(capture);
    char line[c->bufferSize];
    if (c->dto->toLine(key, value, keyPmem, valPmem, line, c->bufferSize)) {
      if (c->flowControl) {
        sendWithFlowControl(line, c->dest);
      } else {
        sendWithoutFlowControl(line, c->dest);
      }
    }
    return true;
  };
  Capture capture(dest, dto, _bufferBytes, flowControl);
  dto->processEntries(entryProcessor, &capture);
}

void StreamableManager::pipe(Stream* src, Stream* dest, FilterFunction filter = nullptr, bool flowControl = false, void* state = nullptr) {
  if (!src) {
#if (defined(DEBUG))
    Serial.println(F("ERROR: StreamableManager::pipe src or dest stream is nullptr"));
#endif    
    return;
  }
  DestinationStream out(dest);
  bool stop = false;
  while (src->available()) {
    char* line = readLine(src);
    if (filter == nullptr) {
      out.println(line, flowControl);
    } else {
      stop = !filter(line, &out, state);
    }
    if (line) delete[] line;
    if (stop) break;
  }
}

