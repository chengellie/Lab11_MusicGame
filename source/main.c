/* Author: Ellie Cheng - echen111
 * Lab Section: 022
 * Assignment: Lab 11 
 * Exercise Description: [optional - include for your own benefit]
 * Music Game
 * I acknowledge all content contained herein, excluding template or example
 * code is my own original work.
 *
 *  Demo Link: Youtube URL> https://youtu.be/bQVA5H4gJ7Q
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "../header/bit.h"
#include "../header/timer.h"
#include "../header/scheduler.h"
#endif

unsigned char rowFlag = 0x00;
unsigned char buttonFlag = 0x00;
unsigned char scoreFlag = 0x00;
unsigned char deductionsFlag = 0x00;
unsigned char penaltyCheckFlag = 0x00;
unsigned char lostFlag = 0x01;
unsigned char levelFlag = 0x00;
unsigned char wonFlag = 0x00;
unsigned char musicFlag = 0x00;

// LED Matrix SM: Displays running lights
enum DISPLAY_States { DISPLAY_SMStart, DISPLAY_shift, DISPLAY_lost, DISPLAY_won };
int DisplaySMTick(int state) {
	// Local Variables
	static unsigned char pattern = 0x00;	// LED pattern - 0: LED off; 1: LED on
	static unsigned char row = 0xFE;  	// (s) displaying pattern. 
							            // 0: display pattern on row
							            // 1: do NOT display pattern on row
    
    static unsigned numRow = 0x05;

	// Transitions
	switch (state) {
        case DISPLAY_SMStart:
            state = DISPLAY_lost;
            break;
		case DISPLAY_shift:	
            if (!lostFlag && !wonFlag) {
                state = DISPLAY_shift;
            }
            else if (lostFlag) {
                row = 0xFE;
                state = DISPLAY_lost;
            }
            else if (wonFlag && !lostFlag) {
                row = 0xE0;
                state = DISPLAY_won;
            }
            break;
        case DISPLAY_lost:
            if (lostFlag) {
                state = DISPLAY_lost;
            }
            else { // !lostFlag
                pattern = 0x01;
                state = DISPLAY_shift;
            }
            break;
        case DISPLAY_won:
            if (wonFlag) {
                state = DISPLAY_won;
            }
            else {
                pattern = 0x01;
                state = DISPLAY_shift;
            }
            break;
		default:	
            state = DISPLAY_SMStart;
			break;
	}	

	// Actions
	switch (state) {
        case DISPLAY_shift:	
            if (pattern == 0x01) { 
				pattern = levelFlag;
                rowFlag = 0x00;		   
                numRow = (int) (rand() % 5 + 1);
                if (numRow == 1) {
                    row = 0xEF;
                }
                else if (numRow == 2) {
                    row = 0xF7;
                }
                else if (numRow == 3) {
                    row = 0xFB;
                }
                else if (numRow == 4) {
                    row = 0xFD;
                }
                else if (numRow == 5) {
                    row = 0xFE;
                }
                else { // Failsafe - select none of the rows
                    row = 0xFF;
                }
			} else { // Shift LED one spot to the right on current row
				pattern >>= 1;
                if (pattern == 0x01) {rowFlag = numRow;}
                else {rowFlag = 0x00;}
			}
			break;
        case DISPLAY_lost:
            row = 0xFF;
            pattern = 0x00;
            rowFlag = 0x00;
            break;
        case DISPLAY_won:
            row = 0xE0;
            pattern = 0xFF;
            rowFlag = 0x00;
            break;
		default:
	        break;
	}
	PORTC = pattern;	// Pattern to display
	PORTD = row;		// Row(s) displaying pattern	
	return state;
}

void set_PWM(double frequency) {
    static double current_frequency;

    if (frequency != current_frequency) {
        if (!frequency) { TCCR3B &= 0x08; }
        else { TCCR3B |= 0x03; }

        if (frequency < 0.954) { OCR3A = 0xFFFF; }
        else if (frequency > 31250) { OCR3A = 0x0000; }
        else { OCR3A = (short) (8000000 / (128 * frequency)) - 1; }

        TCNT3 = 0;
        current_frequency = frequency;
    } 
}

void PWM_on() {
    TCCR3A = (1 << COM3A0);
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
    set_PWM(0);
}

void PWM_off() {
    TCCR3A = 0x00;
    TCCR3B = 0x00;
}

// Player SM: Detects when a button is pressed correctly
enum Player_States { Player_SMStart, Player_wait, Player_note, Player_waitRelease };

int PlayerSMTick(int state) {
    unsigned char button = ~PINB & 0x1F;
    static unsigned char pointCheckFlag = 0x00;

    switch(state) {
        case Player_SMStart:
            state = Player_wait;
            break;
        case Player_wait:
            if (rowFlag) {
                pointCheckFlag = 0x00;
                state = Player_note;
            }
            else if (!rowFlag && button) {
                deductionsFlag++;
                state = Player_waitRelease;
            }
            else { // !rowFlag && !button
                state = Player_wait;
            }
            break;
        case Player_note:
            if (rowFlag) {
                state = Player_note;
            }
            else if (!rowFlag && !button) {
                pointCheckFlag = 0x00;
                state = Player_wait;
            }
            else if (!rowFlag && button) {
                pointCheckFlag = 0x00;
                state = Player_waitRelease;
            }
            break;
        case Player_waitRelease:
            if (!button) {
                state = Player_wait;
            }
            else { // button
                state = Player_waitRelease;
            }
            break;
        default:
            state = Player_SMStart;
            break;
    } 

    switch(state) {    
        case Player_note:
            penaltyCheckFlag = 0x00;
            if (button == 0x01 && (rowFlag == 0x01)) { // Note C - 261.63
                if (pointCheckFlag == 0x00) {
                    pointCheckFlag = 0x01;                
                }
                buttonFlag = button;
            }
            else if (button == 0x02 && (rowFlag == 0x02)) { // Note D - 293.66
                if (pointCheckFlag == 0x00) {
                    pointCheckFlag = 0x01;                
                }
                buttonFlag = button;
            }
            else if (button == 0x04 && (rowFlag == 0x03)) { // Note E - 329.63
                if (pointCheckFlag == 0x00) {
                    pointCheckFlag = 0x01;                
                }
                buttonFlag = button;
            }
            else if (button == 0x08 && (rowFlag == 0x04)) { // Note F - 349.23
                if (pointCheckFlag == 0x00) {
                    pointCheckFlag = 0x01;                
                }
                buttonFlag = button;
            }
            else if (button == 0x10 && (rowFlag == 0x05)) { // Note G - 392.00
                if (pointCheckFlag == 0x00) {
                    pointCheckFlag = 0x01;                
                }
                buttonFlag = button;
            }
            else { // Multiple buttons/no buttons/doesn't match row
                penaltyCheckFlag = 0x01;
                buttonFlag = 0x00;
            }
            // Player presses button during duration of last row
            if (pointCheckFlag == 0x01) {
                scoreFlag++;
                pointCheckFlag = 0x02;
            }
            break;

        default:
            penaltyCheckFlag = 0x01;
            pointCheckFlag = 0x00;
            buttonFlag = 0x00;
            break;
    }
    
    return state;
}

// Tone SM: Plays corresponding note when a button is pressed correctly
// Plays for the duration of the correct button press
enum TONE_States { TONE_SMStart, TONE_wait };

int ToneSMTick(int state) {
    static double noteFrequency = 0x00;

    switch(state) {
        case TONE_SMStart:
            state = TONE_wait;
            break;
        case TONE_wait:
            state = TONE_wait;
            break;
        default:
            state = TONE_SMStart;
            break;
    }

    switch(state) {
        case TONE_wait: 
            if (!musicFlag) {
                if (buttonFlag == 0x01) { // Note C - 261.63
                    noteFrequency = 261.63;
                }
                else if (buttonFlag == 0x02) { // Note D - 293.66
                    noteFrequency = 293.66;
                }
                else if (buttonFlag == 0x04) { // Note E - 329.63
                    noteFrequency = 329.63;
                }
                else if (buttonFlag == 0x08) { // Note F - 349.23
                    noteFrequency = 349.23;
                }
                else if (buttonFlag == 0x10) { // Note G - 392.00
                    noteFrequency = 392.00;
                }
                else { 
                    noteFrequency = 0;
                }
                set_PWM(noteFrequency);
            }
            break;
        default:
            noteFrequency = 0;
            break;
    }
    return state;
}

enum SCORE_States { SCORE_SMStart, SCORE_idle };

int ScoreSMTick(int state) {
    switch(state) {
        case SCORE_SMStart:
            state = SCORE_idle;
            break;
        
        case SCORE_idle:
            if (penaltyCheckFlag) {
                if (rowFlag) {
                    deductionsFlag++; 
                }
            }
            state = SCORE_idle;
            break;

        default:
            state = SCORE_SMStart;
            break;
    }
    return state;
}

enum MUSIC_States { MUSIC_SMStart, MUSIC_wait, MUSIC_play };

int MusicSMTick(int state) {
    unsigned char buttons = ~PINB & 0x1F;
    static double noteFrequency = 0x00;

    switch(state) {
        case MUSIC_SMStart:
            state = MUSIC_wait;
            break;
        case MUSIC_wait:
            if (musicFlag) {
                state = MUSIC_play;
            }
            else { // !musicFlag
                state = MUSIC_wait;
            }
            break;
        case MUSIC_play:
            if (musicFlag) {
                state = MUSIC_play;
            }
            else { // !musicFlag
                state = MUSIC_wait;
            }
            break;
        default:
            state = MUSIC_SMStart;
            break;
    }

    switch(state) {
        case MUSIC_play:
            if (buttons == 0x01) { // Note C - 261.63
                noteFrequency = 261.63;
            }
            else if (buttons == 0x02) { // Note D - 293.66
                noteFrequency = 293.66;
            }
            else if (buttons == 0x04) { // Note E - 329.63
                noteFrequency = 329.63;
            }
            else if (buttons == 0x08) { // Note F - 349.23
                noteFrequency = 349.23;
            }
            else if (buttons == 0x10) { // Note G - 392.00
                noteFrequency = 392.00;
            }
            else { 
                noteFrequency = 0;
            }
            set_PWM(noteFrequency);
            break;
        default:
            break;
    }
    return state;
}

enum LEVEL_States { LEVEL_SMStart, LEVEL_compare, LEVEL_waitForfeit, LEVEL_forfeit, LEVEL_reset, LEVEL_waitReset };

int LevelSMTick(int state) {
    unsigned char resetButton = (~PINB >> 5) & 0x01;
    static unsigned char levelDisplay = 0x00;

    switch(state) {
        case LEVEL_SMStart:
            state = LEVEL_forfeit;
            break;
        case LEVEL_compare:
            if (resetButton) {// Most precedence
                lostFlag = 0x01;
                state = LEVEL_waitForfeit;
            }
            else if (deductionsFlag >= 20) {
                lostFlag = 0x01;
                state = LEVEL_forfeit;
            }
            else { // deductions < 20
                if (scoreFlag == 0x02 && levelFlag > 0x08) {
                    scoreFlag = 0x00;
                    if (levelFlag >= 0x02) {
                        levelFlag = levelFlag >> 1;
                        if (levelFlag != 0x01) {
                            levelDisplay = levelDisplay | levelFlag;
                        }
                    }                    
                }
                else if (scoreFlag == 0x03) {
                    scoreFlag = 0x00;
                    if (levelFlag >= 0x02) {
                        levelFlag = levelFlag >> 1;
                        if (levelFlag != 0x01) {
                            levelDisplay = levelDisplay | levelFlag;
                        }
                    }   
                }
                // Check if player wins
                if (levelFlag == 0x01) {
                    wonFlag = 0x01;
                    musicFlag = 0x01;
                    state = LEVEL_forfeit;
                }
                else {
                    state = LEVEL_compare;
                }
            }
            break;
        case LEVEL_waitForfeit:
            levelDisplay = 0x00;
            if (resetButton) {
                state = LEVEL_waitForfeit;
            }
            else { // !resetButton
                state = LEVEL_forfeit;
            }
            break;
        case LEVEL_forfeit:
            levelDisplay = 0x00;
            if (resetButton) {
                state = LEVEL_waitReset;
            }
            else { // !resetButton
                state = LEVEL_forfeit;
            }
            break;
        case LEVEL_waitReset:
            scoreFlag = 0x00;
            deductionsFlag = 0x00;
            levelDisplay = 0x00;
            if (resetButton) {
                state = LEVEL_waitReset;
            }
            else { // !resetButton
                state = LEVEL_reset;
            }
            break;
        case LEVEL_reset:
            lostFlag = 0x00;
            wonFlag = 0x00;
            musicFlag = 0x00;
            levelFlag = 0x80;
            levelDisplay = levelFlag;
            state = LEVEL_compare;
            break;
        default:
            state = LEVEL_SMStart;
            break;
    }
    PORTA = levelDisplay; 
    return state;    
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0xFF; PORTA = 0x00;
    DDRB = 0xC0; PORTB = 0x3F;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    /* Insert your solution below */

    static task display, player, tone, score, level, music;
    task *tasks[] = { &display, &player, &tone, &score, &level, &music };
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

    const char start = -1;

    display.state = start;
    display.period = 300;
    display.elapsedTime = display.period;
    display.TickFct = &DisplaySMTick;

    player.state = start;
    player.period = 100;
    player.elapsedTime = player.period;
    player.TickFct = &PlayerSMTick;

    tone.state = start;
    tone.period = 100;
    tone.elapsedTime = tone.period;
    tone.TickFct = &ToneSMTick;

    score.state = start;
    score.period = 300;
    score.elapsedTime = score.period;
    score.TickFct = &ScoreSMTick;

    level.state = start;
    level.period = 100;
    level.elapsedTime = level.period;
    level.TickFct = &LevelSMTick;

    music.state = start;
    music.period = 100;
    music.elapsedTime = music.period;
    music.TickFct = &MusicSMTick;

    unsigned short i;
    unsigned long GCD = tasks[0]->period;
    for ( i = 1; i < numTasks; i++ ) {
        GCD = findGCD(GCD,tasks[i]->period);
    }

    PWM_on();
    TimerSet(GCD);
    TimerOn();

    while(1){
        for (i = 0; i < numTasks; i++){
            if (tasks[i]->elapsedTime == tasks[i]->period){
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += GCD;
        }
        while(!TimerFlag);
        TimerFlag = 0;

    }
    return 0;
}