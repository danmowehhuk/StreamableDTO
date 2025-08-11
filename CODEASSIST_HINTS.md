# StreamableDTO Framework - Code Assistant Hints

This file contains key learnings and best practices for working with the StreamableDTO framework, extracted from real-world usage patterns and debugging experiences.

## Core Concepts

### StreamableDTO Architecture
- **Base Class**: All DTOs inherit from `StreamableDTO`
- **Internal Storage**: Uses a hashtable for key-value pairs
- **Serialization**: Converts hashtable to stream format
- **Deserialization**: Parses stream back to hashtable

### Key Methods to Override
```cpp
// Required overrides
virtual uint16_t getTypeId() const = 0;
virtual uint16_t getSerialVersion() const = 0;
virtual uint16_t getMinCompatVersion() const = 0;

// Optional overrides for custom behavior
virtual void parseValue(uint16_t lineNumber, const char* key, const char* value);
virtual void toLine(const char* key, const char* value, Stream& stream);
```

## Storage Patterns

### 1. Direct String Storage (Simple)
```cpp
class SimpleDTO : public StreamableDTO {
private:
    static const char NAME_KEY[] PROGMEM;
    
public:
    void setName(const char* name) {
        put_P(NAME_KEY, name); // Store directly as string
    }
    
    const char* getName() {
        return get(NAME_KEY, true); // Retrieve as string
    }
};
```

### 2. Hybrid Storage (Recommended for Complex Types)
```cpp
class HybridDTO : public StreamableDTO {
private:
    uint64_t _rawValue; // Efficient in-memory storage
    static const char VALUE_KEY[] PROGMEM;
    
    void updateHashtable() {
        char buffer[21];
        if (uint64ToString(_rawValue, buffer, sizeof(buffer))) {
            put_P(VALUE_KEY, buffer); // Update hashtable for serialization
        }
    }
    
public:
    void setValue(uint64_t value) {
        _rawValue = value; // Direct assignment
        updateHashtable(); // Sync with hashtable
    }
    
    uint64_t getValue() {
        return _rawValue; // Direct return
    }
    
protected:
    void parseValue(uint16_t lineNumber, const char* key, const char* value) {
        if (strcmp_P(key, VALUE_KEY) == 0) {
            _rawValue = stringToUint64(value); // Convert string to raw type
        } else {
            StreamableDTO::parseValue(lineNumber, key, value);
        }
    }
};
```

## Serialization/Deserialization

### StreamableManager Usage
```cpp
StreamableManager manager;
StringStream stream;

// Serialization
manager.send(dto, stream);

// Deserialization
stream.toInStream(); // Switch to input mode
manager.load(dto, stream);
```

### StringStream Behavior
- **Output Mode**: `reset()` clears the buffer
- **Input Mode**: `reset()` preserves buffer content
- **Mode Switching**: Use `toInStream()` before `reset()` for deserialization

## PROGMEM Best Practices

### Key Definitions
```cpp
// Always use PROGMEM for string constants
static const char FIELD_NAME_KEY[] PROGMEM = "fn";
static const char FIELD_VALUE_KEY[] PROGMEM = "fv";
```

### Storage Methods
```cpp
// For PROGMEM keys
put_P(PROGMEM_KEY, value);

// For regular strings
put(regularKey, value);
```

## Type Conversion Patterns

### Custom Type Conversion
```cpp
// In parseValue override
void parseValue(uint16_t lineNumber, const char* key, const char* value) {
    if (strcmp_P(key, INT_KEY) == 0) {
        _intValue = atoi(value);
    } else if (strcmp_P(key, BOOL_KEY) == 0) {
        _boolValue = (strcmp(value, "1") == 0);
    } else {
        StreamableDTO::parseValue(lineNumber, key, value);
    }
}

// In setter methods
void setIntValue(int value) {
    _intValue = value;
    char buffer[12];
    itoa(value, buffer, 10);
    put_P(INT_KEY, buffer);
}
```

## Testing Patterns

### Serialization/Deserialization Test
```cpp
void testSerialization() {
    MyDTO dto;
    dto.setValue(12345);
    
    StreamableManager manager;
    StringStream stream;
    
    // Serialize
    manager.send(dto, stream);
    
    // Switch to input mode and reset
    stream.toInStream();
    stream.reset();
    
    // Deserialize
    MyDTO newDto;
    manager.load(newDto, stream);
    
    // Verify
    assert(newDto.getValue() == 12345);
}
```

