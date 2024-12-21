#include <StreamableManager.h>
#include <StringStream.h>
#include "Book.h"

/*
 * This StringStream could be replaced with any kind of Stream,
 * such as a serial UART or a file opened for read
 *
 * Note the addition of the "meta" field with two pieces of
 * information separated by a | character
 * 
 * Also note the __tvid field for type mapping and version 
 * compatibility checking. This is applied automatically when
 * serializing any DTO with a typeId != -1
 */
String _citrStr = "__tvid=1|0\nname=Catcher In The Rye\npages=260\nmeta=Little, Brown and Company|1951\n";
StringStream stream(_citrStr);

/*
 * A type mapping function that returns an instance of the
 * correct sub-class
 */
StreamableDTO* typeMapper(uint16_t typeId) {
  StreamableDTO* dto = nullptr;
  switch (typeId) {
    case 1: // You are responsible for making your typeIds unique
      dto = new Book();
      break;
    default:
      Serial.println("ERROR - Unknown typeId: " + String(typeId));
      break; // return nullptr by default
  }
  return dto;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  StreamableManager streamMgr;
  Book* book = streamMgr.load(&stream, typeMapper);

  Serial.println("Loaded '" + book->getName() + "'");
  Serial.println("This book has " + String(book->getPageCount()) + " pages");
  Serial.print("Published by " + book->getPublisher());
  Serial.println(" in " + String(book->getPublishYear()));
  Serial.println("\nStreaming book to tmp stream...");

  StringStream tmp;
  streamMgr.send(&tmp, book);

  Serial.println("Reloading book from tmp stream...");
  Book* book2 = streamMgr.load(&tmp, typeMapper);

  Serial.println("Streaming book to Serial:");
  streamMgr.send(&Serial, book2);
}

void loop() {}
