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
    void setName(const String& name) {
      put(BOOK_NAME_KEY, name); // ignoring bool return (assume succeeded)
    };
    String getName() {
      return get(BOOK_NAME_KEY, true);
    };
    void setPageCount(int pageCount) {
      put(BOOK_PAGES_KEY, String(pageCount)); // ignoring bool return (assume succeeded)
    };
    int getPageCount() {
      return String(get(BOOK_PAGES_KEY, true)).toInt();
    };

  protected:

    /*
     * Override parseValue to use the PROGMEM keys instead of raw strings
     */
    void parseValue(uint16_t lineNumber, const String& key, const String& value) override {
      if (strcmp_P(key.c_str(), BOOK_NAME_KEY) == 0) {
        setName(value);
      } else if (strcmp_P(key.c_str(), BOOK_PAGES_KEY) == 0) {
        setPageCount(value.toInt());
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