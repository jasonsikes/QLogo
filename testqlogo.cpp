//===-- qlogo/testqlogo.cpp -------*- C++ -*-===//
//
// This file is part of QLogo.
//
// QLogo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// QLogo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with QLogo.  If not, see <http://www.gnu.org/licenses/>.
//

#include CONTROLLER_HEADER
#include "kernel.h"
#include "turtle.h"
#include <QDateTime>
#include <QtTest>

class TestQLogo : public QObject {
  Q_OBJECT
  qint64 startTime;

public:
  TestQLogo();
  ~TestQLogo();

private Q_SLOTS:
  void testKernel_data();
  void testKernel();
};

TestQLogo::TestQLogo() { startTime = QDateTime::currentMSecsSinceEpoch(); }

TestQLogo::~TestQLogo() {
  qint64 endTime = QDateTime::currentMSecsSinceEpoch();
  qint64 elapsedTime = endTime - startTime;
  qDebug() << "Total elapsed time (in msec):" << elapsedTime;
  qDebug() << "sizeof float:" << sizeof(float);
  qDebug() << "sizeof double:" << sizeof(double);
  qDebug() << "sizeof int:" << sizeof(int);
  qDebug() << "sizeof long:" << sizeof(long);
}

void TestQLogo::testKernel() {
  Controller c;
  QFETCH(QString, input);
  QFETCH(QString, expectedOuput);
  QString output = c.run(input);
  QCOMPARE(output, expectedOuput);
}

