# 1 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
# 2 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2
# 3 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2
# 4 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2
# 5 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2
# 6 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2
# 7 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2




# 12 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino" 2
SLIPEncodedSerial SLIPSerial(Serial);


/*******************************************************************************

   Macros and Constants

 ******************************************************************************/
# 28 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
// Change these values to switch which direction increase/decrease pan/tilt




// Use these values to make the encoder more coarse or fine.
// This controls the number of wheel "ticks" the device sends to the console
// for each tick of the encoder. 1 is the default and the most fine setting.
// Must be an integer.

// #define TILT_SCALE 10
const int TILT_SCALE = 10;
//has been moved from original to apply within OSC function rather than within the void loop.






const String HANDSHAKE_QUERY = "ETCOSC?";
const String HANDSHAKE_REPLY = "OK";



// Change these values to alter how long we wait before sending an OSC ping
// to see if Eos is still there, and then finally how long before we
// disconnect and show the splash screen
// Values are in milliseconds



/*******************************************************************************

   Local Types

 ******************************************************************************/
# 62 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
enum WHEEL_TYPE
{
    TILT,
    PAN,
    LEVEL
};
enum WHEEL_MODE
{
    COARSE,
    FINE
};

struct Encoder
{
    uint8_t pinA;
    uint8_t pinB;
    int pinAPrevious;
    int pinBPrevious;
    float pos;
    uint8_t direction;
};
struct Encoder panWheel;
struct Encoder tiltWheel;
struct Encoder levelWheel;

enum ConsoleType
{
    ConsoleNone,
    ConsoleEos,
    ConsoleCobalt,
    ConsoleColorSource
};

/*******************************************************************************

   Global Variables

 ******************************************************************************/
# 99 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
// initialize the library with the numbers of the interface pins

ConsoleType connectedToConsole = ConsoleNone;
unsigned long lastMessageRxTime = 0;
bool timeoutPingSent = false;

/*******************************************************************************

   Given an unknown OSC message we check to see if it's a handshake message.

   If it's a handshake we issue a subscribe, otherwise we begin route the OSC

   message to the appropriate function.



   Parameters:

    msg - The OSC message of unknown importance



   Return Value: void



 ******************************************************************************/
# 116 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
void parseOSCMessage(String &msg)
{
    // check to see if this is the handshake string
    if (msg.indexOf(HANDSHAKE_QUERY) != -1)
    {
        // handshake string found!
        SLIPSerial.beginPacket();
        SLIPSerial.write((const uint8_t *)HANDSHAKE_REPLY.c_str(), (size_t)HANDSHAKE_REPLY.length());
        SLIPSerial.endPacket();

        // An Eos would do nothing until subscribed
        // Let Eos know we want updates on some things
    }
}

/*******************************************************************************

   Initializes a given encoder struct to the requested parameters.



   Parameters:

    encoder - Pointer to the encoder we will be initializing

    pinA - Where the A pin is connected to the Arduino

    pinB - Where the B pin is connected to the Arduino

    direction - Determines if clockwise or counterclockwise is "forward"



   Return Value: void



 ******************************************************************************/
# 143 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
void initEncoder(struct Encoder *encoder, uint8_t pinA, uint8_t pinB, uint8_t direction)
{
    encoder->pinA = pinA;
    encoder->pinB = pinB;
    encoder->pos = 0;
    encoder->direction = direction;

    pinMode(pinA, 0x2);
    pinMode(pinB, 0x2);

    encoder->pinAPrevious = digitalRead(pinA);
    encoder->pinBPrevious = digitalRead(pinB);
}

/*******************************************************************************

   Checks if the encoder has moved by comparing the previous state of the pins

   with the current state. If they are different, we know there is movement.

   In the event of movement we update the current state of our pins.



   Parameters:

    encoder - Pointer to the encoder we will be checking for motion



   Return Value:

    encoderMotion - Returns the 0 if the encoder has not moved

                                1 for forward motion

                               -1 for reverse motion



 ******************************************************************************/
