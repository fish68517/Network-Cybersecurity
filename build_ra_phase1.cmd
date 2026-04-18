call "E:\Program Files (x86)\Microsoft\visualStudio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
"E:\Program Files (x86)\Microsoft\visualStudio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "D:\Acode\Android\complete\MedicalSystemDemo\MedicalSystemDemo\MedicalSystemDemo.vcxproj" /p:Configuration=Simulation /p:Platform=x64 /m:1 /nologo
if errorlevel 1 exit /b 1
"E:\Program Files (x86)\Microsoft\visualStudio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "D:\Acode\Android\complete\MedicalSystemDemo\MedicalSystemDemo_app\MedicalSystemDemo_app.vcxproj" /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /m:1 /nologo
