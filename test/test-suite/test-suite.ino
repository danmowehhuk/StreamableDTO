#include <StreamableDTO.h>
#include <StreamableManager.h>
#include <StringStream.h>
#include "HashtableTestHelper.h"
#include "MyTypedDTO.h"

#define STR(s) String(F(s))

StreamableManager streamMgr;
HashtableTestHelper helper;

static const char PMEM_KEY[]   PROGMEM = "myKey";
static const char PMEM_KEY2[]  PROGMEM = "myKey";
static const char REGMEM_KEY[]         = "myKey";
static const char PMEM_VAL[]   PROGMEM = "myVal-pmem";
static const char REGMEM_VAL[]         = "myVal-regm";

struct TestInvocation {
  String name = String();
  bool success = false;
  String message = String();
};

StreamableDTO* typeMapper(uint16_t typeId) {
  StreamableDTO* dto = nullptr;
  switch (typeId) {
    case 1:
      dto = new MyTypedDTO();
      break;
    default:
      // unknown type
      break;
  }
  return dto;
};

void testDefaultConstructor(TestInvocation* t) {
  t->name = STR("Default constructor");
  StreamableDTO table;
  do {
    if (helper.getTableSize(&table) != 8) {
      t->message = STR("Incorrect starting size");
      break;
    }
    if (helper.getEntryCount(&table) != 0) {
      t->message = STR("Incorrect starting entry count");
      break;
    }
    t->success = true;
  } while (false);
}

void testInitialSizeConstructor(TestInvocation* t) {
  t->name = STR("Initial size constructor");
  StreamableDTO table(32, 0.9);
  do {
    if (helper.getTableSize(&table) != 32) {
      t->message = STR("Incorrect starting size");
      break;
    }
    if (helper.getLoadFactor(&table) != 0.9) {
      t->message = STR("Incorrect load factor");
      break;
    }
    t->success = true;
  } while (false);
}

void testHashFunction(TestInvocation* t) {
  t->name = STR("Hash function");
  StreamableDTO table;
  int oneHash = helper.hash(&table, REGMEM_KEY, false);
  do {
    if (oneHash != helper.hash(&table, PMEM_KEY, true)) {
      t->message = STR("Hash from PROGMEM different from regular memory");
      break;
    }
    t->success = true;
  } while (false);
}

void testKeyMatch(TestInvocation* t) {
  t->name = STR("Key matcher");
  StreamableDTO table;
  do {
    if (!helper.keyMatches(&table, REGMEM_KEY, false, PMEM_KEY, PMEM_VAL, true, true)) {
      t->message = STR("Reg mem key didn't match PROGMEM entry->key");
      break;
    }
    if (!helper.keyMatches(&table, PMEM_KEY, true, PMEM_KEY, PMEM_VAL, true, true)) {
      t->message = STR("PROGMEM key didn't match PROGMEM entry->key");
      break;
    }
    if (!helper.keyMatches(&table, REGMEM_KEY, false, REGMEM_KEY, PMEM_VAL, false, true)) {
      t->message = STR("Reg mem key didn't match reg mem entry->key");
      break;
    }
    if (!helper.keyMatches(&table, PMEM_KEY2, true, REGMEM_KEY, PMEM_VAL, false, true)) {
      t->message = STR("PROGMEM key didn't match reg mem entry->key");
      break;
    }
    t->success = true;
  } while (false);
}

void testPut(TestInvocation* t) {
  t->name = STR("Put new and existing key");
  StreamableDTO table;
  do {
    table.put(PMEM_KEY, PMEM_VAL, true, true);
    table.put(PMEM_KEY, REGMEM_VAL, true, false); // update
    int entryCount = helper.getEntryCount(&table);
    if (entryCount != 1) {
      t->message = STR("Incorrect entry count");
      break;
    }
    if (!helper.verifyEntryCount(&table, entryCount)) {
      t->message = STR("Hashtable count does not match actual entry count");
      break;
    }
    t->success = true;
  } while (false);
}

void testExists(TestInvocation* t) {
  t->name = STR("Check key existence");
  StreamableDTO table;
  do {
    table.put(PMEM_KEY, PMEM_VAL, true, true);
    if (!table.exists(PMEM_KEY, true)) {
      t->message = STR("Basic existence check failed");
      break;
    }
    if (!table.exists(REGMEM_KEY)) {
      t->message = STR("Existence check failed with regular memory key");
      break;
    }
    char key[] = "abc";
    if (table.exists(key)) {
      t->message = STR("Existence negative check failed");
      break;
    }
    t->success = true;
  } while (false);
}

