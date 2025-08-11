#include <StreamableDTO.h>
#include <StreamableManager.h>
#include <StringStream.h>
#include <TestTool.h>
#include "HashtableTestHelper.h"
#include "MyTypedDTO.h"

StreamableManager streamMgr;
HashtableTestHelper helper;

static const char PMEM_KEY[]   PROGMEM = "myKey";
static const char PMEM_KEY2[]  PROGMEM = "myKey";
static const char REGMEM_KEY[]         = "myKey";
static const char PMEM_VAL[]   PROGMEM = "myVal-pmem";
static const char REGMEM_VAL[]         = "myVal-regm";

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
  t->setName(F("Default constructor"));
  StreamableDTO table;
  t->assert(helper.getTableSize(&table) == 8, F("Incorrect starting size"));
  t->assert(helper.getEntryCount(&table) == 0, F("Incorrect starting entry count"));
}

void testInitialSizeConstructor(TestInvocation* t) {
  t->setName(F("Initial size constructor"));
  StreamableDTO table(32, 0.9);
  t->assert(helper.getTableSize(&table) == 32, F("Incorrect starting size"));
  t->assert(helper.getLoadFactor(&table) == 0.9, F("Incorrect load factor"));
}

void testHashFunction(TestInvocation* t) {
  t->setName(F("Hash function"));
  StreamableDTO table;
  int oneHash = helper.hash(&table, REGMEM_KEY, false);
  t->assert(oneHash == helper.hash(&table, PMEM_KEY, true), 
      F("Hash from PROGMEM different from regular memory"));
}

void testKeyMatch(TestInvocation* t) {
  t->setName(F("Key matcher"));
  StreamableDTO table;
  t->assert(helper.keyMatches(&table, REGMEM_KEY, false, PMEM_KEY, PMEM_VAL, true, true), 
      F("Reg mem key didn't match PROGMEM entry->key"));
  t->assert(helper.keyMatches(&table, PMEM_KEY, true, PMEM_KEY, PMEM_VAL, true, true),
      F("PROGMEM key didn't match PROGMEM entry->key"));
  t->assert(helper.keyMatches(&table, REGMEM_KEY, false, REGMEM_KEY, PMEM_VAL, false, true),
      F("Reg mem key didn't match reg mem entry->key"));
  t->assert(helper.keyMatches(&table, PMEM_KEY2, true, REGMEM_KEY, PMEM_VAL, false, true),
      F("PROGMEM key didn't match reg mem entry->key"));
}

void testKeyMatchPROGMEMvsPROGMEM(TestInvocation* t) {
  t->setName(F("PROGMEM vs PROGMEM key matching (infinite loop fix)"));
  
  // Test the specific scenario that had the infinite loop bug
  // Two different PROGMEM strings with the same content
  static const char PROGMEM_KEY_A[] PROGMEM = "testKey";
  static const char PROGMEM_KEY_B[] PROGMEM = "testKey";
  
  StreamableDTO table;
  
  // These should be different pointers but same content
  t->assert(PROGMEM_KEY_A != PROGMEM_KEY_B, F("PROGMEM keys should be different pointers"));
  
  // Test that keyMatches works correctly for this scenario
  bool matches = helper.keyMatches(&table, PROGMEM_KEY_A, true, PROGMEM_KEY_B, PMEM_VAL, true, true);
  t->assert(matches, F("PROGMEM vs PROGMEM key matching failed"));
  
  // Test the reverse direction
  matches = helper.keyMatches(&table, PROGMEM_KEY_B, true, PROGMEM_KEY_A, PMEM_VAL, true, true);
  t->assert(matches, F("PROGMEM vs PROGMEM key matching failed (reverse)"));
  
  // Test with different content
  static const char PROGMEM_KEY_C[] PROGMEM = "different";
  matches = helper.keyMatches(&table, PROGMEM_KEY_A, true, PROGMEM_KEY_C, PMEM_VAL, true, true);
  t->assert(!matches, F("PROGMEM vs PROGMEM should not match different content"));
  
  // Test with same pointer (should be fast path)
  matches = helper.keyMatches(&table, PROGMEM_KEY_A, true, PROGMEM_KEY_A, PMEM_VAL, true, true);
  t->assert(matches, F("PROGMEM vs PROGMEM same pointer should match"));
}

