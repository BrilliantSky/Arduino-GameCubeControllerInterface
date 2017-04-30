#include <GameCube_Controller.h>

GameCubeController controller;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Initializing controller...");
  Serial.flush();
  if( controller.initialize() )
    Serial.println(" success!");
  else
    Serial.println(" failure!");
  Serial.flush();
}

void loop()
{
  if( controller.update() )
  {
    Serial.print("Start: ");
    Serial.println(controller.buttonStart(), DEC);
    Serial.print("A    : ");
    Serial.println(controller.buttonA(), DEC);
    Serial.print("B    : ");
    Serial.println(controller.buttonB(), DEC);
    Serial.print("X    : ");
    Serial.println(controller.buttonX(), DEC);
    Serial.print("Y    : ");
    Serial.println(controller.buttonY(), DEC);
    Serial.print("Z    : ");
    Serial.println(controller.buttonZ(), DEC);
    Serial.print("L    : ");
    Serial.println(controller.buttonL(), DEC);
    Serial.print("R    : ");
    Serial.println(controller.buttonR(), DEC);
    Serial.print("D    : ");
    if( controller.buttonUp() )
      Serial.print("UP");
    if( controller.buttonDown() )
      Serial.print("DOWN");
    if( controller.buttonLeft() )
      Serial.print("LEFT");
    if( controller.buttonRight() )
      Serial.print("RIGHT");
    Serial.println();
    Serial.print("Joystick X: ");
    Serial.println(controller.joystickX(), DEC);
    Serial.print("Joystick Y: ");
    Serial.println(controller.joystickY(), DEC);
    Serial.print("C-stick X:  ");
    Serial.println(controller.CstickX(), DEC);
    Serial.print("C-stick Y:  ");
    Serial.println(controller.CstickY(), DEC);
    Serial.print("Left:  ");
    Serial.println(controller.left(), DEC);
    Serial.print("Right: ");
    Serial.println(controller.right(), DEC);
    Serial.println();
    Serial.flush();
  }
  else
  {
    Serial.println("Error: Controller unplugged");
  }
  delay(1000);
}

