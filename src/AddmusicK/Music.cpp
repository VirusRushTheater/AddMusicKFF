#include "Music.h"
#include "Utility.h"

using namespace AddMusic;

const int tmpTrans[19] 			{ 0, 0, 5, 0, 0, 0, 0, 0, 0, -5, 6, 0, -5, 0, 0, 8, 0, 0, 0 };
const int instrToSample[30] 	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x07, 0x08, 0x09, 0x05, 0x0A,	// \ Instruments
0x0B, 0x01, 0x10, 0x0C, 0x0D, 0x12, 0x0C, 0x11, 0x01,		// /
0x00, 0x00,							// Nothing
0x0F, 0x06, 0x06, 0x0E, 0x0E, 0x0B, 0x0B, 0x0B, 0x0E };		// Percussion

const int hexLengths[] 			{ 2, 2, 3, 4, 4, 1,
2, 3, 2, 3, 2, 4, 2, 2, 3, 4, 2, 4, 4, 3, 2, 4,
1, 4, 4, 3, 2, 9, 3, 4, 2, 3, 3, 2, 5, 1, 1 };

/*
 * Todo: Initialization:
	for (int z = 0; z < 19; z++)
		transposeMap[z] = tmpTrans[z];
    for (int z = 0; z < 16; z++)	// Add some spaces to the end.
		text += ' ';
*/