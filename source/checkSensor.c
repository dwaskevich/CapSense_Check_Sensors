/*
 * checkSensor.c
 *
 *  Created on: Jan 31, 2022
 *      Author: david
 *
 *  Description: checks CapSense sensor(s) for abnormal behavior:
 *  	- Stuck sensor - sensor remains active for too long
 *  	- No Man's Land - sensor signal is stuck between NT and FT+HYS
 *  	- Hyper Event - sensor active on signal much greater than expected (i.e. multiple of FT)
 *  		Note - this check is similar to, but the inverse of, Low Baseline Reset with scan count set
 *  			to 1. Root cause of this condition could be an abrupt increase in raw counts due to
 *  			system noise or, more likely, an abrupt drop in raw counts (due to system noise or
 *  			other cause) where LBR acts to reset the baseline but then raw counts return to "normal"
 *  			and results in what looks like a finger event with large signal.
 *
 *  	- This function only acts on sensors. Widget details (e.g.FT, NT, HYS) are extracted from
 *  		the CapSense data structure but the calling function needs to determine sensor position
 *  		(based on widget/sensor number in the CapSense configuration) and pass the resulting position
 *  		as part of the sensorMask input parameter. Checking all sensors (i.e. passing ALL_SENSORS as
 *  		the bitfield/mask) doesn't require any position information ... all sensors will be checked
 *  		in sequential order.
 *
 *  	- Input parameters:
 *  		- sensorMask:
 *  			- bitfield mask representing sensor(s) to be checked (max of 32 sensors for uint32_t)
 *  			- Note - use ALL_SENSORS to check all sensors
 *  		- resetBaseline:
 *  			- if true, reset baseline(s) for out-of-bounds sensor(s),
 *  				otherwise just report sensor status
 *  	- Returns:
 *  		- status of sensor checks:
 *  			- 0x00 = sensor(s) normal
 *  			- any other value = bitfield/mask of abnormal sensor(s)
 *
 *  	- Stuck sensor configuration (STUCK_SENSOR_COUNT) - number of CapSense scans before sensor is
 *  		considered "stuck" and baseline(s) reset (if resetBaseline = true).
 *  		- Note - a macro for STUCK_SENSOR_COUNT is defined in checkSensor.h that calculates the
 *  			value from STUCK_SENSOR_TIMEOUT_MSEC and CAPSENSE_TOTAL_SCAN_TIME_MSEC. Enter values
 *  			for these macros or define the stuck sensor count directly by overwriting
 *  			STUCK_SENSOR_COUNT with a count value.
 *
 *  	- No Man's Land configuration (NO_MANS_LAND_COUNT) - number of CapSense scans before sensor is
 *  		considered in NML and baseline(s) reset (if resetBaseline = true).
 *  		- Note - a macro for NO_MANS_LAND_COUNT is defined in checkSensor.h that calculates the
 *  			value from NO_MANS_LAND_TIMEOUT_MSEC and CAPSENSE_TOTAL_SCAN_TIME_MSEC. Enter values
 *  			for these macros or define the stuck sensor count directly by overwriting
 *  			NO_MANS_LAND_COUNT with a count value.
 *
 *  	- Hyper event configuration (HYPER_EVENT_FTH_MULTIPLIER) - multiple of CapSense FT before
 *  		baseline(s) reset.
 *  		- Note - this check acts immediately (if resetBaseline is true).
 *
 *
 */

#include "cy_pdl.h"
#include "cycfg_capsense.h"
#include "checkSensor.h"

uint32_t checkSensor(uint32_t sensorMask, bool resetBaseline)
{
	uint32_t result, sensorIndex;

	/* declare arrays to hold scan counts (size determined by <sizeof> structures in CapSense generated source) */
    static uint32_t noMansLandCounter[sizeof(cy_capsense_tuner.sensorContext)/sizeof(cy_stc_capsense_sensor_context_t)];
    static uint32_t stuckCounter[sizeof(cy_capsense_tuner.sensorContext)/sizeof(cy_stc_capsense_sensor_context_t)];

    /* declare pointers to required parameters in CapSense data structure */
    const cy_stc_capsense_widget_config_t * ptrWdConfig; /* pointer to widget configuration parameters in Flash */
    cy_stc_capsense_widget_context_t * ptrWdContext; /* pointer to widget configuration parameters in SRAM */
    cy_stc_capsense_sensor_context_t * ptrSensors; /* pointer to sensor data in SRAM */

    /* set CapSense data structure pointers to first element (widget or sensor) of each array */
    ptrWdConfig = cy_capsense_context.ptrWdConfig; /* required for number of sensors in widget */
    ptrWdContext = cy_capsense_tuner.widgetContext; /* required for widget parameters (FT, NT, HYST, etc) */
    ptrSensors = cy_capsense_tuner.sensorContext; /* required for sensor scan data (raw, diff, status, etc) */

    /* reset result and sensor index */
	result = 0;
	sensorIndex = 0; /* used as index for sensor arrays */

	/* process widgets (sensors are associated with widgets and checked inside widget loop) */
    for(uint8_t widgetID = 0; widgetID < sizeof(cy_capsense_tuner.widgetContext)/sizeof(cy_stc_capsense_widget_context_t); widgetID++)
    {
		 for(uint16_t snsNum = 0; snsNum < ptrWdConfig->numSns; snsNum++)
		 {
			 if(sensorMask & (1 << sensorIndex) ) /* only process if bitfield mask bit is set for this sensor */
			 {
				 /* test for hyper event first - acts immediately on this scan */
				 if(ptrSensors->diff > HYPER_EVENT_FTH_MULTIPLIER * ptrWdContext->fingerTh)
				 {
					 if(true == resetBaseline) /* reset sensor baseline */
						 Cy_CapSense_InitializeSensorBaseline(widgetID, snsNum, &cy_capsense_context);
					 result |= 1 << sensorIndex; /* report sensor anomaly in return variable */
				 }

				 /* test for No Man's Land */
				 if(ptrSensors->diff > ptrWdContext->noiseTh &&
						 ptrSensors->diff < (ptrWdContext->fingerTh + ptrWdContext->hysteresis) )
				 {
					 noMansLandCounter[sensorIndex]++;
					 if(noMansLandCounter[sensorIndex] > NO_MANS_LAND_COUNT)
					 {
						 if(true == resetBaseline) /* reset sensor baseline */
							 Cy_CapSense_InitializeSensorBaseline(widgetID, snsNum, &cy_capsense_context);
						 result |= 1 << sensorIndex; /* report sensor anomaly in return variable */
						 noMansLandCounter[sensorIndex] = 0; /* reset counter */
					 }
				 }

				 /* test for stuck sensor */
				 if(0 != (ptrSensors->status & CY_CAPSENSE_SNS_TOUCH_STATUS_MASK) )
				 {
					 stuckCounter[sensorIndex]++;
					 if(stuckCounter[sensorIndex] > STUCK_SENSOR_COUNT)
					 {
						 if(true == resetBaseline) /* reset sensor baseline */
							 Cy_CapSense_InitializeSensorBaseline(widgetID, snsNum, &cy_capsense_context);
						 result |= 1 << sensorIndex; /* report sensor anomaly in return variable */
						 stuckCounter[sensorIndex] = 0; /* reset counter */
					 }
				 }

			 }

			 ptrSensors++; /* move to next sensor in this widget */
			 sensorIndex++; /* increment sensor index */
		 }

		 ptrWdConfig++; /* move to the next widget */
    }

	return result;
}

