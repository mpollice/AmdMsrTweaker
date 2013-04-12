AmdMsrTweaker
=============

AmdMsrTweaker is a command line tool for Windows which allows to reprogram the 
P-States used by various CPUs and APUs of AMD for their Cool&Quiet power 
saving technology.

Usage
-----

If you just want to use the tool, just download the precompiled binary and 
follow the directions in the readme.txt file:
* [AmdMsrTweaker v1.1](https://mega.co.nz/#!C0wk2ZQC!cMaQODozN40y3elVHbSCDkv-irVJ69HkeR5da2TT0gg)


Development
-----------

The project is set up using Visual Studio 2010. Using the Express Edition is 
also possible, though for compiling 64bit builds the respective Windows SDK is 
needed.

Additionally you need to download the WinRing0 library, which is not included in 
the Git repository:
* [WinRing0_lib.zip](https://mega.co.nz/#!StAywLoT!K0-wx0n-6_9npwH64hb1vmgBKbFqv660X38-9paSw84)

Just check out the repository, extract the WinRing0_lib.zip file and then you 
should be able to compile it in Visual Studio.