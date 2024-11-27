#ifndef _StreamableDTO_h
#define _StreamableDTO_h


#include <Arduino.h>
#include <Hashtable.h>

/*
 * A collection of arbitrary key-value pairs that is accessible with
 * hashtable semantics, but can also be serialized/deserialized from 
 * an Arduino Stream via the StreamableManager class
 */
//template <int16_t TypeId = -1, uint8_t SerialVersion = 0, uint8_t MinCompatVersion = 0>
class StreamableDTO {

  public:
    StreamableDTO() {};
    virtual ~StreamableDTO() {
      _table.clear();
    };

    /*
     * Type and version information for strongly-typed subclasses
     */
    virtual int16_t getTypeId()           { return -1; }; // TypeId;           };
    virtual uint8_t getSerialVersion()    { return 0; }; // SerialVersion;    };
    virtual uint8_t getMinCompatVersion() { return 0; }; //MinCompatVersion; };

    /*
     * Raw key/value access
     */
    void setProperty(const String& key, const String& value);
    String getProperty(const String& key, const String& defaultValue);
    void removeProperty(const String& key);
    bool containsKey(const String& key);
    

    static String pgmToString(const char* pgmString);

    class KVIterator {
      private:
        Hashtable<String, String>::Iterator it;
      public:
        KVIterator(Hashtable<String, String>::Iterator begin): it(begin) {}

        // Prefix increment
        KVIterator& operator++() {
            ++it; // Move to the next element
            return *this;
        }

        // Postfix increment
        KVIterator operator++(int) {
            KVIterator tmp(*this);
            ++(*this); // Use the prefix increment
            return tmp;
        }

        // Inequality check
        bool operator!=(const KVIterator& other) const {
            return it != other.it; // Directly compare the Hashtable iterators
        }

        // Dereference operator
        Hashtable<String, String>::KeyValuePair operator*() const {
            return *it; // Delegate to the Hashtable iterator
        }

        // Key accessor
        String key() const { 
            return it.operator*().key; // Access the key of the current KeyValuePair
        }

        // Value accessor
        String value() const { 
            return it.operator*().value; // Access the value of the current KeyValuePair
        }
    };

    KVIterator begin() { return KVIterator(_table.begin()); };
    KVIterator end() { return KVIterator(_table.end()); };

    SimpleVector<String> getKeys();


  protected:
    friend class StreamableManager;

    // To completely replace StreamableManager's loader, override
    // the following two methods
    virtual bool hasCustomLoader() { return false; };
    virtual void customLoad(Stream* src) {};

    // To completely replace StreamableManager's sender, override
    // the following two methods
    virtual bool hasCustomSender() { return false; };
    virtual void customSend(Stream* dest) {};

    // Default implementation parses a key=value format line. 
    // If there is no =, value is an empty string
    virtual void parseLine(uint16_t lineNumber, const String &line);

    // Parses the special meta line containing typeId and minCompatVersion, verifying
    // compatibility. Returns false of the provided line is not a meta line
    bool parseMetaLine(const String &metaLine);

    // Default implementation simply puts the value in _table under "key"
    virtual void parseValue(uint16_t lineNumber, const String& key, const String& value);

    // Default implementation returns "key=value\n"
    virtual String toLine(const String& key, const String& value);

    static bool stringToBool(const String& s);

  private:
    StreamableDTO(const StreamableDTO &t) = delete;
    Hashtable<String, String> _table;
    bool checkTypeAndVersion(uint16_t streamTypeId, uint8_t streamMinCompatVersion);

};

namespace StreamableDTOStrings {

  static const char META_KEY[]     PROGMEM = "__tvid";
  static const char META_SEP[]     PROGMEM = "|";

};


#endif