#ifndef _Book_h
#define _Book_h

/*
 * Placing known key names in PROGMEM reduces dynamic memory
 * consumption and improves performance since keys can be matched
 * by pointer equality vs string equality
 */
static const char BOOK_NAME_KEY[]    PROGMEM = "name";
static const char BOOK_PAGES_KEY[]   PROGMEM = "pages";
static const char BOOK_META_KEY[]    PROGMEM = "meta";

/*
 * Provides more descriptive method names for the underlying 
 * data. Any other keys loaded into the table are still 
 * available.
 *
 * Also handles parsing and reserializing of the "meta" field
 */
class Book: public StreamableDTO {

  private:
    String _publisher;
    int _publishYear;

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
    void setPublisher(const String publisher) {
      _publisher = publisher;
    };
    String getPublisher() {
      return _publisher;
    };
    void setPublishYear(int publishYear) {
      _publishYear = publishYear;
    };
    int getPublishYear() {
      return _publishYear;
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
      } else if (strcmp_P(key, BOOK_META_KEY) == 0) {
        String val(value);
        int sepIdx = val.indexOf('|');
        String publisher = val.substring(0, sepIdx);
        String year = val.substring(sepIdx + 1);
        publisher.trim();
        setPublisher(publisher);
        year.trim();
        setPublishYear(year.toInt());

        // Need to put an empty key so it's included when reserializing
        putEmpty(BOOK_META_KEY, true); // ignoring bool return (assume succeeded)
      } else {
        /*
         * Default to base implementation so that unrecognized keys are
         * still stored in the table
         */
        StreamableDTO::parseValue(lineNumber, key, value);
      }
    };

    /*
     * Also override toLine to reconstruct the "meta" field
     */
    char* toLine(const char* key, const char* value, bool keyPmem, bool valPmem) override {
      if (key == BOOK_META_KEY) {

        // Ignore the value param (it's empty) and reconstruct "meta" value
        String k = String(reinterpret_cast<const __FlashStringHelper *>(key))
               + "=" + getPublisher() + "|" + String(getPublishYear());
        size_t len = k.length() + 1;  // +1 for null terminator
        char* line = new char[len];
        k.toCharArray(line, len);
        return line;

      } else {
        /*
         * Default to base implementation for all other fields
         */
        return StreamableDTO::toLine(key, value, keyPmem, valPmem);
      }
    };

};


#endif