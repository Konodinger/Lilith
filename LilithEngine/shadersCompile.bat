@echo off
echo Shaders compilation...
for %%G in (.vert, .frag) do (
	for /f %%i in ('FORFILES /P shaders\ /M *%%G /C "cmd /c echo @relpath"') do (
		C:\VulkanSDK\1.3.261.1\Bin\glslc.exe .\shaders\%%~i -o .\shaders\%%~i.spv
	)
)
pause