# 171 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
int8_t updateEncoder(struct Encoder *encoder)
{
    int8_t encoderMotion = 0;
    int pinACurrent = digitalRead(encoder->pinA);
    int pinBCurrent = digitalRead(encoder->pinB);

    // has the encoder moved at all?
    if (encoder->pinAPrevious != pinACurrent)
    {
        // Since it has moved, we must determine if the encoder has moved forwards or backwards
        encoderMotion = (encoder->pinAPrevious == encoder->pinBPrevious) ? -1 : 1;

        // If we are in reverse mode, flip the direction of the encoder motion
        if (encoder->direction == 1)
            encoderMotion = -encoderMotion;
    }
    encoder->pinAPrevious = pinACurrent;
    encoder->pinBPrevious = pinBCurrent;

    return encoderMotion;
}

/*******************************************************************************

   Sends a message to Eos informing them of a wheel movement.



   Parameters:

    type - the type of wheel that's moving (i.e. pan or tilt)

    ticks - the direction and intensity of the movement



   Return Value: void



 ******************************************************************************/
# 204 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
void sendOscMessage(const String &address, float value)
{
    OSCMessage msg(address.c_str());
    msg.add(value);
    SLIPSerial.beginPacket();
    msg.send(SLIPSerial);
    SLIPSerial.endPacket();
}

int everyOtherPan = 0;
int everyOtherTilt = 0;

unsigned long lastPan = 0;
unsigned long lastTilt = 0;

long lastPanVal = 0;
long lastTiltVal = 0;

unsigned long panDelta = 0;
unsigned long tiltDelta = 0;

bool levelToggled = false;

int panDir = 1;
int tiltDir = 1;

void sendEosWheelMove(WHEEL_TYPE type, float ticks)
{
    String wheelMsg("/eos/wheel");

    //   if (digitalRead(SHIFT_BTN) == LOW)
    //     wheelMsg.concat("/fine");
    //   else
    wheelMsg.concat("/fine");

    if (type == PAN)
    {
        wheelMsg.concat("/pan");

        panDelta = millis() - lastPan;

        ticks *= int(5000/panDelta);
        ticks = (ticks > 0 && ticks <= 25) ? 25 : ticks;
        ticks = (ticks >= -25 && ticks < 0) ? -25 : ticks;
        ticks *= panDir;

        if (((ticks - lastPanVal)>0?(ticks - lastPanVal):-(ticks - lastPanVal)) > 4000)
            ticks = 0;

        if (everyOtherPan == 1)
            lastPan = millis();

        everyOtherPan = (everyOtherPan == 1) ? 0 : 1;
        lastPanVal = ticks;
    }
    else if (type == TILT)
    {
        wheelMsg.concat("/tilt");

        tiltDelta = millis() - lastTilt;

        ticks *= int(5000/tiltDelta);
        ticks *= TILT_SCALE;
        ticks = (ticks > 0 && ticks <= 25) ? 25 : ticks;
        ticks = (ticks >= -25 && ticks < 0) ? -25 : ticks;
        ticks *= tiltDir;

        if (((ticks - lastTiltVal)>0?(ticks - lastTiltVal):-(ticks - lastTiltVal)) > (long)(4000L * (long)TILT_SCALE)) //4000*TILT_SCALE
            ticks = 0;

        if (everyOtherTilt == 1)
            lastTilt = millis();

        everyOtherTilt = (everyOtherTilt==1) ? 0 : 1;
        lastTiltVal = ticks;
    }
    else if (type == LEVEL) {
        if (levelToggled) {
            wheelMsg = "/eos/wheel/level";
            ticks *= 4;
        }
        else
            wheelMsg = "/eos/wheel/coarse/iris";
    }
    else
        // something has gone very wrong
        return;

    sendOscMessage(wheelMsg, ticks);
}

/******************************************************************************/

void sendWheelMove(WHEEL_TYPE type, float ticks)
{
    switch (connectedToConsole)
    {
    default:
    case ConsoleEos:
        sendEosWheelMove(type, ticks);
    }
}

/*******************************************************************************

   Here we setup our encoder, lcd, and various input devices. We also prepare

   to communicate OSC with Eos by setting up SLIPSerial. Once we are done with

   setup() we pass control over to loop() and never call setup() again.



   NOTE: This function is the entry function. This is where control over the

   Arduino is passed to us (the end user).



   Parameters: none



   Return Value: void



 ******************************************************************************/
