/*
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

/*All NeverCruncher logic is contained in this file. It is prefixed by "//JimK" */


#include "gui.h"
#include "hud.h"
#include "demo.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "cmd.h"

#include "game_common.h"
#include "game_server.h"
#include "game_proxy.h"
#include "game_client.h"

#include "st_play.h"
#include "st_goal.h"
#include "st_fail.h"
#include "st_pause.h"
#include "st_level.h"
#include "st_shared.h"

//Jim necessary lilbrary for getcwd command.
#include <unistd.h>

//JimK necessary Neverball headers.
#include "progress.h"
#include "solid_all.h"
#include "level.h"
#include "game_client.h"

//JimK- NeverCruncher globals-------------------------------------------------------------------------------------

bool exitPlay = true;             // A bool to exit the run

int runCounter=0;                 // the count of each run, used for statistics setup is done the first run
bool spanBool = false;  // this was an idea I had to evaluate only after a span had elpsed, it was abandoned and I intended to 
                        // remove it from the code.

int exploration_option_choice=0;    // choice of nine options that can be chosen: N NE E SE S SW W NW and <no tilt>.


int optionArrayPosition=0;  // int indexing the array of previous direction options.

float X=0;  // position of ball in level.
float Z=0;


float options_tried[9][6];  // a 2d array of options chosen and properties.This is filled as each step is evaluated
float options_chosen[3000][4];  // a larger array holding options chosen and properties for all steps decided on in the level.

char direction[4]= "nul";   //a char array for directions chosen
float randmax = RAND_MAX;
float reward=0;
int COINS;


int nearRewindCounter[3000];    //ignore
int midRewindCounter[3000];
int farRewindCounter[3000];

int coins_at_step[3000];    //the number of coins at every step.

//base path for write functions
char total_path[200];
int steps_since_coin=0;     // the number of (25* 1/90 second) steps since the last coin

//JK Huge ARRAY
int hateStates[3000][9];    // these are the hatestates, nine columns for each step recorded, if that column has a one in it 
                            // then that option(1-9) is skipped when the state is re-explored

int old_options[3000];  // the recorded chosen option for each state.


bool fallpossible = false;  // a bool that allows only one fallout to be detected.

int TspanRewindCounter[3000][20];   // for each of the (about 20) optimisations the number of times the statement has been tried is counted.
int S=1;                    // ignore this, it should always be one.


//stats stuff
float Xpos, Zpos;   
float XposGrid, ZposGrid;   // the girdpos is what square the coords are in.
float PNV[800000][4];   //this array records position and velocity for all steps, 800 000 is a guess at how many that would be.
int step;               // just an int for recording the above.

float FO[100000][3]; // number of fallouts, 100000 is safe enough
int fallouts;

float HeuristicPositions[3000][2];  //just a reocrd of positions throughout the level.

bool ninthOPT=false;    // this bool is necessary in the case that the ninth option is skipped by a hatestate and the state being explored must be changed.

int failedHeuristicCounter;     // number of times the ball is in the same square for consecutive steps. ** This could be used to improve on exploration **

int steps_since_coinAtgivenstep[3000];  //the steps since a coin was collected at each step.

//int steps_since_fallout;                // not useful really unless you want to count fallout frequency
//int steps_since_falloutAtGivenStep[3000];

bool deccelerating_evaluate_anyway = false;     // this is a bool that allows the tilt to be set contrary to the velocity and fixes it so it is evaluated and exploration options are not tried. 

int option_locked_for_steps=0;          // the number of steps the option is locked for, in Direction Based Optimisation.


int LockRecord =0;  // a counter of how many steps to lock direction for.
bool unlock4onestep=false;  // this allows a random step in the direction based optimisation
bool decrement_lockstep=false;  //bool allows the number of steps the direction is locked for to change.
int savedopt = 0;   // saves the direction that is locked.

float correctionLOG[8000][5];   // the number of times the optimisation is exectuted.
int numberofcorrections = 0;    // should be named number of optimisations. I called them corrections also

bool global_NBO= false; //this bools allows noise based optimisation statements in the optimisation method.
// It means that fallouts are ignored so must be set false by the optimisation when not being used.

/*---------------------------------------------------------------------------*/

static void set_camera(int c)
{
    config_set_d(CONFIG_CAMERA, c);
    hud_cam_pulse(c);
}

static void toggle_camera(void)
{
    int cam = (config_tst_d(CONFIG_CAMERA, CAM_3) ?
               CAM_1 : CAM_3);

    set_camera(cam);
}

static void next_camera(void)
{
    int cam = config_get_d(CONFIG_CAMERA) + 1;

    if (cam <= CAM_NONE || cam >= CAM_MAX)
        cam = CAM_1;

    set_camera(cam);
}

static void keybd_camera(int c)
{
    if (config_tst_d(CONFIG_KEY_CAMERA_1, c))
        set_camera(CAM_1);
    if (config_tst_d(CONFIG_KEY_CAMERA_2, c))
        set_camera(CAM_2);
    if (config_tst_d(CONFIG_KEY_CAMERA_3, c))
        set_camera(CAM_3);

    if (config_tst_d(CONFIG_KEY_CAMERA_TOGGLE, c))
        toggle_camera();
}

static void click_camera(int b)
{
    if (config_tst_d(CONFIG_MOUSE_CAMERA_1, b))
        set_camera(CAM_1);
    if (config_tst_d(CONFIG_MOUSE_CAMERA_2, b))
        set_camera(CAM_2);
    if (config_tst_d(CONFIG_MOUSE_CAMERA_3, b))
        set_camera(CAM_3);

    if (config_tst_d(CONFIG_MOUSE_CAMERA_TOGGLE, b))
        toggle_camera();
}

static void buttn_camera(int b)
{
    if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
        next_camera();
}

/*---------------------------------------------------------------------------*/

