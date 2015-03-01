# IdxSubCutter
Small command line tool to cut idx/sub subtitles based on a 'cut-list'

Usage: IdxSubCutter "Path to the .idx file" "cutlist" "output folder"

cut list is a list of time codes (hh:mm:ss:zzz) which indicate which parts should be kept, in example:
"00:00:00:000-00:00:05:000,00:00:10:000-00:00:15:000" would indicate that the subtitles from 0-5 and 10-15 would be kept. 

Wrote this as a small support tool for 'Mkv Cutter'.

Note: Source code for this isn't optimized at all and the whole process could be done a lot faster.
