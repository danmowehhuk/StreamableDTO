#include "strdto_internal/Hashtable.h"
#include "HashtableTestHelper.h"

#define STR(s) String(F(s))

struct TestInvocation {
  String name = String();
  bool success = false;
  String message = String();
};

HashtableTestHelper helper;

static const char PMEM_KEY[]   PROGMEM = "myKey";
static const char PMEM_KEY2[]  PROGMEM = "myKey";
static const char REGMEM_KEY[]         = "myKey";
static const char PMEM_VAL[]   PROGMEM = "myVal";
static const char REGMEM_VAL[]         = "myVal";

void testDefaultConstructor(TestInvocation* t) {
  t->name = STR("Default constructor");
  Hashtable table;
  do {
    if (helper.getTableSize(&table) != 16) {
      t->message = STR("Incorrect starting size");
      break;
    }
    t->success = true;
  } while (false);
}

void testInitialSizeConstructor(TestInvocation* t) {
  t->name = STR("Initial size constructor");
  Hashtable table(32, 0.9);
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
  Hashtable table;
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
  Hashtable table;
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

  Serial.println(F("Starting Hashtable tests..."));

  void (*tests[])(TestInvocation*) = {

    testDefaultConstructor,
    testInitialSizeConstructor,
    testHashFunction,
    testKeyMatch
    // test functions here...

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

void loop() {}
