#ifndef Magazine_h
#define Magazine_h

/*
 * A pure virtual business object describing a magazine.
 * Note there's no mention of Streamables.
 */

class Magazine {

  public:
    virtual ~Magazine() {}; // Needed for destructor-chaining

    virtual String getName() = 0;
    virtual void setName(const String &name) = 0;
    virtual int getIssue() = 0;
    virtual void setIssue(int issue) = 0;
  
};


#endif
