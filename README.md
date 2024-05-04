Simple FreeRDP fuzzer.


Slightly modified version is here - https://github.com/FreeRDP/FreeRDP/blob/master/libfreerdp/codec/test/TestFuzzCodecs.c

```
1) copy TestFuzzCommonAssistanceBinToHexString.c to common/test directory

2) build freerdp
$ ./build.sh
$ cp build/Testing/TestFuzzCommonAssistanceBinToHexString fz1

3) run fuzzer:
$ ./fz1 samples
```
