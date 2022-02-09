/*
 * checkSensor.h
 *
 *  Created on: Jan 31, 2022
 *      Author: david
 */

#ifndef SOURCE_CHECKSENSOR_H_
#define SOURCE_CHECKSENSOR_H_

#define ALL_SENSORS			(0xffffffff)

#define STUCK_SENSOR_TIMEOUT_MSEC		(6000u)
#define NO_MANS_LAND_TIMEOUT_MSEC		(3000u)
#define HYPER_EVENT_FTH_MULTIPLIER		(2u)

#define CAPSENSE_TOTAL_SCAN_TIME_MSEC	(10u)

#define STUCK_SENSOR_COUNT				(STUCK_SENSOR_TIMEOUT_MSEC/CAPSENSE_TOTAL_SCAN_TIME_MSEC)
#define NO_MANS_LAND_COUNT				(NO_MANS_LAND_TIMEOUT_MSEC/CAPSENSE_TOTAL_SCAN_TIME_MSEC)



/*******************************************************************************
* Function Prototypes
*******************************************************************************/
uint32_t checkSensor(uint32_t sensorMask, bool resetBaseline);


#endif /* SOURCE_CHECKSENSOR_H_ */
