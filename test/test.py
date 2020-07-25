#!/usr/bin/python

# Run one test or all the tests on logo.
#
# usage: ./test.py [testID]
#
# Optional: testID = Will only run test identified by testID,
#                    other tests are skipped. 

import sys
import subprocess

#print "count of params:", len(sys.argv)
#
#if len(sys.argv) > 1:
#    print "param:", sys.argv[1]

# TODO: I need to find a way to make this portable
exe = '/Users/jsikes/Documents/WindowsBackpack/build-QLogo-Desktop_Qt_5_15_0_clang_64bit-Debug/logo'
    
tests = {}


tests['print number'] = {
    'in' :
    "print 2000\n"
    ,
    'out' :
    '? '
    "2000\n"
}

tests['type number'] = {
    'in' :
    "type 2000\n"
    ,
    'out' :
    '? '
    "2000"
}

tests['type word'] = {
    'in' :
    "type \"qwerty\n"
    ,
    'out' :
    '? '
    "qwerty"
}

tests['type words'] = {
    'in' :
    "(type \"this \"is \"a \"test)\n"
    ,
    'out' :
    '? '
    "thisisatest"
}

tests['type list'] = {
    'in' :
    "type [this is a test]\n"
    ,
    'out' :
    '? '
    "[this is a test]"
}

tests['READLIST empty'] = {
    'in' :
    "show readlist\n"
    "\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['READLIST EOF'] = {
    'in' :
    "show readlist\n"
    ,
    'out' :
    '? '
    "\n"
}

tests['READWORD EOF'] = {
    'in' :
    "show readword\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['READRAWLINE EOF'] = {
    'in' :
    "show readrawline\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['READLIST test'] = {
    'in' :
    "show readlist\n"
    "this is a list test\n"
    ,
    'out' :
    '? '
    "[this is a list test]\n"
}

tests['READLIST incomplete list'] = {
    'in' :
    "show readlist\n"
    "this is a [list test\n"
    ,
    'out' :
    '? '
    "[this is a [list test]]\n"
}

tests['READLIST incomplete array'] = {
    'in' :
    "show readlist\n"
    "this is an {ary test\n"
    ,
    'out' :
    '? '
    "[this is an {ary test}]\n"
}

tests['READLIST split'] = {
    'in' :
    "show readlist\n"
    "this is a list [test\n"
    "line two]"
    ,
    'out' :
    '? '
    '[ '
    "[this is a list [test line two]]\n"
}

tests['READLIST expression'] = {
    'in' :
    "show readlist\n"
    "this is 1*2+3\n"
    ,
    'out' :
    '? '
    "[this is 1*2+3]\n"
}

tests['READWORD'] = {
    'in' :
    "show readword\n"
    "this is my test\n"
    ,
    'out' :
    '? '
    "this is my test\n"
}

tests['READWORD split'] = {
    'in' :
    "show readword\n"
    "this is ~\n"
    "my test\n"
    ,
    'out' :
    '? '
    '~ '
    "this is ~\n"
    "my test\n"
}

tests['READRAWLINE'] = {
    'in' :
    "show readrawline\n"
    "this is ~\n"
    ,
    'out' :
    '? '
    "this is ~\n"
}

tests['READCHAR'] = {
    'in' :
    "show readchar\n"
    "a"
    ,
    'out' :
    '? '
    "a\n"
}

tests['READCHARS'] = {
    'in' :
    "show readchars 5\n"
    "chars"
    ,
    'out' :
    '? '
    "chars\n"
}

tests['READCHARS under'] = {
    'in' :
    "show readchars 6\n"
    "stop"
    ,
    'out' :
    '? '
    "stop\n"
}

tests['READCHARS over'] = {
    'in' :
    "show readchars 5\n"
    "boneyard\n"
    ,
    'out' :
    '? '
    "boney\n"
    '? '
    "I don't know how to ard\n"
}

tests['number var'] = {
    'in' :
    "make \"a 100\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "100\n"
}

tests['repeat'] = {
    'in' :
    "make \"a 1\n"
    "repeat 5 [make \"a :a+1]\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "6\n"
}

tests['runparse 1'] = {
    'in' :
    "show runparse [1+1]\n"
    ,
    'out' :
    '? '
    "[1 + 1]\n"
}

tests['run list var'] = {
    'in' :
    "make \"a 1\n"
    "make \"b [make \"a :A+1]\n"
    "repeat 10 :b\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    "11\n"
}

tests['print list var'] = {
    'in' :
    "make \"A [hello there]\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "hello there\n"
}

tests['paren print'] = {
    'in' :
    "(print \"a \"b \"c \"d)\n"
    ,
    'out' :
    '? '
    "a b c d\n"
}

tests['print sqrt'] = {
    'in' :
    "print 1+sqrt 2*2\n"
    ,
    'out' :
    '? '
    "3\n"
}

tests['vbarred bar var'] = {
    'in' :
    "make \"a \"|I am vbarred|\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "I am vbarred\n"
}

tests['summed var'] = {
    'in' :
    "make \"a 2+3\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "5\n"
}

tests['parenned expression var'] = {
    'in' :
    "make \"a (1+2) *4+1\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "13\n"
}

tests['incomplete paren 1'] = {
    'in' :
    "show (\n"
    ,
    'out' :
    '? '
    "')' not found\n"
}

tests['incomplete paren 2'] = {
    'in' :
    "show )\n"
    ,
    'out' :
    '? '
    "unexpected ')'\n"
}

tests['THING'] = {
    'in' :
    "make \"a \"b\n"
    "make \"b 8\n"
    "print thing :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "8\n"
}

tests['expression order 1'] = {
    'in' :
    "make \"a 1+3*3\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "10\n"
}

tests['expression order 2'] = {
    'in' :
    "make \"a 1+3*2+2\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "9\n"
}

tests['equal true'] = {
    'in' :
    "make \"a 3+4=5+2\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['number equal false'] = {
    'in' :
    "make \"a 3+4=5+3\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['notequal false'] = {
    'in' :
    "make \"a 3+9<>6+6\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['notequal true'] = {
    'in' :
    "make \"a 4+6<>8+8\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['more than false'] = {
    'in' :
    "make \"a 2>5\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['number more than true'] = {
    'in' :
    "make \"a 5>2\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['less than false'] = {
    'in' :
    "make \"a 5<2\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['less than true'] = {
    'in' :
    "make \"a 2<5\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['more or equal false'] = {
    'in' :
    "make \"a 5>=8\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['more or equal true'] = {
    'in' :
    "make \"a 8>=5\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['less or eq false'] = {
    'in' :
    "make \"a 5<=3\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['less or eq true'] = {
    'in' :
    "make \"a 3<=5\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['print nested list var'] = {
    'in' :
    "make \"a [hello [there]]\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "hello [there]\n"
}

tests['show nested list var'] = {
    'in' :
    "make \"a [[hello] there]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[[hello] there]\n"
}

tests['WORD'] = {
    'in' :
    "make \"a 12 + word 3 4\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "46\n"
}

tests['LIST'] = {
    'in' :
    "make \"a list \"hello \"there\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[hello there]\n"
}

tests['SENTENCE'] = {
    'in' :
    "make \"a se [hello there [you]] \"guys\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[hello there [you] guys]\n"
}

tests['FPUT word'] = {
    'in' :
    "make \"a fput \"h \"ello\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "hello\n"
}

tests['FPUT list'] = {
    'in' :
    "make \"a fput \"hello [there]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[hello there]\n"
}

tests['LPUT word'] = {
    'in' :
    "make \"a lput \"h \"ello\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    "elloh\n"
}

tests['LPUT list'] = {
    'in' :
    "make \"a lput \"hello [there]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[there hello]\n"
}

tests['ARRAY'] = {
    'in' :
    "make \"a array 5\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "{[] [] [] [] []}\n"
}

tests['array literal 1'] = {
    'in' :
    "make \"a [{} {} {}]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[{} {} {}]\n"
}

tests['array literal 2'] = {
    'in' :
    "make \"a [{hello} {there} {hello there}]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[{hello} {there} {hello there}]\n"
}

tests['array literal 3'] = {
    'in' :
    "make \"a {a b c}@2\n"
    "show item 3 :a\n"
    ,
    'out' :
    '? '
    '? '
    "b\n"
}

tests['LISTTOARRAY'] = {
    'in' :
    "make \"a listtoarray [hello [there]]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "{hello [there]}\n"
}

tests['ARRAYTOLIST'] = {
    'in' :
    "make \"a arraytolist {{hello} there}\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    "[{hello} there]\n"
}

tests['FIRST word'] = {
    'in' :
    "show first \"hello\n"
    ,
    'out' :
    '? '
    "h\n"
}

tests['FIRST list'] = {
    'in' :
    "show first [foo bar]\n"
    ,
    'out' :
    '? '
    "foo\n"
}

tests['FIRST array 1'] = {
    'in' :
    "show first {hello there}\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['FIRST array 2'] = {
    'in' :
    "show first {hello there}@3\n"
    ,
    'out' :
    '? '
    "3\n"
}

tests['LAST word'] = {
    'in' :
    "show last \"hello\n"
    ,
    'out' :
    '? '
    "o\n"
}

tests['LAST list'] = {
    'in' :
    "show last [foo bar]\n"
    ,
    'out' :
    '? '
    "bar\n"
}

tests['LAST array'] = {
    'in' :
    "show last {[hello] there}\n"
    ,
    'out' :
    '? '
    "there\n"
}

tests['FIRSTS list'] = {
    'in' :
    "show firsts [{array1 array2 array3} [list1 list2 list3] foo bar]\n"
    ,
    'out' :
    '? '
    "[1 list1 f b]\n"
}

tests['BUTFIRSTS list'] = {
    'in' :
    "show butfirsts [{array1 array2 array3} [list1 list2 list3] foo bar]\n"
    ,
    'out' :
    '? '
    "[{array2 array3} [list2 list3] oo ar]\n"
}

tests['BUTFIRST list 1'] = {
    'in' :
    "show butfirst [list1 list2 list3]\n"
    ,
    'out' :
    '? '
    "[list2 list3]\n"
}

tests['BUTFIRST word 1'] = {
    'in' :
    "show butfirst \"QLogo\n"
    ,
    'out' :
    '? '
    "Logo\n"
}

tests['BUTFIRST list 2'] = {
    'in' :
    "show butfirst [list1]\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['BUTFIRST word 2'] = {
    'in' :
    "show butfirst \"h\n"
    ,
    'out' :
    '? '
    "\n"
}

tests['BUTFIRST array 1'] = {
    'in' :
    "show butfirst {array1 array2 array3}\n"
    ,
    'out' :
    '? '
    "{array2 array3}\n"
}

tests['BUTFIRST array 2'] = {
    'in' :
    "show butfirst {array1}\n"
    ,
    'out' :
    '? '
    "{}\n"
}

tests['BUTLAST array 1'] = {
    'in' :
    "show butlast {array1 array2 array3}\n"
    ,
    'out' :
    '? '
    "{array1 array2}\n"
}

tests['BUTLAST array 2'] = {
    'in' :
    "show butlast {array1}\n"
    ,
    'out' :
    '? '
    "{}\n"
}

tests['BUTLAST list 1'] = {
    'in' :
    "show butlast [list1 list2 list3]\n"
    ,
    'out' :
    '? '
    "[list1 list2]\n"
}

tests['BUTLAST word 1'] = {
    'in' :
    "show butlast \"QLogo\n"
    ,
    'out' :
    '? '
    "QLog\n"
}

tests['BUTLAST list 2'] = {
    'in' :
    "show butlast [list1]\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['BUTLAST word 2'] = {
    'in' :
    "show butlast \"h\n"
    ,
    'out' :
    '? '
    "\n"
}

tests['ITEM 1'] = {
    'in' :
    "show item 1 {hello there}\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['ITEM 2'] = {
    'in' :
    "show item 2 [hello there]\n"
    ,
    'out' :
    '? '
    "there\n"
}

tests['ITEM 3'] = {
    'in' :
    "show item 3 \"helo\n"
    ,
    'out' :
    '? '
    "l\n"
}

tests['SETITEM list'] = {
    'in' :
    "make \"a [hello there]\n"
    "setitem 1 :a \"bye\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[bye there]\n"
}

tests['SETITEM array'] = {
    'in' :
    "make \"a {hello there}\n"
    "setitem 1 :a \"bye\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "{bye there}\n"
}

tests['SETITEM word'] = {
    'in' :
    "make \"a \"hello\n"
    "setitem 1 :a \"b\n"
    ,
    'out' :
    '? '
    '? '
    "setitem doesn't like hello as input\n"
}

tests['dotSETITEM list'] = {
    'in' :
    "make \"a [hello there]\n"
    ".setitem 1 :a \"bye\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[bye there]\n"
}

tests['dotSETITEM array'] = {
    'in' :
    "make \"a {hello there}\n"
    ".setitem 1 :a \"bye\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "{bye there}\n"
}

tests['dotSETITEM word'] = {
    'in' :
    "make \"a \"hello\n"
    ".setitem 1 :a \"b\n"
    ,
    'out' :
    '? '
    '? '
    ".setitem doesn't like hello as input\n"
}

tests['dotSETFIRST list'] = {
    'in' :
    "make \"a [hello there]\n"
    ".setfirst :a \"bye\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[bye there]\n"
}

tests['dotSETFIRST array'] = {
    'in' :
    "make \"a {hello there}\n"
    ".setfirst :a \"bye\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "{bye there}\n"
}

tests['dotSETFIRST word'] = {
    'in' :
    "make \"a \"hello\n"
    ".setfirst :a \"b\n"
    ,
    'out' :
    '? '
    '? '
    ".setfirst doesn't like hello as input\n"
}

tests['dotSETBF list'] = {
    'in' :
    "make \"a [hello there]\n"
    ".setbf :a [bye you]\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[hello bye you]\n"
}

tests['dotSETBF array'] = {
    'in' :
    "make \"a {hello there}\n"
    ".setbf :a {bye you}\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "{hello bye you}\n"
}

tests['dotSETBF word'] = {
    'in' :
    "make \"a \"hello\n"
    ".setbf :a \"owdy\n"
    ,
    'out' :
    '? '
    '? '
    ".setbf doesn't like hello as input\n"
}

