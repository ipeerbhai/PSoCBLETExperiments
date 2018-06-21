/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

uint8 g_shouldBlink = 0;// Create a global flag for the red LED's blink mode.
CYBLE_CONN_HANDLE_T g_bleConnectionHandle; // A global handle to access the BLE connection system
enum CONNECTION_SATE { DISCONNECTED=0, CONNECTED } g_bleConnectionState = DISCONNECTED; // Are we connected or not?

//---------------------------------------------------------------------------------------------------------------------
// Procedure:
//  Update the state of the red led should blink variable to its opposite.
void NegateShouldBlinkState()
{
    g_shouldBlink = ( g_shouldBlink > 0 ) ? 0 : 1;
}

//---------------------------------------------------------------------------------------------------------------------
// Procedure:
//  Write the state of the LED to GATT
void UpdateGATTDBWithLedState() 
{
        // get a handle to the bluetooth GATT and notification system.
        CYBLE_GATTS_HANDLE_VALUE_NTF_T bleGATTAccessHandlePair; // There is some sort of "pair" concept here, seems to be a dictionary like object.
        bleGATTAccessHandlePair.attrHandle = CYBLE_LEDSTATE_ISBLINKING_CHAR_HANDLE; // set the attribute handle to the charteristic handle we want to update.
        bleGATTAccessHandlePair.value.len = 1; // we should use the negotiated MTU - 3
        bleGATTAccessHandlePair.value.actualLen= 1; // we should use the negotiated MTU - 3
        bleGATTAccessHandlePair.value.val = &g_shouldBlink; // pointer to what data we want to write to GATT
        
        // Update the GATT db
        CyBle_GattsWriteAttributeValue( &bleGATTAccessHandlePair, 0, &g_bleConnectionHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
}

//---------------------------------------------------------------------------------------------------------------------
// Procedure:
//  Actually blink the LED based on the global state variable
void BlinkTheLED()
{
    if ( g_shouldBlink > 0 )
    {
        // Start blinking
        pwm_Start();
    }
    else
    {
        // turn the LED off
        pwm_Stop();
    }
}

//---------------------------------------------------------------------------------------------------------------------
// MACRO:
//  This macro defines a handler for when the user button is pushed that we can later register.
CY_ISR (isrButtonPushed_Handler)
{
    // Invert the blinking state variable, then apply the effect to the pioneer board.
    NegateShouldBlinkState();
    BlinkTheLED();
    
    // Check to see if we have a BLE connection, and if we do, update the GATT db with the button's state
    if (CyBle_GetState() == CYBLE_STATE_CONNECTED)
    {
        UpdateGATTDBWithLedState();
    }
    pinUserButton_ClearInterrupt(); // we handled the button pused ISR, let's clear the interrupt.
}

//---------------------------------------------------------------------------------------------------------------------
// Delegate:
//  Get events from the BLE system and handle them as given by the event.
void BleCallBackDelegate(uint32 event, void* eventParam)
{
    CYBLE_GATTS_WRITE_REQ_PARAM_T* wrReqParam;
    switch(event)
    {
        // Handle disconnected or device boot scenarios
        case CYBLE_EVT_STACK_ON:
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            // turn on the blue led
            g_bleConnectionState = DISCONNECTED;
            blueLED_Write(0); // Write low, as we "sink" to ground to turn the LED on ( LED is connected to VCC by default )
        break;
            
        // device connected
        case CYBLE_EVT_GATT_CONNECT_IND:
            g_bleConnectionState = CONNECTED;
            blueLED_Write(1); // turn OFF the LED.  Yes, this is correct.
            break;
        
        // Gatt write requested from remote host.
        case CYBLE_EVT_GATTS_WRITE_REQ:
            // Get the write request event parameters
            wrReqParam = (CYBLE_GATTS_WRITE_REQ_PARAM_T*) eventParam;
            
            // see if the host wanted to write a new should blink flag
            if ( wrReqParam->handleValPair.attrHandle == CYBLE_LEDSTATE_ISBLINKING_CHAR_HANDLE )
            {
                g_shouldBlink = wrReqParam->handleValPair.value.val[0];
                BlinkTheLED();
                UpdateGATTDBWithLedState();
                CyBle_GattsWriteRsp(g_bleConnectionHandle); // Send a response.
            }
            break;
            
        default:
            break;
    }        
}



int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    CyBle_Start(BleCallBackDelegate);
    
    pwm_Start(); // Let's default to blue LED on and red LED blinking.
    // install the button ISR handler
    isrButtonPushed_StartEx( isrButtonPushed_Handler );

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    for(;;)
    {
        /* Place your application code here. */
        CyBle_ProcessEvents(); // process the BLE events.
    }
}

/* [] END OF FILE */
