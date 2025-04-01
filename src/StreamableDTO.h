/*

  StreamableDTO.h

  Serializable data objects for Arduino

  Copyright (c) 2025, Dan Mowehhuk (danmowehhuk@gmail.com)
  All rights reserved.

*/

#ifndef _strdto_StreamableDTO_h
#define _strdto_StreamableDTO_h


#include <Arduino.h>

/*
 * A collection of arbitrary key-value pairs that is accessible with
 * hashtable semantics, but can also be serialized/deserialized from 
 * an Arduino Stream via the StreamableManager class
 *
 * Stripped-down hashtable implementation that only allows simple char
 * arrays for keys and values. Either the key, value or both may be 
 * located in PROGMEM.
 */

class StreamableDTO {

  private:

    struct Entry {
      const char* key;
      char* value;
      Entry* next;
      bool keyPmem;
      bool valPmem;
      Entry(const char* k, const char* v, bool keyPmem, bool valPmem);
      ~Entry();
    };

    /*
     * Instance vars - note that _tableSize indicates the number of buckets
     * in the table, whether or not they are used/overloaded. _count indicates
     * the actual number of Entry's in the table.
     */
    static const int INITIAL_TABLE_SIZE = 8;
    Entry** _table;
    int _tableSize;
    int _count;
    float _loadFactorThreshold = 0.7;

    /*
     * The hash is based on the _content_ that the char* points to, but the
     * caller must indicate whether the key is a pointer to PROGMEM or
     * regular memory
     */
    int hash(const char* key, bool pmem = false);
    int hash(const __FlashStringHelper* key);

    /*
     * Once a key has been hashed to a bucket, the Entry's in that bucket
     * need to be checked to see which (if any) the key actually matches.
     * Since either or both of the key passed in and the entry->key could
     * be coming from PROGMEM, it may be necessary to copy one of the values
     * into regular memory before performing a strcmp(s1, s2).
     *
     * For efficiency, if both the passed key and the entry->key are in
     * PROGMEM, then only pointer equality is checked.
     */
    bool keyMatches(const char* key, const Entry* entry, bool keyPmem);
    bool keyMatches(const __FlashStringHelper* key, const Entry* entry);

    /*
     * Resize the table, rehashing all keys to redistribute entries.
     */
    bool resize(int newSize);

    struct MetaInfo {
      int16_t typeId;
      uint8_t minCompatVersion;
      MetaInfo(int16_t typeId, uint8_t minCompatVersion): 
            typeId(typeId), minCompatVersion(minCompatVersion) {};
    };

    bool isCompatibleTypeAndVersion(MetaInfo* meta);

    friend class HashtableTestHelper; // test/test-suite/HashtableTestHelper.h


  public:
    StreamableDTO();
    StreamableDTO(size_t initialCapacity, float loadFactor = 0.7) ;
    virtual ~StreamableDTO();

    /*
     * Puts a key-value pair in the table. If the key already exists,
     * update the value. Resize the table if necessary.
     */
    bool put(const char* key, const char* value, bool keyPmem = false, bool valPmem = false);
    bool put(const char* key, const __FlashStringHelper* value, bool keyPmem = false);
    bool put_P(const char* key, const char* value, bool valPmem = false);
    bool put_P(const char* key, const __FlashStringHelper* value);
    bool put(const __FlashStringHelper* key, const char* value, bool valPmem = false);
    bool put(const __FlashStringHelper* key, const __FlashStringHelper* value);
    bool putEmpty(const char* key, bool pmemKey);
    bool putEmpty(const __FlashStringHelper* key);
    bool putEmpty_P(const char* key);

    /*
     * Checks if a key exists in the table.
     */
    bool exists(const char* key, bool keyPmem = false) const;
    bool exists(const __FlashStringHelper* key) const;
    bool exists_P(const char* key) const;

    /*
     * Gets the raw value pointer associated with the given key. The
     * caller is expected to know whether the return value points to 
     * PROGMEM or regular memory.
     *
     * Returns a nullptr if the key is not found.
     */
    char* get(const char* key, bool keyPmem = false) const;
    char* get(const __FlashStringHelper* key) const;
    char* get_P(const char* key) const;

    /*
     * Removes the entry with the given key if it exists. Returns
     * true if an entry was removed, otherwise false.
     */
    bool remove(const char* key, bool keyPmem = false);
    bool remove(const __FlashStringHelper* key);
    bool remove_P(const char* key);

    /*
     * Removes all the entries from the table and resets it to its
     * initial size.
     */
    bool clear();

    // Disable moving and copying
    StreamableDTO(StreamableDTO&& other) = delete;
    StreamableDTO& operator=(StreamableDTO&& other) = delete;
    StreamableDTO(const StreamableDTO&) = delete;
    StreamableDTO& operator=(const StreamableDTO&) = delete;


  protected:
    friend class StreamableManager;

    virtual int16_t getTypeId()          {  return -1; };
    virtual uint8_t getSerialVersion()    {  return 0;  };
    virtual uint8_t getMinCompatVersion() {  return 0;  };

    /*
     * Do something with the key and value extracted from an Entry.
     * StreamableManager prints 'key=value' lines to a destination
     * stream, for example.
     *
     * Return true if successful
     */
    typedef bool (*EntryProcessor)(const char* key, const char* value, bool keyPmem, bool valPmem, void* capture);

    /*
     * Iterate through all the Entry's in the table and pass the keys and values
     * to the entryProcessor as raw char[]s with booleans indicating whether 
     * they are stored in PROGMEM or regular memory. Returns true if all the 
     * Entry's were successfully handled.
     */
    bool processEntries(EntryProcessor entryProcessor, void* capture = nullptr);

    /*
     * Default implementation parses a key=value format line. 
     * If there is no =, value is an empty string. Returns false if
     * parsing fails.
     */
    virtual bool parseLine(uint16_t lineNumber, const char* line);

    /* 
     * Parses the special meta line containing typeId and minCompatVersion, verifying
     * compatibility. Returns false of the provided line is not a meta line
     */
    static MetaInfo* parseMetaLine(const char* metaLine);

    /*
     * Default implementation simply puts the value in _table under "key". You may want
     * to override to switch from the incoming RAM key to a PROGMEM key.
     */
    virtual void parseValue(uint16_t lineNumber, const char* key, const char* value);

    /*
     * Default implementation returns "key=value". The return value must be created using 
     * "new char[lineLength]" and will be automatically free()'d.
     */
    virtual char* toLine(const char* key, const char* value, bool keyPmem, bool valPmem);

};
 


#endif