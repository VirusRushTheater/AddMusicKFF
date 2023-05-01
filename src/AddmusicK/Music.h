#pragma once

#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <initializer_list>

#include "MMLBase.h"
#include "Sample.h"

namespace AddMusic
{

namespace fs = std::filesystem;

class SPCEnvironment;

struct SpaceInfo
{
	int songStartPos;
	int songEndPos;
	int sampleTableStartPos;
	int sampleTableEndPos;
	std::vector<int> individualSampleStartPositions;
	std::vector<int> individualSampleEndPositions;
	std::vector<bool> individialSampleIsImportant;
	int importantSampleCount;
	int echoBufferEndPos;
	int echoBufferStartPos;
};

class Music : public MMLBase
{
	friend class SPCEnvironment;

public:
	// Music();

	void compile(SPCEnvironment* spc_);

private:
	// =======================================================================
	// PRIVATE ATTRIBUTES
	// =======================================================================

	unsigned int errorCount 		{0};

	// Music::Music initializing attributes.
	bool knowsLength 				{false};
	bool playOnce 					{false};
	int totalSize 					{0};
	int spaceForPointersAndInstrs 	{0};
	bool exists 					{false};
	int echoBufferSize 				{0};
	bool hasEchoBufferCommand 		{false};
	bool echoBufferAllocVCMDIsSet 	{false};
	int noteParamaterByteCount 		{0};

	// Music::init initializing attributes
	bool hasYoshiDrums 				{false};
	bool guessLength 				{true};

	double introSeconds;
	double mainSeconds;

	int tempoRatio					{1};
	bool nextHexIsArpeggioNoteLength {false};

	std::string pathlessSongName;
	fs::path basepath;

	std::vector<uint8_t> data[9];
	std::vector<unsigned short> loopLocations[9];	// With remote loops, we can have remote loops in standard loops, so we need that ninth channel.
	
	// SPC header info
	std::string title;
	std::string author;
	std::string game;
	std::string comment;
	
	unsigned short loopPointers[0x10000];
	
	std::vector<uint8_t> allPointersAndInstrs;
	std::vector<uint8_t> instrumentData;
	std::vector<uint8_t> finalData;

	SpaceInfo spaceInfo;

	unsigned int introLength;
	unsigned int mainLength;
	unsigned int seconds 			{0};

	int index;											// Song index. Defined externally.

	std::vector<unsigned short> mySamples;				// Binary representation of samples
	
	unsigned short echoBufferAllocVCMDLoc;				// Defined on markEchoBufferAllocVCMD
	int echoBufferAllocVCMDChannel;						// Defined on markEchoBufferAllocVCMD

	std::string statStr;								// Printable stats.

	int minSize;										// Defined while parsing pad definition
	
	int posInARAM;										// Position in ARAM. Defined externally.

	int remoteDefinitionType 		{0};
	bool inRemoteDefinition 		{false};
	//int remoteDefinitionArg;

	std::map<std::string, std::string> replacements;
	std::vector<const std::pair<const std::string, std::string> *> sortedReplacements;

	int resizedChannel;

	double channelLengths[8] 		{0};				// How many ticks are in each channel.
	double loopLengths[0x10000] 	{0};				// How many ticks are in each loop.
	double normalLoopLength 		{0};				// How many ticks were in the most previously declared normal loop.
	double superLoopLength 			{0};				// How many ticks were in the most previously declared super loop.
	std::vector<std::pair<double, int>> tempoChanges;	// Where any changes in tempo occur. A negative tempo marks the beginning of the main loop, if an intro exists.

	bool baseLoopIsNormal 			{false};
	bool baseLoopIsSuper 			{false};
	bool extraLoopIsNormal 			{false};
	bool extraLoopIsSuper 			{false};

	// =======================================================================
	// INTERNAL PARSER ATTRIBUTES (formerly static on Music.cpp)
	// Defined here for thread-safety among other reasons.
	// =======================================================================
	
	// These are already defined in MMLBase.
	// unsigned int pos 				{0};
	// int line 						{1};

	int channel 					{0};
	int prevChannel;
	int octave 						{4};
	int prevNoteLength 				{-1};
	int defaultNoteLength 			{192/8};
	
	bool inDefineBlock 				{false};

	int prevLoop 					{-1};
	int i 							{0};
	int j 							{0};

	bool noMusic[8][2]				{0};
	bool passedIntro[8]				{0};
	bool passedNote[8]				{0};
	unsigned short phrasePointers[8][2] {0};

	int q[9]						{0};
	int instrument[9]				{0};
	bool updateQ[9]					{0};
	bool usingFA[9]					{0};
	bool usingFC[9]					{0};
	int lastFAGainValue[9]			{0};
	//int lastFADelayValue[8];
	int lastFCGainValue[9]			{0};
	int lastFCDelayValue[9]			{0};

	bool hasIntro					{false};
	bool doesntLoop					{false};
	bool triplet					{false};
	bool inPitchSlide				{false};

	int loopLabel 					{0};
	int currentHex 					{0};
	int hexLeft 					{0};

	int transposeMap[256]			{0};
	bool usedSamples[256]			{0};		// Holds a record of which samples have been used for this song.

	bool ignoreTuning[9];	// Used for AM4 compatibility.  Until an instrument is explicitly declared on a channel, it must not use tuning.

	int songTargetProgram 			{0};	// 0 = indeterminate/unknown/AMK, 1 = AM4, 2 = AMM.
	int targetAMKVersion;
	
