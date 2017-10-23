               NEVERCRUNCHER 1.0 Readme File

********************Basic Operation**********************

To crunch a level, set the relevant parameters and  navigate to the
directory that holds the executable and execute the level from command
line like so;


./neverball -l map-easy/coins.sol


where the path to the level is specified after the -l parameter.Then 
press enter and graphics will be disabled and the level will be 
crunched until the goal condition is met.Then the terminal will 
display this message:

--------------GOAL achieved!!! -- writing log files.



At which point the program will need to be exited and restarted by
repetition of the previous command. Graphics will then be enabled and
the solution to the level will be played in a loop. Statistics for 
that level will have been written to files in the AI_DATA folder
composed of the name of the level. Any subsequent attempts to crunch
that level will find that folder and not re-crunch the level unless
that folder has been renamed or deleted. Pressing Esc during 
crunching will pause the game and progress can be seen. 


****************Setting Parameters***************************

The speed cap is set in ball/st_play.h by the variable MAXSPEED on 
line 50 and should be set to a reasonable speed for the level.

The optimisation conditions for the level are set in the 
CoinBasedRewindFn() function in ball/st_play.c and should
be altered very carefully as they greatly effect the mechanic of
coin optimisation.

The number of times each condition is attempted is controlled by those 
conditonal if statements. These cause a large change in the time taken 
by a level and should not be increased too much.

*************************Building*****************************

The program is a part of the NEVERBALL code and can be compiled using
the native makefile by typing "make" on the command line in Linux. 


************************Warning*******************************

The program should not be applied to levels with raised platforms
or bumps without a change to the falloutcorrection mechanism in 
st_play.c. NeverCruncher may take a disproportionate time for 
some levels.

