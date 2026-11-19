#define R1 PA0
#define Y1 PA1
#define G1 PA2

#define R2 PA3
#define Y2 PA4
#define G2 PA5

#define R3 PA6
#define Y3 PA7
#define G3 PB0

void setup() {
  pinMode(R1, OUTPUT);
  pinMode(Y1, OUTPUT);
  pinMode(G1, OUTPUT);

  pinMode(R2, OUTPUT);
  pinMode(Y2, OUTPUT);
  pinMode(G2, OUTPUT);

  pinMode(R3, OUTPUT);
  pinMode(Y3, OUTPUT);
  pinMode(G3, OUTPUT);

  // Start with all roads RED
  allRed();
}

void allRed() {
  digitalWrite(R1, HIGH);
  digitalWrite(Y1, LOW);
  digitalWrite(G1, LOW);

  digitalWrite(R2, HIGH);
  digitalWrite(Y2, LOW);
  digitalWrite(G2, LOW);

  digitalWrite(R3, HIGH);
  digitalWrite(Y3, LOW);
  digitalWrite(G3, LOW);
}

void loop() {
  // ROAD 1: RED -> YELLOW -> GREEN -> YELLOW -> RED
  allRed();
  delay(3000);

  digitalWrite(Y1, HIGH);
  digitalWrite(R1, LOW);
  delay(2000);

  digitalWrite(Y1, LOW);
  digitalWrite(G1, HIGH);
  delay(5000);

  digitalWrite(G1, LOW);
  digitalWrite(Y1, HIGH);
  delay(2000);

  digitalWrite(Y1, LOW);
  digitalWrite(R1, HIGH);

  // ROAD 2
  delay(1000);
  digitalWrite(R2, LOW);
  digitalWrite(Y2, HIGH);
  delay(2000);

  digitalWrite(Y2, LOW);
  digitalWrite(G2, HIGH);
  delay(5000);

  digitalWrite(G2, LOW);
  digitalWrite(Y2, HIGH);
  delay(2000);

  digitalWrite(Y2, LOW);
  digitalWrite(R2, HIGH);

  // ROAD 3
  delay(1000);
  digitalWrite(R3, LOW);
  digitalWrite(Y3, HIGH);
  delay(2000);

  digitalWrite(Y3, LOW);
  digitalWrite(G3, HIGH);
  delay(5000);

  digitalWrite(G3, LOW);
  digitalWrite(Y3, HIGH);
  delay(2000);

  digitalWrite(Y3, LOW);
  digitalWrite(R3, HIGH);

  delay(1000);
}
