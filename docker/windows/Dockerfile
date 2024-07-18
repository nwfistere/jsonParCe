# escape=`

# Use the latest Windows Server Core 2022 image.
FROM mcr.microsoft.com/windows/servercore:ltsc2022 as builder

SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

RUN `
    curl -OutFile .\vs_buildtools.exe -Uri https://aka.ms/vs/17/release/vs_buildtools.exe `
    ; .\vs_buildtools.exe --quiet --wait --norestart --nocache `
    --add Microsoft.Component.MSBuild `
    --add Microsoft.VisualStudio.Component.DiagnosticTools `
    --add Microsoft.VisualStudio.Component.VC.CoreIde `
    --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    --add Microsoft.VisualStudio.Component.Graphics.Tools `
    --add Microsoft.VisualStudio.Component.VC.DiagnosticTools `
    --add Microsoft.VisualStudio.Component.ManagedDesktop.Prerequisites `
    --add Microsoft.VisualStudio.Component.DotNetModelBuilder `
    --add Microsoft.VisualStudio.Component.VC.ATL `
    --add Microsoft.VisualStudio.Component.VC.Redist.14.Latest `
    --add Microsoft.VisualStudio.Component.VC.TestAdapterForBoostTest `
    --add Microsoft.VisualStudio.Component.VC.TestAdapterForGoogleTest `
    --add Microsoft.VisualStudio.Component.VC.ASAN `
    --add Microsoft.VisualStudio.Component.Windows10SDK.19041

RUN `
  $git_url = 'https://api.github.com/repos/git-for-windows/git/releases/latest'; `
  $asset = Invoke-RestMethod -Method Get -Uri $git_url | % assets | where name -like '*64-bit.exe'; `
  $installer = \"$env:temp\\$($asset.name)\"; `
  Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $installer; `
  $install_args = \"/SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /CLOSEAPPLICATIONS /RESTARTAPPLICATIONS\"; `
  Start-Process -FilePath $installer -ArgumentList $install_args -Wait


RUN `
  curl -OutFile cmake.msi -Uri https://github.com/Kitware/CMake/releases/download/v3.29.6/cmake-3.29.6-windows-x86_64.msi `
  ; msiexec /i cmake.msi /passive /qn /norestart

RUN "[Environment]::SetEnvironmentVariable('Path', $env:Path + ';C:\Program Files\cmake\bin', [EnvironmentVariableTarget]::Machine)"

ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8

SHELL [ "cmd.exe", "/S", "/C" ]

# CMD ["C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat", "&&", "powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]

ENTRYPOINT ["C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat", "&&", "powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