tests['WORDP word'] = {
    'in' :
    "show wordp \"hello\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['WORDP list'] = {
    'in' :
    "show wordp [hello]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['WORD? word'] = {
    'in' :
    "show word? \"hello\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LISTP word'] = {
    'in' :
    "show listp \"hello\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['LISTP list'] = {
    'in' :
    "show listp [hello]\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LIST? word'] = {
    'in' :
    "show list? \"hello\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['ARRAYP array'] = {
    'in' :
    "show arrayp {hello}\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['ARRAYP list'] = {
    'in' :
    "show arrayp [hello]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['ARRAY? array'] = {
    'in' :
    "show array? {hello}\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EMPTYP 1'] = {
    'in' :
    "show emptyp [{hello}]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['EMPTYP 2'] = {
    'in' :
    "show emptyp []\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EMPTY? array'] = {
    'in' :
    "show empty? [{hello}]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['EQUAL? 1'] = {
    'in' :
    "show equal? [{hello}] [{hello}]\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EQUAL? 2'] = {
    'in' :
    "make \"CASEIGNOREDP \"true\n"
    "show equalp [{hello}] [{hellO}]\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['EQUAL? 3'] = {
    'in' :
    "show equalp [{}] [{}]\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EQUAL? 4'] = {
    'in' :
    "show equal? [] []\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EQUAL? 5'] = {
    'in' :
    "show equal? [{}] [{} x]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['EQUAL? 6'] = {
    'in' :
    "show equalp [{}] []\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['EQUAL? 7'] = {
    'in' :
    "show equalp \"1.00 1\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EQUAL? 8'] = {
    'in' :
    "show equal? 1.00 1\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['EQUAL? 9'] = {
        'in' :
    "show equalp [{hello}] [{hellO}]\n"
,
        'out' :
    '? '
"false\n"
}

tests['NOTEQUAL? 1'] = {
    'in' :
    "show notequal? [{hello}] [{hello}]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['NOTEQUAL? 2'] = {
    'in' :
    "make \"CASEIGNOREDP \"true\n"
    "show notequalp [{hello}] [{hellO}]\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['NOTEQUAL? 3'] = {
    'in' :
    "show notequalp [{}] [{}]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['NOTEQUAL? 4'] = {
    'in' :
    "show notequal? [] []\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['NOTEQUAL? 5'] = {
    'in' :
    "show notequal? [{}] [{} x]\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['NOTEQUAL? 6'] = {
    'in' :
    "show notequalp [{}] []\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['NOTEQUAL? 7'] = {
    'in' :
    "show notequalp \"1.00 1\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['NOTEQUAL? 8'] = {
    'in' :
    "show notequal? 1.00 1\n"
    ,
    'out' :
    '? '
    "false\n"
}

# TODO: BAD RESULT
# tests['NOTEQUAL? 9'] = {
#         'in' :
#     "show notequalp [{hello}] [{hellO}]\n"
# ,
#         'out' :
#     '? '
# "true\n"
# }

tests['BEFORE? 1'] = {
    'in' :
    "show before? 3 12\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['BEFORE? 2'] = {
    'in' :
    "show beforep 10 2\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['dotEQ 1'] = {
    'in' :
    "show .eq {hello} {hello}\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['dotEQ 2'] = {
    'in' :
    "make \"a [hello]\n"
    "make \"b :a\n"
    "show .eq :a :b\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "true\n"
}

tests['MEMBERP 1'] = {
    'in' :
    "show memberp \"this [this is a test]\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['MEMBERP 2'] = {
    'in' :
    "show memberp \"that [this is a test]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['MEMBERP 3'] = {
    'in' :
    "show memberp \"e \"hello\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['MEMBERP 4'] = {
    'in' :
    "show memberp \"t \"hello\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['MEMBERP 5'] = {
    'in' :
    "show memberp \"is \"this_is_a_test\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['SUBSTRINGP 1'] = {
    'in' :
    "show substringp \"this [this is a test]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['SUBSTRINGP 2'] = {
    'in' :
    "show substringp \"hi \"this\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['SUBSTRINGP 3'] = {
    'in' :
    "show substringp \"t \"hello\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['SUBSTRINGP 4'] = {
    'in' :
    "show substringp \"is \"this\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['NUMBERP 1'] = {
    'in' :
    "show numberp \"is\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['NUMBERP 2'] = {
    'in' :
    "show numberp \"1.00\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['NUMBERP 3'] = {
    'in' :
    "show numberp 1\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['NUMBERP 4'] = {
    'in' :
    "show numberp [1 2 3]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['VBARREDP 1'] = {
    'in' :
    "show vbarredp \"i\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['VBARREDP 2'] = {
    'in' :
    "show vbarredp \"|(|\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['VBARREDP 3'] = {
    'in' :
    "show vbarredp \"\\(\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['COUNT list'] = {
    'in' :
    "show count [1 2 3]\n"
    ,
    'out' :
    '? '
    "3\n"
}

tests['COUNT array'] = {
    'in' :
    "show count {1 2}\n"
    ,
    'out' :
    '? '
    "2\n"
}

tests['COUNT word'] = {
    'in' :
    "show count \"QLogo\n"
    ,
    'out' :
    '? '
    "5\n"
}

tests['ASCII 1'] = {
    'in' :
    "show ascii 1\n"
    ,
    'out' :
    '? '
    "49\n"
}

tests['ASCII 2'] = {
    'in' :
    "show ascii \"A\n"
    ,
    'out' :
    '? '
    "65\n"
}

tests['ASCII 3'] = {
    'in' :
    "show ascii \"*\n"
    ,
    'out' :
    '? '
    "42\n"
}

tests['ASCII 4'] = {
    'in' :
    "show ascii \"|(|\n"
    ,
    'out' :
    '? '
    "40\n"
}

tests['RAWASCII 1'] = {
    'in' :
    "show rawascii 1\n"
    ,
    'out' :
    '? '
    "49\n"
}

tests['RAWASCII 2'] = {
    'in' :
    "show rawascii \"A\n"
    ,
    'out' :
    '? '
    "65\n"
}

tests['RAWASCII 3'] = {
    'in' :
    "show rawascii \"*\n"
    ,
    'out' :
    '? '
    "42\n"
}

tests['RAWASCII 4'] = {
    'in' :
    "show rawascii \"|(|\n"
    ,
    'out' :
    '? '
    "6\n"
}

tests['CHAR'] = {
    'in' :
    "show char 65\n"
    ,
    'out' :
    '? '
    "A\n"
}

tests['MEMBER 1'] = {
    'in' :
    "show member \"e \"hello\n"
    ,
    'out' :
    '? '
    "ello\n"
}

tests['MEMBER 3'] = {
    'in' :
    "show member \"is [this is a test]\n"
    ,
    'out' :
    '? '
    "[is a test]\n"
}

tests['MEMBER 4'] = {
    'in' :
    "show member \"that [this is a test]\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['LOWERCASE'] = {
    'in' :
    "show lowercase \"Hello\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['UPPERCASE'] = {
    'in' :
    "show uppercase \"Hello\n"
    ,
    'out' :
    '? '
    "HELLO\n"
}

tests['PARSE'] = {
    'in' :
    "show parse \"2\\ 3\n"
    ,
    'out' :
    '? '
    "[2 3]\n"
}

tests['RUNPARSE 1'] = {
    'in' :
    "show runparse [print 2*2]\n"
    ,
    'out' :
    '? '
    "[print 2 * 2]\n"
}

tests['RUNPARSE 2'] = {
    'in' :
    "show runparse \"2*2\n"
    ,
    'out' :
    '? '
    "[2 * 2]\n"
}

tests['procedure params 1'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "tp 4 5\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 1 2 []\n"
}

tests['procedure params 2'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "(tp 4 5 6)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 6 2 []\n"
}

tests['procedure params 3'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "(tp 4 5 6 7)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 6 7 []\n"
}

tests['procedure params 4'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "(tp 4 5 6 7 8)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 6 7 [8]\n"
}

tests['procedure params 5'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "(tp 4 5 6 7 8 9)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 6 7 [8 9]\n"
}

tests['procedure params 6'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "(tp 4 5 6 7 8 9*9)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 6 7 [8 81]\n"
}

tests['procedure params 7'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "(tp 4 5 6 7*7 8)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 5 6 49 [8]\n"
}

tests['procedure params 8'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "tp 4 5*5\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 25 1 2 []\n"
}

tests['procedure params 9'] = {
    'in' :
    "to tp :p1 :p2 [:p3 3*3] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "tp 4 5*5\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 25 9 2 []\n"
}

tests['procedure params 10'] = {
    'in' :
    "to tp :p1 :p2 [:p3 :v1] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "make \"v1 20\n"
    "tp 4 5*5\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    '? '
    "4 25 20 2 []\n"
}

tests['procedure params 11'] = {
    'in' :
    "to tp :p1 :p2 [:p3 \"Iasonas] [:p4 2] [:p5]\n"
    "(show :p1 :p2 :p3 :p4 :p5)\n"
    "end\n"
    "tp 4 5*5\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "4 25 Iasonas 2 []\n"
}

tests['procedure params 12'] = {
    'in' :
    "to tp [:p1 [1 2 3]]\n"
    "show :p1\n"
    "end\n"
    "tp\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "[1 2 3]\n"
}

tests['procedure param err 1'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2]\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    "(tp 1 2 3 4 5)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "too many inputs to tp\n"
}

tests['procedure param err 2'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2]\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    "(tp 1)\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "not enough inputs to tp\n"
}

tests['procedure param err 3'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] 1\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "to doesn't like 1 as input\n"
}

tests['procedure param err 4'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] 4\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "to doesn't like 4 as input\n"
}

tests['procedure param err 5'] = {
    'in' :
    "to tp -2\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "to doesn't like -2 as input\n"
}

tests['procedure param err 6'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4  5 5]\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    "tp 1 2\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "tp defined\n"
    '? '
    "You don't say what to do with 5\n"
}

tests['procedure param err 7'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] 4.4\n"
    "(show :p1 :p2 :p3 :p4)\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "to doesn't like 4.4 as input\n"
}

tests['OUTPUT 1'] = {
    'in' :
    "to t1\n"
    "output 2*2\n"
    "end\n"
    "to t2\n"
    "output 2*t1\n"
    "end\n"
    "show t2\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "t1 defined\n"
    '? '
    '> '
    '> '
    "t2 defined\n"
    '? '
    "8\n"
}

tests['OUTPUT 2'] = {
    'in' :
    "to t1 :x\n"
    "output 2*:x\n"
    "end\n"
    "to t2\n"
    "output 2*t1 5\n"
    "end\n"
    "show t2\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "t1 defined\n"
    '? '
    '> '
    '> '
    "t2 defined\n"
    '? '
    "20\n"
}

tests['OUTPUT 3'] = {
    'in' :
    "to six\n"
    "output print 6\n"
    "end\n"
    "six\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "six defined\n"
    '? '
    "6\n"
    "print didn't output to output\n"
}

tests['procedure factorial 1'] = {
    'in' :
    "to factorial :x\n"
    "if :x = 1 [output 1]\n"
    "output :x * factorial :x-1\n"
    "end\n"
    "show factorial 5\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    '> '
    "factorial defined\n"
    '? '
    "120\n"
}

tests['procedure factorial 2'] = {
    'in' :
    "to factorial :x\n"
    "ifelse :x = 1 [output 1] [output :x * factorial :x-1]\n"
    "end\n"
    "show factorial 4\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "factorial defined\n"
    '? '
    "24\n"
}

tests['SE 1'] = {
    'in' :
    "show (se \"\\( 2 \"+ 3 \"\\))\n"
    ,
    'out' :
    '? '
    "[( 2 + 3 )]\n"
}

tests['SE 2'] = {
    'in' :
    "show (se \"make \"\"|(| 2)\n"
    ,
    'out' :
    '? '
    "[make \"( 2]\n"
}

tests['split []'] = {
    'in' :
    "make \"a [a b\n"
    "c] show :a"
    ,
    'out' :
    '? '
    '[ '
    "[a b c]\n"
}

tests['split {}'] = {
    'in' :
    "make \"a {a b\n"
    "c} show :a"
    ,
    'out' :
    '? '
    '{ '
    "{a b c}\n"
}

tests['split ~'] = {
    'in' :
    "make \"a ~\n"
    "\"c show :a"
    ,
    'out' :
    '? '
    '~ '
    "c\n"
}

tests['split |'] = {
    'in' :
    "make \"a \"|a b\n"
    "c| show :a"
    ,
    'out' :
    '? '
    '| '
    "a b\nc\n"
}

tests['split [~;'] = {
    'in' :
    "make \"a [a b;comment~\n"
    "c] show :a"
    ,
    'out' :
    '? '
    '~ '
    "[a bc]\n"
}

tests['split ~;'] = {
    'in' :
    "make \"a \"ab;comment~\n"
    "c show :a"
    ,
    'out' :
    '? '
    '~ '
    "abc\n"
}

tests['split ~ ;'] = {
    'in' :
    "make \"a [a b ;comment~\n"
    "c] show :a"
    ,
    'out' :
    '? '
    '~ '
    "[a b c]\n"
}

tests['unexpected ]'] = {
    'in' :
    "make \"a ]\n"
    ,
    'out' :
    '? '
    "unexpected ']'\n"
}

tests['unexpected }'] = {
    'in' :
    "make \"a }\n"
    ,
    'out' :
    '? '
    "unexpected '}'\n"
}

tests['double to'] = {
    'in' :
    "to oneThing\n"
    "to another\n"
    "end\n"
    "oneThing\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "oneThing defined\n"
    '? '
    "can't use to inside a procedure in oneThing\n"
    "[to another]\n"
}

tests['built-in defined'] = {
    'in' :
    "to print\n"
    ,
    'out' :
    '? '
    "print is already defined\n"
}

tests['TO (no name)'] = {
    'in' :
    "to\n"
    ,
    'out' :
    '? '
    "not enough inputs to to\n"
}

tests['TO :name'] = {
    'in' :
    "to :name\n"
    ,
    'out' :
    '? '
    "to doesn't like :name as input\n"
}

