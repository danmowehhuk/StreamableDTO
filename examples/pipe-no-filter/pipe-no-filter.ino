#include <StreamableManager.h>
#include "StreamHelper.h"

String testString("a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\n");
StreamHelper testStream(testString);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  StreamableManager strmMgr;
  strmMgr.pipe(&testStream, &Serial);
}

void loop() {
  
}