void testGet(TestInvocation* t) {
  t->name = STR("Get raw value pointer for key");
  StreamableDTO table;
  do {
    table.put(PMEM_KEY, PMEM_VAL, true, true);
    table.put(PMEM_KEY, REGMEM_VAL, true, false); // update
    table.put("foo", "bar");
    if (!helper.verifyEntryCount(&table, 2)) {
      t->message = STR("Hashtable entry count should be 2");
      break;
    }
    char* val1 = table.get(PMEM_KEY, true);
    if (val1 == nullptr) {
      t->message = STR("Get by PROGMEM key failed");
      break;
    }
    char* val2 = table.get(REGMEM_KEY);
    if (val2 == nullptr) {
      t->message = STR("Get by regular memory key failed");
      break;
    }
    if (val1 != val2) {
      t->message = STR("Both gets should have returned same value");
      break;
    }
    if (strcmp(val1, REGMEM_VAL) != 0) {
      t->message = STR("Get returned incorrect value");
      break;
    }
    if (strcmp(table.get("foo"), "bar") != 0) {
      t->message = STR("Get 'foo' returned incorrect value");
      break;
    }
    if (table.get("abc") != nullptr) {
      t->message = STR("Get unknown key should return nullptr");
      break;
    }
    t->success = true;
  } while (false);
}

void testRemove(TestInvocation* t) {
  t->name = STR("Remove key from hashtable");
  StreamableDTO table;
  do {
    table.put("abc","def");
    table.put("ghi","jkl");
    if (!helper.verifyEntryCount(&table, 2)) {
      t->message = STR("Hashtable entry count should be 2");
      break;
    }
    table.remove("ghi");
    if (!helper.verifyEntryCount(&table, 1)) {
      t->message = STR("Hashtable entry count should be 1");
      break;
    }
    if (table.remove("foo")) {
      t->message = STR("Returned true for nonexistent key");
      break;
    }
    t->success = true;
  } while (false);
}

void testClear(TestInvocation* t) {
  t->name = STR("Clear all entries");
  StreamableDTO table;
  do {
    table.put("abc","def");
    table.put("ghi","jkl");
    int entryCount = helper.getEntryCount(&table);
    if (entryCount != 2 || !helper.verifyEntryCount(&table, entryCount)) {
      t->message = STR("Hashtable entry count should be 2");
      break;
    }
    table.clear();
    entryCount = helper.getEntryCount(&table);
    if (entryCount != 0 || !helper.verifyEntryCount(&table, entryCount)) {
      t->message = STR("Hashtable entry count should be 0");
      break;
    }
    t->success = true;
  } while (false);
}

void testResize(TestInvocation* t) {
  t->name = STR("Resize table up and down");
  StreamableDTO table(4); // use a smaller table
  do {
    if (helper.getTableSize(&table) != 4) {
      t->message = STR("Table size should be 4");
      break;
    }
    table.put("abc","def");
    table.put("ghi","jkl");
    if (helper.getTableSize(&table) != 4) {
      t->message = STR("Table size should still be 4");
      break;
    }
    table.put("mno","pqr"); // push it over 70% load
    if (helper.getTableSize(&table) != 8) {
      t->message = STR("Table size should have doubled");
      break;
    }
    int entryCount = helper.getEntryCount(&table);
    if (entryCount != 3 || !helper.verifyEntryCount(&table, entryCount)) {
      t->message = STR("Resized hashtable should have 3 entries");
      break;
    }
    table.put("a","A");
    table.put("b","B");
    table.put("c","C"); // push it over 70% load again
    if (helper.getTableSize(&table) != 16) {
      t->message = STR("Table size should have doubled again");
      break;
    }
    table.clear(); // reset to INITIAL_TABLE_SIZE
    if (helper.getTableSize(&table) != 8) {
      t->message = STR("Table size should have reset");
      break;
    }
    entryCount = helper.getEntryCount(&table);
    if (entryCount != 0 || !helper.verifyEntryCount(&table, entryCount)) {
      t->message = STR("Hashtable entry count should be 0");
      break;
    }
    t->success = true;
  } while (false);
}

