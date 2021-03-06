/*
 * This file is part of the Iñatrix Overflow Project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Github: https://github.com/Geru-Scotland/inatrix_overflow
 */

/**
 * @author Geru-Scotland.
 * @file eventMgr.c
 * @brief Gestor de eventos principal. Gracias a este sistema es posible el
 * poder conrolar eventos de una manera muy sencilla, permitiendo un diseño y
 * desarrollo de juego mucho más fluído y simple.
 */

#include "eventMgr.h"
#include "timer.h"
#include "backgrounds.h"
#include "sprites.h"
#include "nds.h"
#include "game.h"
#include "matrix.h"
#include "movementMgr.h"
#include "objectMgr.h"
#include "consoleUI.h"

/**
 * @var eventList[MAX_EVENTS]: Array que contiene punteros a structs @struct Event.
 * Ésta lista contendrá únicamente los eventos que han de ejecutarse aún.
 */
Event* eventList[MAX_EVENTS];
int numEvents;

#ifdef DEBUG_MODE
int lineDelete = 8;
int lineAdd = 0;
#endif
void eventMgr_InitEventSystem(){
    numEvents = 0;
}
/**
 * @brief Función para borrar un evento en concreto de la lista.
 * Reorganizar el array en base al evento borrado
 * Liberar memoria del puntero al evento.
 * @param event puntero al @struct Event
 */
void eventMgr_DeleteEvent(Event *event){
    uint8 i = event->pos;

#ifdef DEBUG_MODE
    iprintf("\x1b[%i;00H [DEL] NE: %i e.pos: %i - e.id: %i", lineDelete, numEvents, event->pos, event->id);
    lineDelete += 1;
#endif // DEBUG_MODE

    while ((i < numEvents) && (eventList[i] != NULL))
    {
        eventList[i] = eventList[i+1];
        eventList[i]->pos = i;
        i++;
    }

    free(event);
    numEvents--;
#ifdef DEBUG_MODE
    iprintf("\x1b[%i;00H [ARRAY] ={%i, %i, %i, %i}", lineDelete+6, eventList[0]->id, eventList[1]->id,eventList[2]->id,eventList[3]->id);
    lineDelete += 1;
#endif // DEBUG_MODE
}

/**
 * @brief Agrega un evento a la lista para ser ejecutado.
 * @param event puntero al @struct Event
 */
void eventMgr_AddEvent(Event *event){
    if(numEvents < MAX_EVENTS)
    {
        eventList[numEvents] = event;
        event->pos = numEvents;
        numEvents++;
#ifdef DEBUG_MODE
        iprintf("\x1b[%i;00H [ADD] NE: %i e.pos: %i - e.id: %i", lineAdd, numEvents, event->pos, event->id);
        lineAdd += 1;
#endif // DEBUG_MODE
    }
}

/**
 * @brief Función que elimina todos los eventos de la lista.
 */
void eventMgr_cancelAllEvents(){
    for (int i = 0; i < numEvents; i++)
        eventMgr_DeleteEvent(eventList[i]);
}
/**
 * @brief Función "Pública" que es la que realmente se utiliza fuera del eventMgr
 * para poder programar eventos en el tiempo.
 * @param eventId ID del evento
 * @param time Cuando el evento ha de ser ejecutado (con respecto al instante en el que se programe )
 */
void eventMgr_ScheduleEvent(uint8 eventId, int time){
    /*
     * Geru: Reservo memoria dinámica en el heap, porque el stack se vacía
     * Una vez terminado el scope de la función, y queremos que persistan.
     * Importante liberar la memoria reservada correspondiente a cada evento.
     */
    if(numEvents < MAX_EVENTS) {
        Event* e = malloc(sizeof(Event));
        e->id = eventId;
        e->execTime = timer.time + time;
        eventMgr_AddEvent(e);
    }
}

/**
 * @brief Función principal del eventMgr, que se encarga de ir comprobando
 * si alguno de los eventos almacenados en la lista ha de ser ejecutado.
 * Si efectivamente ha de ejecutarse, entonces se gestiona como se considere
 * oportuno.
 *
 * Aquí es donde se va a desarrollar secuencialmente el juego. Separando el "guión"
 * de la lógica.
 *
 * En la cadena de llamadas propiciada por la interrupción del timer, esta función
 * será invocada.
 */
