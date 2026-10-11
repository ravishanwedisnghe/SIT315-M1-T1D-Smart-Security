// =========================================================
// PIN DEFINITIONS
// =========================================================

const byte DOOR_PIN = 8;
const byte WINDOW_PIN = 9;
const byte MOTION_PIN = 10;

const byte GREEN_LED_PIN = 4;
const byte RED_LED_PIN = 5;
const byte BUZZER_PIN = 6;


// =========================================================
// SYSTEM STATES
// =========================================================

const byte SAFE_STATE = 0;
const byte WARNING_STATE = 1;
const byte ALARM_STATE = 2;

byte systemState = SAFE_STATE;


// =========================================================
// VARIABLES SHARED WITH INTERRUPTS
// =========================================================

volatile bool pciEventFlag = false;
volatile bool timerEventFlag = false;

volatile unsigned long systemSeconds = 0;


// =========================================================
// SENSOR STATES
// =========================================================

bool doorActive = false;
bool windowActive = false;
bool motionActive = false;

bool previousDoorState = false;
bool previousWindowState = false;
bool previousMotionState = false;


// =========================================================
// EVENT MEMORY
// =========================================================

unsigned long lastDoorEvent = 0;
unsigned long lastWindowEvent = 0;
unsigned long lastMotionEvent = 0;

bool doorEventStored = false;
bool windowEventStored = false;
bool motionEventStored = false;


// The system remembers sensor events for 3 seconds
const unsigned long eventMemoryTime = 3;


// Alarm remains active for 5 seconds
const unsigned long alarmHoldTime = 5;

unsigned long alarmStartTime = 0;
bool alarmLatched = false;


// =========================================================
// DEBOUNCE VARIABLES
// =========================================================

unsigned long lastPCIProcessingTime = 0;

const unsigned long debounceDelay = 100;


// =========================================================
// TIMER LED VARIABLE
// =========================================================

bool heartbeatState = false;


// =========================================================
// FUNCTION DECLARATIONS
// =========================================================

void configurePins();
void configurePCI();
void configureTimer1();

void handlePCIEvent();
void handleTimerEvent();

void readSensors();
void detectSensorChanges();
void storeSensorEvent(byte sensorNumber);

void removeExpiredEvents();
void processSystemLogic();
void updateOutputs();

void printSensorEvent(
  const char* sensorName,
  const char* eventMessage
);

void printTimerMessage();
void printSystemStatus();


// =========================================================
// SETUP
// =========================================================

void setup()
{
  Serial.begin(9600);

  configurePins();
  configurePCI();
  configureTimer1();

  readSensors();

  previousDoorState = doorActive;
  previousWindowState = windowActive;
  previousMotionState = motionActive;

  processSystemLogic();
  updateOutputs();

  Serial.println("========================================");
  Serial.println("SMART SECURITY SYSTEM VERSION 2");
  Serial.println("DISTINCTION: PCI + TIMER1");
  Serial.println("========================================");

  printSystemStatus();
}


// =========================================================
// MAIN LOOP
// =========================================================

void loop()
{
  handlePCIEvent();
  handleTimerEvent();

  removeExpiredEvents();
  processSystemLogic();
  updateOutputs();
}


// =========================================================
// PIN CONFIGURATION
// =========================================================

void configurePins()
{
  /*
    Internal pull-up resistors are used.

    Button not pressed = HIGH
    Button pressed     = LOW
  */

  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);
  pinMode(MOTION_PIN, INPUT_PULLUP);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}


// =========================================================
// PIN CHANGE INTERRUPT CONFIGURATION
// =========================================================

void configurePCI()
{
  /*
    Arduino Uno Pin Change Interrupt group:

    D8  = PCINT0
    D9  = PCINT1
    D10 = PCINT2

    D8, D9 and D10 are in the same PCI group.
  */

  noInterrupts();

  // Enable Pin Change Interrupt group 0
  PCICR |= B00000001;

  // Enable PCI on D8, D9 and D10
  PCMSK0 |= B00000111;

  // Clear an existing Pin Change Interrupt flag
  PCIFR |= B00000001;

  interrupts();
}


// =========================================================
// TIMER1 CONFIGURATION
// =========================================================

