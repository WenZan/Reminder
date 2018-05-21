#include "SFEMP3Shield.h"
// inslude the SPI library:
#include "SPI.h"
//avr pgmspace library for storing the LUT in program flash instead of sram
#include <avr/pgmspace.h>

//bitrate lookup table      V1,L1  V1,L2   V1,L3   V2,L1  V2,L2+L3
//168 bytes(!!); better to store in progmem or eeprom
PROGMEM prog_uint16_t bitrate_table[14][6] = { {0,0,0,0,0,0},
					       {32,32,32,32,8,8}, //0001
					       {64,48,40,48,16,16}, //0010
					       {96,56,48,56,24,24}, //0011
					       {128,64,56,64,32,32}, //0100
					       {160,80,64,80,40,40}, //0101
					       {192,96,80,96,48,48}, //0110
					       {224,112,96,112,56,56}, //0111
					       {256,128,112,128,64,64}, //1000
					       {288,160,128,144,80,80}, //1001
					       {320,192,160,160,96,69}, //1010
					       {352,224,192,176,112,112}, //1011
					       {384,256,224,192,128,128}, //1100
					       {416,320,256,224,144,144} };//1101


void SFEMP3Shield::SetVolume(unsigned char leftchannel, unsigned char rightchannel){
	
	VolL = leftchannel;
	VolR = rightchannel;

    Mp3WriteRegister(SCI_VOL, leftchannel, rightchannel);
}


//call on a mp3 just with a number
uint8_t SFEMP3Shield::playTrack(uint8_t trackNo){
	
	//a storage place for track names
	char trackName[] = "track001.mp3";
	uint8_t trackNumber = 1;
	
	//tack the number onto the rest of the filename
	sprintf(trackName, "track%03d.mp3", trackNo);
	
	//play the file
    return (SFEMP3Shield::playMP3(trackName));
}


uint8_t SFEMP3Shield::playMP3(char* fileName){

	if (playing) return 1;

	//Open the file in read mode.
	if (!track.open(&root, fileName, O_READ)) return 2;
	   
	playing = TRUE;
	
	//look for first MP3 frame (11 1's)
	bitrate = 0;
	uint8_t temp = 0;
	uint8_t row_num =0;
	
	
	for(uint16_t i = 0; i<65535; i++){
	//for(;;){
		if(track.read() == 0xFF) {
			
			temp = track.read();
			
			if(((temp & 0b11100000) == 0b11100000) && ((temp & 0b00000110) != 0b00000000)) {

				//found the 11 1's
				//parse version, layer and bitrate out and save bitrate
				if(!(temp & 0b00001000)) //!true if Version 1, !false version 2 and 2.5
					row_num = 3;
			    if((temp & 0b00000110) == 0b00000100) //true if layer 2, false if layer 1 or 3
					row_num += 1;
				else if((temp & 0b00000110) == 0b00000010) //true if layer 3, false if layer 2 or 1	
					row_num += 2;
				
				//parse bitrate code from next byte
				temp = track.read();
				temp = temp>>4;
				
				//lookup bitrate
				bitrate = pgm_read_word_near ( temp*5 + row_num );
				//							      bitrate_table[temp][row_num];
				
				//convert kbps to Bytes per mS
				bitrate /= 8;
				
				//record file position
				track.seekCur(-3);
				start_of_music = track.curPosition();
				
				//Serial.print("POS: ");
				//Serial.println(start_of_music);
				
				//Serial.print("Bitrate: ");
				//Serial.println(bitrate);
				
				//break out of for loop
				break;
			
			}
		    
		}
	}
	
	  
	//gotta start feeding that hungry mp3 chip
	refill();
	  
	//attach refill interrupt off DREQ line, pin 2
	attachInterrupt(0, refill, RISING);
	  
	return 0;
}

//close track gracefully, cancel intterupt
void SFEMP3Shield::stopTrack(){
  
	if(playing == FALSE)
		return;
  
	//cancel external interrupt
	detachInterrupt(0);
	playing=FALSE;

	//tell MP3 chip to do a soft reset. Fixes garbles at end, and clears its buffer. 
	//easier then the way your SUPPOSE to do it by the manual, same result as much as I can tell.
	Mp3WriteRegister(SCI_MODE, 0x48, SM_RESET);
	  
	track.close(); //Close out this track
	
	  
	//Serial.println("Track is done!");
  
}

//is there a song playing?
uint8_t SFEMP3Shield::isPlaying(){
  
	if(playing == FALSE)
		return 0;
	else
		return 1;
}

void SFEMP3Shield::trackArtist(char* infobuffer){
	getTrackInfo(TRACK_ARTIST, infobuffer);
}

void SFEMP3Shield::trackTitle(char* infobuffer){
	getTrackInfo(TRACK_TITLE, infobuffer);
}

