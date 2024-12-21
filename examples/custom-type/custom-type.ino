#include <StreamableManager.h>
#include <StringStream.h>
#include "Book.h"

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
  Book book;

  streamMgr.load(&stream, &book);

  Serial.println("Loaded '" + book.getName() + "'");
  Serial.println("This book has " + String(book.getPageCount()) + " pages");
  Serial.println("\nStreaming book to Serial:");
  streamMgr.send(&Serial, &book);
}

void loop() {}