static int play_ready_gui(void)
{
    int id;

    if ((id = gui_label(0, _("Ready?"), GUI_LRG, 0, 0)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int play_ready_enter(struct state *st, struct state *prev)
{
    audio_play(AUD_READY, 1.0f);
    video_set_grab(1);

    hud_cam_pulse(config_get_d(CONFIG_CAMERA));

    return play_ready_gui();
}

static void play_ready_paint(int id, float t)
{
    game_client_draw(0, t);
    hud_cam_paint();
    gui_paint(id);
}

static void play_ready_timer(int id, float dt)
{
    float t = time_state();

    game_client_fly(1.0f - 0.5f * t);

    if (dt > 0.0f && t > 1.0f)
        goto_state(&st_play_set);

    game_step_fade(dt);
    hud_cam_timer(dt);
    gui_timer(id, dt);
}

static int play_ready_click(int b, int d)
{
    if (d)
    {
        click_camera(b);

        if (b == SDL_BUTTON_LEFT)
            goto_state(&st_play_loop);
    }
    return 1;
}

static int play_ready_keybd(int c, int d)
{
    if (d)
    {
        keybd_camera(c);

        if (c == KEY_EXIT)
            goto_state(&st_pause);
    }
    return 1;
}

static int play_ready_buttn(int b, int d)
{
    if (d)
    {
        buttn_camera(b);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_play_loop);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return goto_state(&st_pause);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int play_set_gui(void)
{
    int id;

    if ((id = gui_label(0, _("Set?"), GUI_LRG, 0, 0)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int play_set_enter(struct state *st, struct state *prev)
{
    audio_play(AUD_SET, 1.f);

    return play_set_gui();
}

static void play_set_paint(int id, float t)
{
    game_client_draw(0, t);
    hud_cam_paint();
    gui_paint(id);
}

static void play_set_timer(int id, float dt)
{
    float t = time_state();

    game_client_fly(0.5f - 0.5f * t);

    if (dt > 0.0f && t > 1.0f)
        goto_state(&st_play_loop);

    game_step_fade(dt);
    hud_cam_timer(dt);
    gui_timer(id, dt);
}

static int play_set_click(int b, int d)
{
    if (d)
    {
        click_camera(b);

        if (b == SDL_BUTTON_LEFT)
            goto_state(&st_play_loop);
    }
    return 1;
}

static int play_set_keybd(int c, int d)
{
    if (d)
    {
        keybd_camera(c);

        if (c == KEY_EXIT)
            goto_state(&st_pause);
    }
    return 1;
}

static int play_set_buttn(int b, int d)
{
    if (d)
    {
        buttn_camera(b);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_play_loop);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return goto_state(&st_pause);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    ROT_NONE = 0,
    ROT_ROTATE,
    ROT_HOLD
};

#define DIR_R 0x1
#define DIR_L 0x2

static int   rot_dir;
static float rot_val;

static void rot_init(void)
{
    rot_val = 0.0f;
    rot_dir = 0;
}

static void rot_set(int dir, float value, int exclusive)
{
    rot_val = value;

    if (exclusive)
        rot_dir  = dir;
    else
        rot_dir |= dir;
}

static void rot_clr(int dir)
{
    rot_dir &= ~dir;
}

static int rot_get(float *v)
{
    *v = 0.0f;

    if ((rot_dir & DIR_R) && (rot_dir & DIR_L))
    {
        return ROT_HOLD;
    }
    else if (rot_dir & DIR_R)
    {
        *v = +rot_val;
        return ROT_ROTATE;
    }
    else if (rot_dir & DIR_L)
    {
        *v = -rot_val;
        return ROT_ROTATE;
    }
    return ROT_NONE;
}

/*---------------------------------------------------------------------------*/





static int fast_rotate;
static int show_hud;

//This is the beginning of the NeverCruncher Code.
int ball_AI_Controller(){

    bool doneonceforexploration = true;     // These bools allow the exploration and evaluation to be done once.
    bool doneonceforevaluation= true;

    if(exitPlay==true){                     // This flag calls the method to reset variables for a new run. Or to setup the first run.
        exitandstartRun();
    }

   



    COINS = returns_coins_collected();
    float Xvel =returns_ball_X_velocity();
    float Yvel =returns_ball_Y_velocity();
    float Zvel =returns_ball_Z_velocity(); 

    //returns_timeout_state()==1

    if(returns_timeout_state()==1 && replayInt==0){ //set the goal here. When the goal is reached we write to file and loop.
                                                    // the program often crashes at this point but the data is written successfully and so can be restarted.
        writeEverything();
        exitPlay=true;
        return 0;
    }


    if(fallpossible == true ){                      // should test for fallout once a run, and marks hatestates with the falloutCorrectionMethod.
        if(falloutCorrectionMethod(Xvel, Yvel, Zvel)==1){
            fallpossible = false;
            exitPlay = true;
            return 0;
        }
    }else{
            fallpossible = true;
    }


    // above this is called every frame. but this increments the step every 24*1/90 seconds
    if( NinetyHZCounter== 24){
            I++;
            NinetyHZCounter=0;  
            doneonceforexploration = false;   // if code has been done once set true.
            doneonceforevaluation = false;
            
    }


    // game stats 
    Xpos=returns_ball_X_position();     
    Zpos=returns_ball_Z_position();
    XposGrid = Xpos-fmodf(Xpos, 1.0); 
    ZposGrid = Zpos-fmodf(Zpos, 1.0);
   


    if(I<step_being_explored){              // reads tilt from array if not step being explored.
        X=options_chosen[I][0];
        Z=options_chosen[I][1];
        
    }

    if(I == step_being_explored+1 && replayInt==1 ){ // this should make a loop when goal is achieved
        exitPlay = true;
        return 0;
    }

    if(I == step_being_explored && doneonceforexploration==false){ // calls the exploration method.
        
        exploreOptions(Xvel, Zvel);

        doneonceforexploration = true;
    }

    
    
    if(I == step_being_explored+S && doneonceforevaluation == false){   // calls the evaluation one step later.
        evaluateOptions();
        doneonceforevaluation = true;
        exitPlay=true;
    }



    game_set_x(X);      // sets the tilts.
    game_set_z(Z);

    return 0;
}


int falloutCorrectionMethod(float Xvel, float Yvel, float Zvel){    // rewinds step_being_explored a number of steps based on ball speed magnitude.
float magSQ = Xvel*Xvel + Zvel*Zvel;

    if(returns_fall_state()==1 && option_locked_for_steps==0 && global_NBO == false ){     // the second argument is so fallouts are not tested for during direction based optimisation.
        int N=0;
        if (magSQ < 4.5){       // these numbers are guesswork, and are probably overestimates but were found to work.
            N=2;
        }
        else if (magSQ>4.5 && magSQ<12){
            N=3;
        }
        else if (magSQ>12 && magSQ<22){
            N=4;
        }
        else if (magSQ>22 && magSQ<35){
            N=5;
        }
        else{
            N=6;
        }
        // sets the array value that corresponds to the old option at the old time to be one.
        hateStates[ step_being_explored-N  ][  old_options[step_being_explored-N]   ] =1; 

        FO[fallouts][0] = Xpos;     // just a record of fallouts for the stats.
        FO[fallouts][1] = Zpos;
        FO[fallouts][2] = magSQ;
        fallouts++;


        fprintf(stdout, "xxxxxxxxxxx --- fallout from state %d.    Rewinding %d steps bad Option:--------     %d\n",
          step_being_explored-N, N,old_options[step_being_explored-N] );


        int hs_counter=0;
        for (int i=0; i<9; i++){
            if (hateStates[step_being_explored-N][i] == 1){ // check for full rows of hatestate array, in which case all options have been blocked.
                hs_counter++;
            }
        }

        if(hs_counter==9){
            for (int i=0; i<9; i++){
                    hateStates[step_being_explored-N][i]=0; // wipe array if full.                    
            }
              
              fprintf(stdout, 
              "\n\n\n\n\n<<<<<<<<<<<  hateStates full, rewinding an extra 2 steps  >>>>>>>>>>>>>>> \n\n\n\n\n");
              
              N+=2;  // and jump back TWO extra steps.
        }

        

        if((step_being_explored-N)<= 1){        // ensures doesn't rewind past the start of the level.
            step_being_explored = 1;
        }else{
            step_being_explored-=(N);
        }

        // The steps since a coin is set to the earlier value. 
        // This I cannot remember doing, if the state being rewound to is MORE steps since a coin than the current step then zero the steps since coin?
        

        // if there were less states since the coin that far back then reset steps_since_coin 
        //if(steps_since_coin > (steps_since_coinAtgivenstep[step_being_explored]-1)){
        steps_since_coin = steps_since_coinAtgivenstep[step_being_explored]-1;    // this sets the steps since coin to the relevant value.
        // }else{
        //     steps_since_coin = 0;   // zero coins since exploration if rewinding to a worse state.
        // }

        exploration_option_choice=0; // zero option choice for rewind.
        //steps_since_fallout=0;  // zero steps_since_fallout
        optionArrayPosition = 0;
        
        return 1;
    }
    
    return 0;
}


int find_prevalent_Tilt(float Xv, float Zv){    // this finds what quadrant of 2PI the velocity angle is in.
    int tilt=0;
    double angle = atan2(-Zv,Xv);
    if(angle<0){
        angle += 2*PI;          // negative values are mapped to real ones
    }
    double division = PI/8;
    if(angle<=division || angle>=division*15){
        tilt = 7;
    }
    else if(angle>division && angle<= division*3){
        tilt = 6;
    }
    else if(angle> division*3 && angle<= division*5){
        tilt = 5;
    }
    else if(angle> division*5 && angle<= division*7){
        tilt = 4;
    }
    else if(angle> division*7 && angle<= division*9){
        tilt = 3;
    }
    else if(angle> division*9 && angle<= division*11){
        tilt = 2;
    }
    else if(angle> division*11 && angle<= division*13){
        tilt = 1;
    }
    else if(angle> division*13 && angle< division*15){
        tilt = 8;
    }


    return tilt;
}



int getReverseDirection(int input){         // finds the corresponding quadrant that is contrary to the balls vector.

    //there is a mathsy way to do this, I haven't the time. 
    int output =0;
    switch (input){
        case 0:
            output = 0;
            break;
        case 1:
            output = 5;
            break;
        case 2:
            output = 6;
            break;
        case 3:
            output = 7;
            break;
        case 4:
            output = 8;
            break;
        case 5:
            output = 1;
            break;
        case 6:
            output = 2;
            break;
        case 7:
            output = 3;
            break;
        case 8:
            output = 4;
        
    }
    return output;
}

int leftPerpendicularDirection(int input){      // finds the quadrant left perpendicular to the balls vector.

    int output =0;
    switch (input){
        case 0:
            output = 0;
            break;
        case 1:
            output = 7;
            break;
        case 2:
            output = 8;
            break;
        case 3:
            output = 1;
            break;
        case 4:
            output = 2;
            break;
        case 5:
            output = 3;
            break;
        case 6:
            output = 4;
            break;
        case 7:
            output = 5;
            break;
        case 8:
            output = 6;
        
    }
    return output;
}


int rightPerpendicularDirection(int input){     // finds the quadrant right perpendicular to the balls vector.

    int output =0;
    switch (input){
        case 0:
            output = 0;
            break;
        case 1:
            output = 3;
            break;
        case 2:
            output = 4;
            break;
        case 3:
            output = 5;
            break;
        case 4:
            output = 6;
            break;
        case 5:
            output = 7;
            break;
        case 6:
            output = 8;
            break;
        case 7:
            output = 1;
            break;
        case 8:
            output = 2;
        
    }
    return output;
}

int iterateNumbers[12] = {9,9,9,9,9,9,9,9,9,9,9,9};     // the repetitions of each statement in the OPTIMIZATION
// these should be a multiple of eight plus one for DBO, for NBO they can be anything
// These slow the program greatly. I see no good reason to do more than 9, 17 max.





int CoinBasedRewindFn(){        // this is the direction based optimisation with bools that do noise based optimisation.

        // These statements, if triggered set the direction for a number of steps in the hope that they will not be triggered if the
        // ball follows these directions and collects coins so that the statement is no longer true. If the statement is true enough
        // times the rewind counter will fill and be equal to the iterateNumbers value and this will make the statement untrue.

        
        int optimisation_arrayNo=0;        // the statement number that is executed. This tells us which counter to increment.                 

        // For example the if statement below checks first the counter is less than the iterateNumbers value and
        // the coin change in one step is less than one coin.



        



        if      (TspanRewindCounter[step_being_explored][0]<iterateNumbers[0] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-1]<1 && steps_since_coin==1){
            optimisation_arrayNo=0; 

            global_NBO = false;//this bools allows noise based optimisation statements in this method.
        }
        
        else if      (TspanRewindCounter[step_being_explored][1]<iterateNumbers[1] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-2]<2 && steps_since_coin==2){
            optimisation_arrayNo=1;

            global_NBO = false; 

        }

        else if      (TspanRewindCounter[step_being_explored][2]<iterateNumbers[2] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-2]<1 && steps_since_coin==2){
            optimisation_arrayNo=2; 

            global_NBO = false;
        }

        else if      (TspanRewindCounter[step_being_explored][3]<iterateNumbers[3] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-3]<2 && steps_since_coin==3){
            optimisation_arrayNo=3; 

            global_NBO = false;
        }

        else if      (TspanRewindCounter[step_being_explored][4]<iterateNumbers[4] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-3]<1 && steps_since_coin==3){
            optimisation_arrayNo=4; 

            global_NBO = false;
        }

        else if      (TspanRewindCounter[step_being_explored][5]<iterateNumbers[5] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-4]<2 && steps_since_coin==4){
            optimisation_arrayNo=5; 

            global_NBO = true;
        }

        else if      (TspanRewindCounter[step_being_explored][6]<iterateNumbers[6] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-4]<1 && steps_since_coin==4){
            optimisation_arrayNo=6; 

            global_NBO = true;
        }

         else if      (TspanRewindCounter[step_being_explored][7]<iterateNumbers[7] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-6]<2 && steps_since_coin==6){
            optimisation_arrayNo=7; 

            global_NBO = false;
        }

        else if      (TspanRewindCounter[step_being_explored][8]<iterateNumbers[8] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-6]<1 && steps_since_coin==6){
            optimisation_arrayNo=8; 

            global_NBO = false;
        }

        else if      (TspanRewindCounter[step_being_explored][9]<iterateNumbers[9] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-10]<1 && steps_since_coin==10){
            optimisation_arrayNo=9; 

            global_NBO = true;
        }

        else if      (TspanRewindCounter[step_being_explored][10]<iterateNumbers[10] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-18]<2 && steps_since_coin==18){
            optimisation_arrayNo=10; 

            global_NBO = true;
        }

        else if      (TspanRewindCounter[step_being_explored][11]<iterateNumbers[11] && 
        coins_at_step[step_being_explored]-coins_at_step[step_being_explored-18]<1 && steps_since_coin==18){
            optimisation_arrayNo=11; 
            global_NBO = true;
        }

        else{       // if this executes the optimisation has finished till statements become true again, ie steps since 
                    // coin changes, ie a coin found.
            global_NBO = false;
            return 0;
        }

      
        // the relevant counter is incremented.
        TspanRewindCounter[step_being_explored][optimisation_arrayNo]++;

        numberofcorrections++;          // stats for correction/optimisation (I call this both)
        correctionLOG[numberofcorrections][0]= numberofcorrections;
        correctionLOG[numberofcorrections][1]= Xpos;
        correctionLOG[numberofcorrections][2]= Zpos;
        correctionLOG[numberofcorrections][3]= steps_since_coin;
        correctionLOG[numberofcorrections][4]= TspanRewindCounter[step_being_explored][optimisation_arrayNo];


        

        // here I am setting the option to be locked  DBO
        savedopt = TspanRewindCounter[step_being_explored][optimisation_arrayNo];


        //steps_since_coin-=1;   //ignore last step
        deleteHS(steps_since_coin); // this is important, hatestates are wiped for the states being optimised, so all options are explored.


        // the last optimisation must return the ball to the start without locking the step.
        if(iterateNumbers[optimisation_arrayNo]== TspanRewindCounter[step_being_explored][optimisation_arrayNo]){
            global_NBO = false;// so if the counter is filled if it is NBO the bool is set to false.

            // this will equal zero on the last optimisation.
            exploration_option_choice = 0;
            option_locked_for_steps=0;
            
            fprintf(stdout, "\n\nOptimisation Unsuccessful. Rewinding to stem\n\n ");
        }
        else if(global_NBO==true){
            
             
            exploration_option_choice = 0;
            option_locked_for_steps=0;
            
            fprintf(stdout, "\n\nNBO -- Rewinding %d steps. \n\n ", steps_since_coin);

        }
        else{

            option_locked_for_steps=steps_since_coin;
            LockRecord = option_locked_for_steps; 
            fprintf(stdout, "\n\nDBO -- Rewind Locking option for %d steps\n\n", option_locked_for_steps);
        }
         
        fprintf(stdout, "\nTest statement 1");


        if((step_being_explored-steps_since_coin)<1){   // rewinding past the start.
            step_being_explored = 1;
            //steps_since_fallout = 0;    // zero steps since fallout for a max rewind.
            
        }
        else{
            step_being_explored -=(steps_since_coin);   // this rewinds the optimisation. 

            //steps_since_fallout = steps_since_falloutAtGivenStep[step_being_explored]; // set steps since fallout to be the last value
            // dont have to worry this is set too high by a rewind, logic holds

            if(spanBool==true){      // this span variable has been abandoned
                S=steps_since_coin;  
            }
        }

        // here I am making the option in the range of O..8   
        while(savedopt>8){
            savedopt-=8;
        }

        fprintf(stdout, "\nTest statement 2");
        
       
        // setting the steps since last coin
        steps_since_coin = steps_since_coinAtgivenstep[step_being_explored];


        return 1;
}

    


