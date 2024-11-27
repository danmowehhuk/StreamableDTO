#ifndef StreamableBook_h
#define StreamableBook_h


#include <StreamableDTO.h>
#include "Book.h"

/*
 * Implementation of Book using StreamableDTO to load and hold the data in memory
 * 
 * Note the custom parseLine and toLine methods
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

    // Data for these comes from the "meta" key in the stream so needs to 
    // be specially parsed and re-serialized
    String getPublisher() { return _publisher; };
    String getYear() { return _year; };

    // Custom value parser and re-serializer
    void parseValue(uint16_t lineNumber, const String& key, const String& value) override;
    String toLine(const String& key, const String& value) override;

  private:
    String _publisher;
    String _year;
        
};


#endif
