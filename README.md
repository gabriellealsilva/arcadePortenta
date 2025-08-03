# Arcade Portenta
Personal project to try to replicate an arcade machine CPS-1 using bare-metal C++ on an Arduino Portenta H7(STM32H747XI)

The main goal is to build a retro arcade machine that can emulate the CPS-1 and run Street Fighter II at its native speed. The system will read and load a ROM from an SD card, and the players will interact with it using  JAMMA controllers. The output will be done using VGA for a CRT monitor and most likely I2S for the audio.
