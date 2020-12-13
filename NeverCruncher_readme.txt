               NEVERCRUNCHER 1.0 Readme File

NeverCruncher explores paths through Neverball levels with
no knowledge of the level layout.

********************Basic Operation**********************

To crunch a level from the Linux command line go to the directory 
that holds the "Neverball.exe" executable file and write "-l" followed
the path to the level sol file of the level you want NeverCruncher to 
play. Like so; 

./neverball -l map-easy/coins.sol

The graphics will be disabled and the level will be crunched until the
goal condition is met. Pressing the escape key during crunching will 
pause and re-enable graphics so progress can be seen. When goal conditions
are met the terminal will display this message:

--------------GOAL achieved!!! -- writing log files.

Then the solution and statistics from the crunch will be written in the
"AI Data" folder under the name of the level. Running the level again
will replay with graphics the solution for the level. To recrunch the
level the existing solution folder must be removed or renamed. 

****************Setting Parameters***************************

Several parameters control the path of the ball in the NeverBall
level that will be found by the program. These can radically effect
the solution you will get and the time needed to crunch.

--Speed
The speed cap is set in ball/st_play.h by the variable MAXSPEED on 
line 50. This is usually a good idea as higher speeds can hamper 
necessary change of direction. This would not be true for levels 
that need a high speed (for a jump or a bounce) to succeed.

--Coin Optimisation
The desired number of coins per time variables are set in 
"CoinBasedRewindFn()" in the ball/st_play.c file. These control  
how hard the program will look for coins, how it will look and 
when it will temporarily give up. There are several functions 
that explore in different ways. Small changes to these variables 
can cause large changes in the time taken to crunch a level.

*************************Building with Linux******************

The program is a part of the NEVERBALL source code and can be 
compiled using the native makefile by typing "make" on the command
line.  

************************Warning*******************************

NeverCruncher can take up to ten hours to crunch harder levels,
and exploration is still limited. Raised bumps on the table, for
example, will be under-explored as there is no method to encourage
this. 



