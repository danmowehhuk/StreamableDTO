#ifndef StreamableBook_h
#define StreamableBook_h


#include <StreamableDTO.h>
#include "Book.h"

/*
 * Implementation of Book using StreamableDTO to load and hold the data in memory
 */

class StreamableBook: public Book, public StreamableDTO {

  public:
    StreamableBook() {}; // need a public default constructor

    // Override all the virual methods to put/fetch fields from 
    // StreamableDTO's hash table. Note that all values are stored
    // as Strings
    
    String getName() override {
      return getProperty(String(F("name")), String(F("no name")));
    };
    
    void setName(const String &name) override {
      setProperty(String(F("name")), name);
    };
    
    int getPageCount() override {
      return getProperty(String(F("pages")), String(F("0"))).toInt();
    };
    
    void setPageCount(int pageCount) override {
      setProperty(String(F("pages")), String(pageCount));      
    };

};


#endif
