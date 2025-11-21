@echo off
echo Shaders compilation...
xcopy .\shaders .\shadersSpirv /t /i
for %%G in (.vert, .frag, .comp, .rchit, .rgen, .rmiss, .rahit) do (
	for /f %%i in ('FORFILES /P shaders\ /S /M *%%G /C "cmd /c echo @relpath"') do (
		%VULKAN_SDK%\Bin\glslc.exe --target-env=vulkan1.4 .\shaders\%%~i -o .\shadersSpirv\%%~i.spv
	)
)
pause