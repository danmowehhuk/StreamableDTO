#include "DataManager.h"
#include "Librarian.h"

/*
 * Note the custom parseValue(...) and toLine(...) overrides in StreamableBook.h
 */

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Librarian* librarian = new DataManager();

  Serial.println("Loading book record into memory");
  Book* book = librarian->getBook("Catcher In The Rye");
  Serial.println("This book has " + String(book->getPageCount()) + " pages.\n");

  Serial.println("Parsed from the 'meta' field:");
  Serial.println("  Publisher: " + book->getPublisher());
  Serial.println("  Year: " + book->getYear());
  Serial.println();
  
  Serial.println("Streaming book to Serial:\n");
  librarian->streamToSerial(book);
}

void loop() {
  
}
