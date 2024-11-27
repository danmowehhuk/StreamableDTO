#ifndef _StreamableManager_h
#define _StreamableManager_h


#include <Arduino.h>
#include "StreamableDTO.h"

class StreamableManager {

  private:
    StreamableManager(const StreamableManager &t) = delete;
    size_t _bufferBytes;
    String readStringLine(Stream* s, char terminator = '\n');
    static void sendWithFlowControl(const String &line, Stream* dest);

  public:
    // NOTE: 64 is Arduino's default Serial buffer size
    StreamableManager(size_t bufferBytes = 64): _bufferBytes(bufferBytes) {};

    // Provides a pointer to a StreamableDTO with the associated typeId
    typedef StreamableDTO* (*TypeMapper)(int16_t typeId);

    // Loads the stream data into memory, hydrating the provided DTO
    void load(Stream* src, StreamableDTO* dto, uint16_t lineNumStart = 0);
    
    // Loads the stream data into memory, using the TypeMapper function to 
    // get an instance of the correct type.
    //
    // NOTE: Only use this if you know the incoming stream starts with type
    //       and serial version identifiers. Also, the caller is responsible
    //       for reclaiming the DTO's memory
    StreamableDTO* load(Stream* src, TypeMapper typeMapper);
    
    // Streams the contents of the provided DTO to a stream
    void send(Stream* dest, StreamableDTO* dto);

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

    // Provides access to a line of buffer, which can then be copied or modified
    // before passing along to the destination stream (or not). The "state" arg is
    // a pointer to an object you can use to pass data into or out of the lambda.
    // Returning false terminates streaming.
    typedef bool (*FilterFunction)(const String &line, DestinationStream* dest, void* state);

    // Passes data from src to dest, holding only one line in memory at a time.
    // If the destination stream is nullptr, it's like streaming to /dev/null
    void pipe(Stream* src, Stream* dest, FilterFunction filter = nullptr, void* state = nullptr);

};


#endif