void TestQLogo::testKernel_data() {
  QTest::addColumn<QString>("input");
  QTest::addColumn<QString>("expectedOuput");

  QTest::newRow("print number") << "print 2000\n"
                                << "2000\n";

  QTest::newRow("type number") << "type 2000\n"
                               << "2000";

  QTest::newRow("type word") << "type \"qwerty\n"
                             << "qwerty";

  QTest::newRow("type words") << "(type \"this \"is \"a \"test)\n"
                              << "thisisatest";

  QTest::newRow("type list") << "type [this is a test]\n"
                             << "[this is a test]";

  QTest::newRow("READLIST empty") << "show readlist\n"
                                     "\n"
                                  << "[]\n";

  QTest::newRow("READLIST EOF") << "show readlist\n"
                                << "\n";

  QTest::newRow("READWORD EOF") << "show readword\n"
                                << "[]\n";

  QTest::newRow("READRAWLINE EOF") << "show readrawline\n"
                                   << "[]\n";

  QTest::newRow("READLIST test") << "show readlist\n"
                                    "this is a list test\n"
                                 << "[this is a list test]\n";

  QTest::newRow("READLIST incomplete list") << "show readlist\n"
                                               "this is a [list test\n"
                                            << "[this is a [list test]]\n";

  QTest::newRow("READLIST incomplete array") << "show readlist\n"
                                                "this is an {ary test\n"
                                             << "[this is an {ary test}]\n";

  QTest::newRow("READLIST split") << "show readlist\n"
                                     "this is a list [test\n"
                                     "line two]"
                                  << "[this is a list [test line two]]\n";

  QTest::newRow("READLIST expression") << "show readlist\n"
                                          "this is 1*2+3\n"
                                       << "[this is 1*2+3]\n";

  QTest::newRow("READWORD") << "show readword\n"
                               "this is my test\n"
                            << "this is my test\n";

  QTest::newRow("READWORD split") << "show readword\n"
                                     "this is ~\n"
                                     "my test\n"
                                  << "this is ~\nmy test\n";

  QTest::newRow("READRAWLINE") << "show readrawline\n"
                                  "this is ~\n"
                               << "this is ~\n";

  QTest::newRow("READCHAR") << "show readchar\n"
                               "a"
                            << "a\n";

  QTest::newRow("READCHARS") << "show readchars 5\n"
                                "chars"
                             << "chars\n";

  QTest::newRow("READCHARS under") << "show readchars 6\n"
                                      "stop"
                                   << "stop\n";

  QTest::newRow("READCHARS over") << "show readchars 5\n"
                                     "boneyard\n"
                                  << "boney\nI don't know how to ard\n";

  QTest::newRow("number var") << "make \"a 100\n"
                                 "print :a\n"
                              << "100\n";

  QTest::newRow("repeat") << "make \"a 1\n"
                             "repeat 5 [make \"a :a+1]\n"
                             "print :a\n"
                          << "6\n";

  QTest::newRow("runparse 1") << "show runparse [1+1]\n"
                              << "[1 + 1]\n";

  QTest::newRow("run list var") << "make \"a 1\n"
                                   "make \"b [make \"a :A+1]\n"
                                   "repeat 10 :b\n"
                                   "print :a\n"
                                << "11\n";

  QTest::newRow("print list var") << "make \"A [hello there]\n"
                                     "print :a\n"
                                  << "hello there\n";

  QTest::newRow("paren print") << "(print \"a \"b \"c \"d)\n"
                               << "a b c d\n";

  QTest::newRow("print sqrt") << "print 1+sqrt 2*2\n"
                              << "3\n";

  QTest::newRow("vbarred bar var") << "make \"a \"|I am vbarred|\n"
                                      "print :a\n"
                                   << "I am vbarred\n";

  QTest::newRow("summed var") << "make \"a 2+3\n"
                                 "print :a\n"
                              << "5\n";

  QTest::newRow("parenned expression var") << "make \"a (1+2) *4+1\n"
                                              "print :a\n"
                                           << "13\n";

  QTest::newRow("incomplete paren 1") << "show (\n"
                                      << "')' not found\n";

  QTest::newRow("incomplete paren 2") << "show )\n"
                                      << "unexpected ')'\n";

  QTest::newRow("THING") << "make \"a \"b\n"
                            "make \"b 8\n"
                            "print thing :a\n"
                         << "8\n";

  QTest::newRow("expression order 1") << "make \"a 1+3*3\n"
                                         "print :a\n"
                                      << "10\n";

  QTest::newRow("expression order 2") << "make \"a 1+3*2+2\n"
                                         "print :a\n"
                                      << "9\n";

  QTest::newRow("equal true") << "make \"a 3+4=5+2\n"
                                 "print :a\n"
                              << "true\n";

  QTest::newRow("number equal false") << "make \"a 3+4=5+3\n"
                                         "print :a\n"
                                      << "false\n";

  QTest::newRow("notequal false") << "make \"a 3+9<>6+6\n"
                                     "print :a\n"
                                  << "false\n";

  QTest::newRow("notequal true") << "make \"a 4+6<>8+8\n"
                                    "print :a\n"
                                 << "true\n";

  QTest::newRow("more than false") << "make \"a 2>5\n"
                                      "print :a\n"
                                   << "false\n";

  QTest::newRow("number more than true") << "make \"a 5>2\n"
                                            "print :a\n"
                                         << "true\n";

  QTest::newRow("less than false") << "make \"a 5<2\n"
                                      "print :a\n"
                                   << "false\n";

  QTest::newRow("less than true") << "make \"a 2<5\n"
                                     "print :a\n"
                                  << "true\n";

  QTest::newRow("more or equal false") << "make \"a 5>=8\n"
                                          "print :a\n"
                                       << "false\n";

  QTest::newRow("more or equal true") << "make \"a 8>=5\n"
                                         "print :a\n"
                                      << "true\n";

  QTest::newRow("less or eq false") << "make \"a 5<=3\n"
                                       "print :a\n"
                                    << "false\n";

  QTest::newRow("less or eq true") << "make \"a 3<=5\n"
                                      "print :a\n"
                                   << "true\n";

  QTest::newRow("print nested list var") << "make \"a [hello [there]]\n"
                                            "print :a\n"
                                         << "hello [there]\n";

  QTest::newRow("show nested list var") << "make \"a [[hello] there]\n"
                                           "show :a\n"
                                        << "[[hello] there]\n";

  QTest::newRow("WORD") << "make \"a 12 + word 3 4\n"
                           "print :a\n"
                        << "46\n";

  QTest::newRow("LIST") << "make \"a list \"hello \"there\n"
                           "show :a\n"
                        << "[hello there]\n";

  QTest::newRow("SENTENCE") << "make \"a se [hello there [you]] \"guys\n"
                               "show :a\n"
                            << "[hello there [you] guys]\n";

  QTest::newRow("FPUT word") << "make \"a fput \"h \"ello\n"
                                "print :a\n"
                             << "hello\n";

  QTest::newRow("FPUT list") << "make \"a fput \"hello [there]\n"
                                "show :a\n"
                             << "[hello there]\n";

  QTest::newRow("LPUT word") << "make \"a lput \"h \"ello\n"
                                "print :a\n"
                             << "elloh\n";

  QTest::newRow("LPUT list") << "make \"a lput \"hello [there]\n"
                                "show :a\n"
                             << "[there hello]\n";

  QTest::newRow("ARRAY") << "make \"a array 5\n"
                            "show :a\n"
                         << "{[] [] [] [] []}\n";

  QTest::newRow("array literal 1") << "make \"a [{} {} {}]\n"
                                      "show :a\n"
                                   << "[{} {} {}]\n";

  QTest::newRow("array literal 2")
      << "make \"a [{hello} {there} {hello there}]\n"
         "show :a\n"
      << "[{hello} {there} {hello there}]\n";

  QTest::newRow("array literal 3") << "make \"a {a b c}@2\n"
                                      "show item 3 :a\n"
                                   << "b\n";

  QTest::newRow("LISTTOARRAY") << "make \"a listtoarray [hello [there]]\n"
                                  "show :a\n"
                               << "{hello [there]}\n";

  QTest::newRow("ARRAYTOLIST") << "make \"a arraytolist {{hello} there}\n"
                                  "show :a\n"
                               << "[{hello} there]\n";

  QTest::newRow("FIRST word") << "show first \"hello\n"
                              << "h\n";

  QTest::newRow("FIRST list") << "show first [foo bar]\n"
                              << "foo\n";

  QTest::newRow("FIRST array 1") << "show first {hello there}\n"
                                 << "1\n";

  QTest::newRow("FIRST array 2") << "show first {hello there}@3\n"
                                 << "3\n";

  QTest::newRow("LAST word") << "show last \"hello\n"
                             << "o\n";

  QTest::newRow("LAST list") << "show last [foo bar]\n"
                             << "bar\n";

  QTest::newRow("LAST array") << "show last {[hello] there}\n"
                              << "there\n";

  QTest::newRow("FIRSTS list")
      << "show firsts [{array1 array2 array3} [list1 list2 list3] foo bar]\n"
      << "[1 list1 f b]\n";

  QTest::newRow("BUTFIRSTS list")
      << "show butfirsts [{array1 array2 array3} [list1 list2 list3] foo bar]\n"
      << "[{array2 array3} [list2 list3] oo ar]\n";

  QTest::newRow("BUTFIRST list 1") << "show butfirst [list1 list2 list3]\n"
                                   << "[list2 list3]\n";

  QTest::newRow("BUTFIRST word 1") << "show butfirst \"QLogo\n"
                                   << "Logo\n";

  QTest::newRow("BUTFIRST list 2") << "show butfirst [list1]\n"
                                   << "[]\n";

  QTest::newRow("BUTFIRST word 2") << "show butfirst \"h\n"
                                   << "\n";

  QTest::newRow("BUTFIRST array 1") << "show butfirst {array1 array2 array3}\n"
                                    << "{array2 array3}\n";

  QTest::newRow("BUTFIRST array 2") << "show butfirst {array1}\n"
                                    << "{}\n";

  QTest::newRow("BUTLAST array 1") << "show butlast {array1 array2 array3}\n"
                                   << "{array1 array2}\n";

  QTest::newRow("BUTLAST array 2") << "show butlast {array1}\n"
                                   << "{}\n";

  QTest::newRow("BUTLAST list 1") << "show butlast [list1 list2 list3]\n"
                                  << "[list1 list2]\n";

  QTest::newRow("BUTLAST word 1") << "show butlast \"QLogo\n"
                                  << "QLog\n";

  QTest::newRow("BUTLAST list 2") << "show butlast [list1]\n"
                                  << "[]\n";

  QTest::newRow("BUTLAST word 2") << "show butlast \"h\n"
                                  << "\n";

  QTest::newRow("ITEM 1") << "show item 1 {hello there}\n"
                          << "hello\n";

  QTest::newRow("ITEM 2") << "show item 2 [hello there]\n"
                          << "there\n";

  QTest::newRow("ITEM 3") << "show item 3 \"helo\n"
                          << "l\n";

  QTest::newRow("SETITEM list") << "make \"a [hello there]"
                                   "setitem 1 :a \"bye\n"
                                   "show :a\n"
                                << "[bye there]\n";

  QTest::newRow("SETITEM array") << "make \"a {hello there}"
                                    "setitem 1 :a \"bye\n"
                                    "show :a\n"
                                 << "{bye there}\n";

  QTest::newRow("SETITEM word") << "make \"a \"hello\n"
                                   "setitem 1 :a \"b\n"
                                << "setitem doesn't like hello as input\n";

  QTest::newRow("dotSETITEM list") << "make \"a [hello there]"
                                      ".setitem 1 :a \"bye\n"
                                      "show :a\n"
                                   << "[bye there]\n";

  QTest::newRow("dotSETITEM array") << "make \"a {hello there}"
                                       ".setitem 1 :a \"bye\n"
                                       "show :a\n"
                                    << "{bye there}\n";

  QTest::newRow("dotSETITEM word") << "make \"a \"hello\n"
                                      ".setitem 1 :a \"b\n"
                                   << ".setitem doesn't like hello as input\n";

  QTest::newRow("dotSETFIRST list") << "make \"a [hello there]"
                                       ".setfirst :a \"bye\n"
                                       "show :a\n"
                                    << "[bye there]\n";

  QTest::newRow("dotSETFIRST array") << "make \"a {hello there}"
                                        ".setfirst :a \"bye\n"
                                        "show :a\n"
                                     << "{bye there}\n";

  QTest::newRow("dotSETFIRST word")
      << "make \"a \"hello\n"
         ".setfirst :a \"b\n"
      << ".setfirst doesn't like hello as input\n";

  QTest::newRow("dotSETBF list") << "make \"a [hello there]"
                                    ".setbf :a [bye you]\n"
                                    "show :a\n"
                                 << "[hello bye you]\n";

  QTest::newRow("dotSETBF array") << "make \"a {hello there}"
                                     ".setbf :a {bye you}\n"
                                     "show :a\n"
                                  << "{hello bye you}\n";

  QTest::newRow("dotSETBF word") << "make \"a \"hello\n"
                                    ".setbf :a \"owdy\n"
                                 << ".setbf doesn't like hello as input\n";

  QTest::newRow("WORDP word") << "show wordp \"hello\n"
                              << "true\n";

  QTest::newRow("WORDP list") << "show wordp [hello]\n"
                              << "false\n";

  QTest::newRow("WORD? word") << "show word? \"hello\n"
                              << "true\n";

  QTest::newRow("LISTP word") << "show listp \"hello\n"
                              << "false\n";

  QTest::newRow("LISTP list") << "show listp [hello]\n"
                              << "true\n";

  QTest::newRow("LIST? word") << "show list? \"hello\n"
                              << "false\n";

  QTest::newRow("ARRAYP array") << "show arrayp {hello}\n"
                                << "true\n";

  QTest::newRow("ARRAYP list") << "show arrayp [hello]\n"
                               << "false\n";

  QTest::newRow("ARRAY? array") << "show array? {hello}\n"
                                << "true\n";

  QTest::newRow("EMPTYP 1") << "show emptyp [{hello}]\n"
                            << "false\n";

  QTest::newRow("EMPTYP 2") << "show emptyp []\n"
                            << "true\n";

  QTest::newRow("EMPTY? array") << "show empty? [{hello}]\n"
                                << "false\n";

  QTest::newRow("EQUAL? 1") << "show equal? [{hello}] [{hello}]\n"
                            << "true\n";

  QTest::newRow("EQUAL? 2") << "make \"CASEIGNOREDP \"true\n"
                               "show equalp [{hello}] [{hellO}]\n"
                            << "true\n";

  QTest::newRow("EQUAL? 3") << "show equalp [{}] [{}]\n"
                            << "true\n";

  QTest::newRow("EQUAL? 4") << "show equal? [] []\n"
                            << "true\n";

  QTest::newRow("EQUAL? 5") << "show equal? [{}] [{} x]\n"
                            << "false\n";

  QTest::newRow("EQUAL? 6") << "show equalp [{}] []\n"
                            << "false\n";

  QTest::newRow("EQUAL? 7") << "show equalp \"1.00 1\n"
                            << "true\n";

  QTest::newRow("EQUAL? 8") << "show equal? 1.00 1\n"
                            << "true\n";

  QTest::newRow("EQUAL? 9") << "show equalp [{hello}] [{hellO}]\n"
                            << "false\n";

  QTest::newRow("NOTEQUAL? 1") << "show notequal? [{hello}] [{hello}]\n"
                               << "false\n";

  QTest::newRow("NOTEQUAL? 2") << "make \"CASEIGNOREDP \"true\n"
                                  "show notequalp [{hello}] [{hellO}]\n"
                               << "false\n";

  QTest::newRow("NOTEQUAL? 3") << "show notequalp [{}] [{}]\n"
                               << "false\n";

  QTest::newRow("NOTEQUAL? 4") << "show notequal? [] []\n"
                               << "false\n";

  QTest::newRow("NOTEQUAL? 5") << "show notequal? [{}] [{} x]\n"
                               << "true\n";

  QTest::newRow("NOTEQUAL? 6") << "show notequalp [{}] []\n"
                               << "true\n";

  QTest::newRow("NOTEQUAL? 7") << "show notequalp \"1.00 1\n"
                               << "false\n";

  QTest::newRow("NOTEQUAL? 8") << "show notequal? 1.00 1\n"
                               << "false\n";

  QTest::newRow("NOTEQUAL? 9") << "show notequalp [{hello}] [{hellO}]\n"
                               << "true\n";

  QTest::newRow("BEFORE? 1") << "show before? 3 12\n"
                             << "false\n";

  QTest::newRow("BEFORE? 2") << "show beforep 10 2\n"
                             << "true\n";

  QTest::newRow("dotEQ 1") << "show .eq {hello} {hello}\n"
                           << "false\n";

  QTest::newRow("dotEQ 2") << "make \"a [hello]\n"
                              "make \"b :a\n"
                              "show .eq :a :b\n"
                           << "true\n";

  QTest::newRow("MEMBERP 1") << "show memberp \"this [this is a test]\n"
                             << "true\n";

  QTest::newRow("MEMBERP 2") << "show memberp \"that [this is a test]\n"
                             << "false\n";

  QTest::newRow("MEMBERP 3") << "show memberp \"e \"hello\n"
                             << "true\n";

  QTest::newRow("MEMBERP 4") << "show memberp \"t \"hello\n"
                             << "false\n";

  QTest::newRow("MEMBERP 5") << "show memberp \"is \"this_is_a_test\n"
                             << "false\n";

  QTest::newRow("SUBSTRINGP 1") << "show substringp \"this [this is a test]\n"
                                << "false\n";

  QTest::newRow("SUBSTRINGP 2") << "show substringp \"hi \"this\n"
                                << "true\n";

  QTest::newRow("SUBSTRINGP 3") << "show substringp \"t \"hello\n"
                                << "false\n";

  QTest::newRow("SUBSTRINGP 4") << "show substringp \"is \"this\n"
                                << "true\n";

  QTest::newRow("NUMBERP 1") << "show numberp \"is\n"
                             << "false\n";

  QTest::newRow("NUMBERP 2") << "show numberp \"1.00\n"
                             << "true\n";

  QTest::newRow("NUMBERP 3") << "show numberp 1\n"
                             << "true\n";

  QTest::newRow("NUMBERP 4") << "show numberp [1 2 3]\n"
                             << "false\n";

  QTest::newRow("VBARREDP 1") << "show vbarredp \"i\n"
                              << "false\n";

  QTest::newRow("VBARREDP 2") << "show vbarredp \"|(|\n"
                              << "true\n";

  QTest::newRow("VBARREDP 3") << "show vbarredp \"\\(\n"
                              << "false\n";

  QTest::newRow("COUNT list") << "show count [1 2 3]\n"
                              << "3\n";

  QTest::newRow("COUNT array") << "show count {1 2}\n"
                               << "2\n";

  QTest::newRow("COUNT word") << "show count \"QLogo\n"
                              << "5\n";

  QTest::newRow("ASCII 1") << "show ascii 1\n"
                           << "49\n";

  QTest::newRow("ASCII 2") << "show ascii \"A\n"
                           << "65\n";

  QTest::newRow("ASCII 3") << "show ascii \"*\n"
                           << "42\n";

  QTest::newRow("ASCII 4") << "show ascii \"|(|\n"
                           << "40\n";

  QTest::newRow("RAWASCII 1") << "show rawascii 1\n"
                              << "49\n";

  QTest::newRow("RAWASCII 2") << "show rawascii \"A\n"
                              << "65\n";

  QTest::newRow("RAWASCII 3") << "show rawascii \"*\n"
                              << "42\n";

  QTest::newRow("RAWASCII 4") << "show rawascii \"|(|\n"
                              << "6\n";

  QTest::newRow("CHAR") << "show char 65\n"
                        << "A\n";

  QTest::newRow("MEMBER 1") << "show member \"e \"hello\n"
                            << "ello\n";

  QTest::newRow("MEMBER 3") << "show member \"is [this is a test]\n"
                            << "[is a test]\n";

  QTest::newRow("MEMBER 4") << "show member \"that [this is a test]\n"
                            << "[]\n";

  QTest::newRow("LOWERCASE") << "show lowercase \"Hello\n"
                             << "hello\n";

  QTest::newRow("UPPERCASE") << "show uppercase \"Hello\n"
                             << "HELLO\n";

  QTest::newRow("PARSE") << "show parse \"2\\ 3\n"
                         << "[2 3]\n";

  QTest::newRow("RUNPARSE 1") << "show runparse [print 2*2]\n"
                              << "[print 2 * 2]\n";

  QTest::newRow("RUNPARSE 2") << "show runparse \"2*2\n"
                              << "[2 * 2]\n";

  QTest::newRow("procedure params 1") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "tp 4 5\n"
                                      << "tp defined\n4 5 1 2 []\n";

  QTest::newRow("procedure params 2") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "(tp 4 5 6)\n"
                                      << "tp defined\n4 5 6 2 []\n";

  QTest::newRow("procedure params 3") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "(tp 4 5 6 7)\n"
                                      << "tp defined\n4 5 6 7 []\n";

  QTest::newRow("procedure params 4") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "(tp 4 5 6 7 8)\n"
                                      << "tp defined\n4 5 6 7 [8]\n";

  QTest::newRow("procedure params 5") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "(tp 4 5 6 7 8 9)\n"
                                      << "tp defined\n4 5 6 7 [8 9]\n";

  QTest::newRow("procedure params 5") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "(tp 4 5 6 7 8 9*9)\n"
                                      << "tp defined\n4 5 6 7 [8 81]\n";

  QTest::newRow("procedure params 6") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "(tp 4 5 6 7*7 8)\n"
                                      << "tp defined\n4 5 6 49 [8]\n";

  QTest::newRow("procedure params 7") << "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
                                         "(show :p1 :p2 :p3 :p4 :p5)\n"
                                         "end\n"
                                         "tp 4 5*5\n"
                                      << "tp defined\n4 25 1 2 []\n";

  QTest::newRow("procedure params 8")
      << "to tp :p1 :p2 [:p3 3*3] [:p4 2] [:p5]\n"
         "(show :p1 :p2 :p3 :p4 :p5)\n"
         "end\n"
         "tp 4 5*5\n"
      << "tp defined\n4 25 9 2 []\n";

  QTest::newRow("procedure params 9")
      << "to tp :p1 :p2 [:p3 :v1] [:p4 2] [:p5]\n"
         "(show :p1 :p2 :p3 :p4 :p5)\n"
         "end\n"
         "make \"v1 20\n"
         "tp 4 5*5\n"
      << "tp defined\n4 25 20 2 []\n";

  QTest::newRow("procedure params 10")
      << "to tp :p1 :p2 [:p3 \"Iasonas] [:p4 2] [:p5]\n"
         "(show :p1 :p2 :p3 :p4 :p5)\n"
         "end\n"
         "tp 4 5*5\n"
      << "tp defined\n4 25 Iasonas 2 []\n";

  QTest::newRow("procedure params 11") << "to tp [:p1 [1 2 3]]\n"
                                          "show :p1\n"
                                          "end\n"
                                          "tp\n"
                                       << "tp defined\n"
                                          "[1 2 3]\n";

  QTest::newRow("procedure param err 1")
      << "to tp :p1 :p2 [:p3 1] [:p4 2]\n"
         "(show :p1 :p2 :p3 :p4)\n"
         "end\n"
         "(tp 1 2 3 4 5)\n"
      << "tp defined\ntoo many inputs to tp\n";

  QTest::newRow("procedure param err 2")
      << "to tp :p1 :p2 [:p3 1] [:p4 2]\n"
         "(show :p1 :p2 :p3 :p4)\n"
         "end\n"
         "(tp 1)\n"
      << "tp defined\nnot enough inputs to tp\n";

  QTest::newRow("procedure param err 3") << "to tp :p1 :p2 [:p3 1] 1\n"
                                            "(show :p1 :p2 :p3 :p4)\n"
                                            "end\n"
                                         << "to doesn't like 1 as input\n";

  QTest::newRow("procedure param err 4") << "to tp :p1 :p2 [:p3 1] 4\n"
                                            "(show :p1 :p2 :p3 :p4)\n"
                                            "end\n"
                                         << "to doesn't like 4 as input\n";

  QTest::newRow("procedure param err 5") << "to tp -2\n"
                                            "(show :p1 :p2 :p3 :p4)\n"
                                            "end\n"
                                         << "to doesn't like -2 as input\n";

  QTest::newRow("procedure param err 6") << "to tp :p1 :p2 [:p3 1] [:p4  5 5]\n"
                                            "(show :p1 :p2 :p3 :p4)\n"
                                            "end\n"
                                            "tp 1 2\n"
                                         << "tp defined\n"
                                            "You don't say what to do with 5\n";

  QTest::newRow("procedure param err 7") << "to tp :p1 :p2 [:p3 1] 4.4\n"
                                            "(show :p1 :p2 :p3 :p4)\n"
                                            "end\n"
                                         << "to doesn't like 4.4 as input\n";

  QTest::newRow("OUTPUT 1") << "to t1\n"
                               "output 2*2\n"
                               "end\n"
                               "to t2\n"
                               "output 2*t1\n"
                               "end\n"
                               "show t2\n"
                            << "t1 defined\nt2 defined\n8\n";

  QTest::newRow("OUTPUT 2") << "to t1 :x\n"
                               "output 2*:x\n"
                               "end\n"
                               "to t2\n"
                               "output 2*t1 5\n"
                               "end\n"
                               "show t2\n"
                            << "t1 defined\nt2 defined\n20\n";

  QTest::newRow("OUTPUT 3") << "to six\n"
                               "output print 6\n"
                               "end\n"
                               "six\n"
                            << "six defined\n"
                               "6\n"
                               "print didn't output to output\n";

  QTest::newRow("procedure factorial 1") << "to factorial :x\n"
                                            "if :x = 1 [output 1]\n"
                                            "output :x * factorial :x-1\n"
                                            "end\n"
                                            "show factorial 5\n"
                                         << "factorial defined\n120\n";

  QTest::newRow("procedure factorial 2")
      << "to factorial :x\n"
         "ifelse :x = 1 [output 1] [output :x * factorial :x-1]\n"
         "end\n"
         "show factorial 4\n"
      << "factorial defined\n24\n";

  QTest::newRow("SE 1") << "show (se \"\\( 2 \"+ 3 \"\\))\n"
                        << "[( 2 + 3 )]\n";

  QTest::newRow("SE 2") << "show (se \"make \"\"|(| 2)\n"
                        << "[make \"( 2]\n";

  QTest::newRow("split []") << "make \"a [a b\n"
                               "c] show :a"
                            << "[a b c]\n";

  QTest::newRow("split {}") << "make \"a {a b\n"
                               "c} show :a"
                            << "{a b c}\n";

  QTest::newRow("split ~") << "make \"a ~\n"
                              "\"c show :a"
                           << "c\n";

  QTest::newRow("split |") << "make \"a \"|a b\n"
                              "c| show :a"
                           << "a b\nc\n";

  QTest::newRow("split [~;") << "make \"a [a b;comment~\n"
                                "c] show :a"
                             << "[a bc]\n";

  QTest::newRow("split ~;") << "make \"a \"ab;comment~\n"
                               "c show :a"
                            << "abc\n";

  QTest::newRow("split ~ ;") << "make \"a [a b ;comment~\n"
                                "c] show :a"
                             << "[a b c]\n";

  QTest::newRow("unexpected ]") << "make \"a ]\n"
                                << "unexpected ']'\n";

  QTest::newRow("unexpected }") << "make \"a }\n"
                                << "unexpected '}'\n";

  QTest::newRow("double to") << "to oneThing\n"
                                "to another\n"
                                "end\n"
                                "oneThing\n"
                             << "oneThing defined\n"
                                "can't use to inside a procedure in oneThing\n"
                                "[to another]\n";

  QTest::newRow("built-in defined") << "to print\n"
                                    << "print is already defined\n";

  QTest::newRow("TO (no name)") << "to\n"
                                << "not enough inputs to to\n";

  QTest::newRow("TO :name") << "to :name\n"
                            << "to doesn't like :name as input\n";

  QTest::newRow("TO 'name") << "to \"name\n"
                            << "to doesn't like \"name as input\n";

  QTest::newRow("TO bad param 1") << "to tp :p1 :p2 [:p3 1] [:p4 2 2] [:p5]\n"
                                     "end\n"
                                     "tp 1 2\n"
                                  << "tp defined\n"
                                     "You don't say what to do with 2\n";

  QTest::newRow("TO bad param 2") << "to tp :p1 [:p3 1] :p2 [:p4 2] [:p5]\n"
                                     "end\n"
                                  << "to doesn't like :p2 as input\n";

  QTest::newRow("TO bad param 3") << "to tp :p1 :p2 [:p3 1] [:p5] [:p4 2]\n"
                                     "end\n"
                                  << "to doesn't like [:p4 2] as input\n";

  QTest::newRow("TO bad param 4") << "to tp :p1 [:p3 1] [:p4 2] [:p5] :p2\n"
                                     "end\n"
                                  << "to doesn't like :p2 as input\n";

  QTest::newRow("DEFINE 1") << "define \"c1 [[] [print \"hi]]\n"
                               "c1\n"
                            << "hi\n";

  QTest::newRow("DEFINE 2") << "define \"another [[] [to another]]\n"
                               "another\n"
                            << "can't use to inside a procedure in another\n"
                               "[to another]\n";

  QTest::newRow("DEFINE 3") << "define \"print [[] [type [hello]]]\n"
                            << "print is a primitive\n";

  QTest::newRow("DEFINE 4")
      << "define \"p2 [[p1 p2] [(print \"Hello :p1 :p2)]]\n"
         "p2 \"Iasonas \"Psyches\n"
      << "Hello Iasonas Psyches\n";

  QTest::newRow("DEFINE 5")
      << "define \"p3 [[p1 [p2 \"whatever]] [(print \"Hello :p1 :p2)]]\n"
         "p3 \"Iasonas\n"
      << "Hello Iasonas whatever\n";

  QTest::newRow("DEFINE 6")
      << "define \"p4 [[p1 [p2 \"whatever]] [(print \"Hello :p1 :p2)]]\n"
         "(p4 \"Iasonas \"Psyches)\n"
      << "Hello Iasonas Psyches\n";

  QTest::newRow("DEFINE 7") << "to qw :p1 [:p2 2*2]\n"
                               "(show \"Hello, :p1 :p2)\n"
                               "end\n"
                               "qw 10\n"
                            << "qw defined\n"
                               "Hello, 10 4\n";

  QTest::newRow("DEFINE notList error")
      << "define \"proc1 [[] [print \"hello] \"show\\ 5+5]\n"
      << "define doesn't like [[] [print \"hello] \"show 5+5] as input\n";

  QTest::newRow("no how 1") << "nohow\n"
                            << "I don't know how to nohow\n";

  QTest::newRow("no how 2") << "(nohow)\n"
                            << "I don't know how to nohow\n";

  QTest::newRow("no value 1") << "print :novalue\n"
                              << "novalue has no value\n";

  QTest::newRow("no value 2") << "print thing \"novalue\n"
                              << "novalue has no value\n";

  QTest::newRow("no ')'") << "print (sqrt 2\n"
                          << "')' not found\n";

  QTest::newRow("no say 1") << "sqrt 4\n"
                            << "You don't say what to do with 2\n";

  QTest::newRow("not enough inputs 1") << "print (sqrt)\n"
                                       << "not enough inputs to sqrt\n";

  QTest::newRow("not enough inputs 2") << "print sqrt\n"
                                       << "not enough inputs to sqrt\n";

  QTest::newRow("too many inputs") << "print (sqrt 4 9)\n"
                                   << "too many inputs to sqrt\n";

  QTest::newRow("no output") << "print cs\n"
                             << "cs didn't output to print\n";

  QTest::newRow("make [a]") << "make [a] 3\n"
                            << "make doesn't like [a] as input\n";

  QTest::newRow("add with string") << "print 2 + \"b\n"
                                   << "+ doesn't like b as input\n";

  QTest::newRow("add with list") << "print 2 + [a]\n"
                                 << "+ doesn't like [a] as input\n";

  QTest::newRow("unary minus with number") << "show runparse \"1\\ -1\n"
                                           << "[1 -1]\n";

  QTest::newRow("binary minus with negative number")
      << "show runparse \"1-\\ -1\n"
      << "[1 - -1]\n";

  QTest::newRow("unary minus with var") << "show runparse \"-:a\n"
                                        << "[0 -- :a]\n";

  QTest::newRow("unary minus with var in list") << "show runparse \"1\\ -:a\n"
                                                << "[1 0 -- :a]\n";

  QTest::newRow("number format 1") << "show 2e2\n"
                                   << "200\n";

  QTest::newRow("number format 2") << "show 3.e2\n"
                                   << "300\n";

  QTest::newRow("number format 3") << "show 2.2e2\n"
                                   << "220\n";

  QTest::newRow("number format 4") << "show 5E2\n"
                                   << "500\n";

  QTest::newRow("number format 6") << "show 20e2\n"
                                   << "2000\n";

  QTest::newRow("number format 7") << "show 1e2+2\n"
                                   << "102\n";

  QTest::newRow("number format 8") << "show 2e2+(3*4)\n"
                                   << "212\n";

  QTest::newRow("number format 9") << "show 3e2*-2\n"
                                   << "-600\n";

  QTest::newRow("number format 10") << "show 2e+1\n"
                                    << "20\n";

  QTest::newRow("number format 11") << "show 2e-1\n"
                                    << "0.2\n";

  QTest::newRow("number format 12") << "make \"a 10\n"
                                       "show -:a\n"
                                    << "-10\n";

  QTest::newRow("define operator +") << "to +\n"
                                     << "+ is already defined\n";

  QTest::newRow("define to") << "to to\n"
                             << "to is already defined\n";

  QTest::newRow("standout") << "show standout \"bold\n"
                            << "<b>bold</b>\n";

  QTest::newRow("shell 1") << "show shell [echo hello]\n"
                           << "[[hello]]\n";

  QTest::newRow("shell 2") << "show (shell [echo hello] [])\n"
                           << "[hello]\n";

  QTest::newRow("prefix 1") << "show prefix\n"
                            << "[]\n";

  QTest::newRow("prefix 2") << "setprefix \"newPrefix\n"
                               "show prefix\n"
                            << "newPrefix\n";

  QTest::newRow("file IO 1") << "make \"f \"TestQLogoFileIO1.txt\n"
                                "openwrite :f\n"
                                "setwrite :f\n"
                                "print [this is a test.]\n"
                                "closeall\n"
                                "openread :f\n"
                                "setread :f\n"
                                "show readrawline\n"
                                "closeall\n"
                                "erf :f\n"
                             << "this is a test.\n";

  QTest::newRow("file IO 2") << "make \"f \"TestQLogoFileIO2.txt\n"
                                "openwrite :f\n"
                                "setwrite :f\n"
                                "print [this is a test.]\n"
                                "closeall\n"
                                "openread :f\n"
                                "setread :f\n"
                                "make \"a readrawline\n"
                                "show readpos\n"
                                "closeall\n"
                                "erf :f\n"
#ifdef _WIN32
                             << "17\n";
#else
                             << "16\n";
#endif

  QTest::newRow("file IO 3") << "make \"f \"TestQLogoFileIO3.txt\n"
                                "openwrite :f\n"
                                "setwrite :f\n"
                                "print [this is a test]\n"
                                "make \"a writepos\n"
                                "closeall\n"
                                "erf :f\n"
                                "print :a\n"
#ifdef _WIN32
                             << "16\n";
#else
                             << "15\n";
#endif

  QTest::newRow("file IO 4") << "make \"f \"TestQLogoFileIO4.txt\n"
                                "openwrite :f\n"
                                "setwrite :f\n"
                                "print [this is another test]\n"
                                "closeall\n"
                                "openappend :f\n"
                                "setwrite :f\n"
                                "print [beep]\n"
                                "make \"a writepos\n"
                                "closeall\n"
                                "erf :f\n"
                                "print :a\n"
#ifdef _WIN32
                             << "28\n";
#else
                             << "26\n";
#endif

  QTest::newRow("file IO 5") << "openwrite \"TestQLogoFileIO5.txt\n"
                                "show allopen\n"
                                "close \"TestQLogoFileIO5.txt\n"
                                "erf \"TestQLogoFileIO5.txt\n"
                             << "[TESTQLOGOFILEIO5.TXT]\n";

  QTest::newRow("file IO 6") << "make \"f \"TestQLogoFileIO6.txt\n"
                                "openwrite :f\n"
                                "setwrite :f\n"
                                "print [this was another test]\n"
                                "closeall\n"
                                "openupdate :f\n"
                                "setwrite :f\n"
                                "setwritepos 2\n"
                                "type \"at\n"
                                "setread :f\n"
                                "setreadpos 0\n"
                                "make \"a readrawline\n"
                                "closeall\n"
                                "erf :f\n"
                                "print :a\n"
                             << "that was another test\n";

  QTest::newRow("file IO 7") << "openupdate \"TestQLogoFileIO7.txt\n"
                                "setwrite \"TestQLogoFileIO7.txt\n"
                                "make \"a writer\n"
                                "setwrite []\n"
                                "show :a\n"
                                "setread \"TestQLogoFileIO7.txt\n"
                                "show reader\n"
                                "close \"TestQLogoFileIO7.txt\n"
                                "erf \"TestQLogoFileIO7.txt\n"
                             << "TESTQLOGOFILEIO7.TXT\n"
                                "TESTQLOGOFILEIO7.TXT\n";

  QTest::newRow("file IO 8") << "make \"f \"TestQLogoFileIO8.txt\n"
                                "openupdate :f\n"
                                "setwrite :f\n"
                                "print [this is a test]\n"
                                "setwrite []\n"
                                "setread :f\n"
                                "setreadpos 0\n"
                                "show eofp\n"
                                "show readpos\n"
                                "make \"a readrawline\n"
                                "show eofp\n"
                                "show readpos\n"
                                "close :f\n"
                                "erf :f\n"
#ifdef _WIN32
                             << "false\n0\ntrue\n16\n";
#else
                             << "false\n0\ntrue\n15\n";
#endif

  QTest::newRow("file IO 9") << "openwrite \"TestQLogoFileIO9.txt\n"
                                "close \"TestQLogoFileIO9.txt\n"
                                "show allopen\n"
                                "erf \"TestQLogoFileIO9.txt\n"
                             << "[]\n";

  QTest::newRow("file IO 10") << "openwrite \"TestQLogoFileIO10a.txt\n"
                                 "openwrite \"TestQLogoFileIO10b.txt\n"
                                 "closeall\n"
                                 "show allopen\n"
                                 "erf \"TestQLogoFileIO10a.txt\n"
                                 "erf \"TestQLogoFileIO10b.txt\n"
                              << "[]\n";

  QTest::newRow("string IO 1") << "openwrite [text 100]\n"
                                  "setwrite \"text\n"
                                  "show allopen\n"
                                  "closeall\n"
                                  "show first :text\n"
                                  "show last butlast :text\n"
                               << "[\n]\n";

  QTest::newRow("string IO 2") << "make \"t \"io\n"
                                  "openread [t -50]\n"
                                  "setread \"t\n"
                                  "show readword\n"
                                  "closeall\n"
                               << "io\n";

  QTest::newRow("string IO 3") << "make \"line \"go_\n"
                                  "openwrite [line 50 50]\n"
                                  "setwrite \"line\n"
                                  "print \"Cougs\n"
                                  "closeall\n"
                                  "show :line\n"
                               << "go_Cougs\n\n";

  QTest::newRow("string IO 4") << "openread [line]\n"
                                  "setread \"line\n"
                                  "show readrawline\n"
                               << "[]\n";

  QTest::newRow("sum 1") << "show (sum)\n"
                         << "0\n";

  QTest::newRow("sum 2") << "show (sum 1)\n"
                         << "1\n";

  QTest::newRow("sum 3") << "show (sum 3 4)\n"
                         << "7\n";

  QTest::newRow("sum 4") << "show (sum 7 8 9)\n"
                         << "24\n";

  QTest::newRow("product 1") << "show (product)\n"
                             << "1\n";

  QTest::newRow("product 2") << "show (product 5)\n"
                             << "5\n";

  QTest::newRow("product 3") << "show (product 3 4)\n"
                             << "12\n";

  QTest::newRow("product 4") << "show (product 7 8 9)\n"
                             << "504\n";

  QTest::newRow("difference 1") << "show difference 10 8\n"
                                << "2\n";

  QTest::newRow("MINUS 1") << "show MINUS 10 + 8\n"
                           << "-18\n";

  QTest::newRow("MINUS 2") << "show - 2 + 8\n"
                           << "-10\n";

  QTest::newRow("QUOTIENT 1") << "show QUOTIENT 48 8\n"
                              << "6\n";

  QTest::newRow("QUOTIENT 2") << "show (QUOTIENT 5)\n"
                              << "0.2\n";

  QTest::newRow("QUOTIENT 3") << "show QUOTIENT 4 0\n"
                              << "QUOTIENT doesn't like 0 as input\n";

  QTest::newRow("QUOTIENT 4") << "show (QUOTIENT 0)\n"
                              << "QUOTIENT doesn't like 0 as input\n";

  QTest::newRow("REMAINDER 1") << "show 14 % 6\n"
                               << "2\n";

  QTest::newRow("REMAINDER 2") << "show remainder -21 4\n"
                               << "-1\n";

  QTest::newRow("REMAINDER 3") << "show 4 % 0\n"
                               << "% doesn't like 0 as input\n";

  QTest::newRow("REMAINDER 4") << "show remainder 14 0\n"
                               << "remainder doesn't like 0 as input\n";

  QTest::newRow("MODULO 1") << "show modulo 14 6\n"
                            << "2\n";

  QTest::newRow("MODULO 2") << "show modulo -21 4\n"
                            << "3\n";

  QTest::newRow("MODULO 3") << "show modulo 30 -11\n"
                            << "-3\n";

  QTest::newRow("MODULO 4") << "show modulo 14 0\n"
                            << "modulo doesn't like 0 as input\n";

  QTest::newRow("MODULO 5") << "show modulo -21 -4\n"
                            << "-1\n";

  QTest::newRow("INT 1") << "show int 14\n"
                         << "14\n";

  QTest::newRow("INT 2") << "show int -21\n"
                         << "-21\n";

  QTest::newRow("INT 3") << "show int 30.5\n"
                         << "30\n";

  QTest::newRow("INT 4") << "show int -30.5\n"
                         << "-30\n";

  QTest::newRow("ROUND 1") << "show round 14\n"
                           << "14\n";

  QTest::newRow("ROUND 2") << "show round -21\n"
                           << "-21\n";

  QTest::newRow("ROUND 3") << "show round 30.5\n"
                           << "31\n";

  QTest::newRow("ROUND 4") << "show round -30.5\n"
                           << "-31\n";

  QTest::newRow("POWER 1") << "show power 4 2\n"
                           << "16\n";

  QTest::newRow("POWER 2") << "show power -2 5\n"
                           << "-32\n";

  QTest::newRow("POWER 3") << "show power 9 .5\n"
                           << "3\n";

  QTest::newRow("POWER 4") << "show power -4 .5\n"
                           << "power doesn't like 0.5 as input\n";

  QTest::newRow("EXP 1") << "show first exp 2\n"
                         << "7\n";

  QTest::newRow("EXP 2") << "show exp 0\n"
                         << "1\n";

  QTest::newRow("LOG10 1") << "show log10 10\n"
                           << "1\n";

  QTest::newRow("LOG10 2") << "show log10 0.01\n"
                           << "-2\n";

  QTest::newRow("LN 1") << "show ln 1\n"
                        << "0\n";

  QTest::newRow("LN 2") << "show first ln 100\n"
                        << "4\n";

  QTest::newRow("SIN 1") << "show sin 0\n"
                         << "0\n";

  QTest::newRow("SIN 2") << "show sin 90\n"
                         << "1\n";

  QTest::newRow("SIN 3") << "show sin 270\n"
                         << "-1\n";

  QTest::newRow("RADSIN 1") << "show radsin 0\n"
                            << "0\n";

  QTest::newRow("RADSIN 2") << "show first radsin 4\n"
                            << "-\n";

  QTest::newRow("COS 1") << "show cos 0\n"
                         << "1\n";

  QTest::newRow("COS 2") << "show cos 180\n"
                         << "-1\n";

  QTest::newRow("RADCOS 1") << "show radcos 0\n"
                            << "1\n";

  QTest::newRow("RADCOS 2") << "show first radcos 2\n"
                            << "-\n";

  QTest::newRow("ARCTAN 1") << "show arctan 0\n"
                            << "0\n";

  QTest::newRow("ARCTAN 2") << "show arctan 1\n"
                            << "45\n";

  QTest::newRow("ARCTAN 3") << "show arctan -1\n"
                            << "-45\n";

  QTest::newRow("ARCTAN 4") << "show (arctan -1 -1)\n"
                            << "-135\n";

  QTest::newRow("ARCTAN 5") << "show (arctan 1 -1)\n"
                            << "-45\n";

  QTest::newRow("ARCTAN 6") << "show (arctan -1 1)\n"
                            << "135\n";

  QTest::newRow("RADARCTAN 1") << "show first (radarctan -1 0)\n"
                               << "3\n";

  QTest::newRow("LESSP false") << "show lessp 4 2\n"
                               << "false\n";

  QTest::newRow("LESSP true") << "show lessp 4 8\n"
                              << "true\n";

  QTest::newRow("LESS? false") << "show less? 4 2\n"
                               << "false\n";

  QTest::newRow("LESS? true") << "show less? 4 8\n"
                              << "true\n";

  QTest::newRow("GREATERP false") << "show greaterp 3 6\n"
                                  << "false\n";

  QTest::newRow("GREATERP true") << "show greaterp 5 2\n"
                                 << "true\n";

  QTest::newRow("GREATER? false") << "show greater? 3 4\n"
                                  << "false\n";

  QTest::newRow("GREATER? true") << "show greater? 5 4\n"
                                 << "true\n";

  QTest::newRow("LESSEQUALP false") << "show lessequalp 4 2\n"
                                    << "false\n";

  QTest::newRow("LESSEQUALP true 1") << "show lessequalp 4 8\n"
                                     << "true\n";

  QTest::newRow("LESSEQUALP true 2") << "show lessequalp 5 5\n"
                                     << "true\n";

  QTest::newRow("LESSEQUAL? false") << "show lessequal? 4 2\n"
                                    << "false\n";

  QTest::newRow("LESSEQUAL? true 1") << "show lessequal? 4 8\n"
                                     << "true\n";

  QTest::newRow("LESSEQUAL? true 2") << "show lessequal? 4 4\n"
                                     << "true\n";

  QTest::newRow("GREATEREQUALP false") << "show greaterequalp 2 4\n"
                                       << "false\n";

  QTest::newRow("GREATEREQUALP true 1") << "show greaterequalp 8 4\n"
                                        << "true\n";

  QTest::newRow("GREATEREQUALP true 2") << "show greaterequalp 5 5\n"
                                        << "true\n";

  QTest::newRow("GREATEREQUAL? false") << "show greaterequal? 2 4\n"
                                       << "false\n";

  QTest::newRow("GREATEREQUAL? true 1") << "show greaterequal? 8 4\n"
                                        << "true\n";

  QTest::newRow("GREATEREQUAL? true 2") << "show greaterequal? 4 4\n"
                                        << "true\n";

  QTest::newRow("FORM 1") << "show form 1.1 10 4\n"
                          << "    1.1000\n";

  QTest::newRow("FORM 2") << "show form 1.2 -10 4\n"
                          << "1.2000    \n";

  QTest::newRow("FORM 3") << "show form 1.3 2 0\n"
                          << " 1\n";

  QTest::newRow("FORM 4") << "show form -1.4 10 4\n"
                          << "   -1.4000\n";

  QTest::newRow("BITAND 1") << "show bitand 10 4\n"
                            << "0\n";

  QTest::newRow("BITAND 2") << "show bitand -1 5\n"
                            << "5\n";

  QTest::newRow("BITAND 3") << "show (bitand 15 7 30)\n"
                            << "6\n";

  QTest::newRow("BITOR 1") << "show bitor 10 4\n"
                           << "14\n";

  QTest::newRow("BITOR 2") << "show bitor 2 5\n"
                           << "7\n";

  QTest::newRow("BITOR 3") << "show (bitor 15 7 32)\n"
                           << "47\n";

  QTest::newRow("BITXOR 1") << "show bitxor 10 4\n"
                            << "14\n";

  QTest::newRow("BITXOR 2") << "show bitxor 7 5\n"
                            << "2\n";

  QTest::newRow("BITXOR 3") << "show (bitxor 15 7 32)\n"
                            << "40\n";

  QTest::newRow("BITNOT 1") << "show bitnot 0\n"
                            << "-1\n";

  QTest::newRow("BITNOT 2") << "show bitnot -1\n"
                            << "0\n";

  QTest::newRow("BITNOT 3") << "show bitnot 2\n"
                            << "-3\n";

  QTest::newRow("ASHIFT 1") << "show ashift 0 2\n"
                            << "0\n";

  QTest::newRow("ASHIFT 2") << "show ashift 3 2\n"
                            << "12\n";

  QTest::newRow("ASHIFT 3") << "show ashift 24 -2\n"
                            << "6\n";

  QTest::newRow("ASHIFT 4") << "show ashift -32 -2\n"
                            << "-8\n";

  QTest::newRow("LSHIFT 1") << "show lshift 0 2\n"
                            << "0\n";

  QTest::newRow("LSHIFT 2") << "show lshift 3 2\n"
                            << "12\n";

  QTest::newRow("LSHIFT 3") << "show lshift 24 -2\n"
                            << "6\n";

  QTest::newRow("AND 1") << "show and \"true \"true\n"
                         << "true\n";

  QTest::newRow("AND 2") << "show and \"false \"true\n"
                         << "false\n";

  QTest::newRow("AND 3") << "show and \"true \"false\n"
                         << "false\n";

  QTest::newRow("AND 4") << "show (and \"true \"false \"true)\n"
                         << "false\n";

  QTest::newRow("AND 5") << "show (and \"true \"true \"true)\n"
                         << "true\n";

  QTest::newRow("AND 6") << "show (and \"true)\n"
                         << "true\n";

  QTest::newRow("AND 7") << "show (and \"false)\n"
                         << "false\n";

  QTest::newRow("AND and NOT list") << "show AND [NOT (0 = 0)] [(1 / 0) > .5]\n"
                                    << "false\n";

  QTest::newRow("OR 1") << "show or \"true \"true\n"
                        << "true\n";

  QTest::newRow("OR 2") << "show or \"false \"true\n"
                        << "true\n";

  QTest::newRow("OR 3") << "show or \"true \"false\n"
                        << "true\n";

  QTest::newRow("OR 4") << "show (or \"true \"false \"true)\n"
                        << "true\n";

  QTest::newRow("OR 5") << "show (or \"false \"false \"false)\n"
                        << "false\n";

  QTest::newRow("OR 6") << "show (or \"true)\n"
                        << "true\n";

  QTest::newRow("OR 7") << "show (or \"false)\n"
                        << "false\n";

  QTest::newRow("OR and NOT list") << "show OR [NOT (0 = 0)] [(1 / 1) > .5]\n"
                                   << "true\n";

  QTest::newRow("DRIBBLE") << "make \"d \"dribble.txt\n"
                              "dribble :d\n"
                              "print [hi]\n"
                              "nodribble\n"
                              "openread :d\n"
                              "setread :d\n"
                              "show readrawline\n"
                              "show readrawline\n"
                              "close :d\n"
                              "erf :d\n"
                           << "hi\nhi\n[]\n";

  QTest::newRow("double DRIBBLE") << "make \"d \"dribble2.txt\n"
                                     "dribble :d\n"
                                     "dribble :d\n"
                                     "nodribble\n"
                                     "erf :d\n"
                                  << "already dribbling\n";

  QTest::newRow("HEADING 1") << "rt 90\n"
                                "show heading\n"
                             << "270\n";

  QTest::newRow("HEADING 2") << "rt 120\n"
                                "show (heading \"z)\n"
                             << "240\n";

  QTest::newRow("SETHEADING 1") << "rt 90\n"
                                   "seth 30\n"
                                   "show heading\n"
                                << "30\n";

  QTest::newRow("SETHEADING 2") << "rt 120\n"
                                   "(setheading 40 \"z)\n"
                                   "show (heading \"z)\n"
                                << "40\n";

  QTest::newRow("TOWARDS 1") << "show towards [-1 1]\n"
                             << "45\n";

  QTest::newRow("TOWARDS 2") << "fd 1\n"
                                "show towards [1 1]\n"
                             << "270\n";

  QTest::newRow("SETPOS") << "setpos [-1 1]\n"
                             "show towards [0 2]\n"
                          << "315\n";

  QTest::newRow("PENDOWNP") << "show pendownp\n"
                               "pu show pendown?\n"
                            << "true\nfalse\n";

  QTest::newRow("PENCOLOR 1") << "setpc 0\n"
                                 "show pc\n"
                              << "[0 0 0]\n";

  QTest::newRow("PENCOLOR 2") << "setpc \"magenta\n"
                                 "show pc\n"
                              << "[100 0 100]\n";

  QTest::newRow("PENCOLOR 3") << "setpc [50 50 50]\n"
                                 "show pc\n"
                              << "[50 50 50]\n";

  QTest::newRow("PALETTE 1") << "setpalette 30 [50 50 50]\n"
                                "show palette 30\n"
                             << "[50 50 50]\n";

  QTest::newRow("PALETTE 2") << "setpalette 31 \"yellow\n"
                                "show palette 31\n"
                             << "[100 100 0]\n";

  QTest::newRow("PALETTE 3") << "setpalette 32 7\n"
                                "show palette 32\n"
                             << "[100 100 100]\n";

  QTest::newRow("SCRUNCH zero") << "setscrunch 1 0\n"
                                << "setscrunch doesn't like 0 as input\n";

  QTest::newRow("TEXT 1") << "to qw\n"
                             "show \"Hello\n"
                             "end\n"
                             "show text \"qw\n"
                          << "qw defined\n"
                             "[[] [show \"Hello]]\n";

  QTest::newRow("TEXT 2") << "to qw :p1\n"
                             "(show \"Hello, :p1)\n"
                             "end\n"
                             "show text \"qw\n"
                          << "qw defined\n"
                             "[[P1] [(show \"Hello, :p1)]]\n";

  QTest::newRow("TEXT 3") << "to qw :p1 [:p2 2]\n"
                             "(show \"Hello, :p1 :p2)\n"
                             "end\n"
                             "show text \"qw\n"
                          << "qw defined\n"
                             "[[P1 [P2 2]] [(show \"Hello, :p1 :p2)]]\n";

  QTest::newRow("TEXT 4") << "to qw :p1 [:p2 2*2]\n"
                             "(show \"Hello, :p1 :p2)\n"
                             "end\n"
                             "show text \"qw\n"
                          << "qw defined\n"
                             "[[P1 [P2 2*2]] [(show \"Hello, :p1 :p2)]]\n";

  QTest::newRow("TEXT 5")
      << "to qw :p1 [:p2 2*2] [:p3]\n"
         "(show \"Hello, :p1 :p2 \"and :p3)\n"
         "end\n"
         "show text \"qw\n"
      << "qw defined\n"
         "[[P1 [P2 2*2] [P3]] [(show \"Hello, :p1 :p2 \"and :p3)]]\n";

  QTest::newRow("TEXT 6")
      << "to qw :p1 [:p2 2*2] [:p3] 10\n"
         "(show \"Hello, :p1 :p2 \"and :p3)\n"
         "end\n"
         "show text \"qw\n"
      << "qw defined\n"
         "[[P1 [P2 2*2] [P3] 10] [(show \"Hello, :p1 :p2 \"and :p3)]]\n";

  QTest::newRow("TEXT 7") << "to qw\n"
                             "show \"Hello\n"
                             "end\n"
                             "show first text \"qw\n"
                          << "qw defined\n"
                             "[]\n";

  QTest::newRow("FULLTEXT 1") << "to qw\n"
                                 "show \"Hello\n"
                                 "end\n"
                                 "show fulltext \"qw\n"
                              << "qw defined\n"
                                 "[to qw show \"Hello end]\n";

  QTest::newRow("FULLTEXT 2") << "to qw :p1\n"
                                 "(show \"Hello, :p1)\n"
                                 "end\n"
                                 "show fulltext \"qw\n"
                              << "qw defined\n"
                                 "[to qw :p1 (show \"Hello, :p1) end]\n";

  QTest::newRow("FULLTEXT 3")
      << "to qw :p1 [:p2 2]\n"
         "(show \"Hello, :p1 :p2)\n"
         "end\n"
         "show fulltext \"qw\n"
      << "qw defined\n"
         "[to qw :p1 [:p2 2] (show \"Hello, :p1 :p2) end]\n";

  QTest::newRow("FULLTEXT 4")
      << "to qw :p1 [:p2 2*2]\n"
         "(show \"Hello, :p1 :p2)\n"
         "end\n"
         "show fulltext \"qw\n"
      << "qw defined\n"
         "[to qw :p1 [:p2 2*2] (show \"Hello, :p1 :p2) end]\n";

  QTest::newRow("FULLTEXT 5")
      << "to qw :p1 [:p2 2*2] [:p3]\n"
         "(show \"Hello, :p1 :p2 \"and :p3)\n"
         "end\n"
         "show fulltext \"qw\n"
      << "qw defined\n"
         "[to qw :p1 [:p2 2*2] [:p3] (show \"Hello, :p1 :p2 \"and :p3) end]\n";

  QTest::newRow("FULLTEXT 6") << "to qw :p1 [:p2 2*2] [:p3] 10\n"
                                 "(show \"Hello, :p1 :p2 \"and :p3)\n"
                                 "end\n"
                                 "show fulltext \"qw\n"
                              << "qw defined\n"
                                 "[to qw :p1 [:p2 2*2] [:p3] 10 (show \"Hello, "
                                 ":p1 :p2 \"and :p3) end]\n";

  QTest::newRow("FULLTEXT 7") << "to qw\n"
                                 "show \"Hello\n"
                                 "end\n"
                                 "show first fulltext \"qw\n"
                              << "qw defined\n"
                                 "to qw\n";

  // TODO: COPYDEF is slated for removal
  QTest::newRow("COPYDEF 1") << "to qw\n"
                                "show \"Hello\n"
                                "end\n"
                                "copydef \"we \"qw\n"
                                "we\n"
                             << "qw defined\n"
                                "Hello\n";

  //  QTest::newRow("COPYDEF 2") << "to qw\n"
  //                                "show \"Hello\n"
  //                                "end\n"
  //                                "copydef \"we \"qw\n"
  //                                "show fulltext \"we\n"
  //                             << "qw defined\n"
  //                                "[to we show \"Hello end]\n";

  QTest::newRow("COPYDEF 3") << "copydef \"tnirp \"print\n"
                                "tnirp \"QWERTY\n"
                             << "QWERTY\n";

  QTest::newRow("LOCAL 1") << "to qw :p1\n"
                              "local \"a\n"
                              "make \"a :p1\n"
                              "show :a\n"
                              "end\n"
                              "make \"a 12\n"
                              "qw 23\n"
                              "show :a\n"
                           << "qw defined\n"
                              "23\n"
                              "12\n";

  QTest::newRow("LOCAL 2") << "to qw :p1\n"
                              "local \"a\n"
                              "make \"a :p1\n"
                              "show :a\n"
                              "end\n"
                              "qw 23\n"
                              "show :a\n"
                           << "qw defined\n"
                              "23\n"
                              "a has no value\n";

  QTest::newRow("LOCAL 3") << "to qw :p1 :p2\n"
                              "local [a b]\n"
                              "make \"a :p1\n"
                              "make \"b :p2\n"
                              "(show :a :b)\n"
                              "end\n"
                              "make \"a 12\n"
                              "qw 23 34\n"
                              "show :a\n"
                              "show :b\n"
                           << "qw defined\n"
                              "23 34\n"
                              "12\n"
                              "b has no value\n";

  QTest::newRow("LOCAL 4") << "to qw :p1\n"
                              "local {a}\n"
                              "make \"a :p1\n"
                              "show :a\n"
                              "end\n"
                              "qw 23\n"
                           << "qw defined\n"
                              "local doesn't like {a} as input in qw\n"
                              "[local {a}]\n";

  QTest::newRow("PLIST 1") << "pprop 1 2 3\n"
                              "show gprop 1 2\n"
                           << "3\n";

  QTest::newRow("PLIST 2") << "pprop 1 2 3\n"
                              "pprop 1 3 4\n"
                              "show gprop 1 3\n"
                           << "4\n";

  QTest::newRow("PLIST 3") << "pprop 1 2 3\n"
                              "pprop 1 3 4\n"
                              "show gprop 1 2\n"
                           << "3\n";

  QTest::newRow("PLIST 4") << "pprop 1 2 3\n"
                              "pprop 1 2 4\n"
                              "show gprop 1 2\n"
                           << "4\n";

  QTest::newRow("PLIST 5") << "pprop 1 2 3\n"
                              "pprop 1 3 4\n"
                              "show gprop 1 4\n"
                           << "[]\n";

  QTest::newRow("PLIST 6") << "pprop 1 2 3\n"
                              "pprop 1 3 4\n"
                              "show gprop 2 3\n"
                           << "[]\n";

  QTest::newRow("PLIST 7") << "pprop 1 2 3\n"
                              "pprop 1 3 4\n"
                              "show count plist 1\n"
                           << "4\n";

  QTest::newRow("PLIST 8") << "pprop 1 2 3\n"
                              "pprop 1 3 4\n"
                              "show plist 2\n"
                           << "[]\n";

  QTest::newRow("PROCEDUREP 1") << "show procedurep \"show\n"
                                << "true\n";

  QTest::newRow("PROCEDUREP 2") << "to proc1\n"
                                   "show \"hello\n"
                                   "end\n"
                                   "show procedure? \"proc1\n"
                                << "proc1 defined\n"
                                   "true\n";

  QTest::newRow("PROCEDUREP 3") << "show procedurep \"true\n"
                                << "false\n";

  QTest::newRow("PROCEDUREP 4") << "copydef \"proc2 \"print\n"
                                   "show procedurep \"proc2\n"
                                << "true\n";

  QTest::newRow("PROCEDUREP 5") << "to proc1\n"
                                   "show \"hello\n"
                                   "end\n"
                                   "copydef \"proc2 \"proc1\n"
                                   "show procedure? \"proc2\n"
                                << "proc1 defined\n"
                                   "true\n";

  QTest::newRow("PRIMITIVEP 1") << "show primitivep \"show\n"
                                << "true\n";

  QTest::newRow("PRIMITIVEP 2") << "to proc1\n"
                                   "show \"hello\n"
                                   "end\n"
                                   "show primitive? \"proc1\n"
                                << "proc1 defined\n"
                                   "false\n";

  QTest::newRow("PRIMITIVEP 3") << "show primitivep \"true\n"
                                << "false\n";

  QTest::newRow("PRIMITIVEP 4") << "copydef \"proc2 \"print\n"
                                   "show primitivep \"proc2\n"
                                << "true\n";

  QTest::newRow("PRIMITIVEP 5") << "to proc1\n"
                                   "show \"hello\n"
                                   "end\n"
                                   "copydef \"proc2 \"proc1\n"
                                   "show primitive? \"proc2\n"
                                << "proc1 defined\n"
                                   "false\n";

  QTest::newRow("DEFINEDP 1") << "show definedp \"show\n"
                              << "false\n";

  QTest::newRow("DEFINEDP 2") << "to proc1\n"
                                 "show \"hello\n"
                                 "end\n"
                                 "show defined? \"proc1\n"
                              << "proc1 defined\n"
                                 "true\n";

  QTest::newRow("DEFINEDP 3") << "show definedp \"true\n"
                              << "false\n";

  QTest::newRow("DEFINEDP 4") << "copydef \"proc2 \"print\n"
                                 "show definedp \"proc2\n"
                              << "false\n";

  QTest::newRow("DEFINEDP 5") << "to proc1\n"
                                 "show \"hello\n"
                                 "end\n"
                                 "copydef \"proc2 \"proc1\n"
                                 "show defined? \"proc2\n"
                              << "proc1 defined\n"
                                 "true\n";

  QTest::newRow(("NAMEP 1")) << "show namep \"a\n"
                             << "false\n";

  QTest::newRow(("NAMEP 2")) << "make \"A 1\n"
                                "show name? \"a\n"
                             << "true\n";

  QTest::newRow(("NAMEP 3")) << "make \"a 1\n"
                                "show name? \"A\n"
                             << "true\n";

  QTest::newRow(("NAMEP 4")) << "to f1 :P1\n"
                                "show namep \"p1\n"
                                "end\n"
                                "f1 1\n"
                             << "f1 defined\n"
                                "true\n";

  QTest::newRow(("NAMEP 5")) << "to f1 :P1\n"
                                "show namep \"p1\n"
                                "end\n"
                                "f1 1\n"
                                "show name? \"p1\n"
                             << "f1 defined\n"
                                "true\n"
                                "false\n";

  QTest::newRow("PLISTP 1") << "pprop 1 2 3\n"
                               "show plistp 1\n"
                            << "true\n";

  QTest::newRow("PLISTP 2") << "pprop 1 2 3\n"
                               "show plist? 2\n"
                            << "false\n";

  QTest::newRow("PLISTP 3") << "pprop 1 2 3\n"
                               "remprop 1 2\n"
                               "show plistp 1\n"
                            << "false\n";

  QTest::newRow("CONTENTS 1") << "show contents\n"
                              << "[[] [] []]\n";

  QTest::newRow("CONTENTS 2") << "make \"a 1\n"
                                 "pprop 1 2 3\n"
                                 "to bro\n"
                                 "print 1\n"
                                 "end\n"
                                 "show contents\n"
                              << "bro defined\n"
                                 "[[BRO] [A] [1]]\n";

  QTest::newRow("PROCEDURES 1") << "show procedures\n"
                                << "[]\n";

  QTest::newRow("PROCEDURES 2") << "make \"a 1\n"
                                   "pprop 1 2 3\n"
                                   "to bro\n"
                                   "print 1\n"
                                   "end\n"
                                   "show procedures\n"
                                << "bro defined\n"
                                   "[BRO]\n";

  QTest::newRow("PRIMITIVES") << "show (count primitives) > 50\n"
                              << "true\n";

  QTest::newRow("NAMES 1") << "show names\n"
                           << "[[] []]\n";

  QTest::newRow("NAMES 2") << "make \"a 1\n"
                              "pprop 1 2 3\n"
                              "to bro\n"
                              "print 1\n"
                              "end\n"
                              "show names\n"
                           << "bro defined\n"
                              "[[] [A]]\n";

  QTest::newRow("PLISTS 1") << "show plists\n"
                            << "[[] [] []]\n";

  QTest::newRow("PLISTS 2") << "make \"a 1\n"
                               "pprop 1 2 3\n"
                               "to bro\n"
                               "print 1\n"
                               "end\n"
                               "show plists\n"
                            << "bro defined\n"
                               "[[] [] [1]]\n";

  QTest::newRow("ARITY 1") << "show arity \"print\n"
                           << "[0 1 -1]\n";

  QTest::newRow("ARITY 2") << "to a1\n"
                              "print 1\n"
                              "end\n"
                              "show arity \"a1\n"
                           << "a1 defined\n"
                              "[0 0 0]\n";

  QTest::newRow("ARITY 3") << "to a1 :p1\n"
                              "print :p1\n"
                              "end\n"
                              "show arity \"a1\n"
                           << "a1 defined\n"
                              "[1 1 1]\n";

  QTest::newRow("ARITY 4") << "to a1 [:p1]\n"
                              "print :p1\n"
                              "end\n"
                              "show arity \"a1\n"
                           << "a1 defined\n"
                              "[0 0 -1]\n";

  QTest::newRow("ARITY 5") << "to a1 :p0 [:p1]\n"
                              "(print :p0 :p1)\n"
                              "end\n"
                              "show arity \"a1\n"
                           << "a1 defined\n"
                              "[1 1 -1]\n";

  QTest::newRow("ARITY 6") << "to a1 :p0 [:p1] 5\n"
                              "(print :p0 :p1)\n"
                              "end\n"
                              "show arity \"a1\n"
                           << "a1 defined\n"
                              "[1 5 -1]\n";

  QTest::newRow("PRINTOUT 1") << "to a1 :p1\n"
                                 "show :p1\n"
                                 "end\n"
                                 "make \"q 2*2\n"
                                 "pprop 1 2 3\n"
                                 "pprop 2 3 4\n"
                                 "po [[a1] [q] [1 2]]\n"
                              << "a1 defined\n"
                                 "to a1 :p1\n"
                                 "show :p1\n"
                                 "end\n"
                                 "Make \"Q 4\n"
                                 "Pprop 1 2 3\n"
                                 "Pprop 2 3 4\n";

  QTest::newRow("PRINTOUT 2") << "to a1 [:p1 \"a| |test]\n"
                                 "show :p1\n"
                                 "end\n"
                                 "make \"q 2*2\n"
                                 "pprop \"joe 2 \"hello\n"
                                 "pprop 2 \"la 4\n"
                                 "printout [[a1] [q] [joe 2]]\n"
                              << "a1 defined\n"
                                 "to a1 [:p1 \"a| |test]\n"
                                 "show :p1\n"
                                 "end\n"
                                 "Make \"Q 4\n"
                                 "Pprop \"joe 2 \"hello\n"
                                 "Pprop 2 \"LA 4\n";

  QTest::newRow("PRINTOUT 3") << "to a1 [:p1 \"a| |test]\n"
                                 "show :p1\n"
                                 "end\n"
                                 "make \"q \"34\\ 34\n"
                                 "pprop \"joe 2 \"hello\\ there\n"
                                 "printout [[a1] [q] [joe 2]]\n"
                              << "a1 defined\n"
                                 "to a1 [:p1 \"a| |test]\n"
                                 "show :p1\n"
                                 "end\n"
                                 "Make \"Q \"34\\ 34\n"
                                 "Pprop \"joe 2 \"hello\\ there\n";

  QTest::newRow("PRINTOUT 4") <<"pprop \"test \"test [this is a test]\n"
                                "po [[][][test]]\n"
                             << "Pprop \"test \"TEST [this is a test]\n";

  QTest::newRow("PRINTOUT ERROR 1") << "po [po]\n"
                                    << "po is a primitive\n";

  QTest::newRow("PRINTOUT ERROR 1") << "po [[][bob]]\n"
                                    << "bob has no value\n";

  QTest::newRow("POT 1") << "to a1 :p1\n"
                            "show :p1\n"
                            "end\n"
                            "make \"q 2*2\n"
                            "pprop 1 2 3\n"
                            "pprop 2 3 4\n"
                            "pot [[a1] [q] [1 2]]\n"
                         << "a1 defined\n"
                            "to a1 :P1\n"
                            "Make \"Q 4\n"
                            "Plist 1 = [2 3]\n"
                            "Plist 2 = [3 4]\n";

  QTest::newRow("POT 2") << "to a1 [:p1 \"a| |test]\n"
                            "show :p1\n"
                            "end\n"
                            "make \"q 2*2\n"
                            "pprop \"joe 2 \"hello\n"
                            "pprop 2 \"la 4\n"
                            "pot [[a1] [q] [joe 2]]\n"
                         << "a1 defined\n"
                            "to a1 [:P1 \"|a test|]\n"
                            "Make \"Q 4\n"
                            "Plist \"joe = [2 hello]\n"
                            "Plist 2 = [LA 4]\n";

  QTest::newRow("POT 3") << "to a1 [:p1 \"a| |test]\n"
                            "show :p1\n"
                            "end\n"
                            "make \"q \"34\\ 34\n"
                            "pprop \"joe 2 \"hello\\ there\n"
                            "pot [[a1] [q] [joe 2]]\n"
                         << "a1 defined\n"
                            "to a1 [:P1 \"|a test|]\n"
                            "Make \"Q \"34\\ 34\n"
                            "Plist \"joe = [2 hello\\ there]\n";

  QTest::newRow("ERASE 1") << "to a1 :p1\n"
                              "show :p1\n"
                              "end\n"
                              "make \"q 2*2\n"
                              "pprop 1 2 3\n"
                              "pprop 2 3 4\n"
                              "pot [[a1] [q] [1 2]]\n"
                              "erase [[a1] [q] []]\n"
                              "er [[] [] [1 2]]\n"
                              "pot [[a1]]\n"
                              "pot [[] [q]]\n"
                              "pot [[][][1 2]]\n"
                           << "a1 defined\n"
                              "to a1 :P1\n"
                              "Make \"Q 4\n"
                              "Plist 1 = [2 3]\n"
                              "Plist 2 = [3 4]\n"
                              "I don't know how to a1\n"
                              "q has no value\n";

  QTest::newRow("ERASE 2") << "to a1 :p1\n"
                              "show :p1\n"
                              "end\n"
                              "make \"q [a1 \"hello]\n"
                              "repeat 1 :q\n"
                              "erase [[a1] [] []]\n"
                              "repeat 1 :q\n"
                           << "a1 defined\n"
                              "hello\n"
                              "I don't know how to a1\n";

  QTest::newRow("ERALL 1") << "to a1 :p1\n"
                              "show :p1\n"
                              "end\n"
                              "make \"q 2*2\n"
                              "pprop 1 2 3\n"
                              "pprop 2 3 4\n"
                              "erall\n"
                              "pot [[a1]]\n"
                              "pot [[] [q]]\n"
                              "pot [[][][1 2]]\n"
                           << "a1 defined\n"
                              "I don't know how to a1\n"
                              "q has no value\n";

  QTest::newRow("ERPS 1") << "to a1 :p1\n"
                             "show :p1\n"
                             "end\n"
                             "make \"q 2*2\n"
                             "pprop 1 2 3\n"
                             "pprop 2 3 4\n"
                             "erps\n"
                             "pot [[a1]]\n"
                             "pot [[] [q]]\n"
                             "pot [[][][1 2]]\n"
                          << "a1 defined\n"
                             "I don't know how to a1\n"
                             "Make \"Q 4\n"
                             "Plist 1 = [2 3]\n"
                             "Plist 2 = [3 4]\n";

  QTest::newRow("ERNS 1") << "to a1 :p1\n"
                             "show :p1\n"
                             "end\n"
                             "make \"q 2*2\n"
                             "pprop 1 2 3\n"
                             "pprop 2 3 4\n"
                             "erns\n"
                             "pot [[a1]]\n"
                             "pot [[] [q]]\n"
                             "pot [[][][1 2]]\n"
                          << "a1 defined\n"
                             "to a1 :P1\n"
                             "q has no value\n"
                             "Plist 1 = [2 3]\n"
                             "Plist 2 = [3 4]\n";

  QTest::newRow("ERPLS 1") << "to a1 :p1\n"
                              "show :p1\n"
                              "end\n"
                              "make \"q 2*2\n"
                              "pprop 1 2 3\n"
                              "pprop 2 3 4\n"
                              "erpls\n"
                              "pot [[a1]]\n"
                              "pot [[] [q]]\n"
                              "pot [[][][1 2]]\n"
                           << "a1 defined\n"
                              "to a1 :P1\n"
                              "Make \"Q 4\n";

  QTest::newRow("BURY 1") << "to a1 :p1\n"
                             "show :p1\n"
                             "end\n"
                             "make \"q 2*2\n"
                             "pprop 1 2 3\n"
                             "pprop 2 3 4\n"
                             "bury [[a1] [q] [1 2]]\n"
                             "erall\n"
                             "pot [[a1] [q] [1 2]]\n"
                          << "a1 defined\n"
                             "to a1 :P1\n"
                             "Make \"Q 4\n"
                             "Plist 1 = [2 3]\n"
                             "Plist 2 = [3 4]\n";

  QTest::newRow("BURY 2") << "to a1 :p1\n"
                             "show :p1\n"
                             "end\n"
                             "make \"q 2*2\n"
                             "pprop 1 2 3\n"
                             "pprop 2 3 4\n"
                             "bury [[] [q] [1 2]]\n"
                             "erall\n"
                             "pot [[a1] [] []]\n"
                             "pot [[] [q] []]\n"
                             "pot [[] [] [1 2]]\n"
                          << "a1 defined\n"
                             "I don't know how to a1\n"
                             "Make \"Q 4\n"
                             "Plist 1 = [2 3]\n"
                             "Plist 2 = [3 4]\n";

  QTest::newRow("BURY 3") << "to a1 :p1\n"
                             "show :p1\n"
                             "end\n"
                             "make \"q 2*2\n"
                             "pprop 1 2 3\n"
                             "pprop 2 3 4\n"
                             "bury [[a1] [] [1 2]]\n"
                             "erall\n"
                             "pot [[a1] [] []]\n"
                             "pot [[] [q] []]\n"
                             "pot [[] [] [1 2]]\n"
                          << "a1 defined\n"
                             "to a1 :P1\n"
                             "q has no value\n"
                             "Plist 1 = [2 3]\n"
                             "Plist 2 = [3 4]\n";

  QTest::newRow("BURY 4") << "to a1 :p1\n"
                             "show :p1\n"
                             "end\n"
                             "make \"q 2*2\n"
                             "pprop 1 2 3\n"
                             "pprop 2 3 4\n"
                             "bury [[a1] [q] [1]]\n"
                             "erall\n"
                             "pot [[a1] [] []]\n"
                             "pot [[] [q] []]\n"
                             "pot [[] [] [1 2]]\n"
                          << "a1 defined\n"
                             "to a1 :P1\n"
                             "Make \"Q 4\n"
                             "Plist 1 = [2 3]\n";

  QTest::newRow("UNBURY 1") << "to a1 :p1\n"
                               "show :p1\n"
                               "end\n"
                               "make \"q 2*2\n"
                               "pprop 1 2 3\n"
                               "pprop 2 3 4\n"
                               "bury [[a1] [q] [1 2]]\n"
                               "unbury [[a1] [q] [1 2]]\n"
                               "erall\n"
                               "pot [[a1] [] []]\n"
                               "pot [[] [q] []]\n"
                               "pot [[] [] [1 2]]\n"
                            << "a1 defined\n"
                               "I don't know how to a1\n"
                               "q has no value\n";

  QTest::newRow("BURIEDP 1") << "bury \"buriedproc\n"
                                "show buriedp [buriedproc]\n"
                             << "true\n";

  QTest::newRow("BURIEDP 2") << "bury [[buriedproc]]\n"
                                "show buried? [buriedproc]\n"
                             << "true\n";

  QTest::newRow("BURIEDP 3") << "bury [[][var][]]\n"
                                "show buriedp [[][var][]]\n"
                             << "true\n";

  QTest::newRow("BURIEDP 4") << "bury [[][][plist]]\n"
                                "show buriedp [[][][plist]]\n"
                             << "true\n";

  QTest::newRow("BURIEDP 5") << "show buriedp [buriedproc]\n"
                             << "false\n";

  QTest::newRow("BURIEDP 6") << "show buried? \"buriedproc\n"
                             << "false\n";

  QTest::newRow("BURIEDP 7") << "show buriedp [[][var][]]\n"
                             << "false\n";

  QTest::newRow("BURIEDP 8") << "show buriedp [[][][plist]]\n"
                             << "false\n";

  QTest::newRow("BURIEDP ERROR 1")
      << "show buriedp [[][][]]\n"
      << "buriedp doesn't like [[] [] []] as input\n";

  QTest::newRow("BURIEDP ERROR 2") << "show buriedp [[][]]\n"
                                   << "buriedp doesn't like [[] []] as input\n";

  QTest::newRow("IF w/word 1") << "if \"true \"print\\ \"hello\n"
                               << "hello\n";

  QTest::newRow("ALSE 1") << "show butfirst 12=1\n"
                          << "alse\n";

  QTest::newRow("TRACE 1") << "trace [[* + print][][]]\n"
                              "(print 2+2 3*3)\n"
                           << "( + 2 2 )\n"
                              "+ outputs 4\n"
                              "( * 3 3 )\n"
                              "* outputs 9\n"
                              "( print 4 9 )\n"
                              "4 9\n"
                              "print stops\n";

  QTest::newRow("TRACE 2") << "trace [[* + print][][]]\n"
                              "untrace \"print\n"
                              "(print 2+2 3*3)\n"
                           << "( + 2 2 )\n"
                              "+ outputs 4\n"
                              "( * 3 3 )\n"
                              "* outputs 9\n"
                              "4 9\n";

  QTest::newRow("TRACE 3") << "trace [[][lobar][]]\n"
                              "make \"lobar 2\n"
                           << "Make \"lobar 2\n";

  QTest::newRow("TRACE 4") << "trace [[* + make][def][]]\n"
                              "make \"def 2*2+5\n"
                           << "( * 2 2 )\n"
                              "* outputs 4\n"
                              "( + 4 5 )\n"
                              "+ outputs 9\n"
                              "( make \"def 9 )\n"
                              "Make \"def 9\n"
                              "make stops\n";

  QTest::newRow("TRACE 5") << "trace [[][][list1]]\n"
                              "pprop \"list1 \"loop 2\n"
                           << "Pprop \"list1 \"loop 2\n";

  QTest::newRow("TRACE 6") << "trace [[* + pprop][][list2]]\n"
                              "pprop \"list2 \"item1 4*4+3\n"
                           << "( * 4 4 )\n"
                              "* outputs 16\n"
                              "( + 16 3 )\n"
                              "+ outputs 19\n"
                              "( pprop \"list2 \"item1 19 )\n"
                              "Pprop \"list2 \"item1 19\n"
                              "pprop stops\n";

  QTest::newRow("TRACE 7") << "to l1 :p1\n"
                              "(show \"l1 :p1)\n"
                              "end\n"
                              "to l2 :p1\n"
                              "l1 :p1\n"
                              "end\n"
                              "to l3 :p1\n"
                              "l2 :p1\n"
                              "end\n"
                              "to l4 :p1\n"
                              "l3 :p1\n"
                              "end\n"
                              "trace [[l1 l2 l3 l4]]\n"
                              "l4 10\n"
                           << "l1 defined\n"
                              "l2 defined\n"
                              "l3 defined\n"
                              "l4 defined\n"
                              "( l4 10 )\n"
                              " ( l3 10 )\n"
                              "  ( l2 10 )\n"
                              "   ( l1 10 )\n"
                              "l1 10\n"
                              "   l1 stops\n"
                              "  l2 stops\n"
                              " l3 stops\n"
                              "l4 stops\n";

  QTest::newRow("TRACEDP 1") << "trace \"tracedproc\n"
                                "show tracedp [tracedproc]\n"
                             << "true\n";

  QTest::newRow("TRACEDP 2") << "trace [[tracedproc]]\n"
                                "show traced? [tracedproc]\n"
                             << "true\n";

  QTest::newRow("TRACEDP 3") << "trace [[][var][]]\n"
                                "show tracedp [[][var][]]\n"
                             << "true\n";

  QTest::newRow("TRACEDP 4") << "trace [[][][plist]]\n"
                                "show tracedp [[][][plist]]\n"
                             << "true\n";

  QTest::newRow("TRACEDP 5") << "show tracedp [tracedproc]\n"
                             << "false\n";

  QTest::newRow("TRACEDP 6") << "show traced? \"tracedproc\n"
                             << "false\n";

  QTest::newRow("TRACEDP 7") << "show tracedp [[][var][]]\n"
                             << "false\n";

  QTest::newRow("TRACEDP 8") << "show tracedp [[][][plist]]\n"
                             << "false\n";

  QTest::newRow("TRACEDP ERROR 1")
      << "show tracedp [[][][]]\n"
      << "tracedp doesn't like [[] [] []] as input\n";

  QTest::newRow("TRACEDP ERROR 2") << "show tracedp [[][]]\n"
                                   << "tracedp doesn't like [[] []] as input\n";

  QTest::newRow("STEP 1") << "step [[c1 c10] [p1]]\n"
                             "to c1 [:p1 2*2] [:p2 3+3]\n"
                             "(print :p1 :p2)\n"
                             "end\n"
                             "c1\n"
                             "\n"
                             "to c10\n"
                             "(c1 10 20)\n"
                             "end\n"
                             "c10\n"
                             "\n"
                             "\n"
                          << "c1 defined\n"
                             "P1 shadowed by local in procedure call\n"
                             "[(print :p1 :p2)]"
                             "4 6\n"
                             "c10 defined\n"
                             "[(c1 10 20)]"
                             "P1 shadowed by local in procedure call in c10\n"
                             "[(print :p1 :p2)]"
                             "10 20\n";

  QTest::newRow("STEP 2") << "step [[] [v1]]\n"
                             "local \"v1\n"
                             "make \"v1 \"hello\n"
                             "show :v1\n"
                          << "hello\n";

  QTest::newRow("RUN 1") << "run [print \"hello]\n"
                         << "hello\n";

  QTest::newRow("RUN 2") << "print run [\"hello]\n"
                         << "hello\n";

  QTest::newRow("RUN 3") << "print run [print \"hello]\n"
                         << "hello\n"
                            "run didn't output to print\n";

  QTest::newRow("RUN 4") << "print run [2*2]\n"
                         << "4\n";

  QTest::newRow("RUN 5") << "run \"print\\ \"hello\n"
                         << "hello\n";

  QTest::newRow("RUNRESULT 1") << "show runresult \"print\\ \"hello\n"
                               << "hello\n"
                                  "[]\n";

  QTest::newRow("RUNRESULT 2") << "show runresult [2*2]\n"
                               << "[4]\n";

  QTest::newRow("RUNRESULT 3") << "show runresult [\"hello]\n"
                               << "[hello]\n";

  QTest::newRow("RUNRESULT 3") << "show runresult [\"hello]\n"
                               << "[hello]\n";

  QTest::newRow("REPCOUNT 1")
      << "repeat 3 [repeat 3[show repcount]] show repcount\n"
      << "1\n2\n3\n1\n2\n3\n1\n2\n3\n-1\n";

  QTest::newRow("FOREVER")
      << "to f :p1\n"
         "forever [print repcount if repcount=:p1 [output \"end]]\n"
         "end\n"
         "print f 5\n"
      << "f defined\n"
         "1\n2\n3\n4\n5\n"
         "end\n";

  QTest::newRow("TEST 1") << "test \"true\n"
                             "iftrue [print \"hello]\n"
                          << "hello\n";

  QTest::newRow("TEST 2") << "test \"false\n"
                             "ift [print \"hello]\n"
                             "print \"boogie\n"
                          << "boogie\n";

  QTest::newRow("TEST 3") << "iftrue [print \"hello]\n"
                          << "iftrue without TEST\n";

  QTest::newRow("TEST 4") << "test \"true\n"
                             "iff [print \"hello]\n"
                             "print \"boogie\n"
                          << "boogie\n";

  QTest::newRow("TEST 5") << "to f\n"
                             "ift \"print\\ \"begone\n"
                             "end\n"
                             "test 2=2\n"
                             "f\n"
                          << "f defined\n"
                             "begone\n";

  QTest::newRow("Scope Error 1") << "to f1\n"
                                    "local \"a\n"
                                    "make \"a 22\n"
                                    "make \"b print :a\n"
                                    "end\n"
                                    "f1\n"
                                    "show :a\n"
                                 << "f1 defined\n"
                                    "22\n"
                                    "print didn't output to make in f1\n"
                                    "[make \"b print :a]\n"
                                    "a has no value\n";

  QTest::newRow("Scope Error 2") << "to f1\n"
                                    "test 2=2\n"
                                    "make \"a 22\n"
                                    "make \"b print :a\n"
                                    "end\n"
                                    "f1\n"
                                    "iff [show :a]\n"
                                 << "f1 defined\n"
                                    "22\n"
                                    "print didn't output to make in f1\n"
                                    "[make \"b print :a]\n"
                                    "iff without TEST\n";

  QTest::newRow("Scope Error 3") << "to f1\n"
                                    "local \"a\n"
                                    "make \"a 22\n"
                                    "make \"b print :a\n"
                                    "end\n"
                                    "make \"a \"lobotomy\n"
                                    "f1\n"
                                    "show :a\n"
                                 << "f1 defined\n"
                                    "22\n"
                                    "print didn't output to make in f1\n"
                                    "[make \"b print :a]\n"
                                    "lobotomy\n";

  QTest::newRow("Scope Error 4") << "to f1\n"
                                    "repeat 5[if repcount = 3 [2*2]]\n"
                                    "end\n"
                                    "f1\n"
                                    "show repcount\n"
                                 << "f1 defined\n"
                                    "You don't say what to do with 4\n"
                                    "-1\n";

  QTest::newRow("SETFOO 1") << "make \"allowgetset \"true\n"
                               "to proc\n"
                               "setfoo 1\n"
                               "show :foo\n"
                               "end\n"
                               "proc\n"
                               "show :foo\n"
                            << "proc defined\n"
                               "1\n"
                               "foo has no value\n";

  QTest::newRow("SETFOO 2") << "make \"allowgetset \"true\n"
                               "to proc\n"
                               "global \"foo\n"
                               "setfoo 1\n"
                               "end\n"
                               "proc\n"
                               "show :foo\n"
                            << "proc defined\n"
                               "1\n";

  QTest::newRow("STOP")
      << "to lp :count\n"
         "forever [print repcount if repcount=:count [stop]]\n"
         "end\n"
         "lp 5\n"
      << "lp defined\n"
         "1\n2\n3\n4\n5\n";

  QTest::newRow("CATCH 1") << "catch \"error [notafunc]\n"
                              "show error\n"
                           << "[13 I don't know how to notafunc [] []]\n";

  QTest::newRow("CATCH 2") << "catch \"err [notafunc]\n"
                           << "I don't know how to notafunc\n";

  QTest::newRow("CATCH 3") << "catch \"err [throw \"err]\n"
                              "show error\n"
                           << "[]\n";

  QTest::newRow("CATCH 4") << "catch \"err1 [throw \"err2]\n"
                              "show error\n"
                           << "Can't find catch tag for err2\n"
                              "[]\n";

  QTest::newRow("CATCH 5") << "print catch \"err [(throw \"err \"hello)]\n"
                           << "hello\n";

  QTest::newRow("CATCH 6")
      << "print catch \"er1 [(throw \"er1 [hello there])]\n"
      << "hello there\n";

  QTest::newRow("CATCH PROCEDURE OUTPUT") << "to t\n"
                                             "throw \"q\n"
                                             "end\n"
                                             "to c\n"
                                             "catch \"q [output t]\n"
                                             "print \"caught\n"
                                             "end\n"
                                             "c\n"
                                          << "t defined\n"
                                             "c defined\n"
                                             "caught\n";

  QTest::newRow("THROW 1") << "to throw_error\n"
                              "(throw \"error [this is an error])\n"
                              "end\n"
                              "throw_error\n"
                              "show error\n"
                           << "throw_error defined\n"
                              "this is an error\n"
                              "[]\n";

  QTest::newRow("THROW 2") << "to throw_error\n"
                              "(throw \"error [this is an error] )\n"
                              "end\n"
                              "to level2\n"
                              "throw_error\n"
                              "end\n"
                              "catch \"error [level2]\n"
                              "show error\n"
                           << "throw_error defined\n"
                              "level2 defined\n"
                              "[35 this is an error level2 [throw_error]]\n";

  QTest::newRow("THROW 3") << "to throw_error\n"
                              "not_a_function\n"
                              "end\n"
                              "to level2\n"
                              "catch \"error [throw_error]\n"
                              "show error\n"
                              "end\n"
                              "level2\n"
                           << "throw_error defined\n"
                              "level2 defined\n"
                              "[13 I don't know how to not_a_function "
                              "throw_error [not_a_function]]\n";

  QTest::newRow("THROW 4")
      << "catch \"error [(throw \"error [this is an error])]\n"
         "show error\n"
      << "[35 this is an error [] []]\n";

  QTest::newRow("THROW 5") << "to throw_error\n"
                              "noop\n"
                              "catch \"error [(throw \"error \"misc)]\n"
                              "end\n"
                              "to noop\n"
                              "end\n"
                              "throw_error\n"
                              "show error\n"
                           << "throw_error defined\n"
                              "noop defined\n"
                              "[35 misc [] []]\n";

  QTest::newRow("THROW 6")
      << "to throw_error\n"
         "noop\n"
         "catch \"error [throw \"error]\n"
         "end\n"
         "to noop\n"
         "end\n"
         "throw_error\n"
         "show error\n"
      << "throw_error defined\n"
         "noop defined\n"
         "[21 Throw \"Error throw_error [catch \"error [throw \"error]]]\n";

  QTest::newRow("SETFOO") << "make \"allowgetset \"true\n"
                             "setfoo \"hello\n"
                             "show foo\n"
                          << "hello\n";

  QTest::newRow("? 1") << "show runparse \"?37\n"
                       << "[( ? 37 )]\n";

  QTest::newRow("? 2") << "show runparse \"?alpha\n"
                       << "[?alpha]\n";

  QTest::newRow("NESTED 1") << "make \"allowgetset \"true\n"
                               "make \"a [a b c [d e f]]\n"
                               "make \"b [a b c [d e f]]\n"
                               ".setfirst :a :a\n"
                               ".setfirst :b :b\n"
                               "show a=b\n"
                            << "true\n";

  QTest::newRow("NESTED 2") << "make \"a [a b c [d e f]]\n"
                               ".setfirst :a :a\n"
                               "show :a"
                            << "[... b c [d e f]]\n";

  QTest::newRow("MAYBE PRINT") << "to maybePrint\n"
                                  ".maybeoutput print \"hello\n"
                                  "end\n"
                                  "maybePrint\n"
                               << "maybePrint defined\n"
                                  "hello\n";

  QTest::newRow("MAYBE WORD") << "to maybeWord\n"
                                 ".maybeoutput \"hello\n"
                                 "end\n"
                                 "print maybeWord\n"
                              << "maybeWord defined\n"
                                 "hello\n";

  QTest::newRow("MAYBE PRINT error") << "to maybePrint\n"
                                        ".maybeoutput print \"hello\n"
                                        "end\n"
                                        "print maybePrint\n"
                                     << "maybePrint defined\n"
                                        "hello\n"
                                        "maybePrint didn't output to print\n";

  QTest::newRow("MAYBE WORD error") << "to maybeWord\n"
                                       ".maybeoutput \"hello\n"
                                       "end\n"
                                       "maybeWord\n"
                                    << "maybeWord defined\n"
                                       "You don't say what to do with hello\n";

  QTest::newRow("TRACE procedure iteration 1") << "to iter :p1 :i\n"
                                                  "if :i <=0 [output :p1]\n"
                                                  "output iter :p1 * 2 :i-1\n"
                                                  "end\n"
                                                  "trace \"iter\n"
                                                  "print iter 4 5\n"
                                               << "iter defined\n"
                                                  "( iter 4 5 )\n"
                                                  " ( iter 8 4 )\n"
                                                  "  ( iter 16 3 )\n"
                                                  "   ( iter 32 2 )\n"
                                                  "    ( iter 64 1 )\n"
                                                  "     ( iter 128 0 )\n"
                                                  "     iter outputs 128\n"
                                                  "    iter outputs 128\n"
                                                  "   iter outputs 128\n"
                                                  "  iter outputs 128\n"
                                                  " iter outputs 128\n"
                                                  "iter outputs 128\n"
                                                  "128\n";

  QTest::newRow("TRACE procedure iteration 2") << "to i1 :p :i\n"
                                                  "if :i<=0 [output :p]\n"
                                                  "output i2 :p*2 :i-1\n"
                                                  "end\n"

                                                  "to i2 :p :i\n"
                                                  "if :i<=0 [output :p]\n"
                                                  "output i1 :p+5 :i-1\n"
                                                  "end\n"

                                                  "trace [[i1 i2]]\n"
                                                  "print i1 4 5\n"

                                               << "i1 defined\n"
                                                  "i2 defined\n"
                                                  "( i1 4 5 )\n"
                                                  " ( i2 8 4 )\n"
                                                  "  ( i1 13 3 )\n"
                                                  "   ( i2 26 2 )\n"
                                                  "    ( i1 31 1 )\n"
                                                  "     ( i2 62 0 )\n"
                                                  "     i2 outputs 62\n"
                                                  "    i1 outputs 62\n"
                                                  "   i2 outputs 62\n"
                                                  "  i1 outputs 62\n"
                                                  " i2 outputs 62\n"
                                                  "i1 outputs 62\n"
                                                  "62\n";

  QTest::newRow("ERR in Procedure") << "to err\n"
                                       "print 5*\"me\n"
                                       "end\n"
                                       "err\n"
                                    << "err defined\n"
                                       "* doesn't like me as input in err\n"
                                       "[print 5*\"me]\n";

  QTest::newRow("GOTO 1") << "to proc\n"
                             "goto \"t1\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag \"t1\n"
                             "print [this should print]\n"
                             "end\n"
                             "proc\n"
                          << "proc defined\n"
                             "this should print\n";

  QTest::newRow("GOTO 2") << "to proc\n"
                             "goto :t1\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag \"tag1\n"
                             "print [this should print]\n"
                             "end\n"
                             "make \"t1 \"tag1\n"
                             "proc\n"
                          << "proc defined\n"
                             "this should print\n";

  QTest::newRow("GOTO 3") << "to proc\n"
                             "goto [tag1]\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag [tag1]\n"
                             "print [this shouldn't print, either]\n"
                             "end\n"
                             "proc\n"
                          << "proc defined\n"
                             "goto doesn't like [tag1] as input in proc\n"
                             "[goto [tag1]]\n";

  QTest::newRow("GOTO 4") << "to proc\n"
                             "goto \"tag2\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag \"tag1\n"
                             "print [this shouldn't' print, either]\n"
                             "end\n"
                             "proc\n"
                          << "proc defined\n"
                             "goto doesn't like tag2 as input in proc\n"
                             "[goto \"tag2]\n";

  QTest::newRow("APPLY 1") << "apply \"print [hello there]\n"
                           << "hello there\n";

  QTest::newRow("APPLY 2") << "print apply \"word [hello there everyone]\n"
                           << "hellothereeveryone\n";

  QTest::newRow("APPLY 3") << "apply \"print []\n"
                           << "\n";

  QTest::newRow("APPLY 4") << "apply \"make [hello there]\n"
                              "print :hello\n"
                           << "there\n";

  QTest::newRow("APPLY 5") << "apply \"make [hello there bob]\n"
                           << "too many inputs to make\n";

  QTest::newRow("APPLY 6") << "apply \"make [hello]\n"
                           << "not enough inputs to make\n";

  QTest::newRow("APPLY 7") << "apply \"make \"hello\n"
                           << "apply doesn't like hello as input\n";

  QTest::newRow("APPLY 8") << "show apply [? * ?] [3]\n"
                              "show apply [? + ?2] [3 4]\n"
                              "show apply [[x y] :x*:y] [4 5]\n"
                              "show apply [[x y] [output :x * :y]] [5 6]\n"
                           << "9\n"
                              "7\n"
                              "20\n"
                              "30\n";

  QTest::newRow("MACRO 1")
      << ".macro myrepeat :num :instructions\n"
         "if :num=0 [output []]\n"
         "output se :instructions (list \"myrepeat :num-1 :instructions)\n"
         "end\n"
         "myrepeat 3 [print \"hello]\n"
      << "myrepeat defined\n"
         "hello\n"
         "hello\n"
         "hello\n";

  QTest::newRow("MACRO err 1") << ".macro err1\n"
                                  "end\n"
                                  "err1\n"
                               << "err1 defined\n"
                                  "Macro returned nothing instead of a list\n";

  QTest::newRow("MACROP 1") << ".macro m\n"
                               "output []\n"
                               "end\n"
                               "show macrop \"m\n"
                            << "m defined\n"
                               "true\n";

  QTest::newRow("MACROP 2") << ".defmacro \"m2 [[] [output [print \"hello]]]\n"
                               "show macro? \"m2\n"
                               "m2\n"
                            << "true\n"
                               "hello\n";

  QTest::newRow("FULLPRINTP 1") << "make \"fullprintp \"true\n"
                                   "show \"|hello|\n"
                                   "show \"|hello there|\n"
                                   "print [hello |there you|]\n"
                                << "hello\n"
                                   "|hello there|\n"
                                   "hello |there you|\n";

  QTest::newRow("FULLPRINTP 2") << "make \"fullprintp \"true\n"
                                   "show [hello |there you|]\n"
                                   "show {hello there| |you}\n"
                                   "print \"hello\\ there\n"

                                << "[hello |there you|]\n"
                                   "{hello |there you|}\n"
                                   "hello\\ there\n";

  QTest::newRow("FULLPRINTP 3") << "make \"fullprintp \"true\n"
                                   "show [hello\\ there people]"
                                   "print [[hello\\ there]]\n"
                                   "print [{hello\\ there}]\n"
                                   "print [{|hello there| people}]\n"

                                << "[|hello there| people]\n"
                                   "[|hello there|]\n"
                                   "{|hello there|}\n"
                                   "{|hello there| people}\n";

  QTest::newRow("FULLPRINTP 4") << "make \"fullprintp \"true\n"
                                   "show \"|hello there|\n"
                                   "make \"fullprintp \"false\n"
                                   "show \"|hello there|\n"
                                << "|hello there|\n"
                                   "hello there\n";

  QTest::newRow("PRINTDEPTHLIMIT 1") << "make \"printdepthlimit 1\n"
                                        "show [[[] [] []]]\n"
                                        "show [this is a test]\n"
                                        "show {this is a test}\n"
                                        "show [[this is a test]]\n"
                                        "show {{this is a test}}\n"
                                     << "[[...]]\n"
                                        "[... ... ... ...]\n"
                                        "{... ... ... ...}\n"
                                        "[[...]]\n"
                                        "{{...}}\n";

  QTest::newRow("PRINTDEPTHLIMIT 2") << "make \"printdepthlimit 0\n"
                                        "show \"hello\n"
                                        "show [this is a test]\n"
                                        "show {this is a test}\n"
                                     << "...\n"
                                        "[...]\n"
                                        "{...}\n";

  QTest::newRow("PRINTDEPTHLIMIT 3") << "make \"printdepthlimit 2\n"
                                        "show \"hello\n"
                                        "show [[[this]] is a test]\n"
                                        "show {[{this}] is a test}\n"
                                     << "hello\n"
                                        "[[[...]] is a test]\n"
                                        "{[{...}] is a test}\n";

  QTest::newRow("PRINTWIDTHLIMIT 1") << "make \"printwidthlimit 1\n"
                                        "show [[[] [] []]]\n"
                                        "show [this is a test]\n"
                                        "show {this is a test}\n"
                                        "show [[this is a test]]\n"
                                        "show {{this is a test}}\n"
                                        "show \"12345678901234567890\n"
                                     << "[[[] ...]]\n"
                                        "[this ...]\n"
                                        "{this ...}\n"
                                        "[[this ...]]\n"
                                        "{{this ...}}\n"
                                        "1234567890...\n";

  QTest::newRow("PRINTWIDTHLIMIT 2") << "make \"printwidthlimit 15\n"
                                        "show \"12345678901234567890\n"
                                     << "123456789012345...\n";

  QTest::newRow("UNIX COMMENT") << "#! /usr/bin/logo\n"
                                   "print [success]\n"
                                << "success\n";

  QTest::newRow("FIBLIST")
      << "to fiblist :n\n"
         "if :n<2 [output [1 1]]\n"
         "output newfib fiblist :n-1\n"
         "end\n"
         "to newfib :list\n"
         "output fput (sum first :list first butfirst :list) :list\n"
         "end\n"
         "print fiblist 5\n"
      << "fiblist defined\n"
         "newfib defined\n"
         "8 5 3 2 1 1\n";

  QTest::newRow("Escape Sequence") << "print\t \"hello\n"
                                   << "hello\n";

  QTest::newRow("PO Macro") << ".macro m :p1\n"
                               "output sentence \"print \":p1\n"
                               "end\n"
                               "po \"m\n"
                            << "m defined\n"
                               ".macro m :p1\n"
                               "output sentence \"print \":p1\n"
                               "end\n";

  QTest::newRow("MACRO in TO") << "TO d\n"
                                  ".macro e\n"
                                  "end\n"
                                  "d\n"
                               << "d defined\n"
                                  "can't use .macro inside a procedure in d\n"
                                  "[.macro e]\n";

  QTest::newRow("Already filling")
      << "filled 3 [filled 2 [repeat 4 [fd 100 rt 90]]]\n"
      << "Already filling\n";

  QTest::newRow("TO in PAUSE") << "pause\n"
                                  "to pr\n"
                                  "co\n"
                                  "to fg\n"
                                  "end\n"
                               << "Pausing...\n"
                                  "Can't use to within PAUSE\n"
                                  "fg defined\n";

  QTest::newRow(".MACRO in PAUSE") << "pause\n"
                                      ".macro pr\n"
                                      "co\n"
                                      ".macro fg\n"
                                      "end\n"
                                   << "Pausing...\n"
                                      "Can't use .macro within PAUSE\n"
                                      "fg defined\n";

  QTest::newRow("reparsing list 1") << "make \"a [print \"hello]\n"
                                       "run :a\n"
                                       "setitem 2 :a \"\"hi\n"
                                       "run :a\n"
                                    << "hello\n"
                                       "hi\n";

  QTest::newRow("reparsing list 2") << "make \"a [print \"hello]\n"
                                       "run :a\n"
                                       ".setbf :a [\"hi]\n"
                                       "run :a\n"
                                    << "hello\n"
                                       "hi\n";

  QTest::newRow("reparsing list 3") << "make \"a [show [hello]]\n"
                                       "run :a\n"
                                       ".setfirst :a \"print\n"
                                       "run :a\n"
                                    << "[hello]\n"
                                       "hello\n";

  QTest::newRow("setitem list inside itself")
      << "make \"a [this is a test]\n"
         "setitem 1 :a :a\n"
      << "setitem doesn't like [this is a test] as input\n";

  QTest::newRow("setitem array inside itself")
      << "make \"a {this is a test}\n"
         "setitem 1 :a :a\n"
      << "setitem doesn't like {this is a test} as input\n";

  QTest::newRow("Ice Cream") // from: https://people.eecs.berkeley.edu/~bh/v3ch3/algs.html
          << "make \"one [Ice cream is delicious.]\n"
          "make \"two fput \"Spinach butfirst butfirst :one\n"
          ".setfirst butfirst butfirst :two \"disgusting.\n"
          "print :one\n"
          << "Ice cream is disgusting.\n";

  QTest::newRow("listSize 1")
          << "make \"a {this is an array}\n"
             "make \"b arraytolist :a\n"
             "show count :b\n"
          << "4\n";

  QTest::newRow("listSize 2")
          << "make \"a [this is a list]\n"
             "make \"b :a\n"
             "show count :b\n"
          << "4\n";

  QTest::newRow("listSize 3")
          << "make \"a [this is a bit of a longer list]\n"
             "make \"b member \"a :a\n"
             "show count :b\n"
          << "6\n";

  QTest::newRow("listSize 4")
          << "make \"a [this is a list]\n"
             "make \"b butfirst :a\n"
             "show count :b\n"
          << "3\n";

  QTest::newRow("listSize 5")
          << "make \"a [this is a list]\n"
             "make \"b butlast :a\n"
             "show count :b\n"
          << "3\n";
}

QTEST_APPLESS_MAIN(TestQLogo)

#include "testqlogo.moc"
