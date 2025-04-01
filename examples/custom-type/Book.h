#ifndef _Book_h
#define _Book_h

/*
 * Placing known key names in PROGMEM reduces dynamic memory
 * consumption and improves performance since keys can be matched
 * by pointer equality vs string equality
 */
static const char BOOK_NAME_KEY[]    PROGMEM = "name";
static const char BOOK_PAGES_KEY[]   PROGMEM = "pages";

/*
 * Provides more descriptive method names for the underlying 
 * data. Any other keys loaded into the table are still 
 * available.
 */
class Book: public StreamableDTO {

  public:
    Book(): StreamableDTO() {};
    void setName(const char* name) {
      put(BOOK_NAME_KEY, name, true); // ignoring bool return (assume succeeded)
    };
    String getName() {
      return get(BOOK_NAME_KEY, true);
    };
    void setPageCount(int pageCount) {
      put(BOOK_PAGES_KEY, String(pageCount).c_str(), true); // ignoring bool return (assume succeeded)
    };
    int getPageCount() {
      return atoi(get(BOOK_PAGES_KEY, true));
    };

  protected:

    /*
     * Override parseValue to use the PROGMEM keys instead of raw strings
     */
    void parseValue(uint16_t lineNumber, const char* key, const char* value) override {
      if (strcmp_P(key, BOOK_NAME_KEY) == 0) {
        setName(value);
      } else if (strcmp_P(key, BOOK_PAGES_KEY) == 0) {
        setPageCount(atoi(value));
      } else {
        /*
         * Default to base implementation so that unrecognized keys are
         * still stored in the table
         */
        StreamableDTO::parseValue(lineNumber, key, value);
      }
    };
};


#endif