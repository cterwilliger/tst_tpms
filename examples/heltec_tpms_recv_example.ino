/********************************************************************************************************
    heltec_tpms_recv_example.ino

    This example uses the RadioLib library to receive and decode TST-507 TPMS signals.
    Code base copied from RadioLib SX127x Receive example.

    Board: Heltec WiFi LoRa 32 (V2)

    You must edit the table in validateID to reflect the TPMS sensors you are listening to.

    See README.md for explanation of code.

*********************************************************************************************************/

#include <RadioLib.h>

// Heltec WiFi LoRa has the following connections:
// NSS pin:   18 (CS)        GPIO18
// DIO0 pin:  26 (irq)       GPIO26
// RESET pin: 14 (RST 1278)  GPIO14
// DIO1 pin:  35 (clk)       GPIO35
// DIO2 pin:  34 (data)      GPIO34
SX1278 radio = new Module(18, 26, 14, 35);

int state;  // returned error codes

float receivedPacket = 0; // count packets
float goodCRC = 0;  // good checksums

volatile bool receivedFlag = false;    // flag to indicate that a packet was received
volatile bool enableInterrupt = true;  // disable interrupt when it's not needed

// this function is called when a complete packet is received by the module
void ICACHE_RAM_ATTR setFlag(void) 
{
    // check if the interrupt is enabled
    if ( !enableInterrupt ) 
    {
        return;
    }

    // we got a packet, set the flag
    receivedFlag = true;
} // end setFlag


/*
 * setup
 * -----
 * the usual arduino setup function
 */
void setup() 
{
    Serial.begin(9600);

    // initialize SX1278
    Serial.println("\nInitializing ... ");
    radio.beginFSK();
    radio.setOOK(true);
    radio.setFrequency(433.92);
    radio.setBitRate(19.2);
    radio.setFrequencyDeviation(50);
    radio.setRxBandwidth(125);
    radio.setCRC(false);
    radio.setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_RSSI_INTERRUPT);

    byte syncWord[] = {0xA9};
    radio.setSyncWord(syncWord, 1);
    
    radio.fixedPacketLengthMode(16);
    radio.disableAddressFiltering();
    radio.setEncoding(RADIOLIB_ENCODING_NRZ);
    radio.setOokThresholdType(RADIOLIB_SX127X_OOK_THRESH_PEAK);
    radio.setOokFixedOrFloorThreshold(0x50);   
    radio.setOokPeakThresholdDecrement(RADIOLIB_SX127X_OOK_PEAK_THRESH_DEC_1_4_CHIP);
    radio.setOokPeakThresholdStep(RADIOLIB_SX127X_OOK_PEAK_THRESH_STEP_1_5_DB);
    radio.setRSSIConfig(RADIOLIB_SX127X_RSSI_SMOOTHING_SAMPLES_8);
    radio.setDio0Action(setFlag);

    // start listening for packets
    Serial.print(F("Starting to listen ...\n"));
    state = radio.startReceive();
} // end setup


/*
 * validateID
 * ----------
 * search the array of sensor ID's that are valid
 * edit this to reflect the sensors you are listening to
 * each TST-507 sensor has an ID like "59F5FD"
 */
bool validateID(byte id1, byte id2, byte id3)
{
    char idArray[] = {0x59, 0xF5, 0xFD,
                      0x56, 0x3E, 0xD9, 
                      0x57, 0xF6, 0x39,
                      0x58, 0x03, 0xD4,
                      0x57, 0xF6, 0xDC,
                      0x57, 0xF5, 0x0F,
                      0x51, 0x45, 0xB1,
                      0x52, 0xD3, 0x9D,
                      0x52, 0xD5, 0x72,
                      0x53, 0x74, 0xAE};

    for ( int i = 0; i < 30; (i=i+3) )
    {
        if ( id1 == idArray[i] && id2 == idArray[i+1] && id3 == idArray[i+2] )
        {
            return true;
        } 
    }
    return false;
} // end validateID


/*
 * computeChecksum
 * ---------------
 * compute checksum by summing data bytes 1-7 and performing modulo 256 on the sum
 */
bool computeChecksum(uint8_t *array)
{
    uint8_t byteSum = array[1] + array[2] + array[3] + array[4] + array[5] + array[6] + array[7];

    if ( byteSum % 256 == array[0] )
    {
        goodCRC++;
        return true;
    }  
    return false;
} // end computeChecksum