void configureTimer1()
{
  /*
    Arduino clock frequency = 16 MHz

    Prescaler = 1024

    16,000,000 / 1024 = 15,625 counts

    OCR1A = 15,624

    Timer interrupt occurs every one second.
  */

  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 15624;

  // CTC timer mode
  TCCR1B |= B00001000;

  // Prescaler = 1024
  TCCR1B |= B00000101;

  // Enable Timer1 Compare A Interrupt
  TIMSK1 |= B00000010;

  interrupts();
}


// =========================================================
// PIN CHANGE INTERRUPT SERVICE ROUTINE
// =========================================================

ISR(PCINT0_vect)
{
  /*
    The ISR is kept short.

    It does not use:
    - delay()
    - Serial.print()
    - Long calculations
  */

  pciEventFlag = true;
}


// =========================================================
// TIMER1 INTERRUPT SERVICE ROUTINE
// =========================================================

ISR(TIMER1_COMPA_vect)
{
  /*
    Timer ISR runs every one second.

    It only:
    - Increases the timer
    - Sets a flag
  */

  systemSeconds++;
  timerEventFlag = true;
}


// =========================================================
// HANDLE PCI EVENT
// =========================================================

void handlePCIEvent()
{
  bool eventCopy = false;

  noInterrupts();

  if (pciEventFlag)
  {
    pciEventFlag = false;
    eventCopy = true;
  }

  interrupts();

  if (!eventCopy)
  {
    return;
  }

  // Ignore very fast repeated events caused by button bounce
  if (millis() - lastPCIProcessingTime < debounceDelay)
  {
    return;
  }

  lastPCIProcessingTime = millis();

  readSensors();
  detectSensorChanges();

  processSystemLogic();
  updateOutputs();
}


// =========================================================
// HANDLE TIMER EVENT
// =========================================================

void handleTimerEvent()
{
  bool eventCopy = false;

  noInterrupts();

  if (timerEventFlag)
  {
    timerEventFlag = false;
    eventCopy = true;
  }

  interrupts();

  if (!eventCopy)
  {
    return;
  }

  // Timer-based periodic task
  heartbeatState = !heartbeatState;

  removeExpiredEvents();
  processSystemLogic();
  updateOutputs();

  printTimerMessage();
  printSystemStatus();
}


// =========================================================
// SENSE – READ SENSOR INPUTS
// =========================================================

void readSensors()
{
  doorActive =
    digitalRead(DOOR_PIN) == LOW;

  windowActive =
    digitalRead(WINDOW_PIN) == LOW;

  motionActive =
    digitalRead(MOTION_PIN) == LOW;
}


// =========================================================
// DETECT SENSOR CHANGES
// =========================================================

void detectSensorChanges()
{
  // Door state changed
  if (doorActive != previousDoorState)
  {
    if (doorActive)
    {
      printSensorEvent("Door", "OPEN");

      storeSensorEvent(1);
    }
    else
    {
      printSensorEvent("Door", "CLOSED");
    }

    previousDoorState = doorActive;
  }


  // Window state changed
  if (windowActive != previousWindowState)
  {
    if (windowActive)
    {
      printSensorEvent("Window", "OPEN");

      storeSensorEvent(2);
    }
    else
    {
      printSensorEvent("Window", "CLOSED");
    }

    previousWindowState = windowActive;
  }


  // Motion state changed
  if (motionActive != previousMotionState)
  {
    if (motionActive)
    {
      printSensorEvent("Motion", "DETECTED");

      storeSensorEvent(3);
    }
    else
    {
      printSensorEvent(
        "Motion",
        "NOT DETECTED"
      );
    }

    previousMotionState = motionActive;
  }
}


// =========================================================
// STORE SENSOR EVENT
// =========================================================

void storeSensorEvent(byte sensorNumber)
{
  unsigned long currentTime;

  noInterrupts();
  currentTime = systemSeconds;
  interrupts();

  if (sensorNumber == 1)
  {
    doorEventStored = true;
    lastDoorEvent = currentTime;
  }
  else if (sensorNumber == 2)
  {
    windowEventStored = true;
    lastWindowEvent = currentTime;
  }
  else if (sensorNumber == 3)
  {
    motionEventStored = true;
    lastMotionEvent = currentTime;
  }
}


// =========================================================
// REMOVE EXPIRED SENSOR EVENTS
// =========================================================

