#include <StreamableManager.h>
#include <StringStream.h>


void setup() {
  Serial.begin(9600);
  while (!Serial);

  String testString("a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\n");
  StringStream srcStream(testString);
  StringStream destStream;

  StreamableManager streamableMgr;

  /*
   * Create a state object that will be passed into the filter function
   */
  struct State {
    String keyToRemove = String();
    String valueRemoved = String();
    State(const String& keyToRemove): keyToRemove(keyToRemove) {};
  };

  // Remove lines with the key "c"
  State* state = new State("c");

  /*
   * Filter function parses the lines, filters out the line that has
   * the key "c" but stores its value in the "state" struct
   */
  auto filter = [](const String &line, StreamableManager::DestinationStream* dest, void* state) -> bool {
    State* s = static_cast<State*>(state);
    int separatorIndex = line.indexOf('=');
    String key = line.substring(0, separatorIndex);
    String value = line.substring(separatorIndex + 1);
    if (key.equals(s->keyToRemove)) {
      // Remove the key and save the value
      s->valueRemoved = value;
    } else {
      // Pass it through
      dest->println(line);
    }
    return true; // keep going
  };

  Serial.println("Streaming from srcStream:");
  Serial.println(testString);

  streamableMgr.pipe(&srcStream, &destStream, filter, state);

  Serial.println("Received by destStream:");
  Serial.println(destStream.getString());

  Serial.println("\nFilter removed key '" + state->keyToRemove + "' and captured value '" + state->valueRemoved + "'"); 
}

void loop() {}