	int hTranspose 					{0};
	bool usingHTranspose 			{false};
	
	int currentHexSub 				{-1};

	//int tempLoopLength;		// How long the current [ ] loop is.
	//int e6LoopLength;		// How long the current $E6 loop is.
	//int previousLoopLength;	// How long the last encountered loop was.
	bool inE6Loop 					{false};			// Whether or not we're in an $E6 loop.
	
	int loopNestLevel				{0};				// How deep we're "loop nested".
	
	// If this is 0, then any type of loop is allowed.
	// If this is 1 and we're in a normal loop, then normal loops are disallowed and $E6 loops are allowed.
	// If this is 1 and we're in an $E6 loop, then $E6 loops are disallowed and normal loops are allowed.
	// If this is 2, then no new loops are allowed.

	//unsigned int lengths[8];		// How long each channel is.

	// Music::init initializing attributes
	unsigned int tempo 				{0x36};
	//bool onlyHadOneTempo;
	bool tempoDefined 				{false};

	bool sortReplacements 			{true};
	bool manualNoteWarning 			{true};
	bool manualDurQuantWarning 		{true};
	bool manualPhraseEndWarning 	{true};
	bool nonNativeHexWarning 		{true};
	bool nonNativeCmdWarning 		{true};
	bool caseNoteWarning 			{true};
	bool octaveForDDWarning 		{true};
	bool remoteGainWarning 			{true};
	bool fractionNoteLengthWarning 	{true};
	bool lowNoteWarning 			{true};

	bool channelDefined 			{false};

	bool usingSMWVTable				{true};		// Changed at initialization. Depends on the AMK version of this song.

	// =======================================================================
	// PRIVATE METHODS
	// =======================================================================
	void _init();

	bool doReplacement();
	int divideByTempoRatio(int, bool fractionIsError);	// Divides a value by tempoRatio. Errors out if it can't be done without a decimal (if the parameter is set).

	int multiplyByTempoRatio(int); 		// Multiplies a value by tempoRatio. Errors out if it goes higher than 255.

	void pointersFirstPass();
	void parseComment();
	void parseQMarkDirective();
	void parseExMarkDirective();
	void parseChannelDirective();
	void parseLDirective();
	void parseGlobalVolumeCommand();
	void parseVolumeCommand();
	void parseQuantizationCommand();
	void parsePanCommand();
	void parseIntroDirective();
	void parseT();
	void parseTempoCommand();
	void parseTransposeDirective();
	void parseOctaveDirective();
	void parseInstrumentCommand();
	void parseOpenParenCommand();
	void parseLabelLoopCommand();
	void parseSampleLoadCommand();
	void parseLoopCommand();
	void parseLoopEndCommand();
	void parseStarLoopCommand();
	void parseVibratoCommand();
	void parseTripletOpenDirective();
	void parseTripletCloseDirective();
	void parseRaiseOctaveDirective();
	void parseLowerOctaveDirective();
	void parsePitchSlideCommand();
	void parseHexCommand();
	void parseNote();
	void parseHDirective();
	void parseReplacementDirective();
	void parseNCommand();

	void parseOptionDirective();

	void parseSpecialDirective();
	void parseInstrumentDefinitions();
	void parseSampleDefinitions();
	void parsePadDefinition();
	void parseASMCommand();
	void parseJSRCommand();
	void parseLouderCommand();
	void parseTempoImmunityCommand();
	void parsePath();
	void compileASM();

	void parseDefine();
	void parseIfdef();
	void parseIfndef();
	void parseEndif();
	void parseUndef();

	void parseSPCInfo();

	void printChannelDataNonVerbose(int);
	void parseHFDHex();
	void parseHFDInstrumentHack(int addr, int bytes);
	void insertedZippedSamples(const std::string &path);

	int getInt();
	int getInt(const std::string &str, int &p);
	int getIntWithNegative();
	int getHex(bool anyLength = false);
	int getPitch(int j);
	int getNoteLength(int);
	int getNoteLengthModifier(int, bool);

	void handleNormalLoopEnter();						// Call any time a definition of a loop is entered.
	void handleSuperLoopEnter();						// Call any time a definition of a super loop is entered.
	void handleNormalLoopRemoteCall(int loopCount);		// Call any time a normal loop is called remotely.
	void handleNormalLoopExit(int loopCount);			// Call any time a definition of a loop is exited.
	void handleSuperLoopExit(int loopCount);			// Call any time a definition of a super loop is exited.

	void addNoteLength(double ticks);					// Call this every note.  The correct channel/loop will be automatically updated.
	
	void markEchoBufferAllocVCMD();						// Called when the Hot Patch VCMD is manually defined. Required because of a bit that handles a special case when the echo buffer size is zero.

	// Ported from globals.cpp
	fs::path _resolvePath(const fs::path &fileName);

	void addSample(const fs::path &fileName, bool important);
	void addSample(const std::vector<uint8_t> &sample, const std::string &name, bool important, bool noLoopHeader, int loopPoint = 0, bool isBNK = false);
	void addSampleGroup(const fs::path &groupName);
	void addSampleBank(const fs::path &fileName);
	int getSample(const fs::path &name);

	inline void append (uint8_t value)
	{
		data[channel].push_back(value);
	}

	inline void append (std::initializer_list<uint8_t> values)
	{
		data[channel].insert(data[channel].end(), values);
	}

	inline void musicError(const std::string& msg)
	{
		Logging::error(msg, this);
		errorCount ++;
	}
};

}