tests['TO "name'] = {
    'in' :
    "to \"name\n"
    ,
    'out' :
    '? '
    "to doesn't like \"name as input\n"
}

tests['TO bad param 1'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p4 2 2] [:p5]\n"
    "end\n"
    "tp 1 2\n"
    ,
    'out' :
    '? '
    '> '
    "tp defined\n"
    '? '
    "You don't say what to do with 2\n"
}

tests['TO bad param 2'] = {
    'in' :
    "to tp :p1 [:p3 1] :p2 [:p4 2] [:p5]\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    "to doesn't like :p2 as input\n"
}

tests['TO bad param 3'] = {
    'in' :
    "to tp :p1 :p2 [:p3 1] [:p5] [:p4 2]\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    "to doesn't like [:p4 2] as input\n"
}

tests['TO bad param 4'] = {
    'in' :
    "to tp :p1 [:p3 1] [:p4 2] [:p5] :p2\n"
    "end\n"
    ,
    'out' :
    '? '
    '> '
    "to doesn't like :p2 as input\n"
}

tests['DEFINE 1'] = {
    'in' :
    "define \"c1 [[] [print \"hi]]\n"
    "c1\n"
    ,
    'out' :
    '? '
    '? '
    "hi\n"
}

tests['DEFINE 2'] = {
    'in' :
    "define \"another [[] [to another]]\n"
    "another\n"
    ,
    'out' :
    '? '
    '? '
    "can't use to inside a procedure in another\n"
    "[to another]\n"
}

tests['DEFINE 3'] = {
    'in' :
    "define \"print [[] [type [hello]]]\n"
    ,
    'out' :
    '? '
    "print is a primitive\n"
}

tests['DEFINE 4'] = {
    'in' :
    "define \"p2 [[p1 p2] [(print \"Hello :p1 :p2)]]\n"
    "p2 \"Iasonas \"Psyches\n"
    ,
    'out' :
    '? '
    '? '
    "Hello Iasonas Psyches\n"
}

tests['DEFINE 5'] = {
    'in' :
    "define \"p3 [[p1 [p2 \"whatever]] [(print \"Hello :p1 :p2)]]\n"
    "p3 \"Iasonas\n"
    ,
    'out' :
    '? '
    '? '
    "Hello Iasonas whatever\n"
}

tests['DEFINE 6'] = {
    'in' :
    "define \"p4 [[p1 [p2 \"whatever]] [(print \"Hello :p1 :p2)]]\n"
    "(p4 \"Iasonas \"Psyches)\n"
    ,
    'out' :
    '? '
    '? '
    "Hello Iasonas Psyches\n"
}

tests['DEFINE 7'] = {
    'in' :
    "to qw :p1 [:p2 2*2]\n"
    "(show \"Hello, :p1 :p2)\n"
    "end\n"
    "qw 10\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "Hello, 10 4\n"
}

tests['DEFINE notList error'] = {
    'in' :
    "define \"proc1 [[] [print \"hello] \"show\\ 5+5]\n"
    ,
    'out' :
    '? '
    "define doesn't like [[] [print \"hello] \"show 5+5] as input\n"
}

tests['no how 1'] = {
    'in' :
    "nohow\n"
    ,
    'out' :
    '? '
    "I don't know how to nohow\n"
}

tests['no how 2'] = {
    'in' :
    "(nohow)\n"
    ,
    'out' :
    '? '
    "I don't know how to nohow\n"
}

tests['no value 1'] = {
    'in' :
    "print :novalue\n"
    ,
    'out' :
    '? '
    "novalue has no value\n"
}

tests['no value 2'] = {
    'in' :
    "print thing \"novalue\n"
    ,
    'out' :
    '? '
    "novalue has no value\n"
}

tests['no )'] = {
    'in' :
    "print (sqrt 2\n"
    ,
    'out' :
    '? '
    "')' not found\n"
}

tests['no say 1'] = {
    'in' :
    "sqrt 4\n"
    ,
    'out' :
    '? '
    "You don't say what to do with 2\n"
}

tests['not enough inputs 1'] = {
    'in' :
    "print (sqrt)\n"
    ,
    'out' :
    '? '
    "not enough inputs to sqrt\n"
}

tests['not enough inputs 2'] = {
    'in' :
    "print sqrt\n"
    ,
    'out' :
    '? '
    "not enough inputs to sqrt\n"
}

tests['too many inputs'] = {
    'in' :
    "print (sqrt 4 9)\n"
    ,
    'out' :
    '? '
    "too many inputs to sqrt\n"
}

tests['no output'] = {
    'in' :
    "print make \"a 10\n"
    ,
    'out' :
    '? '
    "make didn't output to print\n"
}

tests['make [a]'] = {
    'in' :
    "make [a] 3\n"
    ,
    'out' :
    '? '
    "make doesn't like [a] as input\n"
}

tests['add with string'] = {
    'in' :
    "print 2 + \"b\n"
    ,
    'out' :
    '? '
    "+ doesn't like b as input\n"
}

tests['add with list'] = {
    'in' :
    "print 2 + [a]\n"
    ,
    'out' :
    '? '
    "+ doesn't like [a] as input\n"
}

tests['unary minus with number'] = {
    'in' :
    "show runparse \"1\\ -1\n"
    ,
    'out' :
    '? '
    "[1 -1]\n"
}

tests['binary minus with negative number'] = {
    'in' :
    "show runparse \"1-\\ -1\n"
    ,
    'out' :
    '? '
    "[1 - -1]\n"
}

tests['unary minus with var'] = {
    'in' :
    "show runparse \"-:a\n"
    ,
    'out' :
    '? '
    "[0 -- :a]\n"
}

tests['unary minus with var in list'] = {
    'in' :
    "show runparse \"1\\ -:a\n"
    ,
    'out' :
    '? '
    "[1 0 -- :a]\n"
}

tests['number format 1'] = {
    'in' :
    "show 2e2\n"
    ,
    'out' :
    '? '
    "200\n"
}

tests['number format 2'] = {
    'in' :
    "show 3.e2\n"
    ,
    'out' :
    '? '
    "300\n"
}

tests['number format 3'] = {
    'in' :
    "show 2.2e2\n"
    ,
    'out' :
    '? '
    "220\n"
}

tests['number format 4'] = {
    'in' :
    "show 5E2\n"
    ,
    'out' :
    '? '
    "500\n"
}

tests['number format 6'] = {
    'in' :
    "show 20e2\n"
    ,
    'out' :
    '? '
    "2000\n"
}

tests['number format 7'] = {
    'in' :
    "show 1e2+2\n"
    ,
    'out' :
    '? '
    "102\n"
}

tests['number format 8'] = {
    'in' :
    "show 2e2+(3*4)\n"
    ,
    'out' :
    '? '
    "212\n"
}

tests['number format 9'] = {
    'in' :
    "show 3e2*-2\n"
    ,
    'out' :
    '? '
    "-600\n"
}

tests['number format 10'] = {
    'in' :
    "show 2e+1\n"
    ,
    'out' :
    '? '
    "20\n"
}

tests['number format 11'] = {
    'in' :
    "show 2e-1\n"
    ,
    'out' :
    '? '
    "0.2\n"
}

tests['number format 12'] = {
    'in' :
    "make \"a 10\n"
    "show -:a\n"
    ,
    'out' :
    '? '
    '? '
    "-10\n"
}

tests['define operator +'] = {
    'in' :
    "to +\n"
    ,
    'out' :
    '? '
    "+ is already defined\n"
}

tests['define to'] = {
    'in' :
    "to to\n"
    ,
    'out' :
    '? '
    "to is already defined\n"
}

# TODO: BAD test
# tests['standout'] = {
#         'in' :
#     "show standout \"bold\n"
# ,
#         'out' :
#     '? '
# "<b>bold</b>\n"
# }

tests['shell 1'] = {
    'in' :
    "show shell [echo hello]\n"
    ,
    'out' :
    '? '
    "[[hello]]\n"
}

tests['shell 2'] = {
    'in' :
    "show (shell [echo hello] [])\n"
    ,
    'out' :
    '? '
    "[hello]\n"
}

tests['prefix 1'] = {
    'in' :
    "show prefix\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['prefix 2'] = {
    'in' :
    "setprefix \"newPrefix\n"
    "show prefix\n"
    ,
    'out' :
    '? '
    '? '
    "newPrefix\n"
}

# TODO: In windows the results for some of these file I/O will be different
tests['file IO 1'] = {
    'in' :
    "make \"f \"TestQLogoFileIO1.txt\n"
    "openwrite :f\n"
    "setwrite :f\n"
    "print [this is a test.]\n"
    "closeall\n"
    "openread :f\n"
    "setread :f\n"
    "show readrawline\n"
    "closeall\n"
    "erf :f\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "this is a test.\n"
    '? '
    '? '
}

tests['file IO 2'] = {
    'in' :
    "make \"f \"TestQLogoFileIO2.txt\n"
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
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "16\n"
    '? '
    '? '
}

tests['file IO 3'] = {
    'in' :
    "make \"f \"TestQLogoFileIO3.txt\n"
    "openwrite :f\n"
    "setwrite :f\n"
    "print [this is a test]\n"
    "make \"a writepos\n"
    "closeall\n"
    "erf :f\n"
    "print :a\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "15\n"
}

tests['file IO 4'] = {
    'in' :
    "make \"f \"TestQLogoFileIO4.txt\n"
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
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "26\n"
}

tests['file IO 5'] = {
    'in' :
    "openwrite \"TestQLogoFileIO5.txt\n"
    "show allopen\n"
    "close \"TestQLogoFileIO5.txt\n"
    "erf \"TestQLogoFileIO5.txt\n"
    ,
    'out' :
    '? '
    '? '
    "[TESTQLOGOFILEIO5.TXT]\n"
    '? '
    '? '
}

tests['file IO 6'] = {
    'in' :
    "make \"f \"TestQLogoFileIO6.txt\n"
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
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "that was another test\n"
}

tests['file IO 7'] = {
    'in' :
    "openupdate \"TestQLogoFileIO7.txt\n"
    "setwrite \"TestQLogoFileIO7.txt\n"
    "make \"a writer\n"
    "setwrite []\n"
    "show :a\n"
    "setread \"TestQLogoFileIO7.txt\n"
    "show reader\n"
    "close \"TestQLogoFileIO7.txt\n"
    "erf \"TestQLogoFileIO7.txt\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    "TESTQLOGOFILEIO7.TXT\n"
    '? '
    '? '
    "TESTQLOGOFILEIO7.TXT\n"
    '? '
    '? '
}

tests['file IO 8'] = {
    'in' :
    "make \"f \"TestQLogoFileIO8.txt\n"
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
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "false\n"
    '? '
    "0\n"
    '? '
    '? '
    "true\n"
    '? '
    "15\n"
    '? '
    '? '
}

tests['file IO 9'] = {
    'in' :
    "openwrite \"TestQLogoFileIO9.txt\n"
    "close \"TestQLogoFileIO9.txt\n"
    "show allopen\n"
    "erf \"TestQLogoFileIO9.txt\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[]\n"
    '? '
}

tests['file IO 10'] = {
    'in' :
    "openwrite \"TestQLogoFileIO10a.txt\n"
    "openwrite \"TestQLogoFileIO10b.txt\n"
    "closeall\n"
    "show allopen\n"
    "erf \"TestQLogoFileIO10a.txt\n"
    "erf \"TestQLogoFileIO10b.txt\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    "[]\n"
    '? '
    '? '
}

