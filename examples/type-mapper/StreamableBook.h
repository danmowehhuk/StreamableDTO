#ifndef StreamableBook_h
#define StreamableBook_h


#include <StreamableDTO.h>
#include "Book.h"

// Ensure that none of your other types use this id and that the value
// never changes for this class.
#define TYPE_ID 1

// Increment this any time you modify this type
#define SERIAL_VERSION 1

// Type must be backwards compatible to this serial version
#define MIN_COMPAT_VERSION 0


/*
 * Implementation of Book using StreamableDTO to load and hold the data in memory
 */

class StreamableBook: public Book, public StreamableDTO {

  public:
    StreamableBook() {}; // need a public default constructor

    virtual int16_t getTypeId() override {
      return TYPE_ID;
    };
    virtual uint8_t getSerialVersion() override {
      return SERIAL_VERSION;
    };
    virtual uint8_t getMinCompatVersion() override {
      return MIN_COMPAT_VERSION;
    };


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