void testPut(TestInvocation* t) {
  t->setName(F("Put new and existing key"));
  StreamableDTO table;
  table.put(PMEM_KEY, PMEM_VAL, true, true);
  table.put(PMEM_KEY, REGMEM_VAL, true, false); // update
  int entryCount = helper.getEntryCount(&table);
  t->assert(entryCount == 1, F("Incorrect entry count"));
  t->assert(helper.verifyEntryCount(&table, entryCount), 
      F("Hashtable count does not match actual entry count"));
}

void testExists(TestInvocation* t) {
  t->setName(F("Check key existence"));
  StreamableDTO table;
  table.put(PMEM_KEY, PMEM_VAL, true, true);
  t->assert(table.exists(PMEM_KEY, true), F("Basic existence check failed"));
  t->assert(table.exists(REGMEM_KEY), F("Existence check failed with regular memory key"));
  char key[] = "abc";
  t->assert(!table.exists(key), F("Existence negative check failed"));
}

void testGet(TestInvocation* t) {
  t->setName(F("Get raw value pointer for key"));
  StreamableDTO table;
  table.put(PMEM_KEY, PMEM_VAL, true, true);
  table.put(PMEM_KEY, REGMEM_VAL, true, false); // update
  table.put("foo", "bar");
  t->assert(helper.verifyEntryCount(&table, 2), F("Hashtable entry count should be 2"));
  char* val1 = table.get(PMEM_KEY, true);
  t->assert(val1, F("Get by PROGMEM key failed"));
  char* val2 = table.get(REGMEM_KEY);
  t->assert(val2, F("Get by regular memory key failed"));
  t->assertEqual(val1, val2, F("Both gets should have returned same value"));
  t->assertEqual(val1, REGMEM_VAL, F("Get returned incorrect value"));
  t->assertEqual(table.get("foo"), F("bar"), F("Get 'foo' returned incorrect value"));
  t->assert(!table.get("abc"), F("Get unknown key should return nullptr"));
}

void testRemove(TestInvocation* t) {
  t->setName(F("Remove key from hashtable"));
  StreamableDTO table;
  table.put("abc","def");
  table.put("ghi","jkl");
  t->assert(helper.verifyEntryCount(&table, 2), F("Hashtable entry count should be 2"));
  table.remove("ghi");
  t->assert(helper.verifyEntryCount(&table, 1), F("Hashtable entry count should be 1"));
  t->assert(!table.remove("foo"), F("Returned true for nonexistent key"));
}

void testClear(TestInvocation* t) {
  t->setName(F("Clear all entries"));
  StreamableDTO table;
  table.put("abc","def");
  table.put("ghi","jkl");
  int entryCount = helper.getEntryCount(&table);
  t->assert((entryCount == 2 && helper.verifyEntryCount(&table, entryCount)), 
      F("Hashtable entry count should be 2"));
  table.clear();
  entryCount = helper.getEntryCount(&table);
  t->assert((entryCount == 0 && helper.verifyEntryCount(&table, entryCount)),
      F("Hashtable entry count should be 0"));
}