//wiping the hatestates
int deleteHS(int rewinding){

    for(int i=0; i<rewinding; i++){
        for(int j=0; j<9; j++){
            hateStates[step_being_explored-i][j]=0;
        }
    }
    return 0;
}


//this is rubbish from the noise based optimisation.
int farRewindFn(){
    if(farRewindCounter[step_being_explored]<6 && steps_since_coin==12 ){
        farRewindCounter[step_being_explored]++;
        if((step_being_explored-steps_since_coin)<1){
            step_being_explored = 1;
        }
        else{
            step_being_explored -=steps_since_coin;
        }
        fprintf(stdout, ">>>> Far-Rewind-Function, rewinding %d steps. \n", steps_since_coin);
        return 1;
    }

    return 0;
}




//this explores the options for the given state/step 
int exploreOptions(float Xvelocity, float Zvelocity){

    // if doing direction based optimisation then leave the option as savedopt 
    if (LockRecord!=option_locked_for_steps && option_locked_for_steps>0){
            exploration_option_choice= savedopt;
        }


    // this is velocity clamping, it sets tilt contrary to the velocity vector.
    //if far enough away from a fallout and speed too great.THis will be skipped if it is a hatestate

    //steps_since_fallout>1 &&


    if( (Xvelocity*Xvelocity+ Zvelocity*Zvelocity)> MAXSPEED && replayInt==0){
         
         int reverse_option = find_prevalent_Tilt(Xvelocity, Zvelocity);//this is misleading, the find_prevalent_Tilt method returns the reverse direction to the velocity
         if(hateStates[step_being_explored][reverse_option] != 1){  // check the reverse_option for hatestates. 
            exploration_option_choice = reverse_option;// function that reverses tilt and evaluate.
            deccelerating_evaluate_anyway = true;
            fprintf(stdout, "velocity %f is too high applying tilt correction. Option:    %d\n",
             (Xvelocity*Xvelocity+ Zvelocity*Zvelocity), exploration_option_choice);
             //return 2;
         }// if hatestate found do nothing the following code will take care of it.
         
     }

    if(hateStates[step_being_explored][exploration_option_choice] == 1){    // if a hatestate exists for that step and option, skip the option.
        
        
        
        fprintf(stdout, "-------------------------------State   %d      -------%d BLOCKED\n",step_being_explored, exploration_option_choice);
        exploration_option_choice++;    // increments the option being explored.
        
        
        if(exploration_option_choice==9){   // if the option to be skipped is the last one some trickery is needed.

            ninthOPT = true;            
            
            return 2;
        } 
        exitPlay=true;
        return 1;       
    }

    
        
    

    switch(exploration_option_choice){      // these are the directions being explored, the strings do nothing currently.
    
            case 0:
                X=0;   Z=0;   strncpy ( direction, "___", sizeof(direction) );
                break;
            case 1:
                X=-30; Z=0;   strncpy ( direction, "N__", sizeof(direction) );
                break;
            case 2:
                X=-30; Z=+30; strncpy ( direction, "NE_", sizeof(direction) );
                break;
            case 3:
                X=0;   Z=+30; strncpy ( direction, "E__", sizeof(direction) );
                break;
            case 4:
                X=+30; Z=+30; strncpy ( direction, "SE_", sizeof(direction) );
                break;
            case 5:
                X=+30; Z=0;  strncpy ( direction, "S__", sizeof(direction) );
                break;
            case 6:
                X=+30; Z=-30; strncpy ( direction, "SW_", sizeof(direction) );
                break;
            case 7:
                X=0;   Z=-30; strncpy ( direction, "W__", sizeof(direction) );
                break;
            case 8:
                X=-30; Z=-30; strncpy ( direction, "NW_", sizeof(direction) );
                break;
            
        }
        return 0;
}