void eventMgr_UpdateScheduledEvents(){
    if(numEvents == 0 || gameData.state == GAME_STATE_PAUSE)
        return;
    for (int i = 0; i < numEvents; i++)
    {
        if(eventList[i]->execTime <= timer.time)
        {
            switch(eventList[i]->id)
            {
                /*
                *********************
                *********************
                ***** MAIN MENU *****
                *********************
                *********************
                */
                case EVENT_MAIN_MENU_START:
                    consoleUI_showMenu();
                    gameData.phase = PHASE_SHOW_MENU;
                    eventMgr_ScheduleEvent(EVENT_MAIN_MENU_HIDE_UI, IN_1_SECONDS);
                    break;
                case EVENT_MAIN_MENU_HIDE_UI:
                    if(gameData.phase == PHASE_SHOW_MENU) {
                        iprintf("\x1b[9;00H |                           |");
                        eventMgr_ScheduleEvent(EVENT_MAIN_MENU_SHOW_UI, IN_1_SECONDS);
                    }
                    break;
                case EVENT_MAIN_MENU_SHOW_UI:
                    if(gameData.phase == PHASE_SHOW_MENU) {
                        iprintf("\x1b[9;00H |  <PRESS START TO BEGIN>   |");
                        eventMgr_ScheduleEvent(EVENT_MAIN_MENU_HIDE_UI, IN_1_SECONDS);
                    }
                    break;
                case EVENT_SHOW_CONTROLS:
                    gameData.phase = PHASE_SHOW_CONTROLS;
                    consoleUI_showControls();
                    break;
                case EVENT_SHOW_GAMEPLAY:
                    gameData.phase = PHASE_SHOW_GAMEPLAY;
                    consoleUI_showGameplay();
                    break;
                case EVENT_SHOW_LORE:
                    gameData.phase = PHASE_SHOW_LORE;
                    consoleUI_showLore();
                    break;
                case EVENT_SHOW_LORE_2:
                    gameData.phase = PHASE_SHOW_LORE_2;
                    consoleUI_showLore2();
                    break;
                /*
                *********************
                *********************
                ******* INTRO *******
                *********************
                *********************
                */
                case EVENT_INTRO_PRE_START:
                    background_setBackground(BG_MATRIX);
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, NO_WAIT);
                    eventMgr_ScheduleEvent(EVENT_INTRO_START, IN_4_SECONDS);
                    break;
                case EVENT_INTRO_START:
                    iprintf("\x1b[09;10H _");
                    iprintf("\x1b[10;00H Wake up, Inatrix...");
                    background_setBackground(BG_MATRIX_INATRIX);
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_3_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_TEXT1, IN_4_SECONDS);
                    break;
                case EVENT_INTRO_TEXT1:
                    iprintf("\x1b[10;00H The Matrix has you...");
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_3_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_TEXT2, IN_5_SECONDS);
                    break;
                case EVENT_INTRO_TEXT2:
                    iprintf("\x1b[10;00H Follow the white rabbit.");
                    background_setBackground(BG_RABBIT);
                    eventMgr_ScheduleEvent(EVENT_INTRO_TEXT3, IN_5_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_RABBIT2, IN_3_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_3_SECONDS);
                    break;
                case EVENT_INTRO_RABBIT2:
                    background_setBackground(BG_RABBIT2);
                    break;
                case EVENT_INTRO_TEXT3:
                    iprintf("\x1b[09;15H _");
                    iprintf("\x1b[10;00H Knock, knock, Inatrix.");
                    background_setBackground(BG_RABBIT3);
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_3_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_TEXT4, IN_4_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_SETBG_3, IN_3_SECONDS);
                    break;
                case EVENT_INTRO_TEXT4:
                    iprintf("\x1b[10;00H So, blue pill or red pill?");
                    iprintf("\x1b[20;00H Blue - Normal");
                    iprintf("\x1b[20;18H Red - Hard");
                    eventMgr_ScheduleEvent(EVENT_INTRO_SHOW_CAPSULES, IN_2_SECONDS);
                    break;
                case EVENT_INTRO_SHOW_CAPSULES:
                    objectMgr_spawnCapsules();
                    gameData.phase = PHASE_WAITING_PLAYER_INPUT;
                    break;
                case EVENT_INTRO_CAPSULE_SELECTED:
                    iprintf("\x1b[2J");
                    char ht1[] = "\x1b[10;00H I see... good choice.";
                    char nt1[] = "\x1b[10;00H You are weak.";
                    iprintf(gameData.mode == DIFFICULTY_HARD_MODE ? ht1 : nt1);
                    objectMgr_manageSelectedCapsule(gameData.mode);
                    gameData.phase = PHASE_MOVE_CAPSULE;
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_2_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_FINISH1, IN_4_SECONDS); // Ojo, algo más introductorio rollo into the matrix.
                    break;
                case EVENT_INTRO_FINISH1:
                    char ht2[] = "\x1b[10;00H or not? hahaha...";
                    char nt2[] = "\x1b[10;00H You will be lost in the Matrix";
                    iprintf(gameData.mode == DIFFICULTY_HARD_MODE ? ht2 : nt2);
                    objectMgr_manageSelectedCapsule(gameData.mode == DIFFICULTY_NORMAL_MODE ? GFX_CAPSULE_RED : GFX_CAPSULE_BLUE);
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_3_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_INTRO_FINISH2, IN_4_SECONDS);
                    break;
                case EVENT_INTRO_FINISH2:
                    consoleUI_showIntro1();
                    objectMgr_spawnInatrix();
                    eventMgr_ScheduleEvent(EVENT_GAME_START, IN_4_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_CLEAR_CONSOLE, IN_3_SECONDS);
                    break;
                /*
                *********************
                *********************
                ******* GAME ********
                *********************
                *********************
                */
                case EVENT_GAME_START:
                    gameData.state = GAME_STATE_GAME;
                    matrix_displayMatrix(true);
                    consoleUI_showIntro2();
                    eventMgr_ScheduleEvent(EVENT_GAME_START_DEST_MATRIX, IN_4_SECONDS);
                    eventMgr_ScheduleEvent(EVENT_GAME_UI_SHOW_BASE, IN_4_SECONDS);
                    break;
                case EVENT_GAME_START_DEST_MATRIX:
                    game_enableDestroyMatrix();
                    gameData.phase = PHASE_WAITING_PLAYER_INPUT;
                    eventMgr_ScheduleEvent(EVENT_GAME_DESTROY_MATRIX_CHECK, IN_1_SECONDS);
                    break;
                case EVENT_GAME_DESTROY_MATRIX_CHECK:
                    if(gameData.destroyMatrixActive){
                        gameData.destroyMatrixTime -= 1;
                        consoleUI_showUI();
                        if(gameData.destroyMatrixTime <= 0){
                            if(game_achievedMinimumOverflows()){
                                game_setDestroyMatrix(false);
                                eventMgr_ScheduleEvent(EVENT_GAME_DESTROY_MATRIX, NO_WAIT);
                            }
                            else
                            {
                                game_manageGameOver(false);
                                return;
                            }
                        }
                        eventMgr_ScheduleEvent(EVENT_GAME_DESTROY_MATRIX_CHECK, IN_1_SECONDS);
                    }
                    break;
                case EVENT_GAME_DROP_BITBLOCK:
                    gameData.phase = PHASE_BITBLOCK_FALLING;
                    break;
                case EVENT_GAME_REGENERATE_BITBLOCK:
                    matrix_regenerateBitBlock();
                    game_setDestroyMatrix(true);
                    gameData.phase = PHASE_WAITING_PLAYER_INPUT;
                    eventMgr_ScheduleEvent(EVENT_GAME_DESTROY_MATRIX_CHECK, IN_1_SECONDS);
                    break;
                case EVENT_GAME_HIDE_MATRIX:
                    gameData.phase = PHASE_REGENERATING_MATRIX;
                    matrix_displayMatrix(false);
                    eventMgr_ScheduleEvent(EVENT_GAME_REGENERATE_MATRIX, IN_5_SECONDS);
                    break;
                case EVENT_GAME_REGENERATE_MATRIX:
                    matrix_regenerateMatrix();
                    matrix_displayMatrix(true);
                    game_enableDestroyMatrix();
                    game_setDestroyMatrix(true);
                    game_increaseMatrixRegens();
                    gameData.phase = PHASE_WAITING_PLAYER_INPUT;
                    eventMgr_ScheduleEvent(EVENT_GAME_DESTROY_MATRIX_CHECK, IN_1_SECONDS);
                    break;
                case EVENT_GAME_DESTROY_MATRIX:
                    consoleUI_showRegeneratingMatrix();
                    gameData.phase = PHASE_DESTROYING_MATRIX;
                    break;
                case EVENT_GAME_INATRIX_MOVE_X:
                    movementMgr_movePosition(MOVEMENT_INATRIX_X);
                    objectMgr_setAnimationActive(ANIMATION_BIT_SHAKE, false);
                    gameData.phase = PHASE_MOVE_INATRIX_X;
                    break;
                case EVENT_GAME_INATRIX_MOVE_Y:
                    movementMgr_movePosition(MOVEMENT_INATRIX_Y);
                    objectMgr_setAnimationActive(ANIMATION_BIT_SHAKE, false);
                    gameData.phase = PHASE_MOVE_INATRIX_Y;
                    break;
                case EVENT_GAME_EVALUATE_BITBLOCK:
                    objectMgr_setAnimationActive(ANIMATION_BIT_SHAKE, false);
                    bool ovf = matrix_evalBitBlockOverflow();
                    if(!game_manageScore(ovf))
                        break;
                    if(ovf){
                        game_setDestroyMatrix(false);
                        consoleUI_showOverflow();
                        eventMgr_ScheduleEvent(EVENT_GAME_UI_SHOW_BASE, IN_4_SECONDS);
                    }else{
                        game_setDestroyMatrix(false);
                        consoleUI_showFail();
                        eventMgr_ScheduleEvent(EVENT_GAME_UI_SHOW_BASE, IN_5_SECONDS);
                    }
                    eventMgr_ScheduleEvent(EVENT_GAME_DROP_BITBLOCK, IN_2_SECONDS);
                    break;
                case EVENT_GAME_UI_SHOW_BASE:
                    game_setDestroyMatrix(true);
                    consoleUI_showUI();
                    break;
                case EVENT_INTRO_SETBG_MAIN:
                    background_setBackground(BG_MAIN);
                    break;
                case EVENT_INTRO_SETBG_2:
                    background_setBackground(BG_MATRIX);
                    break;
                case EVENT_INTRO_SETBG_3:
                    background_setBackground(BG_MATRIX2);
                    break;
                case EVENT_CLEAR_CONSOLE:
                    iprintf("\x1b[2J");
                    break;
                case EVENT_SHOW_STATS:
                    consoleUI_showStats();
                    gameData.state = GAME_STATE_STATS;
                    gameData.phase = PHASE_SHOW_STATS;
                    break;
                case EVENT_LISTEN_INPUT:
                    gameData.phase = PHASE_WAITING_PLAYER_INPUT;
                    break;
                case EVENT_GAME_PAUSE:
                    gameData.state = GAME_STATE_PAUSE;
                    gameData.phase = PHASE_GAME_PAUSE;
                    consoleUI_showPauseUI();
                    break;
                default:
                    break;
            }
            eventMgr_DeleteEvent(eventList[i]);
        }
    }
}

