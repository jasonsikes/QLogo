#ifndef EXPORTS_H
#define EXPORTS_H

#include "compiler_types.h"
#include "visited.h"

#include <QString>

#define generateCallExtern(RET_TYPE, FUNC_NAME, ...) \
    ((void)FUNC_NAME, generateExternFunctionCall(RET_TYPE, #FUNC_NAME, {__VA_ARGS__}))

#ifdef _WIN32
#define EXPORTC extern "C" __declspec(dllexport)
#else
#define EXPORTC extern "C"
#endif

bool isDatumInContainer(VisitedSet &visited, Datum *value, Datum *container, Qt::CaseSensitivity cs);

EXPORTC void printInt(int32_t p);
EXPORTC double getDoubleForDatum(addr_t eAddr, addr_t datumAddr);
EXPORTC bool getValidityOfDoubleForDatum(addr_t eAddr, addr_t datumAddr);
EXPORTC bool getBoolForDatum(addr_t eAddr, addr_t datumAddr);
EXPORTC bool getValidityOfBoolForDatum(addr_t eAddr, addr_t datumAddr);
EXPORTC addr_t getDatumForVarname(addr_t wordAddr);
EXPORTC addr_t stdWriteDatum(addr_t datumAddr, bool useShow);
EXPORTC addr_t stdWriteDatumAry(addr_t datumAddr, uint32_t count, bool useShow, bool addWhitespace);
EXPORTC addr_t getWordForDouble(addr_t eAddr, double val);
EXPORTC addr_t getWordForBool(addr_t eAddr, bool val);
EXPORTC void setDatumForWord(addr_t datumAddr, addr_t wordAddr);
EXPORTC addr_t runList(addr_t eAddr, addr_t listAddr);
EXPORTC addr_t runProcedure(addr_t eAddr, addr_t astnodeAddr, addr_t paramAryAddr, uint32_t paramCount);
EXPORTC addr_t getErrorSystem(addr_t eAddr);
EXPORTC addr_t getErrorNoLike(addr_t eAddr, addr_t whoAddr, addr_t whatAddr);
EXPORTC addr_t getErrorNoSay(addr_t eAddr, addr_t whatAddr);
EXPORTC addr_t getErrorNoTest(addr_t eAddr, addr_t whoAddr);
EXPORTC addr_t getErrorNoOutput(addr_t eAddr, addr_t xAddr, addr_t yAddr);
EXPORTC addr_t getErrorNotEnoughInputs(addr_t eAddr, addr_t xAddr);
EXPORTC addr_t getErrorNoValue(addr_t eAddr, addr_t whatAddr);
EXPORTC addr_t getErrorCustom(addr_t eAddr, addr_t tagAddr, addr_t outputAddr);
EXPORTC addr_t getCtrlReturn(addr_t eAddr, addr_t astNodeAddr, addr_t retvalAddr);
EXPORTC addr_t getCtrlContinuation(addr_t eAddr, addr_t astNodeAddr, addr_t paramAryAddr, uint32_t paramCount);
EXPORTC addr_t getCtrlGoto(addr_t eAddr, addr_t astNodeAddr, addr_t tagAddr);
EXPORTC int32_t getCountOfList(addr_t listAddr);
EXPORTC int32_t getNumberAryFromList(addr_t eAddr, addr_t listAddr, addr_t destAddr);
EXPORTC double random1(int32_t num);
EXPORTC double random2(int32_t start, int32_t end);
EXPORTC addr_t setRandomWithSeed(int32_t seed);
EXPORTC addr_t setRandom();
EXPORTC addr_t getFormForNumber(addr_t eAddr, double num, uint32_t width, int32_t precision);
EXPORTC addr_t repcountAddr(void);
EXPORTC addr_t beginCatch(addr_t eAddr);
EXPORTC addr_t endCatch(addr_t eAddr, addr_t nodeAddr, addr_t errActAddr, addr_t resultAddr, addr_t tagAddr);
EXPORTC addr_t getCurrentError(addr_t eAddr);
EXPORTC addr_t callPause(addr_t eAddr);
EXPORTC addr_t generateContinue(addr_t eAddr, addr_t outputAddr);
EXPORTC addr_t processRunresult(addr_t eAddr, addr_t resultAddr);
EXPORTC void saveTestResult(addr_t eAddr, bool tf);
EXPORTC bool getIsTested(addr_t eAddr);
EXPORTC bool getTestResult(addr_t eAddr);
EXPORTC bool cmpDatumToBool(addr_t d, bool b);
EXPORTC bool cmpDatumToDouble(addr_t d, double n);
EXPORTC bool cmpDatumToDatum(addr_t eAddr, addr_t d1, addr_t d2);
EXPORTC addr_t concatWord(addr_t eAddr, addr_t aryAddr, uint32_t count);
EXPORTC bool isDatumEmpty(addr_t eAddr, addr_t dAddr);
EXPORTC addr_t createList(addr_t eAddr, addr_t aryAddr, uint32_t count);
EXPORTC addr_t createSentence(addr_t eAddr, addr_t aryAddr, uint32_t count);
EXPORTC addr_t fputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr);
EXPORTC addr_t lputList(addr_t eAddr, addr_t thingAddr, addr_t listAddr);
EXPORTC addr_t createArray(addr_t eAddr, int32_t size, int32_t origin);
EXPORTC addr_t listToArray(addr_t eAddr, addr_t listAddr, int32_t origin);
EXPORTC addr_t arrayToList(addr_t eAddr, addr_t arrayAddr);
EXPORTC addr_t firstOfDatum(addr_t eAddr, addr_t thingAddr);
EXPORTC addr_t lastOfDatum(addr_t eAddr, addr_t thingAddr);
EXPORTC addr_t butFirstOfDatum(addr_t eAddr, addr_t thingAddr);
EXPORTC addr_t butLastOfDatum(addr_t eAddr, addr_t thingAddr);
EXPORTC bool isDatumIndexValid(addr_t eAddr, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr);
EXPORTC addr_t itemOfDatum(addr_t eAddr, addr_t thingAddr, double dIndex, addr_t listItemPtrAddr);
EXPORTC bool isDatumContainerOrInContainer(addr_t eAddr, addr_t valueAddr, addr_t containerAddr);
EXPORTC void setDatumAtIndexOfContainer(addr_t eAddr, addr_t valueAddr, double dIndex, addr_t containerAddr);
EXPORTC void setFirstOfList(addr_t eAddr, addr_t listAddr, addr_t valueAddr);
EXPORTC void setButfirstOfList(addr_t eAddr, addr_t listAddr, addr_t valueAddr);
EXPORTC bool isEmpty(addr_t eAddr, addr_t thingAddr);
EXPORTC bool isBefore(addr_t eAddr, addr_t word1Addr, addr_t word2Addr);
EXPORTC bool isMember(addr_t eAddr, addr_t thingAddr, addr_t containerAddr);
EXPORTC bool isSubstring(addr_t eAddr, addr_t thing1Addr, addr_t thing2Addr);
EXPORTC bool isNumber(addr_t eAddr, addr_t thingAddr);
EXPORTC bool isSingleCharWord(addr_t eAddr, addr_t candidateAddr);
EXPORTC bool isVbarred(addr_t eAddr, addr_t cAddr);
EXPORTC double datumCount(addr_t eAddr, addr_t thingAddr);
EXPORTC double ascii(addr_t eAddr, addr_t cAddr);
EXPORTC double rawascii(addr_t eAddr, addr_t cAddr);
EXPORTC addr_t chr(addr_t eAddr, uint32_t c);
EXPORTC addr_t member(addr_t eAddr, addr_t thing1Addr, addr_t thing2Addr);
EXPORTC addr_t lowercase(addr_t eAddr, addr_t wordAddr);
EXPORTC addr_t uppercase(addr_t eAddr, addr_t wordAddr);
EXPORTC addr_t standout(addr_t eAddr, addr_t thingAddr);
EXPORTC addr_t parse(addr_t eAddr, addr_t wordAddr);
EXPORTC addr_t runparseDatum(addr_t eAddr, addr_t wordorlistAddr);
EXPORTC void moveTurtleForward(addr_t eAddr, double distance);
EXPORTC void moveTurtleRotate(addr_t eAddr, double angle);
EXPORTC void setTurtleXY(addr_t eAddr, double x, double y);
EXPORTC void setTurtleX(addr_t eAddr, double x);
EXPORTC void setTurtleY(addr_t eAddr, double y);
EXPORTC void setTurtlePos(addr_t eAddr, addr_t posAddr);
EXPORTC void setTurtleHeading(addr_t eAddr, double newHeading);
EXPORTC void setTurtleMoveToHome(addr_t eAddr);
EXPORTC void drawTurtleArc(addr_t eAddr, double angle, double radius);
EXPORTC addr_t getTurtlePos(addr_t eAddr);
EXPORTC double getTurtleHeading(addr_t eAddr);
EXPORTC double getTurtleTowards(addr_t eAddr, addr_t posAddr);
EXPORTC addr_t getScrunch(addr_t eAddr);
EXPORTC void setTurtleVisible(addr_t eAddr, int visible);
EXPORTC void clean(addr_t eAddr);
EXPORTC void setTurtleMode(addr_t eAddr, int mode);
EXPORTC addr_t getBounds(addr_t eAddr);
EXPORTC void setBounds(addr_t eAddr, double x, double y);
EXPORTC int32_t beginFilledWithColor(addr_t eAddr, addr_t colorAddr);
EXPORTC void endFilled(addr_t eAddr);
EXPORTC void addLabel(addr_t eAddr, addr_t textAddr);
EXPORTC void setLabelHeight(addr_t eAddr, double height);
EXPORTC void setScreenMode(addr_t eAddr, int mode);
EXPORTC bool isTurtleVisible(addr_t eAddr);
EXPORTC addr_t getScreenMode(addr_t eAddr);
EXPORTC addr_t getTurtleMode(addr_t eAddr);
EXPORTC addr_t getLabelSize(addr_t eAddr);
EXPORTC void setPenIsDown(addr_t eAddr, bool isDown);
EXPORTC void setPenMode(addr_t eAddr, int32_t mode);
EXPORTC bool setPenColor(addr_t eAddr, addr_t colorAddr);
EXPORTC addr_t getAllColors(addr_t eAddr);
EXPORTC bool isColorIndexGood(addr_t eAddr, addr_t colorIndexAddr, double lowerLimit);
EXPORTC bool setPalette(addr_t eAddr, addr_t colorIndexAddr, addr_t colorAddr);
EXPORTC void setPenSize(addr_t eAddr, double size);
EXPORTC bool setBackground(addr_t eAddr, addr_t colorAddr);
EXPORTC bool isPenDown(addr_t eAddr);
EXPORTC addr_t getPenMode(addr_t eAddr);
EXPORTC addr_t getPenColor(addr_t eAddr);
EXPORTC addr_t getPaletteColor(addr_t eAddr, addr_t colorIndexAddr);
EXPORTC double getPenSize(addr_t eAddr);
EXPORTC addr_t getBackground(addr_t eAddr);
EXPORTC addr_t savePict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr);
EXPORTC addr_t saveSvgpict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr);
EXPORTC addr_t loadPict(addr_t eAddr, addr_t filenameAddr, addr_t nodeAddr);
EXPORTC addr_t getMousePos(addr_t eAddr);
EXPORTC addr_t getClickPos(addr_t eAddr);
EXPORTC bool isMouseButtonDown(addr_t eAddr);
EXPORTC double getMouseButton(addr_t eAddr);
EXPORTC bool getvarErroract(addr_t eAddr);
EXPORTC addr_t inputProcedure(addr_t eAddr, addr_t nodeAddr);
EXPORTC void setVarAsLocal(addr_t varname);

#endif // EXPORTS_H