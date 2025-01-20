#include <WiFi.h>
#include <ESP32Servo.h>

// Define motor control pins
#define MOTOR1_P1 27
#define MOTOR1_P2 26
#define ENABLE1 14
#define MOTOR2_P1 25
#define MOTOR2_P2 33
#define ENABLE2 32

// Define TCRT sensor pins
#define SENSOR_LEFT 34
#define SENSOR_RIGHT 35

// Define servo motor pin
#define SERVO_PIN 15

// Define stop pin
#define STOP_PIN 4

// Speed control (0-255 for PWM)
int motorSpeed = 240; // Adjust this to control speed

// Wi-Fi credentials
//give your ownSSID and password, using personal Hotspot for the ESP
const char* ssid = "";
const char* password = ""; 
WiFiServer server(8080);

// Global state for motor control
char command = 'S'; // Initial state is Stop
char to = '0';
// Servo object
Servo servo;
bool servoMoved = false; // To track if the servo has already moved

// Loop counters for commands
int leftTurnCounter = 0;
int rightTurnCounter = 0;
int uTurnCounter = 0;
const int maxLeftTurnLoops = 63;
const int maxRightTurnLoops = 63;
const int maxUTurnLoops = 63;
bool rfid = true;

void setup() {
  // Set motor pins as output
  pinMode(MOTOR1_P1, OUTPUT);
  pinMode(MOTOR1_P2, OUTPUT);
  pinMode(ENABLE1, OUTPUT);
  pinMode(MOTOR2_P1, OUTPUT);
  pinMode(MOTOR2_P2, OUTPUT);
  pinMode(ENABLE2, OUTPUT);

  // Set sensor pins as input
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);
  pinMode(STOP_PIN, INPUT);

  // Attach servo to its pin
  servo.attach(SERVO_PIN);

  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Check if STOP_PIN is HIGH
  if (digitalRead(STOP_PIN) == HIGH) {
    stopMotors();
    Serial.println("Stop signal received. Waiting for next instruction...");
    while (digitalRead(STOP_PIN) == HIGH) {
      delay(100); // Wait until STOP_PIN goes LOW
      command = 'S';
      rfid = false;
      if (to == '1') {
        command = 'L';
      }
      else if (to == '2'){
        command = 'R';
      }
      if (to == 'U'){
        command = 'U';
      }
    }
    Serial.println("Stop signal cleared. Resuming operation...");
  }

  // Handle Wi-Fi client connections
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected");
    while (client.connected()) {
      if (client.available()) {
        String message = client.readStringUntil('\n');
        Serial.print("Message received: ");
        Serial.println(message);

        if (message.length() > 0) {
          to = message[0]; // Update command based on client input
          command = 'F';
          if(to == 'U') command = 'U';
          if (to == 'S') command = 'S';
        }

        // Respond to client
        client.println("Command received: " + message);
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }

  // Execute the current command
  switch (command) {
    case 'F':
      moveForward();
      break;
    case 'B':
      moveBackward();
      break;
    case 'L':
      if (leftTurnCounter < maxLeftTurnLoops) {
        turnLeft();
        leftTurnCounter++;
      } else {
        command = 'F';
        leftTurnCounter = 0;
      }
      break;
    case 'R':
      if (rightTurnCounter < maxRightTurnLoops) {
        turnRight();
        rightTurnCounter++;
      } else {
        command = 'F';
        rightTurnCounter = 0;
      }
      break;
    case 'U':
      if (uTurnCounter < maxUTurnLoops) {
        performUTurn();
        uTurnCounter++;
      } else {
        command = 'F';
        uTurnCounter = 0;
      }
      break;
    case 'S':
    default:
      stopMotors();
      break;
  }

  delay(10); // 10ms motors on
  stopMotors(); // Turn motors off
  delay(100); // 100ms off
}

// Function to move forward along the line
void moveForward() {
  int leftSensorValue = digitalRead(SENSOR_LEFT);
  int rightSensorValue = digitalRead(SENSOR_RIGHT);

  if (leftSensorValue == HIGH && rightSensorValue == HIGH) {
    analogWrite(ENABLE1, motorSpeed);
    digitalWrite(MOTOR1_P1, HIGH);
    digitalWrite(MOTOR1_P2, LOW);
    analogWrite(ENABLE2, motorSpeed);
    digitalWrite(MOTOR2_P1, HIGH);
    digitalWrite(MOTOR2_P2, LOW);
  } else if (leftSensorValue == HIGH && rightSensorValue == LOW) {
    turnLeft();
  } else if (rightSensorValue == HIGH && leftSensorValue == LOW) {
    turnRight();
  } else {
    stopMotors();
  }
}

// Function to move backward along the line
void moveBackward() {
  int leftSensorValue = digitalRead(SENSOR_LEFT);
  int rightSensorValue = digitalRead(SENSOR_RIGHT);

  if (leftSensorValue == HIGH && rightSensorValue == HIGH) {
    analogWrite(ENABLE1, motorSpeed);
    digitalWrite(MOTOR1_P1, LOW);
    digitalWrite(MOTOR1_P2, HIGH);
    analogWrite(ENABLE2, motorSpeed);
    digitalWrite(MOTOR2_P1, LOW);
    digitalWrite(MOTOR2_P2, HIGH);
  } else if (leftSensorValue == HIGH && rightSensorValue == LOW) {
    turnLeft();
  } else if (rightSensorValue == HIGH && leftSensorValue == LOW) {
    turnRight();
  } else {
    stopMotors();
  }
}

// Function to turn left (ignores sensors)
void turnLeft() {
  analogWrite(ENABLE1, 0); // Stop left motor
  analogWrite(ENABLE2, motorSpeed);
  digitalWrite(MOTOR2_P1, HIGH);
  digitalWrite(MOTOR2_P2, LOW);
}

// Function to turn right (ignores sensors)
void turnRight() {
  analogWrite(ENABLE1, motorSpeed);
  digitalWrite(MOTOR1_P1, HIGH);
  digitalWrite(MOTOR1_P2, LOW);
  analogWrite(ENABLE2, 0); // Stop right motor
}

// Function to perform a U-turn
void performUTurn() {
  analogWrite(ENABLE1, motorSpeed);
  digitalWrite(MOTOR1_P1, HIGH);
  digitalWrite(MOTOR1_P2, LOW);
  analogWrite(ENABLE2, motorSpeed);
  digitalWrite(MOTOR2_P1, LOW);
  digitalWrite(MOTOR2_P2, HIGH);
}

// Function to stop motors
void stopMotors() {
  analogWrite(ENABLE1, 0);
  analogWrite(ENABLE2, 0);
}
