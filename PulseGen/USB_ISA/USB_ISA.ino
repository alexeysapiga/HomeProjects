void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  String stringOne = "Info from NMR Pulse Gen";
  
  Serial.println(stringOne); 
  delay(300);
}