void removeExpiredEvents()
{
  unsigned long currentTime;

  noInterrupts();
  currentTime = systemSeconds;
  interrupts();


  if (
    doorEventStored &&
    currentTime - lastDoorEvent > eventMemoryTime
  )
  {
    doorEventStored = false;
  }


  if (
    windowEventStored &&
    currentTime - lastWindowEvent > eventMemoryTime
  )
  {
    windowEventStored = false;
  }


  if (
    motionEventStored &&
    currentTime - lastMotionEvent > eventMemoryTime
  )
  {
    motionEventStored = false;
  }
}


// =========================================================
// THINK – PROCESS SYSTEM LOGIC
// =========================================================

void processSystemLogic()
{
  unsigned long currentTime;

  noInterrupts();
  currentTime = systemSeconds;
  interrupts();


  /*
    Alarm condition:

    Motion event and Door event occur within
    the same 3-second event window.

    OR

    Motion event and Window event occur within
    the same 3-second event window.
  */

  bool securityThreat =
    motionEventStored &&
    (doorEventStored || windowEventStored);


  if (securityThreat && !alarmLatched)
  {
    alarmLatched = true;
    alarmStartTime = currentTime;

    Serial.println("========================================");
    Serial.println("ALARM TRIGGERED");
    Serial.println("Multiple security events detected");
    Serial.println("========================================");
  }


  // Keep the alarm active for 5 seconds
  if (alarmLatched)
  {
    if (currentTime - alarmStartTime >= alarmHoldTime)
    {
      alarmLatched = false;

      doorEventStored = false;
      windowEventStored = false;
      motionEventStored = false;

      Serial.println("Alarm period completed.");
      Serial.println("System returned to monitoring mode.");
    }
  }


  if (alarmLatched)
  {
    systemState = ALARM_STATE;
  }
  else if (
    doorEventStored ||
    windowEventStored ||
    motionEventStored ||
    doorActive ||
    windowActive ||
    motionActive
  )
  {
    systemState = WARNING_STATE;
  }
  else
  {
    systemState = SAFE_STATE;
  }
}


// =========================================================
// ACT – CONTROL OUTPUTS
// =========================================================

void updateOutputs()
{
  if (systemState == ALARM_STATE)
  {
    // Alarm mode
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (systemState == WARNING_STATE)
  {
    // Warning mode
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, heartbeatState);
    digitalWrite(BUZZER_PIN, LOW);
  }
  else
  {
    // Safe mode
    digitalWrite(GREEN_LED_PIN, heartbeatState);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}


// =========================================================
// PRINT PCI SENSOR EVENT
// =========================================================

void printSensorEvent(
  const char* sensorName,
  const char* eventMessage
)
{
  Serial.print("[PCI EVENT] ");
  Serial.print(sensorName);
  Serial.print(": ");
  Serial.println(eventMessage);
}


// =========================================================
// PRINT TIMER EVENT
// =========================================================

void printTimerMessage()
{
  unsigned long currentTime;

  noInterrupts();
  currentTime = systemSeconds;
  interrupts();

  Serial.print("[TIMER EVENT] ");
  Serial.print(currentTime);
  Serial.println(" second(s)");
}


// =========================================================
// PRINT SYSTEM STATUS
// =========================================================

void printSystemStatus()
{
  Serial.println("----------------------------------------");

  Serial.print("Door: ");

  if (doorActive)
  {
    Serial.println("OPEN");
  }
  else
  {
    Serial.println("CLOSED");
  }


  Serial.print("Window: ");

  if (windowActive)
  {
    Serial.println("OPEN");
  }
  else
  {
    Serial.println("CLOSED");
  }


  Serial.print("Motion: ");

  if (motionActive)
  {
    Serial.println("DETECTED");
  }
  else
  {
    Serial.println("NOT DETECTED");
  }


  Serial.print("Stored events: ");

  if (
    !doorEventStored &&
    !windowEventStored &&
    !motionEventStored
  )
  {
    Serial.println("NONE");
  }
  else
  {
    if (doorEventStored)
    {
      Serial.print("DOOR ");
    }

    if (windowEventStored)
    {
      Serial.print("WINDOW ");
    }

    if (motionEventStored)
    {
      Serial.print("MOTION ");
    }

    Serial.println();
  }


  Serial.print("System status: ");

  if (systemState == ALARM_STATE)
  {
    Serial.println("ALARM");
  }
  else if (systemState == WARNING_STATE)
  {
    Serial.println("WARNING");
  }
  else
  {
    Serial.println("SAFE");
  }

  Serial.println("----------------------------------------");
}