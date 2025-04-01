/*

  StreamableManager.h

  Send and receive StreamableDTOs over any Stream.

  Copyright (c) 2025, Dan Mowehhuk (danmowehhuk@gmail.com)
  All rights reserved.

*/

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
    size_t _bufferBytes = 64; // Same as Arduino's default serial buffer size

    /*
     * Reads characters from a Stream until a terminator character or the max
     * buffer size is reached (a newline is the default terminator).
     */
    char* readLine(Stream* s, char terminator = '\n');

    /*
     * Sends a string to the destination Stream. A newline character is 
     * sent automatically. This method waits until the destination stream
     * has space in its buffer before sending the next char.
     */
    static void sendWithFlowControl(const char* line, Stream* dest);
    static void sendMetaLine(StreamableDTO* dto, Stream* dest);

  public:
    StreamableManager() {};
    StreamableManager(size_t bufferBytes): _bufferBytes(bufferBytes) {};

    /*
     * Loads the stream data into memory, hydrating the provided DTO and 
     * verifying the sub-type and version for compatibility. If the provided
     * DTO is incompatible with the incoming data, returns false and does
     * not populate the DTO
     */
    bool load(Stream* src, StreamableDTO* dto, uint16_t lineNumStart = 0);
    
    /*
     * Function that returns an instantiation of the StreamableDTO sub-
     * class for the given typeId, or nullptr for an unknown typeId. Note
     * that typeId=-1 indicates that an instance of StreamableDTO itself
     * should be used
     */
    typedef StreamableDTO* (*TypeMapper)(int16_t typeId);

    /*
     * Loads the stream data into memory, using the TypeMapper function to 
     * get an instance of the correct type.
     *
     * NOTE: Only use this if you know the incoming stream starts with type
     *       and serial version identifiers. Also, the caller is responsible
     *       for reclaiming the DTO's memory
     */
    StreamableDTO* load(Stream* src, TypeMapper typeMapper);
    
    /*
     * Streams the contents of the provided DTO to a stream
     */
    void send(Stream* dest, StreamableDTO* dto);

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
    void pipe(Stream* src, Stream* dest, FilterFunction filter = nullptr, void* state = nullptr);

    // Disable moving and copying
    StreamableManager(StreamableManager&& other) = delete;
    StreamableManager& operator=(StreamableManager&& other) = delete;
    StreamableManager(const StreamableManager&) = delete;
    StreamableManager& operator=(const StreamableManager&) = delete;

};


#endif