tests['string IO 1'] = {
    'in' :
    "openwrite [text 100]\n"
    "setwrite \"text\n"
    "show allopen\n"
    "closeall\n"
    "show first :text\n"
    "show last butlast :text\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    "[\n"
    '? '
    "]"
    "\n"
}

tests['string IO 2'] = {
    'in' :
    "make \"t \"io\n"
    "openread [t -50]\n"
    "setread \"t\n"
    "show readword\n"
    "closeall\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    "io\n"
    '? '
}

tests['string IO 3'] = {
    'in' :
    "make \"line \"go_\n"
    "openwrite [line 50 50]\n"
    "setwrite \"line\n"
    "print \"Cougs\n"
    "closeall\n"
    "show :line\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "go_Cougs\n\n"
}

tests['string IO 4'] = {
    'in' :
    "openread [line]\n"
    "setread \"line\n"
    "show readrawline\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[]\n"
}

tests['sum 1'] = {
    'in' :
    "show (sum)\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['sum 2'] = {
    'in' :
    "show (sum 1)\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['sum 3'] = {
    'in' :
    "show (sum 3 4)\n"
    ,
    'out' :
    '? '
    "7\n"
}

tests['sum 4'] = {
    'in' :
    "show (sum 7 8 9)\n"
    ,
    'out' :
    '? '
    "24\n"
}

tests['product 1'] = {
    'in' :
    "show (product)\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['product 2'] = {
    'in' :
    "show (product 5)\n"
    ,
    'out' :
    '? '
    "5\n"
}

tests['product 3'] = {
    'in' :
    "show (product 3 4)\n"
    ,
    'out' :
    '? '
    "12\n"
}

tests['product 4'] = {
    'in' :
    "show (product 7 8 9)\n"
    ,
    'out' :
    '? '
    "504\n"
}

tests['difference 1'] = {
    'in' :
    "show difference 10 8\n"
    ,
    'out' :
    '? '
    "2\n"
}

tests['MINUS 1'] = {
    'in' :
    "show MINUS 10 + 8\n"
    ,
    'out' :
    '? '
    "-18\n"
}

tests['MINUS 2'] = {
    'in' :
    "show - 2 + 8\n"
    ,
    'out' :
    '? '
    "-10\n"
}

tests['QUOTIENT 1'] = {
    'in' :
    "show QUOTIENT 48 8\n"
    ,
    'out' :
    '? '
    "6\n"
}

tests['QUOTIENT 2'] = {
    'in' :
    "show (QUOTIENT 5)\n"
    ,
    'out' :
    '? '
    "0.2\n"
}

tests['QUOTIENT 3'] = {
    'in' :
    "show QUOTIENT 4 0\n"
    ,
    'out' :
    '? '
    "QUOTIENT doesn't like 0 as input\n"
}

tests['QUOTIENT 4'] = {
    'in' :
    "show (QUOTIENT 0)\n"
    ,
    'out' :
    '? '
    "QUOTIENT doesn't like 0 as input\n"
}

tests['REMAINDER 1'] = {
    'in' :
    "show 14 % 6\n"
    ,
    'out' :
    '? '
    "2\n"
}

tests['REMAINDER 2'] = {
    'in' :
    "show remainder -21 4\n"
    ,
    'out' :
    '? '
    "-1\n"
}

tests['REMAINDER 3'] = {
    'in' :
    "show 4 % 0\n"
    ,
    'out' :
    '? '
    "% doesn't like 0 as input\n"
}

tests['REMAINDER 4'] = {
    'in' :
    "show remainder 14 0\n"
    ,
    'out' :
    '? '
    "remainder doesn't like 0 as input\n"
}

tests['MODULO 1'] = {
    'in' :
    "show modulo 14 6\n"
    ,
    'out' :
    '? '
    "2\n"
}

tests['MODULO 2'] = {
    'in' :
    "show modulo -21 4\n"
    ,
    'out' :
    '? '
    "3\n"
}

tests['MODULO 3'] = {
    'in' :
    "show modulo 30 -11\n"
    ,
    'out' :
    '? '
    "-3\n"
}

tests['MODULO 4'] = {
    'in' :
    "show modulo 14 0\n"
    ,
    'out' :
    '? '
    "modulo doesn't like 0 as input\n"
}

tests['MODULO 5'] = {
    'in' :
    "show modulo -21 -4\n"
    ,
    'out' :
    '? '
    "-1\n"
}

tests['INT 1'] = {
    'in' :
    "show int 14\n"
    ,
    'out' :
    '? '
    "14\n"
}

tests['INT 2'] = {
    'in' :
    "show int -21\n"
    ,
    'out' :
    '? '
    "-21\n"
}

tests['INT 3'] = {
    'in' :
    "show int 30.5\n"
    ,
    'out' :
    '? '
    "30\n"
}

tests['INT 4'] = {
    'in' :
    "show int -30.5\n"
    ,
    'out' :
    '? '
    "-30\n"
}

tests['ROUND 1'] = {
    'in' :
    "show round 14\n"
    ,
    'out' :
    '? '
    "14\n"
}

tests['ROUND 2'] = {
    'in' :
    "show round -21\n"
    ,
    'out' :
    '? '
    "-21\n"
}

tests['ROUND 3'] = {
    'in' :
    "show round 30.5\n"
    ,
    'out' :
    '? '
    "31\n"
}

tests['ROUND 4'] = {
    'in' :
    "show round -30.5\n"
    ,
    'out' :
    '? '
    "-31\n"
}

tests['POWER 1'] = {
    'in' :
    "show power 4 2\n"
    ,
    'out' :
    '? '
    "16\n"
}

tests['POWER 2'] = {
    'in' :
    "show power -2 5\n"
    ,
    'out' :
    '? '
    "-32\n"
}

tests['POWER 3'] = {
    'in' :
    "show power 9 .5\n"
    ,
    'out' :
    '? '
    "3\n"
}

tests['POWER 4'] = {
    'in' :
    "show power -4 .5\n"
    ,
    'out' :
    '? '
    "power doesn't like 0.5 as input\n"
}

tests['EXP 1'] = {
    'in' :
    "show first exp 2\n"
    ,
    'out' :
    '? '
    "7\n"
}

tests['EXP 2'] = {
    'in' :
    "show exp 0\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['LOG10 1'] = {
    'in' :
    "show log10 10\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['LOG10 2'] = {
    'in' :
    "show log10 0.01\n"
    ,
    'out' :
    '? '
    "-2\n"
}

tests['LN 1'] = {
    'in' :
    "show ln 1\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['LN 2'] = {
    'in' :
    "show first ln 100\n"
    ,
    'out' :
    '? '
    "4\n"
}

tests['SIN 1'] = {
    'in' :
    "show sin 0\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['SIN 2'] = {
    'in' :
    "show sin 90\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['SIN 3'] = {
    'in' :
    "show sin 270\n"
    ,
    'out' :
    '? '
    "-1\n"
}

tests['RADSIN 1'] = {
    'in' :
    "show radsin 0\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['RADSIN 2'] = {
    'in' :
    "show first radsin 4\n"
    ,
    'out' :
    '? '
    "-\n"
}

tests['COS 1'] = {
    'in' :
    "show cos 0\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['COS 2'] = {
    'in' :
    "show cos 180\n"
    ,
    'out' :
    '? '
    "-1\n"
}

tests['RADCOS 1'] = {
    'in' :
    "show radcos 0\n"
    ,
    'out' :
    '? '
    "1\n"
}

tests['RADCOS 2'] = {
    'in' :
    "show first radcos 2\n"
    ,
    'out' :
    '? '
    "-\n"
}

tests['ARCTAN 1'] = {
    'in' :
    "show arctan 0\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['ARCTAN 2'] = {
    'in' :
    "show arctan 1\n"
    ,
    'out' :
    '? '
    "45\n"
}

tests['ARCTAN 3'] = {
    'in' :
    "show arctan -1\n"
    ,
    'out' :
    '? '
    "-45\n"
}

tests['ARCTAN 4'] = {
    'in' :
    "show (arctan -1 -1)\n"
    ,
    'out' :
    '? '
    "-135\n"
}

tests['ARCTAN 5'] = {
    'in' :
    "show (arctan 1 -1)\n"
    ,
    'out' :
    '? '
    "-45\n"
}

tests['ARCTAN 6'] = {
    'in' :
    "show (arctan -1 1)\n"
    ,
    'out' :
    '? '
    "135\n"
}

tests['RADARCTAN 1'] = {
    'in' :
    "show first (radarctan -1 0)\n"
    ,
    'out' :
    '? '
    "3\n"
}

tests['LESSP false'] = {
    'in' :
    "show lessp 4 2\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['LESSP true'] = {
    'in' :
    "show lessp 4 8\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LESS? false'] = {
    'in' :
    "show less? 4 2\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['LESS? true'] = {
    'in' :
    "show less? 4 8\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['GREATERP false'] = {
    'in' :
    "show greaterp 3 6\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['GREATERP true'] = {
    'in' :
    "show greaterp 5 2\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['GREATER? false'] = {
    'in' :
    "show greater? 3 4\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['GREATER? true'] = {
    'in' :
    "show greater? 5 4\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LESSEQUALP false'] = {
    'in' :
    "show lessequalp 4 2\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['LESSEQUALP true 1'] = {
    'in' :
    "show lessequalp 4 8\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LESSEQUALP true 2'] = {
    'in' :
    "show lessequalp 5 5\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LESSEQUAL? false'] = {
    'in' :
    "show lessequal? 4 2\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['LESSEQUAL? true 1'] = {
    'in' :
    "show lessequal? 4 8\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['LESSEQUAL? true 2'] = {
    'in' :
    "show lessequal? 4 4\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['GREATEREQUALP false'] = {
    'in' :
    "show greaterequalp 2 4\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['GREATEREQUALP true 1'] = {
    'in' :
    "show greaterequalp 8 4\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['GREATEREQUALP true 2'] = {
    'in' :
    "show greaterequalp 5 5\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['GREATEREQUAL? false'] = {
    'in' :
    "show greaterequal? 2 4\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['GREATEREQUAL? true 1'] = {
    'in' :
    "show greaterequal? 8 4\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['GREATEREQUAL? true 2'] = {
    'in' :
    "show greaterequal? 4 4\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['FORM 1'] = {
    'in' :
    "show form 1.1 10 4\n"
    ,
    'out' :
    '? '
    "    1.1000\n"
}

tests['FORM 2'] = {
    'in' :
    "show form 1.2 -10 4\n"
    ,
    'out' :
    '? '
    "1.2000    \n"
}

tests['FORM 3'] = {
    'in' :
    "show form 1.3 2 0\n"
    ,
    'out' :
    '? '
    " 1\n"
}

tests['FORM 4'] = {
    'in' :
    "show form -1.4 10 4\n"
    ,
    'out' :
    '? '
    "   -1.4000\n"
}

tests['BITAND 1'] = {
    'in' :
    "show bitand 10 4\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['BITAND 2'] = {
    'in' :
    "show bitand -1 5\n"
    ,
    'out' :
    '? '
    "5\n"
}

tests['BITAND 3'] = {
    'in' :
    "show (bitand 15 7 30)\n"
    ,
    'out' :
    '? '
    "6\n"
}

tests['BITOR 1'] = {
    'in' :
    "show bitor 10 4\n"
    ,
    'out' :
    '? '
    "14\n"
}

tests['BITOR 2'] = {
    'in' :
    "show bitor 2 5\n"
    ,
    'out' :
    '? '
    "7\n"
}

tests['BITOR 3'] = {
    'in' :
    "show (bitor 15 7 32)\n"
    ,
    'out' :
    '? '
    "47\n"
}

tests['BITXOR 1'] = {
    'in' :
    "show bitxor 10 4\n"
    ,
    'out' :
    '? '
    "14\n"
}

tests['BITXOR 2'] = {
    'in' :
    "show bitxor 7 5\n"
    ,
    'out' :
    '? '
    "2\n"
}

tests['BITXOR 3'] = {
    'in' :
    "show (bitxor 15 7 32)\n"
    ,
    'out' :
    '? '
    "40\n"
}

tests['BITNOT 1'] = {
    'in' :
    "show bitnot 0\n"
    ,
    'out' :
    '? '
    "-1\n"
}

tests['BITNOT 2'] = {
    'in' :
    "show bitnot -1\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['BITNOT 3'] = {
    'in' :
    "show bitnot 2\n"
    ,
    'out' :
    '? '
    "-3\n"
}

tests['ASHIFT 1'] = {
    'in' :
    "show ashift 0 2\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['ASHIFT 2'] = {
    'in' :
    "show ashift 3 2\n"
    ,
    'out' :
    '? '
    "12\n"
}

tests['ASHIFT 3'] = {
    'in' :
    "show ashift 24 -2\n"
    ,
    'out' :
    '? '
    "6\n"
}

tests['ASHIFT 4'] = {
    'in' :
    "show ashift -32 -2\n"
    ,
    'out' :
    '? '
    "-8\n"
}

tests['LSHIFT 1'] = {
    'in' :
    "show lshift 0 2\n"
    ,
    'out' :
    '? '
    "0\n"
}

tests['LSHIFT 2'] = {
    'in' :
    "show lshift 3 2\n"
    ,
    'out' :
    '? '
    "12\n"
}

tests['LSHIFT 3'] = {
    'in' :
    "show lshift 24 -2\n"
    ,
    'out' :
    '? '
    "6\n"
}

tests['AND 1'] = {
    'in' :
    "show and \"true \"true\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['AND 2'] = {
    'in' :
    "show and \"false \"true\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['AND 3'] = {
    'in' :
    "show and \"true \"false\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['AND 4'] = {
    'in' :
    "show (and \"true \"false \"true)\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['AND 5'] = {
    'in' :
    "show (and \"true \"true \"true)\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['AND 6'] = {
    'in' :
    "show (and \"true)\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['AND 7'] = {
    'in' :
    "show (and \"false)\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['AND and NOT list'] = {
    'in' :
    "show AND [NOT (0 = 0)] [(1 / 0) > .5]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['OR 1'] = {
    'in' :
    "show or \"true \"true\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['OR 2'] = {
    'in' :
    "show or \"false \"true\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['OR 3'] = {
    'in' :
    "show or \"true \"false\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['OR 4'] = {
    'in' :
    "show (or \"true \"false \"true)\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['OR 5'] = {
    'in' :
    "show (or \"false \"false \"false)\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['OR 6'] = {
    'in' :
    "show (or \"true)\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['OR 7'] = {
    'in' :
    "show (or \"false)\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['OR and NOT list'] = {
    'in' :
    "show OR [NOT (0 = 0)] [(1 / 1) > .5]\n"
    ,
    'out' :
    '? '
    "true\n"
}

# TODO: BAD test result
# tests['DRIBBLE'] = {
#         'in' :
#     "make \"d \"dribble.txt\n"
#                               "dribble :d\n"
#                               "print [hi]\n"
#                               "nodribble\n"
#                               "openread :d\n"
#                               "setread :d\n"
#                               "show readrawline\n"
#                               "show readrawline\n"
#                               "close :d\n"
#                               "erf :d\n"
# ,
#         'out' :
#     '? '
#     '? '
#     '? '
# "hi\n"
#     '? '
#     '? '
#     '? '
#     '? '
#     '? '
#     "hi\n"
#     '? '
#     "[]\n"
#     '? '
#     '? '
# }

tests['double DRIBBLE'] = {
    'in' :
    "make \"d \"dribble2.txt\n"
    "dribble :d\n"
    "dribble :d\n"
    "nodribble\n"
    "erf :d\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "already dribbling\n"
    '? '
    '? '
}

# These are turtle tests
# tests['HEADING 1'] = {
#         'in' :
#     "rt 90\n"
#                                 "show heading\n"
# ,
#         'out' :
# "270\n"
# }

# tests['HEADING 2'] = {
#         'in' :
#     "rt 120\n"
#                                 "show (heading \"z)\n"
# ,
#         'out' :
# "240\n"
# }

# tests['SETHEADING 1'] = {
#         'in' :
#     "rt 90\n"
#                                    "seth 30\n"
#                                    "show heading\n"
# ,
#         'out' :
# "30\n"
# }

# tests['SETHEADING 2'] = {
#         'in' :
#     "rt 120\n"
#                                    "(setheading 40 \"z)\n"
#                                    "show (heading \"z)\n"
# ,
#         'out' :
# "40\n"
# }

# tests['TOWARDS 1'] = {
#         'in' :
#     "show towards [-1 1]\n"
# ,
#         'out' :
# "45\n"
# }

# tests['TOWARDS 2'] = {
#         'in' :
#     "fd 1\n"
#                                 "show towards [1 1]\n"
# ,
#         'out' :
# "270\n"
# }

# tests['SETPOS'] = {
#         'in' :
#     "setpos [-1 1]\n"
#                              "show towards [0 2]\n"
# ,
#         'out' :
# "315\n"
# }

# tests['PENDOWNP'] = {
#         'in' :
#     "show pendownp\n"
#                                "pu show pendown?\n"
# ,
#         'out' :
# "true\nfalse\n"
# }

# tests['PENCOLOR 1'] = {
#         'in' :
#     "setpc 0\n"
#                                  "show pc\n"
# ,
#         'out' :
# "[0 0 0]\n"
# }

# tests['PENCOLOR 2'] = {
#         'in' :
#     "setpc \"magenta\n"
#                                  "show pc\n"
# ,
#         'out' :
# "[100 0 100]\n"
# }

# tests['PENCOLOR 3'] = {
#         'in' :
#     "setpc [50 50 50]\n"
#                                  "show pc\n"
# ,
#         'out' :
# "[50 50 50]\n"
# }

# tests['PALETTE 1'] = {
#         'in' :
#     "setpalette 30 [50 50 50]\n"
#                                 "show palette 30\n"
# ,
#         'out' :
# "[50 50 50]\n"
# }

# tests['PALETTE 2'] = {
#         'in' :
#     "setpalette 31 \"yellow\n"
#                                 "show palette 31\n"
# ,
#         'out' :
# "[100 100 0]\n"
# }

# tests['PALETTE 3'] = {
#         'in' :
#     "setpalette 32 7\n"
#                                 "show palette 32\n"
# ,
#         'out' :
# "[100 100 100]\n"
# }

# tests['SCRUNCH zero'] = {
#         'in' :
#     "setscrunch 1 0\n"
# ,
#         'out' :
# "setscrunch doesn't like 0 as input\n"
# }

tests['TEXT 1'] = {
    'in' :
    "to qw\n"
    "show \"Hello\n"
    "end\n"
    "show text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[[] [show \"Hello]]\n"
}

tests['TEXT 2'] = {
    'in' :
    "to qw :p1\n"
    "(show \"Hello, :p1)\n"
    "end\n"
    "show text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[[P1] [(show \"Hello, :p1)]]\n"
}

tests['TEXT 3'] = {
    'in' :
    "to qw :p1 [:p2 2]\n"
    "(show \"Hello, :p1 :p2)\n"
    "end\n"
    "show text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[[P1 [P2 2]] [(show \"Hello, :p1 :p2)]]\n"
}

tests['TEXT 4'] = {
    'in' :
    "to qw :p1 [:p2 2*2]\n"
    "(show \"Hello, :p1 :p2)\n"
    "end\n"
    "show text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[[P1 [P2 2*2]] [(show \"Hello, :p1 :p2)]]\n"
}

tests['TEXT 5'] = {
    'in' :
    "to qw :p1 [:p2 2*2] [:p3]\n"
    "(show \"Hello, :p1 :p2 \"and :p3)\n"
    "end\n"
    "show text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[[P1 [P2 2*2] [P3]] [(show \"Hello, :p1 :p2 \"and :p3)]]\n"
}

tests['TEXT 6'] = {
    'in' :
    "to qw :p1 [:p2 2*2] [:p3] 10\n"
    "(show \"Hello, :p1 :p2 \"and :p3)\n"
    "end\n"
    "show text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[[P1 [P2 2*2] [P3] 10] [(show \"Hello, :p1 :p2 \"and :p3)]]\n"
}

tests['TEXT 7'] = {
    'in' :
    "to qw\n"
    "show \"Hello\n"
    "end\n"
    "show first text \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[]\n"
}

tests['FULLTEXT 1'] = {
    'in' :
    "to qw\n"
    "show \"Hello\n"
    "end\n"
    "show fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[to qw show \"Hello end]\n"
}

tests['FULLTEXT 2'] = {
    'in' :
    "to qw :p1\n"
    "(show \"Hello, :p1)\n"
    "end\n"
    "show fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[to qw :p1 (show \"Hello, :p1) end]\n"
}

tests['FULLTEXT 3'] = {
    'in' :
    "to qw :p1 [:p2 2]\n"
    "(show \"Hello, :p1 :p2)\n"
    "end\n"
    "show fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[to qw :p1 [:p2 2] (show \"Hello, :p1 :p2) end]\n"
}

tests['FULLTEXT 4'] = {
    'in' :
    "to qw :p1 [:p2 2*2]\n"
    "(show \"Hello, :p1 :p2)\n"
    "end\n"
    "show fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[to qw :p1 [:p2 2*2] (show \"Hello, :p1 :p2) end]\n"
}

tests['FULLTEXT 5'] = {
    'in' :
    "to qw :p1 [:p2 2*2] [:p3]\n"
    "(show \"Hello, :p1 :p2 \"and :p3)\n"
    "end\n"
    "show fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[to qw :p1 [:p2 2*2] [:p3] (show \"Hello, :p1 :p2 \"and :p3) end]\n"
}

tests['FULLTEXT 6'] = {
    'in' :
    "to qw :p1 [:p2 2*2] [:p3] 10\n"
    "(show \"Hello, :p1 :p2 \"and :p3)\n"
    "end\n"
    "show fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "[to qw :p1 [:p2 2*2] [:p3] 10 (show \"Hello, "
    ":p1 :p2 \"and :p3) end]\n"
}

tests['FULLTEXT 7'] = {
    'in' :
    "to qw\n"
    "show \"Hello\n"
    "end\n"
    "show first fulltext \"qw\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    "to qw\n"
}

# TODO: COPYDEF is slated for removal
tests['COPYDEF 1'] = {
    'in' :
    "to qw\n"
    "show \"Hello\n"
    "end\n"
    "copydef \"we \"qw\n"
    "we\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "qw defined\n"
    '? '
    '? '
    "Hello\n"
}

# TODO: I don't know why this was originally commented out.
# //tests['COPYDEF 2'] = {
#     'in' :
#     "to qw\n"
#     //                                "show \"Hello\n"
#     //                                "end\n"
#     //                                "copydef \"we \"qw\n"
#     //                                "show fulltext \"we\n"
#     //                             ,
#     'out' :
#     "qw defined\n"
#     //                                "[to we show \"Hello end]\n"
# }

tests['COPYDEF 3'] = {
    'in' :
    "copydef \"tnirp \"print\n"
    "tnirp \"QWERTY\n"
    ,
    'out' :
    '? '
    '? '
    "QWERTY\n"
}

tests['LOCAL 1'] = {
    'in' :
    "to qw :p1\n"
    "local \"a\n"
    "make \"a :p1\n"
    "show :a\n"
    "end\n"
    "make \"a 12\n"
    "qw 23\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    "qw defined\n"
    '? '
    '? '
    "23\n"
    '? '
    "12\n"
}

tests['LOCAL 2'] = {
    'in' :
    "to qw :p1\n"
    "local \"a\n"
    "make \"a :p1\n"
    "show :a\n"
    "end\n"
    "qw 23\n"
    "show :a\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    "qw defined\n"
    '? '
    "23\n"
    '? '
    "a has no value\n"
}

tests['LOCAL 3'] = {
    'in' :
    "to qw :p1 :p2\n"
    "local [a b]\n"
    "make \"a :p1\n"
    "make \"b :p2\n"
    "(show :a :b)\n"
    "end\n"
    "make \"a 12\n"
    "qw 23 34\n"
    "show :a\n"
    "show :b\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    '> '
    "qw defined\n"
    '? '
    '? '
    "23 34\n"
    '? '
    "12\n"
    '? '
    "b has no value\n"
}

tests['LOCAL 4'] = {
    'in' :
    "to qw :p1\n"
    "local {a}\n"
    "make \"a :p1\n"
    "show :a\n"
    "end\n"
    "qw 23\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    "qw defined\n"
    '? '
    "local doesn't like {a} as input in qw\n"
    "[local {a}]\n"
}

tests['PLIST 1'] = {
    'in' :
    "pprop 1 2 3\n"
    "show gprop 1 2\n"
    ,
    'out' :
    '? '
    '? '
    "3\n"
}

tests['PLIST 2'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 3 4\n"
    "show gprop 1 3\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "4\n"
}

tests['PLIST 3'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 3 4\n"
    "show gprop 1 2\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "3\n"
}

tests['PLIST 4'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 2 4\n"
    "show gprop 1 2\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "4\n"
}

tests['PLIST 5'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 3 4\n"
    "show gprop 1 4\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[]\n"
}

tests['PLIST 6'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 3 4\n"
    "show gprop 2 3\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[]\n"
}

tests['PLIST 7'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 3 4\n"
    "show count plist 1\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "4\n"
}

tests['PLIST 8'] = {
    'in' :
    "pprop 1 2 3\n"
    "pprop 1 3 4\n"
    "show plist 2\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "[]\n"
}

tests['PROCEDUREP 1'] = {
    'in' :
    "show procedurep \"show\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['PROCEDUREP 2'] = {
    'in' :
    "to proc1\n"
    "show \"hello\n"
    "end\n"
    "show procedure? \"proc1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "proc1 defined\n"
    '? '
    "true\n"
}

tests['PROCEDUREP 3'] = {
    'in' :
    "show procedurep \"true\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['PROCEDUREP 4'] = {
    'in' :
    "copydef \"proc2 \"print\n"
    "show procedurep \"proc2\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['PROCEDUREP 5'] = {
    'in' :
    "to proc1\n"
    "show \"hello\n"
    "end\n"
    "copydef \"proc2 \"proc1\n"
    "show procedure? \"proc2\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "proc1 defined\n"
    '? '
    '? '
    "true\n"
}

tests['PRIMITIVEP 1'] = {
    'in' :
    "show primitivep \"show\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['PRIMITIVEP 2'] = {
    'in' :
    "to proc1\n"
    "show \"hello\n"
    "end\n"
    "show primitive? \"proc1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "proc1 defined\n"
    '? '
    "false\n"
}

tests['PRIMITIVEP 3'] = {
    'in' :
    "show primitivep \"true\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['PRIMITIVEP 4'] = {
    'in' :
    "copydef \"proc2 \"print\n"
    "show primitivep \"proc2\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['PRIMITIVEP 5'] = {
    'in' :
    "to proc1\n"
    "show \"hello\n"
    "end\n"
    "copydef \"proc2 \"proc1\n"
    "show primitive? \"proc2\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "proc1 defined\n"
    '? '
    '? '
    "false\n"
}

tests['DEFINEDP 1'] = {
    'in' :
    "show definedp \"show\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['DEFINEDP 2'] = {
    'in' :
    "to proc1\n"
    "show \"hello\n"
    "end\n"
    "show defined? \"proc1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "proc1 defined\n"
    '? '
    "true\n"
}

tests['DEFINEDP 3'] = {
    'in' :
    "show definedp \"true\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['DEFINEDP 4'] = {
    'in' :
    "copydef \"proc2 \"print\n"
    "show definedp \"proc2\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['DEFINEDP 5'] = {
    'in' :
    "to proc1\n"
    "show \"hello\n"
    "end\n"
    "copydef \"proc2 \"proc1\n"
    "show defined? \"proc2\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "proc1 defined\n"
    '? '
    '? '
    "true\n"
}

tests['NAMEP 1'] = {
    'in' :
    "show namep \"a\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['NAMEP 2'] = {
    'in' :
    "make \"A 1\n"
    "show name? \"a\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['NAMEP 3'] = {
    'in' :
    "make \"a 1\n"
    "show name? \"A\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['NAMEP 4'] = {
    'in' :
    "to f1 :P1\n"
    "show namep \"p1\n"
    "end\n"
    "f1 1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "f1 defined\n"
    '? '
    "true\n"
}

tests['NAMEP 5'] = {
    'in' :
    "to f1 :P1\n"
    "show namep \"p1\n"
    "end\n"
    "f1 1\n"
    "show name? \"p1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "f1 defined\n"
    '? '
    "true\n"
    '? '
    "false\n"
}

tests['PLISTP 1'] = {
    'in' :
    "pprop 1 2 3\n"
    "show plistp 1\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['PLISTP 2'] = {
    'in' :
    "pprop 1 2 3\n"
    "show plist? 2\n"
    ,
    'out' :
    '? '
    '? '
    "false\n"
}

tests['PLISTP 3'] = {
    'in' :
    "pprop 1 2 3\n"
    "remprop 1 2\n"
    "show plistp 1\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "false\n"
}

tests['CONTENTS 1'] = {
    'in' :
    "show contents\n"
    ,
    'out' :
    '? '
    "[[] [] []]\n"
}

tests['CONTENTS 2'] = {
    'in' :
    "make \"a 1\n"
    "pprop 1 2 3\n"
    "to bro\n"
    "print 1\n"
    "end\n"
    "show contents\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '> '
    '> '
    "bro defined\n"
    '? '
    "[[BRO] [A] [1]]\n"
}

tests['PROCEDURES 1'] = {
    'in' :
    "show procedures\n"
    ,
    'out' :
    '? '
    "[]\n"
}

tests['PROCEDURES 2'] = {
    'in' :
    "make \"a 1\n"
    "pprop 1 2 3\n"
    "to bro\n"
    "print 1\n"
    "end\n"
    "show procedures\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '> '
    '> '
    "bro defined\n"
    '? '
    "[BRO]\n"
}

tests['PRIMITIVES'] = {
    'in' :
    "show (count primitives) > 50\n"
    ,
    'out' :
    '? '
    "true\n"
}

tests['NAMES 1'] = {
    'in' :
    "show names\n"
    ,
    'out' :
    '? '
    "[[] []]\n"
}

tests['NAMES 2'] = {
    'in' :
    "make \"a 1\n"
    "pprop 1 2 3\n"
    "to bro\n"
    "print 1\n"
    "end\n"
    "show names\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '> '
    '> '
    "bro defined\n"
    '? '
    "[[] [A]]\n"
}

tests['PLISTS 1'] = {
    'in' :
    "show plists\n"
    ,
    'out' :
    '? '
    "[[] [] []]\n"
}

tests['PLISTS 2'] = {
    'in' :
    "make \"a 1\n"
    "pprop 1 2 3\n"
    "to bro\n"
    "print 1\n"
    "end\n"
    "show plists\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '> '
    '> '
    "bro defined\n"
    '? '
    "[[] [] [1]]\n"
}

tests['ARITY 1'] = {
    'in' :
    "show arity \"print\n"
    ,
    'out' :
    '? '
    "[0 1 -1]\n"
}

tests['ARITY 2'] = {
    'in' :
    "to a1\n"
    "print 1\n"
    "end\n"
    "show arity \"a1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    "[0 0 0]\n"
}

tests['ARITY 3'] = {
    'in' :
    "to a1 :p1\n"
    "print :p1\n"
    "end\n"
    "show arity \"a1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    "[1 1 1]\n"
}

tests['ARITY 4'] = {
    'in' :
    "to a1 [:p1]\n"
    "print :p1\n"
    "end\n"
    "show arity \"a1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    "[0 0 -1]\n"
}

tests['ARITY 5'] = {
    'in' :
    "to a1 :p0 [:p1]\n"
    "(print :p0 :p1)\n"
    "end\n"
    "show arity \"a1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    "[1 1 -1]\n"
}

tests['ARITY 6'] = {
    'in' :
    "to a1 :p0 [:p1] 5\n"
    "(print :p0 :p1)\n"
    "end\n"
    "show arity \"a1\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    "[1 5 -1]\n"
}

tests['PRINTOUT 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "po [[a1] [q] [1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "Make \"Q 4\n"
    "Pprop 1 2 3\n"
    "Pprop 2 3 4\n"
}

tests['PRINTOUT 2'] = {
    'in' :
    "to a1 [:p1 \"a| |test]\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop \"joe 2 \"hello\n"
    "pprop 2 \"la 4\n"
    "printout [[a1] [q] [joe 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    "to a1 [:p1 \"a| |test]\n"
    "show :p1\n"
    "end\n"
    "Make \"Q 4\n"
    "Pprop \"joe 2 \"hello\n"
    "Pprop 2 \"LA 4\n"
}

tests['PRINTOUT 3'] = {
    'in' :
    "to a1 [:p1 \"a| |test]\n"
    "show :p1\n"
    "end\n"
    "make \"q \"34\\ 34\n"
    "pprop \"joe 2 \"hello\\ there\n"
    "printout [[a1] [q] [joe 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    "to a1 [:p1 \"a| |test]\n"
    "show :p1\n"
    "end\n"
    "Make \"Q \"34\\ 34\n"
    "Pprop \"joe 2 \"hello\\ there\n"
}

tests['PRINTOUT 4'] = {
    'in' :
    "pprop \"test \"test [this is a test]\n"
    "po [[][][test]]\n"
    ,
    'out' :
    '? '
    '? '
    "Pprop \"test \"TEST [this is a test]\n"
}

tests['PRINTOUT ERROR 1'] = {
    'in' :
    "po [po]\n"
    ,
    'out' :
    '? '
    "po is a primitive\n"
}

tests['PRINTOUT ERROR 2'] = {
    'in' :
    "po [[][bob]]\n"
    ,
    'out' :
    '? '
    "bob has no value\n"
}

tests['POT 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "pot [[a1] [q] [1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    "Make \"Q 4\n"
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
}

tests['POT 2'] = {
    'in' :
    "to a1 [:p1 \"a| |test]\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop \"joe 2 \"hello\n"
    "pprop 2 \"la 4\n"
    "pot [[a1] [q] [joe 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    "to a1 [:P1 \"|a test|]\n"
    "Make \"Q 4\n"
    "Plist \"joe = [2 hello]\n"
    "Plist 2 = [LA 4]\n"
}

tests['POT 3'] = {
    'in' :
    "to a1 [:p1 \"a| |test]\n"
    "show :p1\n"
    "end\n"
    "make \"q \"34\\ 34\n"
    "pprop \"joe 2 \"hello\\ there\n"
    "pot [[a1] [q] [joe 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    "to a1 [:P1 \"|a test|]\n"
    "Make \"Q \"34\\ 34\n"
    "Plist \"joe = [2 hello\\ there]\n"
}

tests['ERASE 1'] = {
    'in' :
    "to a1 :p1\n"
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
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    "Make \"Q 4\n"
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
    '? '
    '? '
    '? '
    "I don't know how to a1\n"
    '? '
    "q has no value\n"
    '? '
}

tests['ERASE 2'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q [a1 \"hello]\n"
    "repeat 1 :q\n"
    "erase [[a1] [] []]\n"
    "repeat 1 :q\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    "hello\n"
    '? '
    '? '
    "I don't know how to a1\n"
}

tests['ERALL 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "erall\n"
    "pot [[a1]]\n"
    "pot [[] [q]]\n"
    "pot [[][][1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    "I don't know how to a1\n"
    '? '
    "q has no value\n"
    '? '
}

tests['ERPS 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "erps\n"
    "pot [[a1]]\n"
    "pot [[] [q]]\n"
    "pot [[][][1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    "I don't know how to a1\n"
    '? '
    "Make \"Q 4\n"
    '? '
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
}

tests['ERNS 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "erns\n"
    "pot [[a1]]\n"
    "pot [[] [q]]\n"
    "pot [[][][1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    '? '
    "q has no value\n"
    '? '
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
}

tests['ERPLS 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "erpls\n"
    "pot [[a1]]\n"
    "pot [[] [q]]\n"
    "pot [[][][1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    '? '
    "Make \"Q 4\n"
    '? '
}

tests['BURY 1'] = {
    'in' :
    "to a1 :p1\n"
    "show :p1\n"
    "end\n"
    "make \"q 2*2\n"
    "pprop 1 2 3\n"
    "pprop 2 3 4\n"
    "bury [[a1] [q] [1 2]]\n"
    "erall\n"
    "pot [[a1] [q] [1 2]]\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    "Make \"Q 4\n"
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
}

tests['BURY 2'] = {
    'in' :
    "to a1 :p1\n"
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
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "I don't know how to a1\n"
    '? '
    "Make \"Q 4\n"
    '? '
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
}

tests['BURY 3'] = {
    'in' :
    "to a1 :p1\n"
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
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    '? '
    "q has no value\n"
    '? '
    "Plist 1 = [2 3]\n"
    "Plist 2 = [3 4]\n"
}

tests['BURY 4'] = {
    'in' :
    "to a1 :p1\n"
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
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "to a1 :P1\n"
    '? '
    "Make \"Q 4\n"
    '? '
    "Plist 1 = [2 3]\n"
}

tests['UNBURY 1'] = {
    'in' :
    "to a1 :p1\n"
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
    ,
    'out' :
    '? '
    '> '
    '> '
    "a1 defined\n"
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
    "I don't know how to a1\n"
    '? '
    "q has no value\n"
    '? '
}

tests['BURIEDP 1'] = {
    'in' :
    "bury \"buriedproc\n"
    "show buriedp [buriedproc]\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['BURIEDP 2'] = {
    'in' :
    "bury [[buriedproc]]\n"
    "show buried? [buriedproc]\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['BURIEDP 3'] = {
    'in' :
    "bury [[][var][]]\n"
    "show buriedp [[][var][]]\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['BURIEDP 4'] = {
    'in' :
    "bury [[][][plist]]\n"
    "show buriedp [[][][plist]]\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
}

tests['BURIEDP 5'] = {
    'in' :
    "show buriedp [buriedproc]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['BURIEDP 6'] = {
    'in' :
    "show buried? \"buriedproc\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['BURIEDP 7'] = {
    'in' :
    "show buriedp [[][var][]]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['BURIEDP 8'] = {
    'in' :
    "show buriedp [[][][plist]]\n"
    ,
    'out' :
    '? '
    "false\n"
}

tests['BURIEDP ERROR 1'] = {
    'in' :
    "show buriedp [[][][]]\n"
    ,
    'out' :
    '? '
    "buriedp doesn't like [[] [] []] as input\n"
}

tests['BURIEDP ERROR 2'] = {
    'in' :
    "show buriedp [[][]]\n"
    ,
    'out' :
    '? '
    "buriedp doesn't like [[] []] as input\n"
}

tests['IF w/word 1'] = {
    'in' :
    "if \"true \"print\\ \"hello\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['ALSE 1'] = {
    'in' :
    "show butfirst 12=1\n"
    ,
    'out' :
    '? '
    "alse\n"
}

tests['TRACE 1'] = {
    'in' :
    "trace [[* + print][][]]\n"
    "(print 2+2 3*3)\n"
    ,
    'out' :
    '? '
    '? '
    "( + 2 2 )\n"
    "+ outputs 4\n"
    "( * 3 3 )\n"
    "* outputs 9\n"
    "( print 4 9 )\n"
    "4 9\n"
    "print stops\n"
}

tests['TRACE 2'] = {
    'in' :
    "trace [[* + print][][]]\n"
    "untrace \"print\n"
    "(print 2+2 3*3)\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    "( + 2 2 )\n"
    "+ outputs 4\n"
    "( * 3 3 )\n"
    "* outputs 9\n"
    "4 9\n"
}

tests['TRACE 3'] = {
    'in' :
    "trace [[][lobar][]]\n"
    "make \"lobar 2\n"
    ,
    'out' :
    '? '
    '? '
    "Make \"lobar 2\n"
}

tests['TRACE 4'] = {
    'in' :
    "trace [[* + make][def][]]\n"
    "make \"def 2*2+5\n"
    ,
    'out' :
    '? '
    '? '
    "( * 2 2 )\n"
    "* outputs 4\n"
    "( + 4 5 )\n"
    "+ outputs 9\n"
    "( make \"def 9 )\n"
    "Make \"def 9\n"
    "make stops\n"
}

tests['TRACE 5'] = {
    'in' :
    "trace [[][][list1]]\n"
    "pprop \"list1 \"loop 2\n"
    ,
    'out' :
    '? '
    '? '
    "Pprop \"list1 \"loop 2\n"
}

tests['TRACE 6'] = {
    'in' :
    "trace [[* + pprop][][list2]]\n"
    "pprop \"list2 \"item1 4*4+3\n"
    ,
    'out' :
    '? '
    '? '
    "( * 4 4 )\n"
    "* outputs 16\n"
    "( + 16 3 )\n"
    "+ outputs 19\n"
    "( pprop \"list2 \"item1 19 )\n"
    "Pprop \"list2 \"item1 19\n"
    "pprop stops\n"
}

tests['TRACE 7'] = {
    'in' :
    "to l1 :p1\n"
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
    ,
    'out' :
    '? '
    '> '
    '> '
    "l1 defined\n"
    '? '
    '> '
    '> '
    "l2 defined\n"
    '? '
    '> '
    '> '
    "l3 defined\n"
    '? '
    '> '
    '> '
    "l4 defined\n"
    '? '
    '? '
    "( l4 10 )\n"
    " ( l3 10 )\n"
    "  ( l2 10 )\n"
    "   ( l1 10 )\n"
    "l1 10\n"
    "   l1 stops\n"
    "  l2 stops\n"
    " l3 stops\n"
    "l4 stops\n"
}

tests['TRACEDP 1'] = {
        'in' :
    "trace \"tracedproc\n"
                                "show tracedp [tracedproc]\n"
,
        'out' :
    '? '
    '? '
"true\n"
}

tests['TRACEDP 2'] = {
        'in' :
    "trace [[tracedproc]]\n"
                                "show traced? [tracedproc]\n"
,
        'out' :
    '? '
    '? '
"true\n"
}

tests['TRACEDP 3'] = {
        'in' :
    "trace [[][var][]]\n"
                                "show tracedp [[][var][]]\n"
,
        'out' :
    '? '
    '? '
"true\n"
}

tests['TRACEDP 4'] = {
        'in' :
    "trace [[][][plist]]\n"
                                "show tracedp [[][][plist]]\n"
,
        'out' :
    '? '
    '? '
"true\n"
}

tests['TRACEDP 5'] = {
        'in' :
    "show tracedp [tracedproc]\n"
,
        'out' :
    '? '
"false\n"
}

tests['TRACEDP 6'] = {
        'in' :
    "show traced? \"tracedproc\n"
,
        'out' :
    '? '
"false\n"
}

tests['TRACEDP 7'] = {
        'in' :
    "show tracedp [[][var][]]\n"
,
        'out' :
    '? '
"false\n"
}

tests['TRACEDP 8'] = {
        'in' :
    "show tracedp [[][][plist]]\n"
,
        'out' :
    '? '
"false\n"
}

tests['TRACEDP ERROR 1'] = {
'in' :
"show tracedp [[][][]]\n"
,
'out' :
    '? '
"tracedp doesn't like [[] [] []] as input\n"
}

tests['TRACEDP ERROR 2'] = {
    'in' :
    "show tracedp [[][]]\n"
    ,
    'out' :
    '? '
    "tracedp doesn't like [[] []] as input\n"
}

tests['STEP 1'] = {
    'in' :
    "step [[c1 c10] [p1]]\n"
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
    ,
    'out' :
    '? '
    '? '
    '> '
    '> '
    "c1 defined\n"
    '? '
    "P1 shadowed by local in procedure call\n"
    "[(print :p1 :p2)]"
    ' >>>'
    "4 6\n"
    '? '
    '> '
    '> '
    "c10 defined\n"
    '? '
    "[(c1 10 20)]"
    ' >>>'
    "P1 shadowed by local in procedure call in c10\n"
    "[(print :p1 :p2)]"
    ' >>>'
    "10 20\n"
}

tests['STEP 2'] = {
    'in' :
    "step [[] [v1]]\n"
    "local \"v1\n"
    "make \"v1 \"hello\n"
    "show :v1\n"
    ,
    'out' :
    '? '
    '? '
    '? '
    '? '
    "hello\n"
}

tests['RUN 1'] = {
    'in' :
    "run [print \"hello]\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['RUN 2'] = {
    'in' :
    "print run [\"hello]\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['RUN 3'] = {
    'in' :
    "print run [print \"hello]\n"
    ,
    'out' :
    '? '
    "hello\n"
    "run didn't output to print\n"
}

tests['RUN 4'] = {
    'in' :
    "print run [2*2]\n"
    ,
    'out' :
    '? '
    "4\n"
}

tests['RUN 5'] = {
    'in' :
    "run \"print\\ \"hello\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['RUNRESULT 1'] = {
    'in' :
    "show runresult \"print\\ \"hello\n"
    ,
    'out' :
    '? '
    "hello\n"
    "[]\n"
}

tests['RUNRESULT 2'] = {
    'in' :
    "show runresult [2*2]\n"
    ,
    'out' :
    '? '
    "[4]\n"
}

tests['RUNRESULT 4'] = {
    'in' :
    "show runresult [\"hello]\n"
    ,
    'out' :
    '? '
    "[hello]\n"
}

tests['RUNRESULT 3'] = {
    'in' :
    "show runresult [\"hello]\n"
    ,
    'out' :
    '? '
    "[hello]\n"
}

tests['REPCOUNT 1'] = {
        'in' :
"repeat 3 [repeat 3[show repcount]] show repcount\n"
,
        'out' :
    '? '
"1\n"
"2\n"
"3\n"
"1\n"
"2\n"
"3\n"
"1\n"
"2\n"
"3\n"
"-1\n"
}

tests['FOREVER'] = {
        'in' :
"to f :p1\n"
         "forever [print repcount if repcount=:p1 [output \"end]]\n"
         "end\n"
         "print f 5\n"
,
        'out' :
    '? '
    '> '
    '> '
"f defined\n"
    '? '
         "1\n"
"2\n"
"3\n"
"4\n"
"5\n"
         "end\n"
}

tests['TEST 1'] = {
        'in' :
    "test \"true\n"
                             "iftrue [print \"hello]\n"
,
        'out' :
    '? '
    '? '
"hello\n"
}

tests['TEST 2'] = {
        'in' :
    "test \"false\n"
                             "ift [print \"hello]\n"
                             "print \"boogie\n"
,
        'out' :
    '? '
    '? '
    '? '
"boogie\n"
}

tests['TEST 3'] = {
        'in' :
    "iftrue [print \"hello]\n"
,
        'out' :
    '? '
"iftrue without TEST\n"
}

tests['TEST 4'] = {
        'in' :
    "test \"true\n"
                             "iff [print \"hello]\n"
                             "print \"boogie\n"
,
        'out' :
    '? '
    '? '
    '? '
"boogie\n"
}

tests['TEST 5'] = {
        'in' :
    "to f\n"
                             "ift \"print\\ \"begone\n"
                             "end\n"
                             "test 2=2\n"
                             "f\n"
,
        'out' :
    '? '
    '> '
    '> '
"f defined\n"
    '? '
    '? '
                             "begone\n"
}

tests['Scope Error 1'] = {
        'in' :
    "to f1\n"
                                    "local \"a\n"
                                    "make \"a 22\n"
                                    "make \"b print :a\n"
                                    "end\n"
                                    "f1\n"
                                    "show :a\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
"f1 defined\n"
    '? '
                                    "22\n"
                                    "print didn't output to make in f1\n"
                                    "[make \"b print :a]\n"
    '? '
                                    "a has no value\n"
}

tests['Scope Error 2'] = {
        'in' :
    "to f1\n"
                                    "test 2=2\n"
                                    "make \"a 22\n"
                                    "make \"b print :a\n"
                                    "end\n"
                                    "f1\n"
                                    "iff [show :a]\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
"f1 defined\n"
    '? '
                                    "22\n"
                                    "print didn't output to make in f1\n"
                                    "[make \"b print :a]\n"
    '? '
                                    "iff without TEST\n"
}

tests['Scope Error 3'] = {
        'in' :
    "to f1\n"
                                    "local \"a\n"
                                    "make \"a 22\n"
                                    "make \"b print :a\n"
                                    "end\n"
                                    "make \"a \"lobotomy\n"
                                    "f1\n"
                                    "show :a\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
"f1 defined\n"
    '? '
    '? '
                                    "22\n"
                                    "print didn't output to make in f1\n"
                                    "[make \"b print :a]\n"
    '? '
                                    "lobotomy\n"
}

tests['Scope Error 4'] = {
        'in' :
    "to f1\n"
                                    "repeat 5[if repcount = 3 [2*2]]\n"
                                    "end\n"
                                    "f1\n"
                                    "show repcount\n"
,
        'out' :
    '? '
    '> '
    '> '
"f1 defined\n"
    '? '
                                    "You don't say what to do with 4\n"
    '? '
                                    "-1\n"
}

# TODO: tests for FOO. tests for when not ALLOWGETSET

tests['SETFOO 1'] = {
        'in' :
    "make \"allowgetset \"true\n"
                               "to proc\n"
                               "setfoo 1\n"
                               "show :foo\n"
                               "end\n"
                               "proc\n"
                               "show :foo\n"
,
        'out' :
    '? '
    '? '
    '> '
    '> '
    '> '
"proc defined\n"
    '? '
                               "1\n"
    '? '
                               "foo has no value\n"
}

tests['SETFOO 2'] = {
        'in' :
    "make \"allowgetset \"true\n"
                               "to proc\n"
                               "global \"foo\n"
                               "setfoo 1\n"
                               "end\n"
                               "proc\n"
                               "show :foo\n"
,
        'out' :
    '? '
    '? '
    '> '
    '> '
    '> '
"proc defined\n"
    '? '
    '? '
                               "1\n"
}

tests['STOP'] = {
'in' :
"to lp :count\n"
"forever [print repcount if repcount=:count [stop]]\n"
"end\n"
"lp 5\n"
,
'out' :
    '? '
    '> '
    '> '
"lp defined\n"
    '? '
"1\n"
    "2\n"
    "3\n"
    "4\n"
    "5\n"
}

tests['CATCH 1'] = {
    'in' :
    "catch \"error [notafunc]\n"
    "show error\n"
    ,
    'out' :
    '? '
    '? '
    "[13 I don't know how to notafunc [] []]\n"
}

tests['CATCH 2'] = {
    'in' :
    "catch \"err [notafunc]\n"
    ,
    'out' :
    '? '
    "I don't know how to notafunc\n"
}

tests['CATCH 3'] = {
    'in' :
    "catch \"err [throw \"err]\n"
    "show error\n"
    ,
    'out' :
    '? '
    '? '
    "[]\n"
}

tests['CATCH 4'] = {
    'in' :
    "catch \"err1 [throw \"err2]\n"
    "show error\n"
    ,
    'out' :
    '? '
    "Can't find catch tag for err2\n"
    '? '
    "[]\n"
}

tests['CATCH 5'] = {
    'in' :
    "print catch \"err [(throw \"err \"hello)]\n"
    ,
    'out' :
    '? '
    "hello\n"
}

tests['CATCH 6'] = {
        'in' :
"print catch \"er1 [(throw \"er1 [hello there])]\n"
,
        'out' :
    '? '
"hello there\n"
}

tests['CATCH PROCEDURE OUTPUT'] = {
        'in' :
    "to t\n"
                                             "throw \"q\n"
                                             "end\n"
                                             "to c\n"
                                             "catch \"q [output t]\n"
                                             "print \"caught\n"
                                             "end\n"
                                             "c\n"
,
        'out' :
    '? '
    '> '
    '> '
"t defined\n"
    '? '
    '> '
    '> '
    '> '
                                             "c defined\n"
    '? '
                                             "caught\n"
}

tests['THROW 1'] = {
        'in' :
    "to throw_error\n"
                              "(throw \"error [this is an error])\n"
                              "end\n"
                              "throw_error\n"
                              "show error\n"
,
        'out' :
    '? '
    '> '
    '> '
"throw_error defined\n"
    '? '
                              "this is an error\n"
    '? '
                              "[]\n"
}

tests['THROW 2'] = {
        'in' :
    "to throw_error\n"
                              "(throw \"error [this is an error] )\n"
                              "end\n"
                              "to level2\n"
                              "throw_error\n"
                              "end\n"
                              "catch \"error [level2]\n"
                              "show error\n"
,
        'out' :
    '? '
    '> '
    '> '
"throw_error defined\n"
    '? '
    '> '
    '> '
                              "level2 defined\n"
    '? '
    '? '
                              "[35 this is an error level2 [throw_error]]\n"
}

tests['THROW 3'] = {
        'in' :
    "to throw_error\n"
                              "not_a_function\n"
                              "end\n"
                              "to level2\n"
                              "catch \"error [throw_error]\n"
                              "show error\n"
                              "end\n"
                              "level2\n"
,
        'out' :
    '? '
    '> '
    '> '
"throw_error defined\n"
    '? '
    '> '
    '> '
    '> '
                              "level2 defined\n"
    '? '
                              "[13 I don't know how to not_a_function "
                              "throw_error [not_a_function]]\n"
}

tests['THROW 4'] = {
'in' :
"catch \"error [(throw \"error [this is an error])]\n"
"show error\n"
,
'out' :
    '? '
    '? '
"[35 this is an error [] []]\n"
}

tests['THROW 5'] = {
    'in' :
    "to throw_error\n"
    "noop\n"
    "catch \"error [(throw \"error \"misc)]\n"
    "end\n"
    "to noop\n"
    "end\n"
    "throw_error\n"
    "show error\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    '> '
    "throw_error defined\n"
    '? '
    '> '
    "noop defined\n"
    '? '
    '? '
    "[35 misc [] []]\n"
}

tests['THROW 6'] = {
        'in' :
"to throw_error\n"
         "noop\n"
         "catch \"error [throw \"error]\n"
         "end\n"
         "to noop\n"
         "end\n"
         "throw_error\n"
         "show error\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
"throw_error defined\n"
    '? '
    '> '
         "noop defined\n"
    '? '
    '? '
         "[21 Throw \"Error throw_error [catch \"error [throw \"error]]]\n"
}

tests['SETFOO 3'] = {
        'in' :
    "make \"allowgetset \"true\n"
                             "setfoo \"hello\n"
                             "show foo\n"
,
        'out' :
    '? '
    '? '
    '? '
"hello\n"
}

tests['? 1'] = {
        'in' :
    "show runparse \"?37\n"
,
        'out' :
    '? '
"[( ? 37 )]\n"
}

tests['? 2'] = {
        'in' :
    "show runparse \"?alpha\n"
,
        'out' :
    '? '
"[?alpha]\n"
}

tests['NESTED 1'] = {
        'in' :
    "make \"allowgetset \"true\n"
                               "make \"a [a b c [d e f]]\n"
                               "make \"b [a b c [d e f]]\n"
                               ".setfirst :a :a\n"
                               ".setfirst :b :b\n"
                               "show a=b\n"
,
        'out' :
    '? '
    '? '
    '? '
    '? '
    '? '
    '? '
"true\n"
}

tests['NESTED 2'] = {
        'in' :
    "make \"a [a b c [d e f]]\n"
                               ".setfirst :a :a\n"
                               "show :a"
,
        'out' :
    '? '
    '? '
    '? '
"[... b c [d e f]]\n"
}

tests['MAYBE PRINT'] = {
        'in' :
    "to maybePrint\n"
                                  ".maybeoutput print \"hello\n"
                                  "end\n"
                                  "maybePrint\n"
,
        'out' :
    '? '
    '> '
    '> '
"maybePrint defined\n"
    '? '
                                  "hello\n"
}

tests['MAYBE WORD'] = {
        'in' :
    "to maybeWord\n"
                                 ".maybeoutput \"hello\n"
                                 "end\n"
                                 "print maybeWord\n"
,
        'out' :
    '? '
    '> '
    '> '
"maybeWord defined\n"
    '? '
                                 "hello\n"
}

tests['MAYBE PRINT error'] = {
        'in' :
    "to maybePrint\n"
                                        ".maybeoutput print \"hello\n"
                                        "end\n"
                                        "print maybePrint\n"
,
        'out' :
    '? '
    '> '
    '> '
"maybePrint defined\n"
    '? '
                                        "hello\n"
                                        "maybePrint didn't output to print\n"
}

tests['MAYBE WORD error'] = {
        'in' :
    "to maybeWord\n"
                                       ".maybeoutput \"hello\n"
                                       "end\n"
                                       "maybeWord\n"
,
        'out' :
    '? '
    '> '
    '> '
"maybeWord defined\n"
    '? '
                                       "You don't say what to do with hello\n"
}

tests['TRACE procedure iteration 1'] = {
        'in' :
    "to iter :p1 :i\n"
                                                  "if :i <=0 [output :p1]\n"
                                                  "output iter :p1 * 2 :i-1\n"
                                                  "end\n"
                                                  "trace \"iter\n"
                                                  "print iter 4 5\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
"iter defined\n"
    '? '
    '? '
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
                                                  "128\n"
}

tests['TRACE procedure iteration 2'] = {
        'in' :
    "to i1 :p :i\n"
                                                  "if :i<=0 [output :p]\n"
                                                  "output i2 :p*2 :i-1\n"
                                                  "end\n"

                                                  "to i2 :p :i\n"
                                                  "if :i<=0 [output :p]\n"
                                                  "output i1 :p+5 :i-1\n"
                                                  "end\n"

                                                  "trace [[i1 i2]]\n"
                                                  "print i1 4 5\n"

,
        'out' :
    '? '
    '> '
    '> '
    '> '
"i1 defined\n"
    '? '
    '> '
    '> '
    '> '
                                                  "i2 defined\n"
    '? '
    '? '
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
                                                  "62\n"
}

tests['ERR in Procedure'] = {
        'in' :
    "to err\n"
                                       "print 5*\"me\n"
                                       "end\n"
                                       "err\n"
,
        'out' :
    '? '
    '> '
    '> '
"err defined\n"
    '? '
                                       "* doesn't like me as input in err\n"
                                       "[print 5*\"me]\n"
}

tests['GOTO 1'] = {
        'in' :
    "to proc\n"
                             "goto \"t1\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag \"t1\n"
                             "print [this should print]\n"
                             "end\n"
                             "proc\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    '> '
    '> '
"proc defined\n"
    '? '
                             "this should print\n"
}

tests['GOTO 2'] = {
        'in' :
    "to proc\n"
                             "goto :t1\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag \"tag1\n"
                             "print [this should print]\n"
                             "end\n"
                             "make \"t1 \"tag1\n"
                             "proc\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    '> '
    '> '
"proc defined\n"
    '? '
    '? '
                             "this should print\n"
}

tests['GOTO 3'] = {
        'in' :
    "to proc\n"
                             "goto [tag1]\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag [tag1]\n"
                             "print [this shouldn't print, either]\n"
                             "end\n"
                             "proc\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    '> '
    '> '
"proc defined\n"
    '? '
                             "goto doesn't like [tag1] as input in proc\n"
                             "[goto [tag1]]\n"
}

tests['GOTO 4'] = {
        'in' :
    "to proc\n"
                             "goto \"tag2\n"
                             "tag \"y2\n"
                             "print [this shouldn't print]\n"
                             "tag \"tag1\n"
                             "print [this shouldn't' print, either]\n"
                             "end\n"
                             "proc\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
    '> '
    '> '
    '> '
"proc defined\n"
    '? '
                             "goto doesn't like tag2 as input in proc\n"
                             "[goto \"tag2]\n"
}

tests['APPLY 1'] = {
        'in' :
    "apply \"print [hello there]\n"
,
        'out' :
    '? '
"hello there\n"
}

tests['APPLY 2'] = {
        'in' :
    "print apply \"word [hello there everyone]\n"
,
        'out' :
    '? '
"hellothereeveryone\n"
}

tests['APPLY 3'] = {
        'in' :
    "apply \"print []\n"
,
        'out' :
    '? '
"\n"
}

tests['APPLY 4'] = {
        'in' :
    "apply \"make [hello there]\n"
                              "print :hello\n"
,
        'out' :
    '? '
    '? '
"there\n"
}

tests['APPLY 5'] = {
        'in' :
    "apply \"make [hello there bob]\n"
,
        'out' :
    '? '
"too many inputs to make\n"
}

tests['APPLY 6'] = {
        'in' :
    "apply \"make [hello]\n"
,
        'out' :
    '? '
"not enough inputs to make\n"
}

tests['APPLY 7'] = {
        'in' :
    "apply \"make \"hello\n"
,
        'out' :
    '? '
"apply doesn't like hello as input\n"
}

tests['APPLY 8'] = {
        'in' :
    "show apply [? * ?] [3]\n"
                              "show apply [? + ?2] [3 4]\n"
                              "show apply [[x y] :x*:y] [4 5]\n"
                              "show apply [[x y] [output :x * :y]] [5 6]\n"
,
        'out' :
    '? '
"9\n"
    '? '
                              "7\n"
    '? '
                              "20\n"
    '? '
                              "30\n"
}

tests['MACRO 1'] = {
'in' :
".macro myrepeat :num :instructions\n"
"if :num=0 [output []]\n"
"output se :instructions (list \"myrepeat :num-1 :instructions)\n"
"end\n"
"myrepeat 3 [print \"hello]\n"
,
'out' :
    '? '
    '> '
    '> '
    '> '
"myrepeat defined\n"
    '? '
"hello\n"
"hello\n"
"hello\n"
}

tests['MACRO err 1'] = {
    'in' :
    ".macro err1\n"
    "end\n"
    "err1\n"
    ,
    'out' :
    '? '
    '> '
    "err1 defined\n"
    '? '
    "Macro returned nothing instead of a list\n"
}

tests['MACROP 1'] = {
    'in' :
    ".macro m\n"
    "output []\n"
    "end\n"
    "show macrop \"m\n"
    ,
    'out' :
    '? '
    '> '
    '> '
    "m defined\n"
    '? '
    "true\n"
}

tests['MACROP 2'] = {
    'in' :
    ".defmacro \"m2 [[] [output [print \"hello]]]\n"
    "show macro? \"m2\n"
    "m2\n"
    ,
    'out' :
    '? '
    '? '
    "true\n"
    '? '
    "hello\n"
}

tests['FULLPRINTP 1'] = {
    'in' :
    "make \"fullprintp \"true\n"
    "show \"|hello|\n"
    "show \"|hello there|\n"
    "print [hello |there you|]\n"
    ,
    'out' :
    '? '
    '? '
    "hello\n"
    '? '
    "|hello there|\n"
    '? '
    "hello |there you|\n"
}

tests['FULLPRINTP 2'] = {
    'in' :
    "make \"fullprintp \"true\n"
    "show [hello |there you|]\n"
    "show {hello there| |you}\n"
    "print \"hello\\ there\n"

,
    'out' :
    '? '
    '? '
    "[hello |there you|]\n"
    '? '
    "{hello |there you|}\n"
    '? '
    "hello\\ there\n"
}

tests['FULLPRINTP 3'] = {
    'in' :
    "make \"fullprintp \"true\n"
    "show [hello\\ there people]\n"
    "print [[hello\\ there]]\n"
    "print [{hello\\ there}]\n"
    "print [{|hello there| people}]\n"

,
    'out' :
    '? '
    '? '
    "[|hello there| people]\n"
    '? '
    "[|hello there|]\n"
    '? '
    "{|hello there|}\n"
    '? '
    "{|hello there| people}\n"
}

tests['FULLPRINTP 4'] = {
    'in' :
    "make \"fullprintp \"true\n"
    "show \"|hello there|\n"
    "make \"fullprintp \"false\n"
    "show \"|hello there|\n"
    ,
    'out' :
    '? '
    '? '
    "|hello there|\n"
    '? '
    '? '
    "hello there\n"
}

tests['PRINTDEPTHLIMIT 1'] = {
    'in' :
    "make \"printdepthlimit 1\n"
    "show [[[] [] []]]\n"
    "show [this is a test]\n"
    "show {this is a test}\n"
    "show [[this is a test]]\n"
    "show {{this is a test}}\n"
    ,
    'out' :
    '? '
    '? '
    "[[...]]\n"
    '? '
    "[... ... ... ...]\n"
    '? '
    "{... ... ... ...}\n"
    '? '
    "[[...]]\n"
    '? '
    "{{...}}\n"
}

tests['PRINTDEPTHLIMIT 2'] = {
    'in' :
    "make \"printdepthlimit 0\n"
    "show \"hello\n"
    "show [this is a test]\n"
    "show {this is a test}\n"
    ,
    'out' :
    '? '
    '? '
    "...\n"
    '? '
    "[...]\n"
    '? '
    "{...}\n"
}

tests['PRINTDEPTHLIMIT 3'] = {
    'in' :
    "make \"printdepthlimit 2\n"
    "show \"hello\n"
    "show [[[this]] is a test]\n"
    "show {[{this}] is a test}\n"
    ,
    'out' :
    '? '
    '? '
    "hello\n"
    '? '
    "[[[...]] is a test]\n"
    '? '
    "{[{...}] is a test}\n"
}

tests['PRINTWIDTHLIMIT 1'] = {
    'in' :
    "make \"printwidthlimit 1\n"
    "show [[[] [] []]]\n"
    "show [this is a test]\n"
    "show {this is a test}\n"
    "show [[this is a test]]\n"
    "show {{this is a test}}\n"
    "show \"12345678901234567890\n"
    ,
    'out' :
    '? '
    '? '
    "[[[] ...]]\n"
    '? '
    "[this ...]\n"
    '? '
    "{this ...}\n"
    '? '
    "[[this ...]]\n"
    '? '
    "{{this ...}}\n"
    '? '
    "1234567890...\n"
}

tests['PRINTWIDTHLIMIT 2'] = {
    'in' :
    "make \"printwidthlimit 15\n"
    "show \"12345678901234567890\n"
    ,
    'out' :
    '? '
    '? '
    "123456789012345...\n"
}

tests['UNIX COMMENT'] = {
    'in' :
    "#! /usr/bin/logo\n"
    "print [success]\n"
    ,
    'out' :
    '? '
    '? '
    "success\n"
}

tests['FIBLIST'] = {
        'in' :
"to fiblist :n\n"
         "if :n<2 [output [1 1]]\n"
         "output newfib fiblist :n-1\n"
         "end\n"
         "to newfib :list\n"
         "output fput (sum first :list first butfirst :list) :list\n"
         "end\n"
         "print fiblist 5\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
"fiblist defined\n"
    '? '
    '> '
    '> '
         "newfib defined\n"
    '? '
         "8 5 3 2 1 1\n"
}

tests['Escape Sequence'] = {
        'in' :
    "print\t \"hello\n"
,
        'out' :
    '? '
"hello\n"
}

tests['PO Macro'] = {
        'in' :
    ".macro m :p1\n"
                               "output sentence \"print \":p1\n"
                               "end\n"
                               "po \"m\n"
,
        'out' :
    '? '
    '> '
    '> '
"m defined\n"
    '? '
                               ".macro m :p1\n"
                               "output sentence \"print \":p1\n"
                               "end\n"
}

tests['MACRO in TO'] = {
        'in' :
    "TO d\n"
                                  ".macro e\n"
                                  "end\n"
                                  "d\n"
,
        'out' :
    '? '
    '> '
    '> '
"d defined\n"
    '? '
                                  "can't use .macro inside a procedure in d\n"
                                  "[.macro e]\n"
}

# TODO: Graphics not initialized

# tests['Already filling'] = {
# 'in' :
# "filled 3 [filled 2 [repeat 4 [fd 100 rt 90]]]\n"
# ,
# 'out' :
#     '? '
# "Already filling\n"
# }

tests['TO in PAUSE'] = {
    'in' :
    "pause\n"
    "to pr\n"
    "co\n"
    "to fg\n"
    "end\n"
    ,
    'out' :
    '? '
    "Pausing...\n"
    '? '
    "Can't use to within PAUSE\n"
    '? '
    '? '
    '> '
    "fg defined\n"
}

tests['.MACRO in PAUSE'] = {
    'in' :
    "pause\n"
    ".macro pr\n"
    "co\n"
    ".macro fg\n"
    "end\n"
    ,
    'out' :
    '? '
    "Pausing...\n"
    '? '
    "Can't use .macro within PAUSE\n"
    '? '
    '? '
    '> '
    "fg defined\n"
}

tests['reparsing list 1'] = {
    'in' :
    "make \"a [print \"hello]\n"
    "run :a\n"
    "setitem 2 :a \"\"hi\n"
    "run :a\n"
    ,
    'out' :
    '? '
    '? '
    "hello\n"
    '? '
    '? '
    "hi\n"
}

tests['reparsing list 2'] = {
    'in' :
    "make \"a [print \"hello]\n"
    "run :a\n"
    ".setbf :a [\"hi]\n"
    "run :a\n"
    ,
    'out' :
    '? '
    '? '
    "hello\n"
    '? '
    '? '
    "hi\n"
}

tests['reparsing list 3'] = {
    'in' :
    "make \"a [show [hello]]\n"
    "run :a\n"
    ".setfirst :a \"print\n"
    "run :a\n"
    ,
    'out' :
    '? '
    '? '
    "[hello]\n"
    '? '
    '? '
    "hello\n"
}

tests['setitem list inside itself'] = {
    'in' :
"make \"a [this is a test]\n"
         "setitem 1 :a :a\n"
,
        'out' :
    '? '
    '? '
"setitem doesn't like [this is a test] as input\n"
}

tests['setitem array inside itself'] = {
    'in' :
"make \"a {this is a test}\n"
"setitem 1 :a :a\n"
,
'out' :
    '? '
    '? '
"setitem doesn\'t like {this is a test} as input\n"
}

# from: https://people.eecs.berkeley.edu/~bh/v3ch3/algs.html
tests['Ice Cream'] = {
        'in' :
"make \"one [Ice cream is delicious.]\n"
          "make \"two fput \"Spinach butfirst butfirst :one\n"
          ".setfirst butfirst butfirst :two \"disgusting.\n"
          "print :one\n"
,
        'out' :
    '? '
    '? '
    '? '
    '? '
"Ice cream is disgusting.\n"
}

tests['listSize 1'] = {
        'in' :
"make \"a {this is an array}\n"
             "make \"b arraytolist :a\n"
             "show count :b\n"
,
        'out' :
    '? '
    '? '
    '? '
"4\n"
}

tests['listSize 2'] = {
'in' :
"make \"a [this is a list]\n"
"make \"b :a\n"
"show count :b\n"
,
'out' :
    '? '
    '? '
    '? '
"4\n"
}

tests['listSize 3'] = {
        'in' :
"make \"a [this is a bit of a longer list]\n"
             "make \"b member \"a :a\n"
             "show count :b\n"
,
        'out' :
    '? '
    '? '
    '? '
"6\n"
}

tests['listSize 4'] = {
        'in' :
"make \"a [this is a list]\n"
             "make \"b butfirst :a\n"
             "show count :b\n"
,
        'out' :
    '? '
    '? '
    '? '
"3\n"
}

tests['listSize 5'] = {
'in' :
"make \"a [this is a list]\n"
"make \"b butlast :a\n"
"show count :b\n"
,
'out' :
    '? '
    '? '
    '? '
"3\n"
}

tests['stack overflow error'] = {
        'in' :
"to qw\n"
             "qw\n"
             "end\n"
             "qw\n"
,
        'out' :
    '? '
    '> '
    '> '
"qw defined\n"
    '? '
    "Stack overflow in qw\n"
    "[qw]\n"
}

# If this test causes a segfault, then tail recursion optomization is broken.
# This test takes a whole second on my hardware.
tests['tail recursion optomization'] = {
        'in' :
"to qw :i\n"
             "if :i < 0 [output 0]\n"
             "output qw :i-1\n"
             "end\n"
             "print qw 100000\n"
,
        'out' :
    '? '
    '> '
    '> '
    '> '
"qw defined\n"
    '? '
    "0\n"
}

tests['fput list to word'] = {
'in' :
"fput [hi] \"hello\n"
,
'out' :
    '? '
"fput doesn't like hello as input\n"
}

bad_tests  = 0
good_tests = 0

for name in sorted(tests.keys()):
    test = tests[name]
    t_in = test['in']
    t_ex = test['out']

    if name != 'EQUAL? 9':
        continue

    print name,'...',

    proc = subprocess.Popen([exe],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    proc.stdin.write(t_in)
    proc.stdin.close()

    while proc.returncode is None:
        proc.poll()

    t_out = proc.stdout.read()
    if t_out == t_ex:
        print 'OK'
        good_tests += 1
    else:
        print 'ERROR!'
        print 
        print 'INPUT: ------------------------------------------------'
        print t_in
        print
        print 'EXPECTED: ---------------------------------------------'
        print t_ex
        print
        print 'OUTPUT: -----------------------------------------------'
        print t_out
        print
        bad_tests += 1
        
print "Passed tests:", good_tests
print "Failed tests:", bad_tests

