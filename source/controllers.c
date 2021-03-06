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
 *  Github: https://github.com/Geru-Scotland/inatrix_overflow
 */

/**
 * @file controllers.c
 * @brief Invocar y gestionar los controladores oportunos, permitiendo una
 * configuración modular del sistema de entrada-salida.
 */

#include "nds.h"
#include <stdio.h>
#include "defines.h"
#include "input.h"
#include "backgrounds.h"
#include "sprites.h"
#include "eventMgr.h"
#include "game.h"
#include "timer.h"
#include "consoleUI.h"


/**
 * @brief Función encargada de llamar a los controladores
 * oportunos, configurando así nuestro sistema de
 * Entrada y salida de manera modular.
 */
void controllers_InitSetup(){
    controllers_EnableIntMaster();
    controllers_EnableKeyPadInt();
    controllers_ConfigureTimer();
    controllers_ConfigureInput();
    controllers_SetInterruptionVector();
}

/*
*********************
*********************
** REGISTER CONFIG **
*********************
*********************
*/

void controllers_EnableIntMaster(){
    IME = 1;
}

void controllers_DisableIntMaster(){
    IME = 0;
}

void controllers_ConfigureTimer(){
    timer_ConfigureTimer(0, 0x0000);
}

/**
 * @brief Establece el latch y la máscara
 * para los registros TIMER0_CNT y
 * TIMER0_DAT.
 * B y Select por interrupción.
 */
void controllers_ConfigureInput(){
    input_ConfigureInput(0x4000 | 0x0006);
}

void controllers_EnableKeyPadInt(){
    IME=0;
    IE |= IRQ_KEYS;
    IME=1;
}

void controllers_DisableKeyInt(){
    IME=0;
    IE &= ~IRQ_KEYS;
    IME=1;
}

/*
*********************
*********************
***** HANDLERS ******
*********************
*********************
*/

/**
 * @brief Esta rutina es la encargada de atender las interrupciones
 * realizadas al timer.
 *
 * Ésta misma, a su vez, invocará:
 * Timer: Calculará los segundos y en base a ello realizará cieras llamadas.
 * EventMgr: Actualizará las fases y las animaciones.
 */
void controllers_TimerHandler(){
    timer_UpdateTimer();
    eventMgr_UpdatePhases();
    eventMgr_UpdateAnimations();
}

/**
 * @brief Rutina de atención para el teclado.
 * Cada vez que haya una interrupción, genera
 * una "interferencia en Matrix".
 * Select y B son las teclas que generarán
 * la interrupción.
 */
void controllers_KeyPadHandler(){
    if(gameData.state == GAME_STATE_GAME)
        game_surrender();
}

/**
 * @brief Función que establece el vector de interrupciones.
 * Especificamos las direcciones de memoria donde se alojan
 * las rutina de atención para:
 *
 * 1. IRQ_KEYS: Cada vez que una tecla sea pulsada y se detecte por interrupción.
 * 2. IRQ_TIMER0: Cada vez que el timer lance una interrupción.
 */
void controllers_SetInterruptionVector()
{
    irqSet(IRQ_KEYS, controllers_KeyPadHandler);
    irqSet(IRQ_TIMER0, controllers_TimerHandler);
}
