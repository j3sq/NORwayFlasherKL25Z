NORway Flasher KL25Z port V0.1 Oct-18 2014

Disclaimer
----------

WARNING: Use this software at your own risk. The author accepts no
responsibility for the consequences of your use of this software.


This project has been ported for the FRDM-KL25Z board from hjudges NORway at https://github.com/hjudges/NORway.
For detailed documentation and information refer to hjudges repo.

USB CDC functioanlity is based on Erich Styger work.

I've ported and tested this on my slim ps3 model CECH 2004B ,DYN001 board, NOR flash Spansion S29GL128P90TFIR2.
In theory it should work on other NOR based models but NO testing have been done. Proceed at your responsibility.

Average Performance during my tests (mm:ss):
Full Dump: 04:46 
Chip Erase: 00:47
Full Write : 11:44 (buffered write mode)

Usage:
1- Dump NOR : norway.py comXX dump filename.bin
2- Erase Chip: norway.py comXX erasechip
3- Write to NOR: norway.py comXX write filename.bin (must erase chip first. See notes)

Notes:
-For wiring and pinout refer to wiring.xlsx
-Sector Erase functionality is currently broken. The workaround is to do erase chip before writing to flash 
	as this eliminate the need to execute erase sector.
-The provided python script NORway.py is ported from hjudges, most changes are just to address the different 
	way USB is handled between the two devices.

   

--J3SQ (jqut.dev@gmail.com)

