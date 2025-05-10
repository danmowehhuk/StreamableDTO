# StreamableDTO

## Introduction

**StreamableDTO** is a C++ library for Arduino that makes it easy to serialize and deserialize structured data objects 
(DTOs) over any stream (Serial, file, etc.). It allows you to represent data as key–value pairs, send it over a `Stream`
in a human-readable format, and load it back into an object with minimal effort. This helps you avoid writing ad-hoc 
parsing code for sending data between Arduinos or storing configuration data.

**Key Features:**

- **Forward/Backward Compatibility:** Supports versioned data objects. New fields can be added without breaking older 
  receivers, and outdated fields are safely ignored.
- **Clean Data/Transport Separation:** Your data classes only include `StreamableDTO.h`. Business logic that involves 
  serial communication includes `StreamableManager.h`.
- **Field-Level Access:** Easily work with individual fields by name using `put()` and `get()`—no custom parsers needed.

### Installation

You can install StreamableDTO in the Arduino IDE like any other library:

- **Using Library Manager (recommended):** Open the Arduino Library Manager and search for “StreamableDTO”. Click 
  Install to add it to your libraries.
- **Manual Installation:** Download or clone this repository. Copy the StreamableDTO folder into your Arduino libraries
  directory, or use the Arduino IDE’s Sketch > Include Library > Add .ZIP Library... option and select the downloaded ZIP file.

After installation, include the library in your sketch:

```cpp
#include <StreamableDTO.h>
#include <StreamableManager.h>
#include <StringStream.h>
```

Now you’re ready to use StreamableDTO in your Arduino projects.

## Untyped StreamableDTO

The simplest way to use StreamableDTO is as a generic key/value store without specifying a type or version. This is 
useful for quick serialization of data when you don’t need strict versioning. You can use `put()` to add fields and 
`get()` to retrieve them by key:

```cpp
#include <StreamableDTO.h>

StreamableDTO data;
data.put("name", "Catcher in the Rye");
data.put("pages", "277");

// Access fields by key
Serial.println(data.get("name"));   // prints "Catcher in the Rye"
Serial.println(data.get("pages"));  // prints "277"
```

