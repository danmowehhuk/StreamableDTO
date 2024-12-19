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
      // unknown type
      break;
  }
  return dto;
};

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
