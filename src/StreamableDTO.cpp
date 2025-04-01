#include "StreamableDTO.h"

StreamableDTO::StreamableDTO() : _tableSize(INITIAL_TABLE_SIZE), _count(0) {
  _table = new Entry*[_tableSize]();
}

StreamableDTO::StreamableDTO(size_t initialCapacity, float loadFactor = 0.7) 
    : _tableSize(initialCapacity), _count(0), _loadFactorThreshold(loadFactor) {
  _table = new Entry*[_tableSize]();
}

 StreamableDTO::~StreamableDTO() {
  clear();
  delete[] _table;
}

StreamableDTO::Entry::Entry(const char* k, const char* v, bool keyPmem, bool valPmem):
    key(nullptr), value(nullptr), next(nullptr), keyPmem(keyPmem), valPmem(valPmem) {
  key = keyPmem ? k : strdup(k);
  value = valPmem ? v : strdup(v);
}

StreamableDTO::Entry::~Entry() {
  if (key && !keyPmem) delete[] key;
  if (value && !valPmem) delete[] value;
};

int StreamableDTO::hash(const char* key, bool pmem = false) {
  unsigned long h = 0;
  size_t length = pmem ? strlen_P(key) : strlen(key);
  for (size_t i = 0; i < length; i++) {
    char c = pmem ? pgm_read_byte(key + i) : key[i];
    h = 31 * h + c;
  }
  return h % _tableSize;
}

int StreamableDTO::hash(const __FlashStringHelper* key) {
  return hash(reinterpret_cast<const char*>(key), true);
}

