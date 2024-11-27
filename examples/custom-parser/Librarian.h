#ifndef Librarian_h
#define Librarian_h


#include "Book.h"

/*
 * A singleton service that provides access to Books.
 * Note there's no mention of Streamables.
 */

class Librarian {

  public:
    Librarian() {};
    virtual Book* getBook(const String& name) = 0;
    virtual void streamToSerial(Book* book) = 0;
 
  protected:
    // for storing a Book loaded into memory
    Book* _book = nullptr;

  private:
    // Since this is a singleton, disable the copy constructor
    // to ensure only references are passed around
    Librarian(const Librarian &t) = delete; 
  
};


#endif
