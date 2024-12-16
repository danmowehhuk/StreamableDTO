#ifndef _test_HashtableTestHelper_h
#define _test_HashtableTestHelper_h


#include "strdto_internal/Hashtable.h"

class HashtableTestHelper {

  public:
    HashtableTestHelper() {};
    int getTableSize(Hashtable* table) {
      return table->_tableSize;
    };
    float getLoadFactor(Hashtable* table) {
      return table->_loadFactorThreshold;
    };
    int hash(Hashtable* table, const char* key, bool pmem) {
      return table->hash(key, pmem);
    };
    bool keyMatches(Hashtable* table, const char* key, bool keyPmem, const char* entryKey, const char* entryValue, 
            bool entryKeyPmem, bool entryValPmem) {
      Hashtable::EntryMemoryType type = table->getMemoryType(entryKeyPmem, entryValPmem);
      Hashtable::Entry* entry = new Hashtable::Entry(type, entryKey, entryValue);
      bool out = table->keyMatches(key, entry, keyPmem);
      delete entry;
      return out;
    };

  private:
    HashtableTestHelper(HashtableTestHelper &t) = delete;

};



#endif