void testLoadUntypedStreamableDTO(TestInvocation* t) {
  t->name = STR("Load untyped StreamableDTO");
  String data = STR("foo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = new StreamableDTO();
  do {
    if (!streamMgr.load(&ss, dto)) {
      t->message = STR("DTO load failed");
      break;
    }
    if (!dto->exists("foo") || !dto->exists("abc")) {
      t->message = STR("Missing key(s)");
      break;
    }
    if (strcmp(dto->get("foo"),"bar") != 0
        || strcmp(dto->get("abc"), "def") != 0) {
      t->message = STR("Unexpected values for keys");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}

void testLoadLongLine(TestInvocation* t) {
  t->name = STR("Long line untyped StreamableDTO");
  String longValue = STR("this is a long line this is a long line this is a long line this is a long line this is a long line");
  String data = STR("foo=bar\nabc=") + longValue + STR("\n");
  StringStream ss(data);
  StreamableDTO* dto = new StreamableDTO();
  streamMgr.load(&ss, dto);
  do {
    if (!dto->exists("foo") || !dto->exists("abc")) {
      t->message = STR("Missing key(s)");
      break;
    }
    if (strcmp(dto->get("foo"),"bar") != 0) {
      t->message = STR("Unexpected values for key 'foo'");
      break;
    }
    if (String(dto->get("abc")).length() > 60) {
      t->message = STR("Values for key 'abc' should have been truncated");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}

void testSendUntypedStreamableDTO(TestInvocation* t) {
  t->name = STR("Send untyped StreamableDTO");
  String data = STR("foo=bar\nabc=def\n");
  StringStream src(data);
  StringStream dest;
  StreamableDTO* dto = new StreamableDTO();
  do {
    if (!streamMgr.load(&src, dto)) {
      t->message = STR("DTO load failed");
      break;
    }
  streamMgr.send(&dest, dto);
    if (dest.getString().length() != data.length()) {
      t->message = STR("Output length does not match input");
      break;
    }
    if (dest.getString().indexOf(STR("foo=bar")) == -1) {
      t->message = STR("foo=bar missing from output");
      break;
    }
    if (dest.getString().indexOf(STR("abc=def")) == -1) {
      t->message = STR("abc=def missing from output");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}

void testLoadTypedStreamableDTO(TestInvocation* t) {
  t->name = STR("Load typed StreamableDTO");
  String data = STR("__tvid=1|0\nfoo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = streamMgr.load(&ss, typeMapper);
  do {
    if (!dto) {
      t->message = STR("Failed to load MyTypedDTO");
      break;
    }
    if (!dto->exists("foo") || !dto->exists("abc")) {
      t->message = STR("Missing key(s)");
      break;
    }
    if (strcmp(dto->get("foo"),"bar") != 0
        || strcmp(dto->get("abc"), "def") != 0) {
      t->message = STR("Unexpected values for keys");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}

void testSendTypedStreamableDTO(TestInvocation* t) {
  t->name = STR("Send typed StreamableDTO");
  MyTypedDTO dtoSent;
  dtoSent.put("foo", "bar");
  dtoSent.put("abc", "def");
  StringStream dest;
  streamMgr.send(&dest, &dtoSent);
  String sentDto = dest.getString();
  StringStream src(sentDto);
  StreamableDTO* dtoRcvd = streamMgr.load(&src, typeMapper);
  do {
    if (!dtoRcvd) {
      t->message = STR("Failed to load MyTypedDTO");
      break;
    }
    if (!dtoRcvd->exists("foo") || !dtoRcvd->exists("abc")) {
      t->message = STR("Missing key(s)");
      break;
    }
    if (strcmp(dtoRcvd->get("foo"),"bar") != 0
        || strcmp(dtoRcvd->get("abc"), "def") != 0) {
      t->message = STR("Unexpected values for keys");
      break;
    }
    t->success = true;
  } while (false);
  delete dtoRcvd;
}

void testLoadIncorrectType(TestInvocation* t) {
  t->name = STR("Load incorrect type");
  String data = STR("__tvid=2|0\nfoo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = streamMgr.load(&ss, typeMapper);
  do {
    if (dto) {
      t->message = STR("Should have failed to load DTO");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}

void testLoadIncompatibleVersion(TestInvocation* t) {
  t->name = STR("Load incompatible version");
  String data = STR("__tvid=1|6\nfoo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = streamMgr.load(&ss, typeMapper);
  do {
    if (dto) {
      t->message = STR("Should have failed to load DTO");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}



bool printAndCheckResult(TestInvocation* t) {
  uint8_t nameWidth = 48;
  String name;
  name.reserve(nameWidth);
  name = STR("  ") + t->name + STR("...");
  if (name.length() > nameWidth) {
    name = name.substring(0, nameWidth);
  } else if (name.length() < nameWidth) {
    uint8_t padding = nameWidth - name.length();
    for (uint8_t i = 0; i < padding; i++) {
      name += " ";
    }
  }
  Serial.print(name + " ");
  if (t->success) {
    Serial.println(F("PASSED"));
  } else {
    Serial.println(F("FAILED"));
    if (t->message.length() > 0) {
      Serial.print(F("    "));
      Serial.println(t->message);
    }
    return false;
  }
  return true;
}

bool invokeTest(void (*testFunction)(TestInvocation* t)) {
  TestInvocation t;
  testFunction(&t);
  return printAndCheckResult(&t);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("Starting StreamableDTO tests..."));

  void (*tests[])(TestInvocation*) = {
    testDefaultConstructor,
    testInitialSizeConstructor,
    testHashFunction,
    testKeyMatch,
    testPut,
    testExists,
    testGet,
    testRemove,
    testClear,
    testResize,
    testLoadUntypedStreamableDTO,
    testLoadLongLine,
    testSendUntypedStreamableDTO,
    testLoadTypedStreamableDTO,
    testSendTypedStreamableDTO,
    testLoadIncorrectType,
    testLoadIncompatibleVersion
  };

  bool success = true;
  for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
    success &= invokeTest(tests[i]);
  }
  if (success) {
    Serial.println(F("All tests passed!"));
  } else {
    Serial.println(F("Test suite failed!"));
  }
}

void loop() {

}
