AMDMsrTweaker

Description
-----------

AmdMsrTweaker is a command line tool for Windows which allows to reprogram the 
P-States used by various CPUs and APUs of AMD for their Cool&Quiet power 
saving technology.

The following CPU lines are supported:
- K10 / K10.5 CPUs (various Athlon/Phenom models): CPU P-States, NB P-States on supported models
- Llano (Family 12h):  CPU P-States
- Ontario / Zacate (Family 14h):  CPU P-States
- Bulldozer, Piledriver, Trinity, Richland, Kaveri (Family 15h): CPU P-States, NB P-States


Usage
-----

The following quick guide was taken from the original thread.

Quick guide: I'm too lazy to write a complete guide just now, so please take a look at the following examples:
AmdMsrTweaker
=> no parameters: info
AmdMsrTweaker P0=12.5@1.4 P2=8 P3=@0.85
=> modifies P0 (multi=12.5, VID=1.4V), P2 (multi=8) and P3 (VID=0.85V)
AmdMsrTweaker P2
=> switches to P2 (note: if C&Q is enabled, the cores are likely to switch very soon to another P-state, so this makes only sense if C&Q is disabled or if the high-performance power-profile is active)
AmdMsrTweaker Turbo=0
=> disables the turbo (use 1 to enable it)
AmdMsrTweaker APM=0
=> disables Application Power Management (TDP limiting) for Bulldozer (use 1 to enable it)
AmdMsrTweaker NB_P0=8@1.3 NB_P1=@1.1 NB_low=3
=> modifies the NorthBridge P0 state (multi=8 (multis only supported by Bulldozer), VID=1.3V), its P1 state (VID=1.1V) and uses NB_P0 for all P-states < 3 and NB_P1 for all P-states >= 3
You can combine all parameters above

Do note that from version 1.1 onwards, different voltage steps are supported.
The voltage step supported on your platform is indicated on the info output.


Version History
---------------

v1.1	2013-04-09
- Trinity and Richland support
- preliminary Kaveri support

v1.0	2011-12-18
- initial release