### Constructor Initialization
```cpp
class MyDTO : public StreamableDTO {
public:
    MyDTO() : StreamableDTO() {
        // Initialize default values
        setValue(0); // This calls updateHashtable()
    }
};
```

## Common Pitfalls

### 1. Buffer Overflow
```cpp
// WRONG - Buffer too small
char buffer[8];
encodeBase64(value, buffer); // Needs 9 bytes (8 chars + null)

// CORRECT - Proper buffer size
char buffer[9];
encodeBase64(value, buffer);
```

### 2. PROGMEM Key Comparison
```cpp
// WRONG - Direct comparison
if (key == PROGMEM_KEY) { ... }

// CORRECT - Use strcmp_P
if (strcmp_P(key, PROGMEM_KEY) == 0) { ... }
```

### 3. Stream Mode Issues
```cpp
// WRONG - Missing mode switch
manager.send(dto, stream);
stream.reset(); // Clears buffer in output mode
manager.load(newDto, stream); // Empty stream

// CORRECT - Switch to input mode
manager.send(dto, stream);
stream.toInStream();
stream.reset(); // Preserves buffer in input mode
manager.load(newDto, stream);
```

### 4. Missing Hashtable Updates
```cpp
// WRONG - Only updates raw storage
void setValue(uint64_t value) {
    _rawValue = value; // Hashtable not updated
}

// CORRECT - Sync with hashtable
void setValue(uint64_t value) {
    _rawValue = value;
    updateHashtable(); // Keep hashtable in sync
}
```

## Performance Optimization

### Memory Usage
- Use PROGMEM for all string constants
- Prefer stack allocation over heap
- Use meaningful buffer sizes with comments
- Implement hybrid storage for complex types

### Serialization Efficiency
- Only store necessary fields
- Use minimal field names (e.g., "id" not "identifier")
- Consider field ordering for optimal parsing

## Debugging Techniques

### Stream Content Inspection
```cpp
// Debug serialization
manager.send(dto, stream);
Serial.print("Stream content: ");
Serial.println(stream.getString());
```

### Key-Value Tracing
```cpp
// In parseValue override
void parseValue(uint16_t lineNumber, const char* key, const char* value) {
    Serial.print("Parsing key: ");
    Serial.print(key);
    Serial.print(" value: ");
    Serial.println(value);
    // ... rest of parsing logic
}
```

### Hashtable Verification
```cpp
// Check if field exists in hashtable
const char* value = get(FIELD_KEY, true);
if (value) {
    Serial.print("Field found: ");
    Serial.println(value);
} else {
    Serial.println("Field not found");
}
```

## Version Management

### Type Versioning
```cpp
class MyDTO : public StreamableDTO {
private:
    static const uint16_t MY_TYPE_ID = 100;
    static const uint16_t MY_TYPE_VER = 1;
    static const uint16_t MY_TYPE_MIN_COMPAT_VER = 0;
    
public:
    uint16_t getTypeId() const { return MY_TYPE_ID; }
    uint16_t getSerialVersion() const { return MY_TYPE_VER; }
    uint16_t getMinCompatVersion() const { return MY_TYPE_MIN_COMPAT_VER; }
};
```

### Backward Compatibility
```cpp
void parseValue(uint16_t lineNumber, const char* key, const char* value) {
    if (strcmp_P(key, OLD_FIELD_KEY) == 0) {
        // Handle old format
        _value = atoi(value);
    } else if (strcmp_P(key, NEW_FIELD_KEY) == 0) {
        // Handle new format
        _value = stringToUint64(value);
    } else {
        StreamableDTO::parseValue(lineNumber, key, value);
    }
}
```

## Best Practices Summary

1. **Always use PROGMEM for string constants**
2. **Implement hybrid storage for complex types**
3. **Keep hashtable in sync with raw storage**
4. **Use proper buffer sizes and validation**
4. **Only use StringStream in tests**
5. **Switch StringStream to input mode before deserialization**
6. **Test serialization/deserialization round-trips**
7. **Use meaningful field names and versioning**
8. **Handle backward compatibility in parseValue**
9. **Validate input parameters in setters**
10. **Use stack allocation and avoid dynamic memory**

These patterns will help ensure reliable, efficient, and maintainable StreamableDTO implementations.
