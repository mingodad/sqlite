The 'build' script that is provided generates a makefile to build a dynamically
loadable library. The library can be tested in the sqlite3 command-line tool,
provided that the .load command is supported (check that it is listed when
you type .help)
Note: on Mac the Xcode command-line tools are required (free).

If .load isn't supported by the sqlite3 program that is installed (it may
happen on a Mac and it is tested in the 'build' script) you will be asked to
download the latest version from http://www.sqlite.org (a zip file) into
the current directory. When you re-run the script, it will unzip the file
if you haven't done it and recompile the command-line SQL interpreter
as 'sqlshell' instead of 'sqlite3' in order to avoid any conflict.
You will then be able to load the dynamically loadable library using
the .load command (depending on the environment, you may need to omit the
filename extension when loading the file). Then you will be able to use
functions in your SQL statements. You can of course also link them to a
C program of your own, other that the command-line SQL interpreter.

Note that if you have downloaded the SQLite distribution elsewhere than in
the current directory, you need to unzip it yourself and pass the location
of the directory created when unzipping it to the 'build' script using the
-d flag:

./build -d <directory name>

