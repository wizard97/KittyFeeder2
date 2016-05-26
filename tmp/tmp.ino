#include "SoundPlayer.h"
SoundPlayer player(9, 10);
void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
//player.play(&SoundPlayer::boot);
}

void loop() {
  // put your main code here, to run repeatedly:
  player.click();
  delay(100);
}
