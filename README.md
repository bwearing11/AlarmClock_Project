# AlarmClock_Project
## University project to design and construct a functioning alarm clock using a Tiva C Board

**Project Structure**
All functionality was provided through main.c for ease of use when marking.

- Subsystem setup provided through functions named setupX (ie. setup_ssi0)
- Clock functions provided through functions named clockX (ie. clock24Hour
- Functions named hackX are used to get around clunky equipment. (ie. hackPot provides the functionality for the potentiometer which had to be manually setup to allow for the correct values to be set through it)
- Interrupt handler functions are provided through functions named x_Handler (ie. GPIOB_Handler)


When this was fully implemented through a Tiva C Board and appropriate connections to other equipment, the alarm clock was fully functional (ie. Alarm went off, time correctly displayed, time was able to be set).