// here the options are evaluated and the one with the most reward is chosen and saved as the tilt to be read for that state/step
// this is a horribly hacky function but is really the crux of the program.
int evaluateOptions(){

        fprintf(stdout, "\nTest statement 3");

        if (LockRecord==option_locked_for_steps){   // this unlocks the direction for the first step in a direction based optimisation
            unlock4onestep = true;
        }



        if(ninthOPT==false){    // normal options. (ie not skipping the last option whilst exploring a state).

        int HeuristicInt = Heuristic(XposGrid, ZposGrid);   // this calls a method which compares position with previous positions and returns one if it is repeated

        reward= COINS*4 - HeuristicInt*2  + (rand()/randmax)*0.6;  /// reward signal. You can see coins are twice what the maximum Heuristic pernalty is
                                                                    // and the noise function is a fraction of .6


        options_tried[optionArrayPosition][0]= reward;      // so these are the columns of the options_tried array which is sorted by the first column
        options_tried[optionArrayPosition][1]= X;
        options_tried[optionArrayPosition][2]= Z;
        options_tried[optionArrayPosition][3]= COINS;
        options_tried[optionArrayPosition][4]= exploration_option_choice;
        options_tried[optionArrayPosition][5]= HeuristicInt;

        //fprintf(stdout, "*********************TRYING OPTION            %d         ARRAY Position:  %d\n", 
        //exploration_option_choice, optionArrayPosition);

        optionArrayPosition++;      // increments index as options are tried.
        
        exploration_option_choice++;    // this increments the options tried as one is evaluated.   
        }

        fprintf(stdout, "\nTest statement 4");


        if(ninthOPT==true){         // a little blurb for the skipping of the last option.
            fprintf(stdout, "eighth option skipped. Evaluating\n");
            ninthOPT=false;
        }  
        
        if(option_locked_for_steps>0 && unlock4onestep==false){     // this keeps the option fixed for direction based optimisation.
            exploration_option_choice--;    // un increment the option that was incremented. Is easier than a bunch of if statements
            fprintf(stdout, "Lockstep Integer: %d. Option Locked: %d \n", option_locked_for_steps, (int)options_tried[0][4]);
            
        }

        fprintf(stdout, "\nTest statement 5");

        if(deccelerating_evaluate_anyway==true){    // if the tilt has been set to be contrary to the velocity then just chose the option given
                                                    // I do this lazily by setting the exploration option choice to 9 so as it is picked up by the following method

            deccelerating_evaluate_anyway = false;
            fprintf(stdout, "decceleration tilt received ---  exploration_option_choice : %d\n", (int)options_tried[0][4]);
            exploration_option_choice = 9;
        }


        fprintf(stdout, "\nTest statement 6");


        if(exploration_option_choice==9 || (option_locked_for_steps>0 && unlock4onestep==false) ){             
            // this sorts the (up to 9) options that have been tried and takes the best one and adds it to the list of tilts.

            decrement_lockstep = true;

            if(unlock4onestep==true){   // once the step has been evaluated the bool is set to false for the random first step of DBO.
                unlock4onestep = false;
                fprintf(stdout, "first step of optimisation unlocked!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            }
            //deccelerating_evaluate_anyway = false;
            
            //this is just adding to the position and velocity array
            step++;
            PNV[step][0]= step;
            PNV[step][1]= Xpos;
            PNV[step][2]= Zpos;
            PNV[step][3] = reward;

            // this sorts all options tried by reward, note array length and size and comparison function
            qsort(options_tried,optionArrayPosition, 6*sizeof(options_tried[0][0]), cmp);

            // S is one ignore this for loop.
            for(int i=0; i<(S); i++){
                
            options_chosen[step_being_explored+i][0]=options_tried[0][1]; // this is the X tilt of the chosen option
            options_chosen[step_being_explored+i][1]=options_tried[0][2]; // Z of chosen option.  
            options_chosen[step_being_explored+i][2]=options_tried[0][3]; // coins of chosen option.
            options_chosen[step_being_explored+i][3]=options_tried[0][5]; // HeuristicInt of chosen option.

            old_options[step_being_explored+i]=options_tried[0][4]; // option chosen for that step. This is used when looking back at hatestates.
            
            
            HeuristicPositions[step_being_explored+i][0] = XposGrid;    // adds position at step to heuristic grid.
            HeuristicPositions[step_being_explored+i][1] = ZposGrid;
            }

            if(options_chosen[step_being_explored][3] == options_chosen[step_being_explored-1][3] && step_being_explored>1 ){
                failedHeuristicCounter++;
            }else{
                failedHeuristicCounter=0;
            }       // counts number of steps the heuristic failed for in a row.   This could be useful for a heuristic based optimisation

            coins_at_step[step_being_explored] = options_chosen[step_being_explored][2];// this is just a renamed variable for the coins at a given step

            // this increments if the coin number is the same for consecutive steps.
            if(options_chosen[step_being_explored][2] == options_chosen[step_being_explored-1][2] && step_being_explored>1){
                steps_since_coin+=1;//(S);
            }else{
                steps_since_coin=0;
            }

            steps_since_coinAtgivenstep[step_being_explored]= steps_since_coin; // once incremented stores the steps since coin at step


            //steps_since_fallout++;  // steps since fallout. This is not used.
            //steps_since_falloutAtGivenStep[step_being_explored] = steps_since_fallout;


            fprintf(stdout, "\nTest statement 7");


            fprintf(stdout, "------> State: %d steps since coinchange: %d     Option Selected: %d\n",
             step_being_explored, steps_since_coin,  (int ) options_tried[0][4] );
            
            // zeros the index of options
            optionArrayPosition = 0;


            if(option_locked_for_steps>0 && decrement_lockstep==true){      // here we decrement the lockstep integer if the direction is locked
                decrement_lockstep = false; 
                option_locked_for_steps--;
            }

            // If the direction is NOT locked, zero the exploration option.
            if(option_locked_for_steps==0){
                exploration_option_choice=0; // options have been tried they need zeroed in any case
            }
            fprintf(stdout, "\nTest statement 8");

            CoinBasedRewindFn();  // This is the call to the OPTIMIZATION. This is currently Direction based with a random first tilt.
            step_being_explored+=S; // else move on. S is one and should be removed.                
                
                      
        }

            
        



    return 0;
}


int Heuristic(float XPG,float ZPG){ // this takes in grid positions and compares them to previous ones.

    for(int i=0; i<(step_being_explored-S); i++ ){
        if(HeuristicPositions[i][0] == XPG && HeuristicPositions[i][1] == ZPG ){
            //fprintf(stdout, ":::::::::: Heuristic Reward Modifier Fired\n");
            return 1;
        }
    }

    return 0;
}


int cmp (const void * a, const void * b)
{
    float x = *(float*)a;
    float y = *(float*)b;
    return x > y ? -1 : x == y ? 0 : 1;
}


int exitandstartRun(){
    progress_same();// this is a Neverball method that resets the game.
    goto_state(&st_play_loop);//this is a Neverball method that restarts play.
    NinetyHZCounter=0;// This zeros the counter, it is so named as it is incremented by Neverball every 1/90 of a second
    I=0;// this is the current state, which is the timestep the ball is currently at.
    if(runCounter==0){// this is the runcounter and is zero at the first run.
        srand( (unsigned) time(NULL) * getpid());   // the seed. <You may wanna set this to get the same result each time, or to compare methods>
        fprintf(stdout, "\n seed initialized\n");
        step_being_explored =1;// this is the step that gets explored and evaluated.
        exploration_option_choice =0;//this is the option currently being explored for that step/state.
        optionArrayPosition =0;// this is the array index of the option being evaluated.
        replayInt=0;//this is one when the game has been solved and loops the level.
        fallpossible = true;//just a bool that allows only one fallout to be detected per run.
        step=0;
        if(checkfordata()==1){  // this checks whether level data exists already.
            fprintf(stdout, "Old data Found\n");
            readTiltFile();
        }
        else{
            fprintf(stdout, "No Data Found\n");
        }
    }
    runCounter++;
    exitPlay = false;

    return 0;
}


int readTiltFile(){// this reads the tilt from file and sets the replayInt to one so the level plays a loop.
    
    char str0[400];
    int readstep =0;
    FILE *fpm;   
    char fileNameString02[10] = "/Tilt.txt";
    char TILTfiledest[250];
    sprintf(TILTfiledest, "%s%s", total_path, fileNameString02  );
    fpm=fopen(TILTfiledest, "r");
    if(fpm != NULL){
        while(  fgets (str0, sizeof(str0), fpm)!= NULL  ){
            sscanf(str0, "%f %f", &options_chosen[readstep][0], &options_chosen[readstep][1] );
            readstep++;
        }
    }

    step_being_explored = readstep;
    replayInt = 1;


    return 0;
}




int checkfordata(){     // checks whether file exists

    const char * levelname;
    levelname= returns_the_level_string(); 
    char base_folder[21] = "AI_DATA/";
    sprintf(total_path, "%s%s", base_folder,  levelname );



    char fileNameString02[10] = "/Tilt.txt";
    char TILTfiledest[250];
    sprintf(TILTfiledest, "%s%s", total_path, fileNameString02  );


    fprintf(stdout, "checking for existence  :  %s\n", TILTfiledest );


    if(file_exist(TILTfiledest)){
        return 1;
    }
    
    return 0;
}






int writeEverything(){

    fprintf(stdout, "\n\n\n  --------------GOAL achieved!!! -- writing log files.\n\n");    // this will print if the level is solved.
    replayInt =1; // set when level crunched
    setupFolder();

    writeTiltFile();    // this is the tilt file.
    writeMaster();
    writePNV();     // these last three are just stats to be graphed.
    writeFO();
    writeCorrectionLOG();

    


    return 0;
}





int setupFolder(){  // this makes the directory should it not exist already.
    const char * levelname;
    levelname= returns_the_level_string(); 
    char base_folder[21] = "AI_DATA/";
    sprintf(total_path, "%s%s", base_folder,  levelname );
    if (!file_exist(total_path) ) {
        mkdir(total_path, 0777); //make path, create folder
    };

    return 0;
}

int writeMaster(){  // this is just a file that pretty much says whether the level has been crunched
    
    FILE *fpm;   
    char fileNameString01[15] = "/MASTER.txt";
    char MASTERfiledest[250];
    sprintf(MASTERfiledest, "%s%s", total_path, fileNameString01  );
    fpm=fopen(MASTERfiledest, "w");
    if(fpm != NULL){       
        fprintf(fpm, "%d %d %d\n", replayInt, I, runCounter );
        fclose(fpm);
    }
    return 0;
}


int writePNV(){     // the rest of these just write the stats to file.
    
    FILE *fpm;   
    char fileNameString02[9] = "/PNV.txt";
    char PNVfiledest[250];
    sprintf(PNVfiledest, "%s%s", total_path, fileNameString02  );
    fpm=fopen(PNVfiledest, "w");
    if(fpm != NULL){
        for(int i=0; i<step; i++){      
            fprintf(fpm, "%f %f %f %f\n", PNV[i][0], PNV[i][1], PNV[i][2], PNV[i][3] );
        }
            fclose(fpm);
        }
    return 0;
}



int writeCorrectionLOG(){
    
    FILE *fpm;   
    char fileNameString04[17] = "/Corrections.txt";
    char Corrfiledest[250];
    sprintf(Corrfiledest, "%s%s", total_path, fileNameString04  );
    fpm=fopen(Corrfiledest, "w");
    if(fpm != NULL){
        for(int i=0; i<numberofcorrections; i++){      
            fprintf(fpm, "%f %f %f %f %f\n", 
            correctionLOG[i][0],correctionLOG[i][1],correctionLOG[i][2],correctionLOG[i][3],correctionLOG[i][4] );
        }
            fclose(fpm);
        }
    return 0;
}






int writeFO(){
    
    FILE *fpm;   
    char fileNameString03[8] = "/FO.txt";
    char FOfiledest[250];
    sprintf(FOfiledest, "%s%s", total_path, fileNameString03  );
    fpm=fopen(FOfiledest, "w");
    if(fpm != NULL){
        for(int i=0; i<fallouts; i++){      
            fprintf(fpm, "%f %f %f\n", FO[i][0], FO[i][1], FO[i][2] );
        }
            fclose(fpm);
        }
    return 0;
}



int writeTiltFile(){
    
    FILE *fpm;   
    char fileNameString02[10] = "/Tilt.txt";
    char TILTfiledest[250];
    sprintf(TILTfiledest, "%s%s", total_path, fileNameString02  );
    fpm=fopen(TILTfiledest, "w");
    if(fpm != NULL){
        for(int i=0;i<I;i++){
            fprintf(fpm, "%f %f\n", options_chosen[i][0], options_chosen[i][1] );
        }
        fclose(fpm);
    }
    return 0;
}





int file_exist (char *filename)
{
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}
// end of NeverCruncher code.

static int play_loop_gui(void)
{
    int id;
    
    if ((id = gui_label(0, _("GO!"), GUI_LRG, gui_blu, gui_grn)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int play_loop_enter(struct state *st, struct state *prev)
{
    rot_init();
    fast_rotate = 0;

    if (prev == &st_pause)
        return 0;

    audio_play(AUD_GO, 1.f);

    game_client_fly(0.0f);

    show_hud = 1;
    hud_update(0);

    return play_loop_gui();
}

static void play_loop_paint(int id, float t)
{
    if(replayInt==1){
        game_client_draw(0, t);
        //ball_AI_Controller();//JK
        
        if (show_hud)
            hud_paint();

        if (time_state() < 1.f)
            gui_paint(id);
    }
}

static void play_loop_timer(int id, float dt)
{
   


    float k = (fast_rotate ?
               (float) config_get_d(CONFIG_ROTATE_FAST) / 100.0f :
               (float) config_get_d(CONFIG_ROTATE_SLOW) / 100.0f);

    float r = 0.0f;

    gui_timer(id, dt);

    hud_timer(dt);

    switch (rot_get(&r))
    {
    case ROT_HOLD:
        /*
         * Cam 3 could be anything. But let's assume it's a manual cam
         * and holding down both rotation buttons freezes the camera
         * rotation.
         */
        game_set_rot(0.0f);
        game_set_cam(CAM_3);
        break;

    case ROT_ROTATE:
    case ROT_NONE:
        game_set_rot(r * k);
        game_set_cam(config_get_d(CONFIG_CAMERA));
        break;
    }

    game_step_fade(dt);

    game_server_step(dt);
    game_client_sync(demo_fp);
    game_client_blend(game_server_blend());

    switch (curr_status())
    {
    case GAME_GOAL: // code goal -- Logik.
        
        goto_state(&st_goal); 
        // if(cated_string!=0){
        // // const char commandstring3[400];
        // // sprintf(commandstring3, "%s%s%s%s%s" , "sed -i ", "s/", runIdentifier, "/GOAL/g ",cated_string);
        // // system(commandstring3);
        // //fprintf(stdout, "running %s\n", commandstring3);
        // }
        
        progress_stat(GAME_GOAL);
        // strcpy(runIdentifier, "wrong");
        // progress_same();
        // goto_state(&st_play_loop);
        
        break;

    case GAME_FALL:
           // this is obsolete*************************
        

    case GAME_TIME:

        
            progress_stat(GAME_TIME);
            goto_state(&st_fail);
        

        // //JK same for timeout.
        
        
        break;

    default:
        progress_step();
        break;
    }
}

static void play_loop_point(int id, int x, int y, int dx, int dy)
{
    game_set_pos(dx, dy);
}





static void play_loop_stick(int id, int a, float v, int bump)
{

    
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
        game_set_z(v);
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
        game_set_x(v);
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X1, a))
    {
        if      (v > 0.0f)
            rot_set(DIR_R, +v, 1);
        else if (v < 0.0f)
            rot_set(DIR_L, -v, 1);
        else
            rot_clr(DIR_R | DIR_L);
    }
}


static int play_loop_click(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rot_set(DIR_R, 1.0f, 0);
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            rot_set(DIR_L, 1.0f, 0);

        click_camera(b);
    }
    else
    {
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rot_clr(DIR_R);
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            rot_clr(DIR_L);
    }

    return 1;
}

static int play_loop_keybd(int c, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_KEY_CAMERA_R, c))
            rot_set(DIR_R, 1.0f, 0);
        if (config_tst_d(CONFIG_KEY_CAMERA_L, c))
            rot_set(DIR_L, 1.0f, 0);
        if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
            fast_rotate = 1;

        keybd_camera(c);

        if (config_tst_d(CONFIG_KEY_RESTART, c) &&
            progress_same_avail())
        {
            if (progress_same())
                goto_state(&st_play_ready);
        }
        if (c == KEY_EXIT)
            goto_state(&st_pause);
    }
    else
    {
        if (config_tst_d(CONFIG_KEY_CAMERA_R, c))
            rot_clr(DIR_R);
        if (config_tst_d(CONFIG_KEY_CAMERA_L, c))
            rot_clr(DIR_L);
        if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
            fast_rotate = 0;
    }

    if (d && c == KEY_LOOKAROUND && config_cheat())
        return goto_state(&st_look);

    if (d && c == KEY_POSE)
        show_hud = !show_hud;

    if (d && c == SDLK_c && config_cheat())
    {
        progress_stat(GAME_GOAL);
        return goto_state(&st_goal);
    }
    return 1;
}

