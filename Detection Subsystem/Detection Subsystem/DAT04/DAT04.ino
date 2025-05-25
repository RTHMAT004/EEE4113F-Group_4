// Code for testing UART communication.

#define GPIO_PIN_1 1
#define GPIO_PIN_3 3

HardwareSerial interfaceSystem(0);
String last = "LD";

void setup() {
  
  interfaceSystem.begin(9600, SERIAL_8N1, GPIO_PIN_3, GPIO_PIN_1);
}

void loop() 
{
  if(last=="LD")
  {
    interfaceSystem.println(String("HB \n"));
    last="HB";
  }
  else if (last=="HB") 
  {
    interfaceSystem.println(String("CL \n"));
    last="CL";
  }
  else
  {
    interfaceSystem.println(String("LD \n"));
    last="LD";
  }
}