# Building an Installer

Frasy applications are distributed as standalone Windows executables with supporting files
(Lua scripts, assets, config). [Inno Setup](https://jrsoftware.org/isinfo.php) is the
recommended tool for packaging everything into a single installer `.exe` that operators can
run on test stations.

---

## Prerequisites

| Tool | Purpose |
|---|---|
| [Inno Setup](https://jrsoftware.org/isdl.php) | Installer compiler |
| A **RelWithDebInfo** build | Optimized + debug symbols for crash reports |
| **VC_redist.x64.exe** | Microsoft Visual C++ Redistributable (required on target machines) |

!!! warning "Always use RelWithDebInfo"
    The application must be built with `RelWithDebInfo` to properly recover stacktraces from
    crashes. A pure `Release` build will produce unusable crash reports.

---

## What Gets Packaged

The build output directory contains everything needed to run the application:

```
build/bin/MyApp_v1.0.0/
  MyApp.exe              ← the application
  MyApp.pdb              ← debug symbols (for crash reports)
  wkhtmltox.dll          ← PDF generation library
  config.json            ← runtime configuration
  assets/
    textures/            ← UI icons and images
    fonts/               ← application fonts
  lua/
    core/                ← Frasy Lua SDK
    user/                ← your product test scripts
```

The installer bundles all of this into a single distributable file.

---

## The Inno Setup Script

The template includes an `inno_script.iss` file at the project root. Here's a complete,
annotated version:

```iss
; --- Preprocessor Definitions ---
; Adjust these to match your build output
#define AppName "MyApp"
#define AppVersion "1.0.0"
#define BinDir "cmake-build-relwithdebinfo-visual-studio\bin\" + AppName + "_v" + AppVersion
#define ExeName AppName

[Setup]
; Application metadata
AppName={#AppName}
AppVersion={#AppVersion}
AppId=YOUR-UNIQUE-GUID-HERE
AppPublisher=Your Company Name
AppPublisherURL=https://your-company.com
AppSupportURL=https://your-company.com/support
AppCopyright=Your Company © 2024

; Installer behavior
WizardStyle=modern
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
AllowUNCPath=no
AlwaysShowComponentsList=no
PrivilegesRequired=lowest
OutputBaseFilename={#AppName}Installer_v{#AppVersion}
OutputDir=installer_output

[Files]
; Application executable and debug symbols
Source: "{#BinDir}\{#ExeName}.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\{#ExeName}.pdb"; DestDir: "{app}"; Flags: ignoreversion

; Required DLLs
Source: "{#BinDir}\wkhtmltox.dll"; DestDir: "{app}"; Flags: ignoreversion

; Configuration
Source: "{#BinDir}\config.json"; DestDir: "{app}"; Flags: onlyifdoesntexist

; Assets and Lua scripts
Source: "{#BinDir}\assets\*"; DestDir: "{app}\assets"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#BinDir}\lua\*"; DestDir: "{app}\lua"; Flags: recursesubdirs createallsubdirs ignoreversion

; Visual C++ Redistributable (place in a known location)
Source: "redist\VC_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
; Start menu and desktop shortcuts
Name: "{userstartmenu}\{#AppName}"; Filename: "{app}\{#ExeName}.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\{#ExeName}.exe"; WorkingDir: "{app}"

[Run]
; Install the VC++ redistributable silently
Filename: "{tmp}\VC_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ Redistributable..."
```

---

## Step-by-Step Instructions

### 1. Build the Application

```bat
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

Verify the output directory contains all required files:

```bat
dir build\bin\MyApp_v1.0.0\
```

You should see the `.exe`, `.pdb`, `wkhtmltox.dll`, `config.json`, `assets/`, and `lua/`.

### 2. Obtain the VC++ Redistributable

Download the latest **Visual C++ Redistributable for x64** from
[Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).

Place it at `redist/VC_redist.x64.exe` relative to your project root (or adjust the path in
the script).

### 3. Generate a Unique AppId

Every application needs a unique GUID for Windows to distinguish it from other installed
software. Generate one using PowerShell:

```powershell
[guid]::NewGuid().ToString().ToUpper()
```

Replace `YOUR-UNIQUE-GUID-HERE` in the script with the generated GUID.

### 4. Customize the Script

Update the preprocessor definitions at the top:

```iss
#define AppName "YourAppName"
#define AppVersion "1.0.0"
#define BinDir "cmake-build-relwithdebinfo-visual-studio\bin\YourAppName_v1.0.0"
#define ExeName "YourAppName"
```

Update the `[Setup]` section with your company information.

### 5. Compile the Installer

Open `inno_script.iss` in Inno Setup and press ++ctrl+f9++ to compile, or use the command line:

```bat
iscc inno_script.iss
```

The output is a single `.exe` file (e.g., `MyAppInstaller_v1.0.0.exe`) in the `OutputDir`
directory.

---

## Key Decisions

### `config.json`: Overwrite or Preserve?

The template uses `Flags: onlyifdoesntexist` for `config.json`. This means:

- **First install**: the config is placed in the app directory.
- **Upgrades**: the existing config is preserved (operator customizations are kept).

If you want upgrades to always reset the config, change to `Flags: ignoreversion`.

### PDB Files

Including the `.pdb` file in the installer enables meaningful crash stacktraces on operator
machines. This adds ~100–150 MB to the installer size. If size is a concern, you can:

- Ship the PDB separately and instruct support to retrieve it when debugging.
- Store PDBs on a network share indexed by version for symbol server lookup.

### PrivilegesRequired

The template uses `PrivilegesRequired=lowest`, which installs per-user (no admin required).
This is suitable for most test station deployments. If you need machine-wide installation
(e.g., for shared stations), change to `PrivilegesRequired=admin` and update `DefaultDirName`
to use `{commonpf}` instead of `{autopf}`.

---

## Updating Test Scripts Without Reinstalling

In production environments, you may need to update Lua test scripts without rebuilding or
reinstalling the entire application. Since Lua scripts are plain files in the `lua/user/`
directory, you can:

1. Copy updated `.lua` files to the install directory (`{app}\lua\user\`).
2. Regenerate hashes by running `generate_hashes.bat` targeting the install directory.
3. Restart the application (or press ++f9++ to reload products).

!!! note
    Hash verification must pass for the updated scripts to run. Either regenerate hashes after
    copying, or run in debug mode (++f10++) to skip verification during development.

---

## Automating the Build

For CI/CD pipelines, the full build-and-package flow:

```bat
@echo off
set APP_NAME=MyApp
set APP_VERSION=1.0.0

:: Build
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo

:: Verify output exists
if not exist "build\bin\%APP_NAME%_v%APP_VERSION%\%APP_NAME%.exe" (
    echo ERROR: Build output not found
    exit /b 1
)

:: Compile installer
iscc inno_script.iss
if %ERRORLEVEL% neq 0 (
    echo ERROR: Installer compilation failed
    exit /b 1
)

echo Installer created successfully.
```

---

## Checklist

Before distributing an installer:

- [ ] Built with **RelWithDebInfo** configuration
- [ ] `config.json` contains the correct `usbWhitelist` for your hardware
- [ ] All products in `lua/user/` are present and tested
- [ ] Script hashes are generated and up-to-date
- [ ] `VC_redist.x64.exe` is included for machines without the runtime
- [ ] `AppId` is a unique GUID (not shared with other applications)
- [ ] `AppVersion` matches the version in `CMakeLists.txt`
- [ ] Tested on a clean machine (no Visual Studio installed)
