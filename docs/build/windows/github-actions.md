# Windows - GitHub Actions developer build

Use the `Windows Developer Build` workflow when development happens on Linux or
WSL, but the result must run as a native Windows client. GitHub provides the
Windows SDK, MSVC, .NET 10 Native AOT toolchain, and vcpkg dependencies.
Nothing from that toolchain needs to be installed on the test computer.

The workflow produces a Windows x64 Release build with the MU Editor and the
developer control socket enabled. Release is intentional: its redistributable
MSVC runtime can be staged with the client, unlike the Visual Studio Debug CRT.
The workflow runs the unit tests before publishing an artifact.

## Run the workflow

The workflow runs automatically for branches named `codex/**`. For any other
branch:

1. Open **Actions** in the GitHub repository.
2. Select **Windows Developer Build**.
3. Choose **Run workflow**.
4. Select the branch to build.

When the run finishes, download the artifact named
`mu-client-windows-x64-release-editor-<run-number>`.

## Prepare a Windows test directory

The artifact intentionally excludes `Data/` and `fonts/`. Keep one persistent
test directory which contains compatible copies of both directories. They can
come from `src/bin/` in the same source revision or from the compatible data
release linked from a MuMain release.

To install a downloaded artifact while preserving those directories and an
existing `config.ini`, run from Windows PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/stage-windows-dev-build.ps1 `
  -ArtifactZip C:\Users\you\Downloads\mu-client-windows-x64-release-editor.zip `
  -RuntimeDirectory C:\MuMainDev
```

The script checks that the artifact contains both `Main.exe` and
`MUnique.Client.Library.dll` before copying it. It does not delete the runtime
directory. If a local `config.ini` already exists, it is retained.

## Launch with MUnique.OpenMU.ClientLauncher

Point the launcher at `<runtime-directory>\Main.exe` and add this server entry:

| Setting | Value |
|---------|-------|
| Description | Local OpenMU - MuMain |
| Address | `127.127.127.127` |
| Port | `44406` |

Port `44406` selects the extended protocol used by MuMain. Do not use the
launcher's original-client default of `44405`. The launcher starts the client
with the runtime directory as its working directory, so the client can locate
the network library, shaders, game data, fonts, and `config.ini`.

The workflow builds the editor into the executable. Add `--editor` when direct
launching is used and the editor should start open; otherwise press F12 after
the client starts.

## Local OpenMU networking

For a server and client on the same Windows machine, configure OpenMU's IP
resolver as **Loopback**. The connect server then advertises
`127.127.127.127` for its game servers. Docker must publish connect port
`44406` and the configured game-server ports, normally `55901` through
`55906`.

A successful cloud build proves that the source compiles, its tests pass, and
the required Windows runtime files were staged. It cannot connect to an OpenMU
server running on a developer's private computer; launch the downloaded client
locally for that final integration test.
