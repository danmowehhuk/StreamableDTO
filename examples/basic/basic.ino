#include <StreamableDTO.h>
#include <StreamableManager.h>
#include <StringStream.h>

/*
 * This StringStream could be replaced with any kind of Stream,
 * such as a serial UART or a file opened for read
 */
String _citrStr = "name=Catcher In The Rye\npages=260\n";
StringStream stream(_citrStr);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  StreamableManager streamMgr;
  StreamableDTO book;

  streamMgr.load(&stream, &book);

  Serial.println("Loaded '" + String(book.get("name")) + "'");
  Serial.println("This book has " + String(book.get("pages")) + " pages");
  Serial.println("\nStreaming book to Serial:");
  streamMgr.send(&Serial, &book);
}

void loop() {}
