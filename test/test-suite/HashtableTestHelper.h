#ifndef _test_HashtableTestHelper_h
#define _test_HashtableTestHelper_h


#include <StreamableDTO.h>

class HashtableTestHelper {

  public:
    HashtableTestHelper() {};
    int getTableSize(StreamableDTO* table) {
      return table->_tableSize;
    };
    float getLoadFactor(StreamableDTO* table) {
      return table->_loadFactorThreshold;
    };
    int hash(StreamableDTO* table, const char* key, bool pmem) {
      return table->hash(key, pmem);
    };
    bool keyMatches(StreamableDTO* table, const char* key, bool keyPmem, const char* entryKey, const char* entryValue, 
            bool entryKeyPmem, bool entryValPmem) {
      StreamableDTO::Entry* entry = new StreamableDTO::Entry(entryKey, entryValue, entryKeyPmem, entryValPmem);
      bool out = table->keyMatches(key, entry, keyPmem);
      delete entry;
      return out;
    };
    int getEntryCount(StreamableDTO* table) {
      return table->_count;
    };
    bool verifyEntryCount(StreamableDTO* table, int count) {
      int entryCount = 0;
      for (int i = 0; i < table->_tableSize; i++) {
        StreamableDTO::Entry* entry = table->_table[i];
        while (entry != nullptr) {
          entryCount++;
          entry = entry->next;
        }
      }
      return (entryCount == count);
    };

  private:
    HashtableTestHelper(HashtableTestHelper &t) = delete;

};



#endif