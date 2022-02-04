/******************************************************************************************
    CC1101_tpms_recv_example.ino

    CPU: Heltec WiFi ESP32
    CC1101: Solu board

    This example uses the RadioLib library to receive and decode TST-507 TPMS signals.
    Code base copied from RadioLib CC1101 Receive example.

    Board: Heltec WiFi LoRa 32 (V2)

    You must edit the table in validateID to reflect the TPMS sensors you are listening to.

    See README.md for explanation of code.

*******************************************************************************************/

// include the library
#include <RadioLib.h>

// Solu CC1101 has the following connections:
// Solu       ESP32
//  1 -  GND  - GND blk
//  2 -  Vcc  - 3v3 red
//  3 - GDO0  -  2  wht
//  4 -  CSN  -  5  brn
//  5 - SCLK  - 18  yel
//  6 -   SI  - 23  grn
//  7 -   SO  - 19  blu
//  8 - GDO2  - 12  vio
CC1101 radio = new Module(5, 2, RADIOLIB_NC, 12);  // cs, irq, reset, opt gpio

float receivedPacket = 0; // count packets
float goodCRC = 0;  // good checksums

volatile bool receivedFlag = false;  // flag to indicate that a packet was received
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
}


/*
 * setup
 * --------------------------------
 * the usual arduino setup function
 */
void setup() 
{
    Serial.begin(9600);

    // initialize CC1101
    Serial.println("\nInitializing ... ");
    radio.begin();
    radio.setOOK(true);
    radio.setFrequency(433.92);
    radio.setBitRate(19.2);
    radio.setFrequencyDeviation(50);
    radio.setRxBandwidth(125);
    radio.setCrcFiltering(false);
    
    radio.setSyncWord(0xAA, 0xA9, 1, true);

    radio.fixedPacketLengthMode(16);
    radio.disableAddressFiltering();
    radio.setEncoding(RADIOLIB_ENCODING_NRZ);
    radio.setGdo0Action(setFlag);

    // start listening for packets
    Serial.println("Starting to listen ... ");
    radio.startReceive();
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
                receivedPacket++;   // global, all sensors

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

                // print LQI (Link Quality Indicator)
                // of the last received packet, lower is better
                Serial.printf("LQI:\t\t%u\n", radio.getLQI());

                // print good packet %
                Serial.printf("Good Packets:\t%3.1f %%\n", (goodCRC/receivedPacket)*100);
                
                Serial.println();
            }
        } 
        else 
        {
            // some other error occurred
            Serial.printf("failed, code ", state);
        }

        // put module back to listen mode
        radio.startReceive();

        // we're ready to receive more packets,
        // enable interrupt service routine
        enableInterrupt = true;
    } // end loop

}