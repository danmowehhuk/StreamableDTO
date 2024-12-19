#ifndef _strdto_StreamableDTO_h
#define _strdto_StreamableDTO_h


/*
 * A collection of arbitrary key-value pairs that is accessible with
 * hashtable semantics, but can also be serialized/deserialized from 
 * an Arduino Stream via the StreamableManager class
 *
 * Stripped-down hashtable implementation that only allows char*'s for keys
 * and values. Either the key, value or both may be located in PROGMEM. Any
 * non-PROGMEM char*'s must be null-terminated.
 */
class StreamableDTO {

  private:
    StreamableDTO(const StreamableDTO &t) = delete;
    friend class HashtableTestHelper; // test/HashtableTest/HashtableTestHelper.h

    /*
     * Indicates whether the key, value or both should be read from PROGMEM
     * versus regular memory. This is stored in the Entry so that iterating
     * operations read the key/value properly.
     */
    enum EntryMemoryType : uint8_t {
      PMEM_KEY,
      PMEM_VALUE,
      PMEM_KEY_VALUE,
      NON_PMEM
    };

    EntryMemoryType getMemoryType(bool keyPmem, bool valPmem) {
      if (keyPmem && !valPmem) {
        return PMEM_KEY;
      } else if (!keyPmem && valPmem) {
        return PMEM_VALUE;
      } else if (keyPmem && valPmem) {
        return PMEM_KEY_VALUE;
      } else {
        return NON_PMEM;
      }
    };

    struct Entry {
      EntryMemoryType type;
      const char* key;
      const char* value;
      Entry* next;
      Entry(EntryMemoryType type, const char* k, const char* v):
          type(type), key(nullptr), value(nullptr), next(nullptr) {
        if (type == PMEM_VALUE || type == NON_PMEM) {
          key = new char[strlen(k) + 1];
          strcpy(key, k);
        } else {
          key = k; 
        }
        if (type == PMEM_KEY || type == NON_PMEM) {
          value = new char[strlen(v) + 1];
          strcpy(value, v);
        } else {
          value = v; 
        }
      };
      ~Entry() {
        if (type == PMEM_VALUE || type == NON_PMEM) {
          delete[] key;
        }
        if (type == PMEM_KEY || type == NON_PMEM) {
          delete[] value;
        }
      };
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
    int hash(const char* key, bool pmem = false) {
      unsigned long h = 0;
      size_t length = pmem ? strlen_P(key) : strlen(key);
      for (size_t i = 0; i < length; i++) {
        char c = pmem ? pgm_read_byte(key + i) : key[i];
        h = 31 * h + c;
      }
      return h % _tableSize;
    };

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
    bool keyMatches(const char* key, const Entry* entry, bool keyPmem) {
      bool keysMatch = false;
      if (keyPmem) {
        if (entry->type == PMEM_KEY || entry->type == PMEM_KEY_VALUE) {
          // key and entry->key are both in PROGMEM
          // For efficiency, only check pointer equality
          keysMatch = (entry->key == key);
        } else {
          // key in PROGMEM, entry->key in regular memory
          size_t size = strlen_P(key) + 1;
          char buffer[size];
          strcpy_P(buffer, key);
          buffer[size-1] = '\0';
          keysMatch = (strcmp(entry->key, buffer) == 0);
        }
      } else {
        if (entry->type == PMEM_KEY || entry->type == PMEM_KEY_VALUE) {
          // key in regular memory, entry->key in PROGMEM
          size_t size = strlen_P(entry->key) + 1;
          char buffer[size];
          strcpy_P(buffer, entry->key);
          buffer[size-1] = '\0';
          keysMatch = (strcmp(buffer, key) == 0);
        } else {
          // key and entry->key are both in regular memory
          keysMatch = (strcmp(entry->key, key) == 0);
        }
      }
      return keysMatch;
    };

