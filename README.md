# ICS_462
AI class

This is for the AI class in case I mess up and wanna go back to where I was prior.

Notes on issues surrounding assignment 1 so far.

1) Could not get working on laptop with windows. When trying to install the dependencies to create the executable, it would keep failing on random (regex, glew, sdl2, and one other). Hours wasted on this.

2) Was able to get game compiled and working on Ubuntu (windows was recommended, only had instructions for windows and mac on class website).

3) Instructions were not very clear and made it sound like bzfs and bzflag were the same thing, but they are different things. Bzfs is the server and bzflag is the game. Bzfs will not start without some acceleration configurations given to it, so originally it seemed like bzflag wasn't working and didn't compile correctly because when typing bzfs into the command line the game wouldn't start. Eventually tried typing in bzflag and the game started up, which made me realize the thing had compiled correctly.

4) The instructions on the class website said the sample.conf (or really any configuration file) needs to be in the folder with the bzfs executable. This is false. It needs to be in the same folder in which you start the server from on the command line. Many hours were wasted trying to figure out how to get the configuration file to work.

5) Also many hours were wasted trying to figure out how to get the .bzw world file to work/be read from how it was written in the configuration file and were the file was on the computer (compounded probably by the issue with where we assumed .conf file should have been. You can have it listed in the .conf file like "-world empty.bzw" if the file is in the same place as the .conf file, which should be where your starting your server.

6) When modifying the code and compiling it, more hours were wasted trying to figure out why the game wasn't running any differently after modifying the code and it compiling successfully. Testing was done first by removing successively more and more functionality from the robot till it should have just sat it in place but the robot would still hunt the player down and shoot at it like normal, even when the functions in the RobotPlayer.cxx were basically empty. Eventually decided to try and uninstall the server and game and recompile and reinstall it. I noticed something was wrong when after uninstalling the server and program, both will still start when bzfs or bzflag was typed into the terminal. Tried using gui application manager but it didn't show either program. Looked at the path to find where the binary executables for programs would be stored and found the bzfs and bzflag executable. Moved them from their location to elsewhere and tried running each program from the command line again and success, programs didn't work and terminal suggested installing with sudo apt-get. Looked at the properties of the executables and noticed none had been changed in the past couple days, which shouldn't have been the case cause we had been successfully creating the program dozens of times. Turns out if you just type "make" instead of "make install", the binary executable won't be changed. It's probabably just changing the executable in the bzflag folder, but the executable the terminal calls to isn't being changed.
