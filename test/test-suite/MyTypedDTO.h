#ifndef _tests_MyTypedDTO_h
#define _tests_MyTypedDTO_h


#include <StreamableDTO.h>

#define TYPE_ID 1
#define SERIAL_VERSION 4
#define MIN_COMPAT_VERSION 2

class MyTypedDTO: public StreamableDTO {

  public:
    MyTypedDTO() {};

    virtual int16_t getTypeId() override {
      return TYPE_ID;
    };
    virtual uint8_t getSerialVersion() override {
      return SERIAL_VERSION;
    };
    virtual uint8_t getMinCompatVersion() override {
      return MIN_COMPAT_VERSION;
    };

};


#endif