In this untyped mode, the DTO will just hold the keys and values (as strings). You can also remove a field with 
`remove()` or check if a key exists with `exists()`. When you serialize an untyped DTO (explained in 
[StreamableManager Usage](#streamablemanager-usage) below), it will output each field as `key=value` on separate lines. 

**Important:** The untyped usage does not include any type or version metadata in the serialized output. This means the
receiver must know what keys to expect. Untyped DTOs are great for simple scenarios or internal use, but if you need 
forward/backward compatibility guarantees across devices or firmware versions, you should use strongly typed DTOs with 
type IDs and versioning (see [Strong Types and Versioning](#strong-types-and-versioning)).

## PROGMEM Keys

On memory-constrained Arduino boards, it’s often beneficial to store constant strings (like field names) in flash memory
 PROGMEM) instead of RAM. StreamableDTO supports this by allowing you to use the Arduino `F()` macro or `PROGMEM` string
 pointers for keys (and values). For example, using `F()` for a key literal:
```cpp
StreamableDTO data;
data.put(F("status"), "OK");         // "status" is stored in flash, not RAM
Serial.println(data.get(F("status")));  // retrieves the value for "status"
```

In this snippet, the key `"status"` is stored in program memory, saving RAM. The library provides overloads of `put()` 
and `get()` that accept `const __FlashStringHelper*` (the type of strings produced by the `F()` macro) so you can 
seamlessly use flash-stored keys. Under the hood, `StreamableDTO` handles the `PROGMEM` lookup and comparison for you. 
If you will be referencing a key a lot, you can also declare it as a `PROGMEM` `static const char` array and use the 
`put_P()` and `get_P()` functions with the PROGMEM flag. Using PROGMEM for keys can also improve performance because keys
in flash can sometimes be compared by pointer instead of by content. This optimization is optional, but it can be useful
for reducing RAM usage when you have many constant field names.

## Custom Field Handling

`StreamableDTO` can be extended via subclassing to provide custom field accessors and handling logic. This lets you 
encapsulate the details of your DTO’s fields behind a nice interface, and even perform custom parsing/serialization 
for certain fields.

For example, suppose you have a `Book` data object with fields like `"name"`, `"pages"`, and a combined `"meta"` 
field that contains publisher and year information in one string. You can create a subclass with methods for these:
```cpp
#include <StreamableDTO.h>

static const char BOOK_NAME_KEY[]    PROGMEM = "name";
static const char BOOK_PAGES_KEY[]   PROGMEM = "pages";
static const char BOOK_META_KEY[]    PROGMEM = "meta";

class Book: public StreamableDTO {

  private:
    String _publisher;
    int _publishYear;

  public:
    Book(): StreamableDTO() {};
    void setName(const char* name) {
      put_P(BOOK_NAME_KEY, name);
    };
    String getName() {
      return get_P(BOOK_NAME_KEY);
    };
    void setPageCount(int pageCount) {
      put_P(BOOK_PAGES_KEY, String(pageCount).c_str());
    };
    int getPageCount() {
      return atoi(get_P(BOOK_PAGES_KEY));
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
     * Override parseValue to split the "meta" key
     */
    void parseValue(uint16_t lineNumber, const char* key, const char* value) override {
      if (strcmp_P(key, BOOK_META_KEY) == 0) {
        String val(value);
        int sepIdx = val.indexOf('|');
        String publisher = val.substring(0, sepIdx);
        String year = val.substring(sepIdx + 1);
        publisher.trim();
        setPublisher(publisher);
        year.trim();
        setPublishYear(year.toInt());

        // Need to put an empty key so it's included when reserializing
        putEmpty_P(BOOK_META_KEY);
      } else {
        // For other keys, fallback to default behavior (store in the table)
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

```

In this example, `Book` extends `StreamableDTO` to provide typed getters/setters (`setName`, `getPageCount`, etc.) 
instead of dealing with raw string keys in the rest of your code. It also overrides `parseValue` and `toLine` to handle
a combined `"meta"` field specially: when loading, it splits the publisher and year and stores them in `_publisher` 
and `_year` member variables, and when saving, it reconstructs the `"meta"` line from those members. All other 
fields (recognized or not) are still stored in the base class’s internal table by calling the base implementation.

By extending `StreamableDTO` in this way, you get a cleaner API for your DTO and can encapsulate how certain fields are
represented in the serialized form. (See the [examples/custom-type-field](/examples/custom-type-field) example for more
info)

## Strong Types and Versioning

For robust interoperability, `StreamableDTO` supports strong typing and versioning of your DTO classes. This is achieved
by assigning each DTO subclass a unique type ID and defining version numbers that indicate compatibility. With this
mechanism, a receiver can automatically determine what type of object is coming in and whether it can be parsed, before
even reading the fields. 

To create a strongly-typed DTO class, subclass `StreamableDTO` and override three virtual methods: `getTypeId()`, 
`getSerialVersion()`, and `getMinCompatVersion()`: 
- `getTypeId()` should return a unique `int16_t` identifier for your DTO class. This ID is included in the serialized 
output. Ensure each distinct DTO class in your system has a different type ID (and that the ID is the same across 
devices and versions).
- `getSerialVersion()` returns a `uint8_t` version number of the format/structure that the class currently serializes. 
You should increment this when you make changes to the data format of the class.
- `getMinCompatVersion()` returns a `uint8_t` indicating the oldest serial version that this class can understand. If 
you introduce a change that older code would not understand, you would increase this value. If the incoming data’s 
version is below this, the object will be considered incompatible.

For example, continuing the `Book` class:
```cpp
#define BOOK_TYPE_ID 1
#define BOOK_VERSION 0
#define BOOK_MIN_COMPAT_VERSION 0

class Book : public StreamableDTO {
  // ... (custom field methods as above) ...

protected:
  int16_t getTypeId() override { return BOOK_TYPE_ID; }
  uint8_t getSerialVersion() override { return BOOK_VERSION; }
  uint8_t getMinCompatVersion() override { return BOOK_MIN_COMPAT_VERSION; }
};
```

In this case, `Book` has type ID `1`. We set `BOOK_VERSION` to `0` (for the initial version of our `Book` format) and 
`BOOK_MIN_COMPAT_VERSION` to `0` (meaning this class can parse version `0` data — itself, and we haven’t broken 
compatibility with any older format). If in the future we add new mandatory fields to `Book` that old code wouldn’t 
handle, we might set `getSerialVersion()` to return `1` and, if the change is not backward-compatible, also set 
`getMinCompatVersion()` to `1` to indicate that version `0` data is no longer fully compatible.

When a `StreamableDTO` has a type ID, the `StreamableManager`
will include a special metadata line at the beginning of the serialized output containing the type ID and version. This
line looks like:
```cpp
__tvid=<typeId>|<serialVersion>
```

For example, a `Book` object with type ID `1` and serialVersion `0` will begin with `__tvid=1|0` on the first line 
when serialized (the library handles this automatically). The receiving side uses this information to decide how to 
instantiate the object and whether it can parse it.

**Type Mapping (Factory Function)**: In a system with multiple DTO types, you’ll typically maintain a single 
`TypeMapper` function that knows how to create a new object of the correct subclass given a type ID. This is essentially
a factory for your `StreamableDTO` types. For example:
```cpp
StreamableDTO* createDTOByType(uint16_t typeId) {
  switch (typeId) {
    case BOOK_TYPE_ID:
      return new Book();
    // case 2: return new SomeOtherDTO();
    // ... handle other known types ...
    default:
      return nullptr; // Unknown type
  }
}
```

This function can be passed to `StreamableManager` when loading data, so that the library will call it to create an 
instance of the right class based on the incoming type ID. We’ll see how this works in the next section.

## StreamableManager Usage
`StreamableManager` is the utility class that actually reads from and writes to streams. It provides methods like 
`send()` to serialize a DTO to a stream, `load()` to parse a DTO from a stream, and `pipe()` (covered later) to relay 
data between streams. By using `StreamableManager`, you don’t have to manually iterate over keys or handle stream I/O 
byte-by-byte.

Here are common usage patterns for `StreamableManager`:
- **Sending a DTO**: Use `send(Stream* dest, StreamableDTO* dto)` to write the DTO’s contents to the destination stream.
- **Loading into a DTO**: Use `load(Stream* src, StreamableDTO* dto)` to read from the source stream and populate the 
given DTO object’s fields.
- **Loading with Type Mapping**: Use `load(Stream* src, TypeMapper func)` to read from the source and automatically 
create a new DTO of the appropriate subclass based on the type ID in the data. `TypeMapper` is a function with signature
`StreamableDTO* func(uint16_t typeId)` (like the `createDTOByType` factory described above).

Because StreamableManager works with Arduino `Stream` objects, you can use it with any source or destination: `Serial`,
`SoftwareSerial`, `File` (SD card), etc. The library also provides a `StringStream` class, which is extremely handy for 
testing and in-memory operations. `StringStream` allows you to use a String as a `Stream` for both input and output.

**Example – Serializing and Deserializing (Untyped):**
```cpp
#include <StreamableDTO.h>
#include <StreamableManager.h>
#include <StringStream.h>

StreamableDTO sensorData;
sensorData.put("sensor", "DHT22");
sensorData.put("temperature", "24.7");

// Create a manager and a memory stream for testing
StreamableManager manager;
StringStream memOut;

// Send the DTO to the memory stream instead of Serial
manager.send(&memOut, &sensorData);

// memOut.getString() now contains the serialized data:
Serial.println("Serialized output:");
Serial.println(memOut.getString());
// Example output:
// sensor=DHT22
// temperature=24.7

// Now load it back into a new DTO from the memory stream
StringStream memIn(memOut.getString());
StreamableDTO received;
manager.load(&memIn, &received);

Serial.println("Loaded fields:");
Serial.println(received.get("sensor"));       // "DHT22"
Serial.println(received.get("temperature"));  // "24.7"
```

In this example, we used a `StringStream` to capture the output of `send()` and then read from it with `load()`. This 
demonstrates how you can unit test your DTO serialization logic entirely in memory, or just inspect the serialized form
easily. In a real scenario, you might call `manager.send(&Serial, &sensorData)` to send over a hardware `Serial` port, 
or `manager.load(&file, &received)` to read from an SD card file stream, etc.

If you’re using **strongly typed DTOs** with type IDs, the usage is similar, but you would use the type-mapping overload
of `load()`. For example:
```cpp
// Assume inputStream is a Stream that has a serialized Book object (with __tvid line)
StreamableManager mgr;
StreamableDTO* dto = mgr.load(&inputStream, createDTOByType);
if (dto) {
    // Successfully created and loaded an object
    if (dto->getTypeId() == BOOK_TYPE_ID) {
        Book* book = static_cast<Book*>(dto);
        Serial.println("Received book title: " + book->getName());
        // ... use book ...
    }
}
```

Here we pass `createDTOByType` (our `TypeMapper` function) to `load()`. The manager will read the `__tvid` line, 
determine the type (say `1` for `Book`), call `createDTOByType(1)` to get a new `Book` instance, and then fill it with
the incoming data. We can then cast the returned pointer to `Book*` and use it. If the type ID was unknown or the 
version was incompatible, `load()` would return `nullptr`.

## Piping Data
Sometimes you may want to relay a DTO message from one stream to another without fully loading it into an object. This 
can be useful in scenarios like forwarding data from one serial port to another (acting as a bridge or repeater) or 
capturing a message to a log file while also sending it onward. StreamableDTO provides `StreamableManager::pipe()` for 
this purpose.
```cpp
void StreamableManager::pipe(Stream* src, Stream* dest, 
                             FilterFunction filter = nullptr, void* state = nullptr);
```
`pipe()` reads from the `src` stream line by line and writes each line to the `dest` stream. It does not allocate a big
buffer for the whole DTO or attempt to interpret the fields (aside from optional filtering, discussed next). This means
it can handle large DTOs or continuous data with very low memory overhead – it’s essentially streaming the text through.

**Why use pipe()?** If your device needs to pass along messages to another device or layer (perhaps your Arduino is just
a conduit), you don’t need to parse every field only to re-serialize it. `pipe()` will efficiently forward the data. It
also ensures that you aren’t introducing extra delays by processing the entire message first; lines are forwarded as 
they are read.

Basic usage is straightforward:
```cpp
StreamableManager manager;
// Relay all data from Serial to Serial1 as it arrives
manager.pipe(&Serial, &Serial1);
```

This will read each line from `Serial` and immediately write it to `Serial1`. The function continues until the source 
stream has no more data (end of stream or message). It handles the `__tvid` line and all field lines in the same way 
(just passing them through).

## Filter Functions
The `pipe()` function becomes even more powerful with an optional filter function. A `FilterFunction` allows you to 
inspect or modify each line of the DTO as it passes through the pipe, or even to suppress certain lines. This is useful
for tasks like stripping out or redacting sensitive fields, converting units, or tweaking values without needing to 
fully parse into a DTO object and then reserialize.

A filter function has the signature:
```cpp
bool filter(const char* line, StreamableManager::DestinationStream* out, void* state);
```

- `line` is a C-string containing the line read from the source (without the newline).
- `out` is a helper object you use to write to the destination (it has `print`/`println` methods to send data to the 
`dest` stream).
- `state` is an arbitrary pointer you can use to pass in context or accumulate results (it can be `nullptr` if not 
used).
- The function should return `true` to continue piping the next line, or `false` to stop the piping early (if, for 
example, you found what you needed or decided to abort).

**Example – Modifying a field in transit:**

Suppose we want to forward a DTO, but if we encounter a field `"pages"`, we’ll modify its value (say, double the number
of pages for demonstration purposes):
```cpp
StreamableManager mgr;

// Define a filter function that doubles the "pages" field's value
auto filter = [](const char* line, StreamableManager::DestinationStream* out, void* state) -> bool {
    // Check if the line starts with "pages="
    if (strncmp(line, "pages=", 6) == 0) {
        int pages = atoi(line + 6);     // parse the number after "pages="
        pages *= 2;                     // modify the value (double it)
        out->print("pages=");           
        out->println(pages);            // write the modified line to dest
        return true;                    // continue with next line
    }
    // For all other lines, pass them through unchanged
    out->println(line);
    return true;
};

// Now use pipe with the filter
mgr.pipe(&sourceStream, &destStream, filter);
```

In this snippet, as `mgr.pipe` reads each line from `sourceStream`, the function inspects it. If the key is `"pages"`,
it changes the value before printing it to the `destStream`; otherwise it just forwards the line as-is. The result is 
that everything is forwarded untouched except the pages count, which will be doubled.

You can also use the `state` parameter to make the filter more flexible. For example, you might define a struct holding
a specific key to remove or a threshold to apply, and pass a pointer to that struct as the `state`. Inside the filter,
you can cast `state` back to your struct type and use its data. The filter function in the example above doesn’t need 
external state, so we passed none (`state` is unused and could be `nullptr`). But if, for instance, you wanted to remove
a field whose name is determined at runtime, you could pass that name in the `state` and have the filter compare against
it.

**Example – Using state to filter out a key:**
```cpp
struct FilterState { String keyToRemove; };
FilterState st { "secret" };

auto removeFilter = [](const char* line, StreamableManager::DestinationStream* out, void* state) -> bool {
    FilterState* st = static_cast<FilterState*>(state);
    // If the line starts with the key we want to remove, skip it
    if (st && strncmp(line, st->keyToRemove.c_str(), st->keyToRemove.length()) == 0 
            && line[st->keyToRemove.length()] == '=') {
        // Do not forward this line (effectively removing the field)
        return true; // continue to next line (but we don't output this one)
    }
    // Otherwise, forward the line
    out->println(line);
    return true;
};

// This will forward all lines except those starting with "secret="
mgr.pipe(&src, &dest, removeFilter, &st);
```

In summary, the filter mechanism allows you to tweak the streaming data on the fly. This can be far more efficient than
fully parsing and reconstructing a DTO just to change one field. Use it to implement features like data sanitization, 
on-the-fly unit conversion (e.g., convert all `"tempC"` fields to `"tempF"`), or selective logging. The `void* state` 
parameter is provided so you can pass any additional info or storage to the filter function without using global 
variables.

---
With StreamableDTO, you get a flexible system for handling structured data on Arduino: start simple with untyped DTOs, 
and scale up to typed, versioned DTOs as your project grows. You can cleanly send and receive data, maintain 
compatibility across versions, and even plug in custom behavior or filters when needed. Check out the examples included
in the repository for more insight into how to use the library in various scenarios. Enjoy!