bool StreamableDTO::keyMatches(const char* key, const Entry* entry, bool keyPmem) {
  bool keysMatch = false;
  if (keyPmem) {
    if (entry->keyPmem) {
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
    if (entry->keyPmem) {
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
}

bool StreamableDTO::keyMatches(const __FlashStringHelper* key, const Entry* entry) {
  return keyMatches(reinterpret_cast<const char*>(key), entry, true);
}

bool StreamableDTO::resize(int newSize) {
  Entry** newTable = new Entry*[newSize]();
  if (!newTable) {
    return false;
  }
  for (int i = 0; i < _tableSize; ++i) {
    Entry* entry = _table[i];
    while (entry) {
      Entry* next = entry->next;
      int index = hash(entry->key, entry->keyPmem) % newSize;
      entry->next = newTable[index];
      newTable[index] = entry;
      entry = next;
    }
  }
  delete[] _table;
  _table = newTable;
  _tableSize = newSize;
  return true;
}

bool StreamableDTO::isCompatibleTypeAndVersion(MetaInfo* meta) {
  if (getTypeId() != meta->typeId) {
#if defined(DEBUG)
    Serial.print(F("ERROR: Type mismatch! Can't load DTO typeId="));
    Serial.print(meta->typeId);
    Serial.print(F(" into typeId="));
    Serial.println(getTypeId());
#endif
    return false;
  }
  if (getSerialVersion() < meta->minCompatVersion) {
#if defined(DEBUG)
    Serial.print(F("ERROR: Incompatible version for DTO typeId="));
    Serial.print(getTypeId());
    Serial.print(F(", have DTO v"));
    Serial.print(getSerialVersion());
    Serial.print(F(" but stream requires >=v"));
    Serial.println(meta->minCompatVersion);
#endif
    return false;
  }
  return true;
}

bool StreamableDTO::put(const char* key, const char* value, bool keyPmem = false, bool valPmem = false) {
  int index = hash(key, keyPmem);
  Entry* entry = _table[index];
  while (entry != nullptr) {
    if (keyMatches(key, entry, keyPmem)) {
      if (!entry->valPmem) delete[] entry->value;
      if (valPmem) {
        entry->value = value;
      } else {
        entry->value = strdup(value);
      }
      entry->valPmem = valPmem;
      return true;
    }
    entry = entry->next;
  }
  Entry* newEntry = new Entry(key, value, keyPmem, valPmem);
  newEntry->next = _table[index];
  _table[index] = newEntry;
  _count++;
  if (static_cast<float>(_count) / _tableSize > _loadFactorThreshold) {
    if (!resize(_tableSize * 2)) {
      return false;
    }
  }
  return true;
}

bool StreamableDTO::put(const char* key, const __FlashStringHelper* value, bool keyPmem = false) {
  return put(key, reinterpret_cast<const char*>(value), keyPmem, true);
}

bool StreamableDTO::put(const __FlashStringHelper* key, const char* value, bool valPmem = false) {
  return put(reinterpret_cast<const char*>(key), value, true, valPmem);
}

bool StreamableDTO::put(const __FlashStringHelper* key, const __FlashStringHelper* value) {
  return put(reinterpret_cast<const char*>(key), reinterpret_cast<const char*>(value), true, true);
}

bool StreamableDTO::put_P(const char* key, const char* value, bool valPmem = false) {
  return put(key, value, true, valPmem);
}

bool StreamableDTO::put_P(const char* key, const __FlashStringHelper* value) {
  return put(key, reinterpret_cast<const char*>(value), true, true);
}

bool StreamableDTO::putEmpty(const char* key, bool pmemKey) {
  return put(key, F(""), pmemKey);
}

bool StreamableDTO::putEmpty(const __FlashStringHelper* key) {
  return put(key, F(""));
}

bool StreamableDTO::putEmpty_P(const char* key) {
  return put(key, F(""), true);
}

bool StreamableDTO::exists(const char* key, bool keyPmem = false) const {
  int index = hash(key, keyPmem);
  Entry* entry = _table[index];
  while (entry != nullptr) {
    if (keyMatches(key, entry, keyPmem)) {
      return true;
    }
    entry = entry->next;
  }
  return false;
}

bool StreamableDTO::exists(const __FlashStringHelper* key) const {
  return exists(reinterpret_cast<const char*>(key), true);
}

bool StreamableDTO::exists_P(const char* key) const {
  return exists(key, true);
}

char* StreamableDTO::get(const char* key, bool keyPmem = false) const {
  int index = hash(key, keyPmem);
  Entry* entry = _table[index];
  while (entry != nullptr) {
    if (keyMatches(key, entry, keyPmem)) {
      return entry->value;
    }
    entry = entry->next;
  }
  return nullptr;
}

char* StreamableDTO::get(const __FlashStringHelper* key) const {
  return get(reinterpret_cast<const char*>(key), true);
}

char* StreamableDTO::get_P(const char* key) const {
  return get(key, true);
}

bool StreamableDTO::remove(const char* key, bool keyPmem = false) {
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
}

bool StreamableDTO::remove(const __FlashStringHelper* key) {
  return remove(reinterpret_cast<const char*>(key), true);
}

bool StreamableDTO::remove_P(const char* key) {
  return remove(key, true);
}

bool StreamableDTO::clear() {
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
}

bool StreamableDTO::processEntries(EntryProcessor entryProcessor, void* capture = nullptr) {
  for (int i = 0; i < _tableSize; ++i) {
    Entry* entry = _table[i];
    while (entry != nullptr) {
      if (!entryProcessor(entry->key, entry->value, entry->keyPmem, entry->valPmem, capture)) {
        return false;
      }
      entry = entry->next;
    }
  }
  return true;
}

StreamableDTO::MetaInfo* StreamableDTO::parseMetaLine(const char* metaLine) {
  static const char typeIdKey[] PROGMEM = "__tvid=";
  const char* typeIdStart = strstr_P(metaLine, typeIdKey);
  if (!typeIdStart) return nullptr;
  typeIdStart += strlen_P(typeIdKey);
  const char* sep = strchr(typeIdStart, '|');
  if (!sep) return nullptr;
  char typeIdStr[8] = {0}; // fits int16_t
  strncpy(typeIdStr, typeIdStart, sep - typeIdStart);
  int16_t typeId = atoi(typeIdStr);
  uint8_t minCompatVersion = atoi(sep + 1);
  return new MetaInfo(typeId, minCompatVersion);
}

bool StreamableDTO::parseLine(uint16_t lineNumber, const char* line) {
  const char* sep = strchr(line, '=');
  if (!sep) {
    parseValue(lineNumber, line, "");
    return true;
  }
  size_t keyLen = sep - line;
  size_t valLen = strlen(sep + 1);
  char key[keyLen + 1];
  char val[valLen + 1];
  strncpy(key, line, keyLen); key[keyLen] = '\0';
  strncpy(val, sep + 1, valLen); val[valLen] = '\0';

  // Trim leading
  char* k = key; while (isspace(*k)) k++;
  char* v = val; while (isspace(*v)) v++;

  // Trim trailing
  for (char* end = k + strlen(k) - 1; end >= k && isspace(*end); --end) *end = '\0';
  for (char* end = v + strlen(v) - 1; end >= v && isspace(*end); --end) *end = '\0';

  parseValue(lineNumber, k, v);
  return true;
}

void StreamableDTO::parseValue(uint16_t lineNumber, const char* key, const char* value) {
  put(key, value);
}

char* StreamableDTO::toLine(const char* key, const char* value, bool keyPmem, bool valPmem) {
  size_t keyLen = keyPmem ? strlen_P(key) : strlen(key);
  size_t valLen = valPmem ? strlen_P(value) : strlen(value);
  char* out = new char[keyLen + valLen + 2](); // key + '=' + val + '\0'
  char* p = out;
  if (keyPmem) strcpy_P(p, key); else strcpy(p, key);
  p += keyLen;
  *p++ = '=';
  if (valPmem) strcpy_P(p, value); else strcpy(p, value);
  return out;
}
