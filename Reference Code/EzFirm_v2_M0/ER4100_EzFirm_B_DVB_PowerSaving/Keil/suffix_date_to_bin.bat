@echo off
::Obtain date,format is YYYYMMDD
set datevar=%date:~0,4%%date:~5,2%%date:~8,2%
::Obtain hour of Time,format is 24H
set timevar=%time:~0,2%
if /i %timevar% LSS 10 (
set timevar=0%time:~1,1%
)
::Obtain minute and second of time
set timevar=%timevar%%time:~3,2%%time:~6,2%
@echo %datevar%%timevar%

copy .\obj\TransTxRx.bin .\TransTxRx_%datevar%%timevar%.bin
