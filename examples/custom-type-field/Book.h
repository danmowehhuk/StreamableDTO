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
    void setPublisher(const String& publisher) {
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
    void parseValue(uint16_t lineNumber, const String& key, const String& value) override {
      if (strcmp_P(key.c_str(), BOOK_NAME_KEY) == 0) {
        setName(value);
      } else if (strcmp_P(key.c_str(), BOOK_PAGES_KEY) == 0) {
        setPageCount(value.toInt());
      } else if (strcmp_P(key.c_str(), BOOK_META_KEY) == 0) {
        int sepIdx = value.indexOf('|');
        String publisher = value.substring(0, sepIdx);
        String year = value.substring(sepIdx + 1);
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
    String toLine(const char* key, const char* value, bool keyPmem, bool valPmem) override {
      // Can use pointer equality because the "meta" key is in PROGMEM
      if (keyPmem && key == BOOK_META_KEY) {

        // Ignore the value param (it's empty) and reconstruct "meta" value
        String k = String(reinterpret_cast<const __FlashStringHelper *>(key));
        return k + "=" + getPublisher() + "|" + String(getPublishYear());

      } else {
        /*
         * Default to base implementation for all other fields
         */
        return StreamableDTO::toLine(key, value, keyPmem, valPmem);
      }
    };

};


#endif