void testResize(TestInvocation* t) {
  t->setName(F("Resize table up and down"));
  StreamableDTO table(4); // use a smaller table
  t->assert(helper.getTableSize(&table) == 4, F("Table size should be 4"));
  table.put(F("abc"),F("def"));
  table.put(F("ghi"),F("jkl"));
  t->assert(helper.getTableSize(&table) == 4, F("Table size should still be 4"));
  table.put(F("mno"),F("pqr")); // push it over 70% load
  t->assert(helper.getTableSize(&table) == 8, F("Table size should have doubled"));
  int entryCount = helper.getEntryCount(&table);
  t->assert((entryCount == 3 && helper.verifyEntryCount(&table, entryCount)),
      F("Resized hashtable should have 3 entries"));
  table.put(F("a"),F("A"));
  table.put(F("b"),F("B"));
  table.put(F("c"),F("C")); // push it over 70% load again
  t->assert(helper.getTableSize(&table) == 16, F("Table size should have doubled again"));
  table.clear(); // reset to INITIAL_TABLE_SIZE
  t->assert(helper.getTableSize(&table) == 8, F("Table size should have reset"));
  entryCount = helper.getEntryCount(&table);
  t->assert((entryCount == 0 && helper.verifyEntryCount(&table, entryCount)),
      F("Hashtable entry count should be 0"));
}