void SFEMP3Shield::trackAlbum(char* infobuffer){
	getTrackInfo(TRACK_ALBUM, infobuffer);
}

//reads and returns the track tag information
void SFEMP3Shield::getTrackInfo(uint8_t offset, char* infobuffer){

	//disable interupts
	pauseDataStream();
	
	//record current file position
	uint32_t currentPos = track.curPosition();
	
	//skip to end
	track.seekEnd((-128 + offset));
	
	//read 30 bytes of tag informat at -128 + offset
	track.read(infobuffer, 30);
	
	//seek back to saved file position
	track.seekSet(currentPos);
	
	//renable interupt
	resumeDataStream();
	
}

//cancels interrupt feeding MP3 decoder
void SFEMP3Shield::pauseDataStream(){

	//cancel external interrupt
	if(playing)
		detachInterrupt(0);

}

//resumes interrupt feeding MP3 decoder
void SFEMP3Shield::resumeDataStream(){

	//make sure SPI is right speed
	SPI.setDataMode(SPI_MODE0);

	if(playing)	{
		//see if it is already ready for more
		refill();

		//attach refill interrupt off DREQ line, pin 2
		attachInterrupt(0, refill, RISING);
	}

}

//skips to a certain point in th track
bool SFEMP3Shield::skipTo(uint32_t timecode){

	if(playing) {
	
		//stop interupt for now
		detachInterrupt(0);
		playing=FALSE;
		
		//seek to new position in file
		if(!track.seekSet((timecode * bitrate) + start_of_music))
			return 2;
			
		Mp3WriteRegister(SCI_VOL, 0xFE, 0xFE);
		//seeked successfully
		
		//tell MP3 chip to do a soft reset. Fixes garbles at end, and clears its buffer. 
	    //easier then the way your SUPPOSE to do it by the manual, same result as much as I can tell.
	    Mp3WriteRegister(SCI_MODE, 0x48, SM_RESET);
		
		//gotta start feeding that hungry mp3 chip
		refill();
		
		//again, I'm being bad and not following the spec sheet.
		//I already turned the volume down, so when the MP3 chip gets upset at me
		//for just slammin in new bits of the file, you won't hear it. 
		//so we'll wait a bit, and restore the volume to previous level
		delay(50);
		
		//one of these days I'll come back and try to do it the right way.
		SFEMP3Shield::SetVolume(VolL,VolR);
		  
		//attach refill interrupt off DREQ line, pin 2
		attachInterrupt(0, refill, RISING);
		playing=TRUE;
		
		return 0;
	}
	
	return 1;
}

//returns current timecode in ms. Not very accurate/detministic 
uint32_t SFEMP3Shield::currentPosition(){

	return((track.curPosition() - start_of_music) / bitrate );
}

//force bit rate, useful if auto-detect failed
void SFEMP3Shield::setBitRate(uint16_t bitr){

	bitrate = bitr;
	return;
}

void Mp3WriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte){

	//cancel interrupt if playing
	if(playing)
		detachInterrupt(0);
	
	//Wait for DREQ to go high indicating IC is available
	while(!digitalRead(MP3_DREQ)) ; 
	//Select control
	digitalWrite(MP3_XCS, LOW); 

	//SCI consists of instruction byte, address byte, and 16-bit data word.
	SPI.transfer(0x02); //Write instruction
	SPI.transfer(addressbyte);
	SPI.transfer(highbyte);
	SPI.transfer(lowbyte);
	while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete
	digitalWrite(MP3_XCS, HIGH); //Deselect Control
	
	//resume interrupt if playing. 
	if(playing)	{
		//see if it is already ready for more
		refill();

		//attach refill interrupt off DREQ line, pin 2
		attachInterrupt(0, refill, RISING);
	}
	
}

//Read the 16-bit value of a VS10xx register
unsigned int Mp3ReadRegister (unsigned char addressbyte){
  
	//cancel interrupt if playing
	if(playing)
		detachInterrupt(0);
	  
	while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating IC is available
	digitalWrite(MP3_XCS, LOW); //Select control

	//SCI consists of instruction byte, address byte, and 16-bit data word.
	SPI.transfer(0x03);  //Read instruction
	SPI.transfer(addressbyte);

	char response1 = SPI.transfer(0xFF); //Read the first byte
	while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete
	char response2 = SPI.transfer(0xFF); //Read the second byte
	while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete

	digitalWrite(MP3_XCS, HIGH); //Deselect Control

	unsigned int resultvalue = response1 << 8;
	resultvalue |= response2;
	return resultvalue;
  
	//resume interrupt if playing. 
	if(playing)	{
		//see if it is already ready for more
		refill();

		//attach refill interrupt off DREQ line, pin 2
		attachInterrupt(0, refill, RISING);
	}
}







































