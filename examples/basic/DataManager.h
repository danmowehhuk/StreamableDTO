#ifndef DataManager_h
#define DataManager_h


#include <StreamableManager.h>
#include "Librarian.h"
#include "StreamHelper.h"

/*
 * Implements the Librarian's methods using StreamableDTO on the backend.
 */

class DataManager: public Librarian, public StreamableManager {

  public:
    DataManager(): Librarian(), StreamableManager(),
        _citr(_citrStr),
        _lesMis(_lesMisStr),
        _unknown(_unknownStr) {};
    Book* getBook(const String& name) override;
    void streamToSerial(Book* book) override;

  private:
    // Since this is a singleton, disable the copy constructor
    // to ensure only references are passed around
    DataManager(const DataManager &t) = delete;
    Stream* getStream(const String& name);

    /*
     * The following fake streams are just key=value lines that could
     * just as easily come from a file, UART or other serial source
     */
    String _citrStr = "name=Catcher In The Rye\npages=260\n";
    String _lesMisStr = "name=Les Miserables\npages=1066\n";
    String _unknownStr = "UNKNOWN\n";

    StreamHelper _citr;
    StreamHelper _lesMis;
    StreamHelper _unknown;

};


#endif
