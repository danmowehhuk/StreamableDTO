#ifndef _strdto_StreamableManager_h
#define _strdto_StreamableManager_h


#include <Arduino.h>
#include "StreamableDTO.h"

/*
 * Serializes and deserializes StreamableDTO objects to/from Streams. If using
 * the base StreamableDTO class, all keys and values will be stored in regular
 * memory as null-terminated char arrays. Subclasses of StreamableDTO can 
 * define their keys in PROGMEM for better memory and compute efficiency.
 *
 * This class can also pipe from one stream to another line by line, applying
 * a per-line filter function.
 */
class StreamableManager {

  private:
    StreamableManager(const StreamableManager &t) = delete;
    
    size_t _bufferBytes = 64; // Same as Arduino's default serial buffer size

    /*
     * Reads characters from a Stream until a terminator character or the max
     * buffer size is reached (a newline is the default terminator).
     */
    char* readLine(Stream* s, char terminator = '\n') {
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
    };

    /*
     * Sends a string to the destination Stream. A newline character is 
     * sent automatically. This method waits until the destination stream
     * has space in its buffer before sending the next char.
     */
    static void sendWithFlowControl(const char* line, Stream* dest) {
      for (size_t i = 0; i < strlen(line); i++) {
        while (dest->availableForWrite() == 0) {} // wait
        dest->write(line[i]);
      }
      while (dest->availableForWrite() == 0) {}
      dest->write('\n');
    };

    static void sendMetaLine(StreamableDTO* dto, Stream* dest) {
      constexpr size_t keyLen = 6;
      char key[keyLen + 1];
      strcpy_P(key, PSTR("__tvid"));
      constexpr size_t totalLen = keyLen + 1 + 5 + 1 + 3 + 1; // "__tvid=65535|255\0"
      char metaLine[totalLen];
      const uint16_t typeId = dto->getTypeId();
      const uint8_t minCompat = dto->getMinCompatVersion();
      static const char format[] PROGMEM = "%s=%u|%u";
      snprintf_P(metaLine, totalLen, format, key, typeId, minCompat);
      sendWithFlowControl(metaLine, dest);
    };

  public:
    StreamableManager() {};
    StreamableManager(size_t bufferBytes): _bufferBytes(bufferBytes) {};

    /*
     * Function that returns an instantiation of the StreamableDTO sub-
     * class for the given typeId, or nullptr for an unknown typeId. Note
     * that typeId=-1 indicates that an instance of StreamableDTO itself
     * should be used
     */
    typedef StreamableDTO* (*TypeMapper)(int16_t typeId);

    /*
     * Loads the stream data into memory, hydrating the provided DTO and 
     * verifying the sub-type and version for compatibility. If the provided
     * DTO is incompatible with the incoming data, returns false and does
     * not populate the DTO
     */
    bool load(Stream* src, StreamableDTO* dto, uint16_t lineNumStart = 0) {
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
              if (line) free(line);
              return false; // incompatible type or version
            }
          }
        }
        if (!dto->parseLine(lineNumber++, line)) {
          if (line) free(line);
          return false;
        }
        if (line) free(line);
      }
      return true;
    };
    
    /*
     * Loads the stream data into memory, using the TypeMapper function to 
     * get an instance of the correct type.
     *
     * NOTE: Only use this if you know the incoming stream starts with type
     *       and serial version identifiers. Also, the caller is responsible
     *       for reclaiming the DTO's memory
     */
    StreamableDTO* load(Stream* src, TypeMapper typeMapper) {
      char* metaLine;
      if (src->available()) {
        metaLine = readLine(src);
      }
      StreamableDTO::MetaInfo* meta = StreamableDTO::parseMetaLine(metaLine);
      if (metaLine) free(metaLine);
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
      } else {
        // Incorrect type or incompatible version
        delete dto;
        delete meta;      
        return nullptr;
      }
      delete meta;      
      return dto;
    };
    
    /*
     * Streams the contents of the provided DTO to a stream
     */
    void send(Stream* dest, StreamableDTO* dto) {
      if (dto->getTypeId() != -1) {
        sendMetaLine(dto, dest);
      }
      struct Capture {
        Stream* dest;
        StreamableDTO* dto;
        Capture(Stream* dest, StreamableDTO* dto): dest(dest), dto(dto) {};
      };
      Capture capture(dest, dto);
      auto entryProcessor = [](const char* key, const char* value, bool keyPmem, bool valPmem, void* capture) -> bool {
        Capture* c = static_cast<Capture*>(capture);
        char* line = c->dto->toLine(key, value, keyPmem, valPmem);
        if (line) {
          sendWithFlowControl(line, c->dest);
          free(line);
        }
        return true;
      };
      dto->processEntries(entryProcessor, &capture);
    };

    // Wraps a raw stream providing null checking and flow control
    class DestinationStream {
      public:
        DestinationStream(Stream* dest): _dest(dest) {};
        void println(const char* line) {
          if (_dest != nullptr) {
            StreamableManager::sendWithFlowControl(line, _dest);
          }
        };
      private:
        DestinationStream(const DestinationStream &t) = delete;
        Stream* _dest = nullptr;
    };

    /*
     * Provides access to a line of buffer, which can then be copied or modified
     * before passing along to the destination stream (or not). The "state" arg is
     * a pointer to an object you can use to pass data into or out of the lambda.
     * Returning false terminates streaming.
     */
    typedef bool (*FilterFunction)(const char* line, DestinationStream* dest, void* state);

    /*
     * Passes data from src to dest, holding only one line in memory at a time.
     * If the destination stream is nullptr, it's like streaming to /dev/null
     */
    void pipe(Stream* src, Stream* dest, FilterFunction filter = nullptr, void* state = nullptr) {
      DestinationStream out(dest);
      while (src->available()) {
        char* line = readLine(src);
        if (filter == nullptr) {
          out.println(line);
        } else {
          if (!filter(line, &out, state)) break;
        }
        if (line) free(line);
      }
    };

};


#endif