#include <StreamableManager.h>
#include <StringStream.h>


void setup() {
  Serial.begin(9600);
  while (!Serial);

  String testString("a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\n");
  StringStream srcStream(testString);
  StringStream destStream;

  StreamableManager streamableMgr;

  Serial.println("Streaming from srcStream:");
  Serial.println(testString);

  streamableMgr.pipe(&srcStream, &destStream);

  Serial.println("Received by destStream:");
  Serial.println(destStream.getString());
}

void loop() {}