    /*
     * Resize the table, rehashing all keys to redistribute entries.
     */
    bool resize(int newSize) {
      Entry** newTable = new Entry*[newSize]();
      if (!newTable) {
        return false;
      }
      for (int i = 0; i < _tableSize; ++i) {
        Entry* entry = _table[i];
        while (entry) {
          Entry* next = entry->next;
          bool pmemKey = (entry->type == PMEM_KEY || entry->type == PMEM_KEY_VALUE);
          int index = hash(entry->key, pmemKey) % newSize;
          entry->next = newTable[index];
          newTable[index] = entry;
          entry = next;
        }
      }
      delete[] _table;
      _table = newTable;
      _tableSize = newSize;
      return true;
    };

    struct MetaInfo {
      int16_t typeId;
      uint8_t minCompatVersion;
      MetaInfo(int16_t typeId, uint8_t minCompatVersion): 
            typeId(typeId), minCompatVersion(minCompatVersion) {};
    };

    bool isCompatibleTypeAndVersion(MetaInfo* meta) {
      if (getTypeId() != meta->typeId) {
#if defined(DEBUG)
        Serial.println(String(F("ERROR: Type mismatch! Can't load DTO typeId=")) 
            + meta->typeId + " into typeId=" + getTypeId());
#endif
        return false;
      }
      if (getSerialVersion() < meta->minCompatVersion) {
#if defined(DEBUG)
        Serial.println(String(F("ERROR: Incompatible version for DTO typeId=")) 
              + getTypeId() + String(F(", have DTO v")) + getSerialVersion() 
              + String(F(" but stream requires >=v"))
              + meta->minCompatVersion);
#endif
        return false;
      }
      return true;
    };


  public:
    StreamableDTO() : _tableSize(INITIAL_TABLE_SIZE), _count(0) {
      _table = new Entry*[_tableSize]();
    };
    StreamableDTO(size_t initialCapacity, float loadFactor = 0.7) 
        : _tableSize(initialCapacity), _count(0), _loadFactorThreshold(loadFactor) {
      _table = new Entry*[_tableSize]();
    };
    virtual ~StreamableDTO() {
        clear();
        delete[] _table;
    };

    /*
     * Puts a key-value pair in the table. If the key already exists,
     * update the value. Resize the table if necessary.
     */
    bool put(const char* key, const char* value, bool keyPmem = false, bool valPmem = false) {
      int index = hash(key, keyPmem);
      Entry* entry = _table[index];
      while (entry != nullptr) {
        if (keyMatches(key, entry, keyPmem)) {
          if (entry->type == PMEM_KEY || entry->type == NON_PMEM) {
            delete[] entry->value;
          }
          if (valPmem) {
            entry->value = value;
          } else {
            entry->value = new char[strlen(value) + 1];
            strcpy(entry->value, value);
          }
          bool existingKeyPmem = (entry->type == PMEM_KEY || entry->type == PMEM_KEY_VALUE);
          entry->type = getMemoryType(existingKeyPmem, valPmem);
          return true;
        }
        entry = entry->next;
      }
      EntryMemoryType type = getMemoryType(keyPmem, valPmem);
      Entry* newEntry = new Entry(type, key, value);
      newEntry->next = _table[index];
      _table[index] = newEntry;
      _count++;
      if (static_cast<float>(_count) / _tableSize > _loadFactorThreshold) {
        if (!resize(_tableSize * 2)) {
          return false;
        }
      }
      return true;
    };

    /*
     * Checks if a key exists in the table.
     */
    bool exists(const char* key, bool keyPmem = false) const {
      int index = hash(key, keyPmem);
      Entry* entry = _table[index];
      while (entry != nullptr) {
        if (keyMatches(key, entry, keyPmem)) {
          return true;
        }
        entry = entry->next;
      }
      return false;
    };

    /*
     * Gets the raw value pointer associated with the given key. The
     * caller is expected to know whether the return value points to 
     * PROGMEM or regular memory.
     *
     * Returns a nullptr if the key is not found.
     */
    char* get(const char* key, bool keyPmem = false) const {
      int index = hash(key, keyPmem);
      Entry* entry = _table[index];
      while (entry != nullptr) {
        if (keyMatches(key, entry, keyPmem)) {
          return entry->value;
        }
        entry = entry->next;
      }
      return nullptr;
    };
    
