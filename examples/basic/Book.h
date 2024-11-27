#ifndef Book_h
#define Book_h

/*
 * A pure virtual business object describing a book.
 * Note there's no mention of Streamables.
 */

class Book {

  public:
    virtual ~Book() {}; // Needed for destructor-chaining
    
    virtual String getName() = 0;
    virtual void setName(const String &name) = 0;
    virtual int getPageCount() = 0;
    virtual void setPageCount(int pageCount) = 0;
  
};


#endif
