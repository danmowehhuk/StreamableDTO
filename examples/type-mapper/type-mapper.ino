#include <StreamableManager.h>
#include "StreamHelper.h"
#include "StreamableBook.h"
#include "StreamableMagazine.h"

/*
 * Note the use of typeId and serialVersionId in Book and Magazine
 */

// __tvid=<typeId>|<minCompatVersion>
String _citrStr = "__tvid=1|0\nname=Catcher In The Rye\npages=260\n";
String _peopleStr = "__tvid=2|1\nname=People\nissue=245\n";

StreamHelper _citr(_citrStr);
StreamHelper _people(_peopleStr);


void setup() {
  Serial.begin(9600);
  while (!Serial);

  StreamableManager strmMgr;

  auto typeMapper = [](uint16_t typeId) {
    StreamableDTO* dto = nullptr;
    switch (typeId) {
      case 1:
        dto = new StreamableBook();
        break;
      case 2:
        dto = new StreamableMagazine();
        break;
      default:
        Serial.println("Unknown type: " + String(typeId));
        while(true) {}
    }
    return dto;
  };

  // load(...) takes care of type and serial version checking
  StreamableBook* book = strmMgr.load(&_citr, typeMapper);
  Serial.println("Loaded book: " + book->getName());
  Serial.println("Page count: " + String(book->getPageCount()));

  Serial.println();
  Serial.println("Streaming book back to Serial");
  strmMgr.send(&Serial, book);
  Serial.println("End of book");

  delete book;

  Serial.println();

  StreamableMagazine* mag = strmMgr.load(&_people, typeMapper);
  Serial.println("Loaded magazine: " + mag->getName());
  Serial.println("Issue: " + String(mag->getIssue()));
  delete mag;

}

void loop() {
  
}
