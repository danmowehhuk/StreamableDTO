#include <StreamableManager.h>
#include "StreamHelper.h"

String testString("a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\n");
StreamHelper testStream(testString);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  auto filter = [](const String &line, StreamableManager::DestinationStream* dest, void* state) -> bool {
    int separatorIndex = line.indexOf('=');
    String key = line.substring(0, separatorIndex);
    String value = line.substring(separatorIndex + 1);
    if (!key.equals("c")) {
      dest->println(line);    
    }
    return true;
  };

  StreamableManager strmMgr;
  strmMgr.pipe(&testStream, &Serial, filter);
  
}

void loop() {
  
}
