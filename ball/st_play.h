#ifndef ST_PLAY_H
#define ST_PLAY_H

int ball_AI_Controller( void);
int exitandstartRun(void);
int cmp (const void * , const void * );
int exploreOptions(float, float);
int evaluateOptions(void);
int writeEverything(void);
int file_exist (char *);
int setupFolder(void);
int writeMaster(void);
int writeTiltFile(void);//defunct
// int nearRewindFn(void);
// int midRewindFn(void);
// int farRewindFn(void);
int falloutCorrectionMethod(float, float, float);
int CoinBasedRewindFn(void);

int deleteHS(int);

//stats
int writePNV(void);
int writeFO(void);
int writeCorrectionLOG(void);

int Heuristic(float, float);

int getReverseDirection(int);
int leftPerpendicularDirection(int );
int rightPerpendicularDirection(int );

int find_prevalent_Tilt(float, float);

//the check for previous data method.
int checkfordata(void);

int readTiltFile(void);




float diff;
int NinetyHZCounter;

int replayInt;

// these I put in for the variable timespeed
#define interim 25
#define MAXSPEED 90000.2            // TWEAK ME
#define PI 3.14159265
// int state_being_explored;
// int T; // Time, which now only is set to multiples of the interim.
// int I; // iterator for timesteps
// int TOTALTIME;

#define gameSpeed 2.0
  // this can be any factor of 96, 1 2 3 4 6 8 12 24
//#define physicalfactor 1

int I;                  //initialized because exitplay is true
int step_being_explored;


float variableSpeed;

float stepSpeed;// one speed for stepping old steps and one for the one being explored
float calcSpeed;

extern struct state st_play_ready;

extern struct state st_play_set;
extern struct state st_play_loop;

extern struct state st_look;

#endif