    /*
     * Removes the entry with the given key if it exists. Returns
     * true if an entry was removed, otherwise false.
     */
    bool remove(const char* key, bool keyPmem = false) {
      int index = hash(key, keyPmem);
      Entry* current = _table[index];
      Entry* prev = nullptr;
      while (current != nullptr) {
        if (keyMatches(key, current, keyPmem)) {
          if (prev != nullptr) {
            prev->next = current->next;
          } else {
            _table[index] = current->next;
          }
          delete current;
          _count--;
          return true;
        }
        prev = current;
        current = current->next;
      }
      return false;
    };

    /*
     * Removes all the entries from the table and resets it to its
     * initial size.
     */
    bool clear() {
      for (int i = 0; i < _tableSize; ++i) {
        Entry* entry = _table[i];
        while (entry != nullptr) {
          Entry* toDelete = entry;
          entry = entry->next;
          delete toDelete;
        }
        _table[i] = nullptr;
      }
      _count = 0;
      if (_tableSize > INITIAL_TABLE_SIZE) {
        return resize(INITIAL_TABLE_SIZE);
      }
      return true;
    };

  protected:
    friend class StreamableManager;

    virtual int16_t getTypeId()          {  return -1; };
    virtual uint8_t getSerialVersion()    {  return 0;  };
    virtual uint8_t getMinCompatVersion() {  return 0;  };

    /*
     * Does something with the line of text produced by converting an Entry
     * into a String. StreamableManager prints these lines to a destination
     * stream, for example.
     *
     * Return true if successful
     */
    typedef bool (*LineHandlerFunction)(const String& line, void* capture);

    /*
     * Iterate through all the Entry's in the table and convert each to a
     * key=value formatted String, which is then passed to the lineHandler.
     * Returns true if all the Entry's were successfully converted and 
     * handled.
     */
    bool entriesToLines(LineHandlerFunction lineHandler, void* capture = nullptr) {
      for (int i = 0; i < _tableSize; ++i) {
        Entry* entry = _table[i];
        while (entry != nullptr) {
          bool keyPmem = (entry->type == PMEM_KEY || entry->type == PMEM_KEY_VALUE);
          bool valPmem = (entry->type == PMEM_VALUE || entry->type == PMEM_KEY_VALUE);
          String line = toLine(entry->key, entry->value, keyPmem, valPmem);
          if (!lineHandler(line, capture)) {
            return false;
          }
          entry = entry->next;
        }
      }
      return true;
    };

    /*
     * Default implementation parses a key=value format line. 
     * If there is no =, value is an empty string. Returns false if
     * parsing fails.
     */
    virtual bool parseLine(uint16_t lineNumber, const String &line) {
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
      return true;
    };

    /* 
     * Parses the special meta line containing typeId and minCompatVersion, verifying
     * compatibility. Returns false of the provided line is not a meta line
     */
    static MetaInfo* parseMetaLine(const String &metaLine) {
      String typeIdKey(String(F("__tvid")));
      String minVerIdKey(String(F("|")));
      int typeIdIdx = metaLine.indexOf(typeIdKey);
      int minVerIdIdx = metaLine.indexOf(minVerIdKey);
      if (typeIdIdx != -1) {
        int16_t typeId = metaLine.substring(typeIdIdx + typeIdKey.length() + 1).toInt();
        uint8_t minCompatVersion = metaLine.substring(minVerIdIdx + minVerIdKey.length()).toInt();
        return new MetaInfo(typeId, minCompatVersion);
      }
      // not a meta line
      return nullptr;
    };

    /*
     * Default implementation simply puts the value in _table under "key"
     */
    virtual void parseValue(uint16_t lineNumber, const String& key, const String& value) {
      put(key.c_str(), value.c_str());
    };

    /*
     * Default implementation returns "key=value"
     */
    virtual String toLine(const char* key, const char* value, bool keyPmem, bool valPmem) {
      String k = keyPmem ? String(reinterpret_cast<const __FlashStringHelper *>(key)) : String(key);
      String v = valPmem ? String(reinterpret_cast<const __FlashStringHelper *>(value)) : String(value);
      return k + "=" + v;
    };


};
 


#endif