void testLoadUntypedStreamableDTO(TestInvocation* t) {
  t->setName(F("Load untyped StreamableDTO"));
  String data = F("foo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = new StreamableDTO();
  t->assert(streamMgr.load(&ss, dto), F("DTO load failed"));
  t->assert((dto->exists("foo") && dto->exists(F("abc"))), F("Missing key(s)"));
  t->assertEqual(dto->get("foo"), F("bar"));
  t->assertEqual(dto->get(F("abc")), F("def"));
  delete dto;
}

void testLoadLongLine(TestInvocation* t) {
  t->setName(F("Long line untyped StreamableDTO"));
  String longValue = F("this is a long line this is a long line this is a long line this is a long line this is a long line");
  String data = String(F("foo=bar\nabc=")) + longValue + String(F("\n"));
  StringStream ss(data);
  StreamableDTO* dto = new StreamableDTO();
  streamMgr.load(&ss, dto);
  t->assert((dto->exists(F("foo")) && dto->exists("abc")), F("Missing key(s)"));
  t->assertEqual(dto->get("foo"), "bar");
  t->assert(String(dto->get("abc")).length() == 59, // 64 minus 'abc=' minus terminator
      F("Values for key 'abc' should have been truncated"));
  delete dto;
}

void testSendUntypedStreamableDTO(TestInvocation* t) {
  t->setName(F("Send untyped StreamableDTO"));
  String data = F("foo=bar\nabc=def\n");
  StringStream src(data);
  StringStream dest;
  StreamableDTO* dto = new StreamableDTO();
  t->assert(streamMgr.load(&src, dto), F("DTO load failed"));
  streamMgr.send(&dest, dto);
  t->assert(dest.getString().length() == data.length(), F("Output length does not match input"));
  t->assert(dest.getString().indexOf(F("foo=bar")) != -1, F("foo=bar missing from output"));
  t->assert(dest.getString().indexOf(F("abc=def")) != -1, F("abc=def missing from output"));
  delete dto;
}

void testLoadTypedStreamableDTO(TestInvocation* t) {
  t->setName(F("Load typed StreamableDTO"));
  String data = F("__tvid=1|2\nfoo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = streamMgr.load(&ss, typeMapper);
  t->assert(dto, F("Failed to load MyTypedDTO"));
  t->assert((dto->exists("foo") && dto->exists("abc")), F("Missing key(s)"));
  t->assertEqual(dto->get("foo"), "bar");
  t->assertEqual(dto->get("abc"), "def");
  delete dto;
}

void testSendTypedStreamableDTO(TestInvocation* t) {
  t->setName(F("Send typed StreamableDTO"));
  MyTypedDTO dtoSent;
  dtoSent.put("foo", "bar");
  dtoSent.put("abc", "def");
  StringStream dest;
  streamMgr.send(&dest, &dtoSent);
  String sentDto = dest.getString();
  StringStream src(sentDto);
  StreamableDTO* dtoRcvd = streamMgr.load(&src, typeMapper);
  t->assert(dtoRcvd, F("Failed to load MyTypedDTO"));
  t->assert((dtoRcvd->exists("foo") && dtoRcvd->exists("abc")), F("Missing key(s)"));
  t->assertEqual(dtoRcvd->get("foo"), "bar");
  t->assertEqual(dtoRcvd->get("abc"), "def");
  delete dtoRcvd;
}

void testLoadIncorrectType(TestInvocation* t) {
  t->setName(F("Load incorrect type"));
  String data = F("__tvid=2|0\nfoo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = streamMgr.load(&ss, typeMapper);
  t->assert(!dto, F("Should have failed to load DTO"));
  delete dto;
}

void testLoadIncompatibleVersion(TestInvocation* t) {
  t->setName(F("Load incompatible version"));
  String data = F("__tvid=1|1\nfoo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = streamMgr.load(&ss, typeMapper);
  t->assert(!dto, F("Should have failed to load DTO"));
  delete dto;
}

void testMemoryBehavior(TestInvocation* t) {
  t->setName(F("StreamableDTO Memory Behavior"));
  StreamableDTO dto;
  t->assert(dto.put(REGMEM_KEY, REGMEM_VAL), F("put RAM->RAM failed"));
  char* storedRamRam = dto.get(REGMEM_KEY);
  t->assert(storedRamRam != nullptr, F("get RAM->RAM failed"));
  t->assert(storedRamRam != REGMEM_VAL, F("RAM->RAM value should be copied"));
  t->assert(dto.put(PMEM_KEY, REGMEM_VAL), F("put Flash->RAM failed"));
  char* storedFlashRam = dto.get(PMEM_KEY);
  t->assert(storedFlashRam != nullptr, F("get Flash->RAM failed"));
  t->assert(storedFlashRam != REGMEM_VAL, F("Flash->RAM value should be copied"));
  t->assert(dto.put(REGMEM_KEY, reinterpret_cast<const __FlashStringHelper*>(PMEM_VAL)), F("put RAM->Flash failed"));
  char* storedRamFlash = dto.get(REGMEM_KEY);
  t->assert(storedRamFlash != nullptr, F("get RAM->Flash failed"));
  t->assert(storedRamFlash == PMEM_VAL, F("RAM->Flash should store flash pointer"));
  t->assert(dto.put(reinterpret_cast<const __FlashStringHelper*>(PMEM_KEY),
      reinterpret_cast<const __FlashStringHelper*>(PMEM_VAL)), F("put Flash->Flash failed"));
  char* storedFlashFlash = dto.get(PMEM_KEY, true);
  t->assert(storedFlashFlash != nullptr, F("get Flash->Flash failed"));
  t->assert(storedFlashFlash == PMEM_VAL, F("Flash->Flash should store flash pointer"));
}

void testDestructionSafety(TestInvocation* t) {
  t->setName(F("StreamableDTO Destruction Safety"));

  char* dynamicKey = strdup("dynamicKey");
  char* dynamicVal = strdup("dynamicVal");

  {
    StreamableDTO dto;
    t->assert(dto.put(dynamicKey, dynamicVal), F("put dynamic->dynamic failed"));
    t->assert(dto.put(PMEM_KEY, PMEM_VAL), F("put flash->flash failed"));
  }
  // StreamableDTO is now out of scope and destroyed

  free(dynamicKey);
  free(dynamicVal);

  // If StreamableDTO incorrectly stored direct pointers, double-free would occur
  // If program doesn't crash, destructor behaved correctly!
  t->assert(true, F("StreamableDTO destruction completed safely"));
}


void setup() {
  Serial.begin(9600);
  while (!Serial);

  TestFunction tests[] = {
    testDefaultConstructor,
    testInitialSizeConstructor,
    testHashFunction,
    testKeyMatch,
    testKeyMatchPROGMEMvsPROGMEM,
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
    testLoadIncompatibleVersion,
    testMemoryBehavior,
    testDestructionSafety    
  };

  runTestSuiteShowMem(tests);

}

void loop() {}
