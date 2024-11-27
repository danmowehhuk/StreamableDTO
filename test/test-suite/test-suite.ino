#include <SimpleVector.h>
#include <StreamableDTO.h>
#include <StreamableManager.h>
#include <StringStream.h>
#include "MyTypedDTO.h"

#define STR(s) String(F(s))

StreamableManager streamMgr;

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
      Serial.println(STR("FATAL! Unknown type: ") + String(typeId));
      break;
  }
  return dto;
};

void testPropertyOperations(TestInvocation* t) {
  t->name = String(F("Standard property operations"));
  StreamableDTO* dto = new StreamableDTO();
  do {
    dto->setProperty(STR("foo"), STR("bar"));
    if (!dto->containsKey(STR("foo"))) {
      t->message = STR("Key not created");
      break;
    }
    if (!dto->getProperty(STR("foo"),String()).equals(STR("bar"))) {
      t->message = STR("Incorrect value for key");
      break;
    }
    dto->setProperty(STR("foo"),STR("updatedBar"));
    if (!dto->getProperty(STR("foo"),String()).equals(STR("updatedBar"))) {
      t->message = STR("Failed to update value of key");
      break;
    }
    if (!dto->getProperty("abc","def").equals(STR("def"))) {
      t->message = STR("Default value not returned");
      break;
    }
    dto->removeProperty(STR("foo"));
    if (dto->containsKey(STR("foo"))) {
      t->message = STR("Remove property failed");
      break;
    }
    t->success = true;
  } while (false);
  delete dto;
}

void testIteratorUntyped(TestInvocation* t) {
  t->name = String(F("Iterator operations (untyped DTO)"));
  StreamableDTO* dto = new StreamableDTO();
  do {
    dto->setProperty(STR("foo"), STR("bar"));
    dto->setProperty(STR("abc"), STR("123"));
    uint8_t keyCount = 0;

    SimpleVector<String> keys = dto->getKeys();
    if (keys.size() != 2) {
      t->message = STR("Expected 2 keys but got ") + String(keys.size());
      break;
    }


    for (StreamableDTO::KVIterator it = dto->begin(); it != dto->end(); ++it) {
      String key = it.key();
      Serial.println("key: '" + key + "'");
      if (!key.equals(STR("foo")) && !key.equals(STR("abc"))) {
        t->message = STR("Unexpected key in iterator: '") + key + STR("'");
        goto end_tests;
      }
      keyCount++;
    }
    if (keyCount != 2) {
      t->message = STR("Expected 2 keys but got ") + keyCount;
      break;
    }
    t->success = true;
  } while (false);
  end_tests:
  delete dto;
}

void testLoadUntypedStreamableDTO(TestInvocation* t) {
  t->name = STR("Load untyped StreamableDTO");
  String data = STR("foo=bar\nabc=def\n");
  StringStream ss(data);
  StreamableDTO* dto = new StreamableDTO();
  streamMgr.load(&ss, dto);
  do {
    if (!dto->containsKey(STR("foo")) || !dto->containsKey(STR("abc"))) {
      t->message = STR("Missing key(s)");
      break;
    }
    if (!dto->getProperty(STR("foo"),String()).equals(STR("bar"))
        || !dto->getProperty(STR("abc"),String()).equals(STR("def"))) {
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
    if (!dto->containsKey(STR("foo")) || !dto->containsKey(STR("abc"))) {
      t->message = STR("Missing key(s)");
      break;
    }
    if (!dto->getProperty(STR("foo"),String()).equals(STR("bar"))
        || !dto->getProperty(STR("abc"),String()).equals(longValue)) {
      t->message = STR("Unexpected values for keys");
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
  streamMgr.load(&src, dto);
  streamMgr.send(&dest, dto);
  do {
    if (!dest.getString().equals(data)) {
      t->message = STR("Stream does not match original");
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
    uint8_t keyCount = 0;
    for (StreamableDTO::KVIterator it = dto->begin(); it != dto->end(); ++it) {
      String key = it.key();
      if (!key.equals(STR("foo")) && !key.equals(STR("abc"))) {
        t->message = STR("Unexpected key in iterator");
        goto end_tests;
      }
      keyCount++;
    }
    if (keyCount != 2) {
      t->message = STR("Expected 2 keys but got ") + keyCount;
      break;
    }
    t->success = true;
  } while (false);
  end_tests:
  delete dto;
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
    testPropertyOperations,
    testIteratorUntyped,
    testLoadUntypedStreamableDTO,
    testLoadLongLine,
    testSendUntypedStreamableDTO,
    testLoadTypedStreamableDTO,
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
