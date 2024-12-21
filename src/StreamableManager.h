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
    String readStringLine(Stream* s, char terminator = '\n') {
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
    };

    /*
     * Sends a String to the destination Stream. A newline character is 
     * sent automatically. This method waits until the destination stream
     * has space in its buffer before sending the next char.
     */
    static void sendWithFlowControl(const String &line, Stream* dest) {
      for (size_t i = 0; i < line.length(); i++) {
        while (dest->availableForWrite() == 0) {} // wait
        dest->write(line[i]);
      }
      while (dest->availableForWrite() == 0) {}
      dest->write('\n');
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
        String line = readStringLine(src);
        if (lineNumber == 0) {
          StreamableDTO::MetaInfo* meta = dto->parseMetaLine(line);
          if (meta) {
            if (dto->isCompatibleTypeAndVersion(meta)) {
              delete meta;
              lineNumber++;
              continue;
            } else {
              return false; // incompatible type or version
            }
          }
        }
        if (!dto->parseLine(lineNumber++, line)) {
          return false;
        }
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
      String metaLine;
      if (src->available()) {
        metaLine = readStringLine(src);
      }
      StreamableDTO::MetaInfo* meta = StreamableDTO::parseMetaLine(metaLine);
      if (!meta) {
#if defined(DEBUG)
        Serial.println(F("ERROR: Could not determine type from stream"));
#endif
        return nullptr;        
      }
      StreamableDTO* dto = typeMapper(meta->typeId);
      if (!dto) {
#if defined(DEBUG)
        Serial.println(String(F("ERROR: Unknown typeId: ")) + String(meta->typeId));
#endif
        return nullptr;        
      }
      if (dto->isCompatibleTypeAndVersion(meta)) {
        load(src, dto, 1);
      } else {
        // Incorrect type or incompatible version
        delete dto;
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
        String metaLine(String(F("__tvid")));
        metaLine += String(F("="));
        metaLine += String(dto->getTypeId());
        metaLine += String(F("|"));
        metaLine += String(dto->getMinCompatVersion());
        sendWithFlowControl(metaLine, dest);
      }
      auto lineHandler = [](const String& line, void* capture) -> bool {
        Stream* dest = static_cast<Stream*>(capture);
        sendWithFlowControl(line, dest);
        return true;
      };
      dto->entriesToLines(lineHandler, dest);
    };

    // Wraps a raw stream providing null checking and flow control
    class DestinationStream {
      public:
        DestinationStream(Stream* dest): _dest(dest) {};
        void println(const String &line) {
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
    typedef bool (*FilterFunction)(const String &line, DestinationStream* dest, void* state);

    /*
     * Passes data from src to dest, holding only one line in memory at a time.
     * If the destination stream is nullptr, it's like streaming to /dev/null
     */
    void pipe(Stream* src, Stream* dest, FilterFunction filter = nullptr, void* state = nullptr) {
      DestinationStream out(dest);
      while (src->available()) {
        String line = readStringLine(src);
        if (filter == nullptr) {
          out.println(line);
        } else {
          if (!filter(line, &out, state)) break;
        }
      }
    };

};


#endif