/***********************************************************************************************//**
 * \file   app.c
 * \brief  Event handling and application code for Empty NCP Host application example
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/* Own header */
#include "app.h"

// App booted flag
static bool appBooted = false;

typedef struct dtmtest {

	uint8_t OnFlag;
	uint8_t ToneFlag;
	uint8_t TxPower;
	uint8_t Channel;
	uint8_t PHY;
	uint8_t PacketType;
	uint8_t PacketSize;
	union {
			uint8_t  Data[4];
			uint32_t Value;
			}Result;

	union {
			uint8_t  Data[2];
			uint16_t Value;
			}TestTime;

} DTMTEST;

//Structure that holds the DTM teste details
DTMTEST DTM;

uint16_t TimerCounter;
uint8_t TimerFlag;
uint8_t DataCounter;

void initDTM(void);
int startDTMTest(void);



void initDTM(void)
{
  //Initializes the DTM Test Parameters

	DTM.Channel = 0; 		//2402Mhz
	DTM.PacketType = 0;			// PRBS9 (GFSK)
	DTM.PHY = 1; 			//1M PHY
	DTM.TxPower = 0;		//0dBM
	DTM.PacketSize = 37; 	//37 Packets
	DTM.TestTime.Value = 10;		//10 Seconds

	DTM.Result.Value = 0;
	DTM.OnFlag = 0;
	DTM.ToneFlag = 0;

	TimerCounter = 0;
	TimerFlag = 0;
	DataCounter = 0;

}

int startDTMTest(void)
{

	DTM.Result.Value = 0;
  //It starts the Soft timer (1s, Handle 1, Non-Stop)
	gecko_cmd_hardware_set_soft_timer(32768, 1, 0);
	gecko_cmd_test_dtm_tx(DTM.PacketType,DTM.PacketSize,DTM.Channel,DTM.PHY);
	return 1;
}



/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
#if defined(DEBUG)
    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
#endif
    usleep(50000);
    return;
  }

  /* Handle events */
  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:

      appBooted = true;
      //It initializes the DTM test Variables
      initDTM();

      printf("Starting DTM test\n");

      startDTMTest();

      //printf("System booted. Starting advertising... \n");

      /* Set advertising parameters. 100ms advertisement interval. All channels used.
       * The first two parameters are minimum and maximum advertising interval, both in
       * units of (milliseconds * 1.6). The third parameter '7' sets advertising on all channels. */
      gecko_cmd_le_gap_set_adv_parameters(160, 160, 7);

      /* Start general advertising and enable connections. */
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

      //printf("Device is being advertised.\n");

      break;

      case gecko_evt_hardware_soft_timer_id:
        {
          //DTM test is on Handle 1
          if (evt->data.evt_hardware_soft_timer.handle == 1)
          {
						//printf("Soft Timer Handle %d\n ",evt->data.evt_hardware_soft_timer.handle);
						printf("SoftTimer cycle #%d\n",TimerCounter );
						if (TimerCounter++==DTM.TestTime.Value)
            {
							printf("DTM test Completed\n");
              gecko_cmd_hardware_set_soft_timer(0, 1, 0);
              TimerCounter =0;
              gecko_cmd_test_dtm_end();
              TimerFlag = 1;
            }

          }

        }
        break;

        case gecko_evt_test_dtm_completed_id:
        			{
        				if ( TimerFlag == 1)
        				{
        					DTM.Result.Value = evt->data.evt_test_dtm_completed.number_of_packets;
        					DTM.OnFlag = 0;
        					 TimerFlag = 0;

        					//gecko_cmd_system_reset(0);

        				    	if (DTM.ToneFlag==1)
        				    	{
        				    		DTM.PacketType = 254;
        				    		DTM.TestTime.Value = 300;
        				    		DTM.ToneFlag = 0;
        				        	gecko_cmd_hardware_set_soft_timer(32768, 1, 0);
        				        	gecko_cmd_test_dtm_tx(DTM.PacketType,DTM.PacketSize,DTM.Channel,DTM.PHY);
        				    	}

        				    	else
        				    	{
        							// gecko_cmd_le_gap_start_advertising(0,2,2);
        							 gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);

        				    	}

        				}

        			}
        	break;

    case gecko_evt_le_connection_closed_id:

      /* Restart general advertising and re-enable connections after disconnection. */
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

      break;

    default:
      break;
  }
}