static int play_loop_buttn(int b, int d)
{
    if (d == 1)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            goto_state(&st_pause);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            rot_set(DIR_R, 1.0f, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            rot_set(DIR_L, 1.0f, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b))
            fast_rotate = 1;

        buttn_camera(b);
    }
    else
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            rot_clr(DIR_R);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            rot_clr(DIR_L);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b))
            fast_rotate = 0;
    }
    return 1;
}



/*---------------------------------------------------------------------------*/

static float phi;
static float theta;

static int look_enter(struct state *st, struct state *prev)
{
    phi   = 0;
    theta = 0;
    return 0;
}

static void look_leave(struct state *st, struct state *next, int id)
{
}

static void look_paint(int id, float t)
{
    game_client_draw(0, t);
}

static void look_point(int id, int x, int y, int dx, int dy)
{
    phi   +=  90.0f * dy / video.device_h;
    theta += 180.0f * dx / video.device_w;

    if (phi > +90.0f) phi = +90.0f;
    if (phi < -90.0f) phi = -90.0f;

    if (theta > +180.0f) theta -= 360.0f;
    if (theta < -180.0f) theta += 360.0f;

    game_look(phi, theta);
}

static int look_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT || c == KEY_LOOKAROUND)
            return goto_state(&st_play_loop);
    }

    return 1;
}

static int look_buttn(int b, int d)
{
    if (d && (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b)))
        return goto_state(&st_play_loop);

    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_play_ready = {
    play_ready_enter,
    shared_leave,
    play_ready_paint,
    play_ready_timer,
    NULL,
    NULL,
    NULL,
    play_ready_click,
    play_ready_keybd,
    play_ready_buttn
};

struct state st_play_set = {
    play_set_enter,
    shared_leave,
    play_set_paint,
    play_set_timer,
    NULL,
    NULL,
    NULL,
    play_set_click,
    play_set_keybd,
    play_set_buttn
};

struct state st_play_loop = {
    play_loop_enter,
    shared_leave,
    play_loop_paint,
    play_loop_timer,
    play_loop_point,
    play_loop_stick,
    shared_angle,
    play_loop_click,
    play_loop_keybd,
    play_loop_buttn
};

struct state st_look = {
    look_enter,
    look_leave,
    look_paint,
    NULL,
    look_point,
    NULL,
    NULL,
    NULL,
    look_keybd,
    look_buttn
};
