/*
 * lm75.c
 *
 *   AFCIPMI  --
 *
 *   Copyright (C) 2015
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "port.h"
#include "sdr.h"
#include "task_priorities.h"
#include "board_version.h"
#include "lm75.h"

void vTaskLM75( void* Parameters )
{
    TickType_t xLastWakeTime;
    /* Task will run every 100ms */
    const TickType_t xFrequency = LM75_UPDATE_RATE / portTICK_PERIOD_MS;

    uint8_t i2c_bus_id;

    /*! @bug LM75 is addressed randomly through the SDR */
    uint8_t LM75_address[LM75_MAX_COUNT] = { 0x4C, 0x4D, 0x4E, 0x4F };

    uint8_t i, j;
    SDR_type_01h_t *pSDR;
    sensor_data_entry_t * pDATA;

    /* Populate the table with the I2C addressess of the LM75 and configure the ICs */
    for ( j = 0, i = 0; i < NUM_SDR && j < LM75_MAX_COUNT; i++, j++ ) {
	if (*(sensor_array[i].task_handle) != xTaskGetCurrentTaskHandle() ) {
	    continue;
	}
	if (afc_i2c_take_by_busid(I2C_BUS_CPU_ID, &i2c_bus_id, (TickType_t) 50) == pdFALSE) {
	    continue;
	}
        pDATA = sensor_array[i].data;
        pDATA->address = LM75_address[j];

#ifdef LM75_CFG
	uint8_t cmd[3] = {0x00, 0x01, 0x9F};
	xI2CMasterWrite(i2c_id, pDATA->address, ch, 3);
#endif
	afc_i2c_give( i2c_bus_id );
    }

    /* Initialise the xLastWakeTime variable with the current time. */
    xLastWakeTime = xTaskGetTickCount();

    for ( ;; ) {

        /* Update all temperature sensors readings */
        for ( i = 0; i < NUM_SDR; i++ ) {
	    /* Check if the handle pointer is not NULL */
            if (sensor_array[i].task_handle == NULL) {
		continue;
	    }
	    /* Check if this task should update the selected SDR */
	    if ( *(sensor_array[i].task_handle) != xTaskGetCurrentTaskHandle() ) {
		continue;
	    }

            pSDR = (SDR_type_01h_t *) sensor_array[i].sdr;
            pDATA = sensor_array[i].data;

            /* Try to gain the I2C bus */
            if (afc_i2c_take_by_busid(I2C_BUS_CPU_ID, &i2c_bus_id, (TickType_t)100) == pdFALSE) {
                continue;
            }
            /* Update the temperature reading */
	    uint8_t temp[2] = {0};

            if (xI2CMasterWriteRead( i2c_bus_id, pDATA->address, 0x00, &temp[0], 2) == 2) {
		/*! @todo Apply the conversion formula before writing the value to SDR */
                pDATA->readout_value = temp[0];
            }

	    afc_i2c_give(i2c_bus_id);
        }
	vTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}

void LM75_init( void )
{
    xTaskCreate( vTaskLM75, "LM75", configMINIMAL_STACK_SIZE, (void *) NULL, tskLM75SENSOR_PRIORITY, &vTaskLM75_Handle);
}
