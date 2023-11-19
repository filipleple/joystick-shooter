 float potLevel = 0;

void setup() {
  pinMode(A0, INPUT);
  pinMode(A2, INPUT); 
  Serial.begin(9600);

}

void loop() {
  potLevel = analogRead(A0);
  Serial.print("-x");
  Serial.print(potLevel);
  

  if(!analogRead(A2)){
    Serial.println("f");
  }
  
  //Serial.println(1234);
  delay(25);
}
