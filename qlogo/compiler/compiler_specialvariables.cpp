//===-- qlogo/compiler_specialvariables.cpp - Special variables documentation -------*- C++ -*-===//
//
// Copyright 2017-2026 Jason Sikes
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted under the conditions specified in the
// license found in the LICENSE file in the project root.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the documentation of the special variables of the
/// Compiler class. The variables are defined in include/workspace/specialvars.h.
/// However, the help documentation database is sourced from files in this directory.
///
//===----------------------------------------------------------------------===//


/***DOC ERRACT
ERRACT                          (variable)

    When set to a value that is not FALSE nor an empty string nor an empty list,
    the command interpreter will execute PAUSE to enable the user to
    inspect the state of the program.

COD***/

/***DOC LOGOPLATFORM
LOGOPLATFORM						(variable)

    one of the following words: OSX, WINDOWS, or UNIX.


COD***/

/***DOC LOGOVERSION
LOGOVERSION						(variable)

    a real number indicating the Logo version number, e.g., 5.5

COD***/

/***DOC COMMANDLINE
COMMANDLINE						(variable)

    contains all text on the command line used to start Logo.

COD***/

/***DOC BUTTONACT
BUTTONACT						(variable)

	if nonempty, should be an instruction list that will be evaluated
	whenever a mouse button is pressed.  Note that the user may have
	released the button before the instructions are evaluated.  BUTTON
	will still output which button was most recently pressed.  CLICKPOS
	will output the position of the mouse cursor at the moment the
	button was pressed; this may be different from MOUSEPOS if the
	user moves the mouse after clicking.

	Note that it's possible for the user to press a button during the
	evaluation of the instruction list.  If this would confuse your
	program, prevent it by temporarily setting BUTTONACT to the empty
	list.  One easy way to do that is the following:

		make "buttonact [button.action]

		to button.action [:buttonact []]
		... ; whatever you want the button to do
		end

COD***/

/***DOC KEYACT
KEYACT							(variable)

	if nonempty, should be an instruction list that will be evaluated
	whenever a key is pressed on the keyboard.  The instruction list
	can use READCHAR to find out what key was pressed.  Note that only
	keys that produce characters qualify; pressing SHIFT or CONTROL
	alone will not cause KEYACT to be evaluated.

	Note that it's possible for the user to press a key during the
	evaluation of the instruction list.  If this would confuse your
	program, prevent it by temporarily setting KEYACT to the empty
	list.  One easy way to do that is the following:

		make "keyact [key.action]

		to key.action [:keyact []]
		... ; whatever you want the key to do
		end

COD***/

/***DOC LOADNOISILY
LOADNOISILY						(variable)

	if TRUE, prints the names of procedures defined when loading
	from a file (including the temporary file made by EDIT).

COD***/

/***DOC PRINTDEPTHLIMIT
PRINTDEPTHLIMIT						(variable)

	if a nonnegative integer, indicates the maximum depth of sublist
	structure that will be printed by PRINT, etc.

COD***/

/***DOC PRINTWIDTHLIMIT
PRINTWIDTHLIMIT						(variable)

	if a nonnegative integer, indicates the maximum number of members
	in any one list that will be printed by PRINT, etc.

COD***/

/***DOC REDEFP
REDEFP							(variable)

	if TRUE, allows primitives to be erased (ERASE) or redefined (COPYDEF).

COD***/

/***DOC STARTUP
STARTUP							(variable)

	if assigned a list value in a file loaded by LOAD, that value is
	run as an instructionlist after the loading.

COD***/

/***DOC UNBURYONEDIT
UNBURYONEDIT						(variable)

	if TRUE, causes any procedure defined during EDIT or LOAD to be
	unburied, so that it will be saved by a later SAVE.  Files that
	want to define and bury procedures must do it in that order.

COD***/

/***DOC FULLPRINTP
FULLPRINTP						(variable)

	if TRUE, then words that were created using backslash or vertical bar
	(to include characters that would otherwise not be treated as part of
	a word) are printed with the backslashes or vertical bars shown, so
	that the printed result could be re-read by Logo to produce the same
	value.  If FULLPRINTP is TRUE then the empty word (however it was
	created) prints as ||.  (Otherwise it prints as nothing at all.)

COD***/

/***DOC USEALTERNATENAMES
USEALTERNATENAMES					(variable)

	if TRUE, causes Logo to generate non-English words (from the
	Messages file) instead of TRUE, FALSE, END, etc.

COD***/

/***DOC ALLOWGETSET
ALLOWGETSET						(variable)

	if TRUE, indicates that an attempt to use a procedure that doesn't
	exist should be taken as an implicit getter or setter procedure
	(setter if the first three letters of the name are SET) for a variable
	of the same name (without the SET if appropriate).

COD***/

/***DOC CASEIGNOREDP
CASEIGNOREDP						(variable)

	if TRUE, indicates that lower case and upper case letters should be
	considered equal by EQUALP, BEFOREP, MEMBERP, etc.  Logo initially
	makes this variable TRUE, and buries it.

COD***/