# 320 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
void setup()
{
    SLIPSerial.begin(115200);
    // This is a hack around an Arduino bug. It was taken from the OSC library
    // examples




    while (!Serial)
        ;


    // This is necessary for reconnecting a device because it needs some time
    // for the serial port to open. The handshake message may have been sent
    // from the console before #lighthack was ready

    SLIPSerial.beginPacket();
    SLIPSerial.write((const uint8_t *)HANDSHAKE_REPLY.c_str(), (size_t)HANDSHAKE_REPLY.length());
    SLIPSerial.endPacket();

    initEncoder(&panWheel, 7, 6, 0);
    initEncoder(&tiltWheel, 5, 4, 0);
    initEncoder(&levelWheel, 3, 2, 0);

    //for the buttons on each encoder
    pinMode(A5, 0x0);
    pinMode(A4, 0x0);
    pinMode(A3, 0x0);
}

/*******************************************************************************

   Here we service, monitor, and otherwise control all our peripheral devices.

   First, we retrieve the status of our encoders and buttons and update Eos.

   Next, we check if there are any OSC messages for us.

   Finally, we update our display (if an update is necessary)



   NOTE: This function is our main loop and thus this function will be called

   repeatedly forever



   Parameters: none



   Return Value: void



 ******************************************************************************/
# 366 "/Users/matthewschneider/Desktop/EOSInterface/EOSInterface.ino"
unsigned long levelDebounce = 0;
int lastLevelReadVal = 0x0;

unsigned long panDebounce = 0;
int lastPanReadVal = 0x0;

unsigned long tiltDebounce = 0;
int lastTiltReadVal = 0x0;

void clickHandler() {

    if (digitalRead(A5) != lastLevelReadVal) {
        //button has been depressed

        if (digitalRead(A5)==0x1 && millis() - levelDebounce > 50) {
            //only run once
            levelToggled = !levelToggled;
            levelDebounce = millis();
        }

        lastLevelReadVal = digitalRead(A5);
    }

    if (digitalRead(A4) != lastTiltReadVal) {
        //button has been depressed

        if (digitalRead(A4)==0x1 && millis() - tiltDebounce > 50) {
            //only run once
            tiltDir *= -1;
            tiltDebounce = millis();
        }

        lastTiltReadVal = digitalRead(A4);
    }

    if (digitalRead(A3) != lastPanReadVal) {
        //button has been depressed

        if (digitalRead(A3)==0x1 && millis() - panDebounce > 50) {
            //only run once
            panDir *= -1;
            panDebounce = millis();
        }

        lastPanReadVal = digitalRead(A3);
    }
}

void loop()
{
    static String curMsg;
    int size;
    // get the updated state of each encoder
    int32_t panMotion = updateEncoder(&panWheel);
    int32_t tiltMotion = updateEncoder(&tiltWheel);
    int32_t levelMotion = updateEncoder(&levelWheel);

    clickHandler();

    // Scale the result by a scaling factor
    panMotion *= 1;
    //tiltMotion *= TILT_SCALE;
    //above moved to within later called function
    //levelMotion *= LEVEL_SCALE;

    // now update our wheels
    if (tiltMotion != 0)
        sendWheelMove(TILT, tiltMotion);

    if (panMotion != 0)
        sendWheelMove(PAN, panMotion);

    if (levelMotion != 0)
        sendWheelMove(LEVEL, levelMotion);

    // Then we check to see if any OSC commands have come from Eos
    // and update the display accordingly.
    size = SLIPSerial.available();
    if (size > 0)
    {
        // Fill the msg with all of the available bytes
        while (size--)
            curMsg += (char)(SLIPSerial.read());
    }
    if (SLIPSerial.endofPacket())
    {
        parseOSCMessage(curMsg);
        lastMessageRxTime = millis();
        // We only care about the ping if we haven't heard recently
        // Clear flag when we get any traffic
        timeoutPingSent = false;
        curMsg = String();
    }

    if (lastMessageRxTime > 0)
    {
        unsigned long diff = millis() - lastMessageRxTime;
        // We first check if it's been too long and we need to time out
        if (diff > 5000)
        {
            connectedToConsole = ConsoleNone;
            lastMessageRxTime = 0;
            timeoutPingSent = false;
        }

        // It could be the console is sitting idle. Send a ping once to
        //  double check that it's still there, but only once after 2.5s have passed
        if (!timeoutPingSent && diff > 2500)
        {
            OSCMessage ping("/eos/ping");
            ping.add("box1" "_hello"); // This way we know who is sending the ping
            SLIPSerial.beginPacket();
            ping.send(SLIPSerial);
            SLIPSerial.endPacket();
            timeoutPingSent = true;
        }
    }
}