/*
 * decodeManI
 * ----------
 * Manchester I (IEEE 802.3) bitstream decoder
 * Manchester encoded bit stream uses 2 bits to represent 1 bit of data.  Manchester I encodes a "1" as a transition 
 * from low to high and "0" as a transition from high to low.  Each byte decodes to 1 nibble.
 */
void decodeManI(byte *array, int size)
{ 
    // array of all possible Manchester I values
    byte manIarray[16] = {0xAA, 0xA9, 0xA6, 0xA5, 0x9A, 0x99, 0x96, 0x95, 0x6A, 0x69, 0x66, 0x65, 0x5A, 0x59, 0x56, 0x55};
    int k = 0;

    // convert each Manchester I byte to it's NRZ equivalent
    for ( int i = 0; i < size; i=i+2 ) // look at every 2 bytes in the payload
    {
        for ( int j = 0; j < 16; j++ )
        {
            if ( array[i] == manIarray[j] ) // search for ManI pattern
            {
                array[k] = j << 4; // shift left one nibble, put back into passed array
            }
        } 

        for ( int j = 0; j < 16; j++ )
        {
            if ( array[i+1] == manIarray[j] )
            {
                array[k] = array[k] | j; // combine previous nibble with this one
            }
        }
        k++;
    }

} // end decodeManI


/*
 * shiftBlockRight
 * ---------------
 *  bit shift entire payload right by 2 bits and pre-pend 0b01 to restore what the packet engine removed (sync word)
 */
void shiftBlockRight(byte *inBytes, byte *outBytes, int size, short bitShift)
{
    for (int index=0; index < size; index++)
    {
        byte newByteMSB = (byte)(inBytes[index] >> bitShift);
        byte newByteLSB = (byte)((index==0)?((byte)0x01):(inBytes[index-1]));       
        newByteLSB = (byte) (newByteLSB << (8-bitShift));
        outBytes[index] = (byte) ( newByteMSB | newByteLSB);
    }
    return;
} // end shiftBlockRight


/*
 * loop
 * ----
 * the usual arduino loop function
 */
void loop() 
{
    // check if the flag is set
    if ( receivedFlag ) 
    {
        // disable the interrupt service routine while processing the data
        enableInterrupt = false;

        // reset flag
        receivedFlag = false;

        // read data packet
        byte byteArr[16];
        int state = radio.readData(byteArr, 16);

        if ( state == RADIOLIB_ERR_NONE ) 
        {
            byte newArr[16];
            shiftBlockRight(byteArr, newArr, 16, 2);      
            
            // decode manchester I
            decodeManI(newArr, 16);

            // only pass sensor ID's we want
            if ( validateID(newArr[1], newArr[2], newArr[3]) )
            {
                // packet was successfully received
                receivedPacket++;

                // Sensor ID
                Serial.printf("Sensor:\t\t%02X%02X%02X\n", newArr[1], newArr[2], newArr[3]);

                // compute the checksum
                if ( computeChecksum(newArr) )
                {
                    Serial.println("Good Checksum!");
                } 
                else 
                {
                    Serial.println("BAD Checksum.");
                }

                // print Pressure (2nd nibble of byte 7 + byte 4)
                float press = (newArr[7] & 0x0F) << 8 | newArr[4];
                press = (press / 0.4) / 6.895;  // kPa then psi
                Serial.printf("Pressure:\t%3.1f psi\n", press);

                // print Temperature (byte 5)
                float temp = ((newArr[5] - 50) * 1.8) + 32;  // deg C to F
                Serial.printf("Temperature:\t%3.1f F\n", temp);

                // print RSSI (Received Signal Strength Indicator)
                Serial.printf("RSSI:\t\t%3.1f dBm\n", radio.getRSSI());

                // print good packet %
                Serial.printf("Good Packets:\t%3.1f %%\n", (goodCRC/receivedPacket)*100);
                Serial.println();
            }
        }
        else
        {
            // some other error occurred
            Serial.printf("Failed, code ", state);
        }

        // put module back to listen mode
        radio.startReceive();

        // we're ready to receive more packets,
        // enable interrupt service routine
        enableInterrupt = true;
    }
} // end loop