/**
 * @brief Son los eventos que van ocurriendo en base a la fase del estado, de
 * manera instantánea.
 *
 * En la cadena de llamadas propiciada por la interrupción del timer, esta función
 * será invocada.
 */
void eventMgr_UpdatePhases(){
    if((timer.ticks % 15 != 0) || gameData.state == GAME_STATE_PAUSE)
        return;

    switch(gameData.phase){
        case PHASE_BITBLOCK_FALLING:
            if(!matrix_dropBitBlockEffect()){
                // Si vas a hacer el efecto de spawn desde diferentes posiciones
                // Que se vean como 0,5sec después de que comience a caer el bitblock
                eventMgr_ScheduleEvent(EVENT_GAME_REGENERATE_BITBLOCK, IN_1_SECONDS);
                gameData.phase = PHASE_NULL;
            }
            break;
        case PHASE_DESTROYING_MATRIX:
            if(!matrix_destroyMatrixEffect()){
                eventMgr_ScheduleEvent(EVENT_GAME_REGENERATE_MATRIX, IN_3_SECONDS);
                gameData.phase = PHASE_NULL;
            }
            break;
        case PHASE_MOVE_INATRIX_X:
            if(movementMgr_nextPositionReached(MOVEMENT_INATRIX_X)){
                matrix_updatePivot(movementMgr_getPositionY(), movementMgr_getPositionX());
                objectMgr_setAnimationActive(ANIMATION_BIT_SHAKE, true);
                gameData.phase = PHASE_WAITING_PLAYER_INPUT;
            }
            break;
        case PHASE_MOVE_INATRIX_Y:
            if(movementMgr_nextPositionReached(MOVEMENT_INATRIX_Y)){
                matrix_updatePivot(movementMgr_getPositionY(), movementMgr_getPositionX());
                objectMgr_setAnimationActive(ANIMATION_BIT_SHAKE, true);
                gameData.phase = PHASE_WAITING_PLAYER_INPUT;
            }
            break;
        case PHASE_MOVE_CAPSULE:
            if(movementMgr_hasGfxReachedDest(
                    gameData.mode == DIFFICULTY_NORMAL_MODE ? GFX_CAPSULE_BLUE : GFX_CAPSULE_RED)){
                gameData.phase = PHASE_NULL;
            }
            break;
        default:
            break;
    }

    oamUpdate(&oamMain);
}

/**
 * @brief Actualiza las animaciones en caso de haber alguna activa. Por ahora únicamente trata
 * el movimiento en el eje X del bit seleccionado de manera pasiva por los dos Iñatrix.
 * Pero será de ayuda con el bitConjunctionEffect
 */
void eventMgr_UpdateAnimations(){
    if((timer.ticks % 3 != 0) || gameData.state == GAME_STATE_PAUSE)
        return;

    for(int anim = 0; anim < ANIMATIONS_SIZE; anim++){
        if(animations[anim]->active){
            switch(anim){
                case ANIMATION_BIT_SHAKE:
                    animations[anim]->state *= -1;
                    matrix_bitShakeEffect(animations[anim]->state);
                    break;
            }
        }
    }
}