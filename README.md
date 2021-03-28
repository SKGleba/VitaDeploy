# VitaDeploy
Toolbox that makes homebrewing the Playstation Vita/TV easier

![ref0](https://github.com/SKGleba/VitaDeploy/raw/main/screenshots/main_menu.png)

## Features
- file manager (VitaShell)
- sd2vita mount/format
- firmware updater/downgrader
  - 3.65 preset "Quick Install"
  - optional enso install
  - optional taihen setup
- app downloader (for core hb apps)
- internal memory creation tool (imcunlock)
- storage format utility
- AutoAVLS enable/disable
- kiosk and manufacturing mode disable
- can run entirely on vs0

### Firmware installer
- VitaDeploy can download the following OS: 3.60, 3.65 or 3.68
- The following tai presets are available for download via VD:
  - Vanilla - taiHEN and HENkaku only
  - Recommended - taiHENkaku and some core homebrew plugins
  - R + YAMT - Recommended preset with the YAMT storage tool
- "Quick install" installs the 3.65 OS with the R+YAMT preset
- You can install your own PUP by putting it in ud0:PSP2UPDATE/

![ref1](https://github.com/SKGleba/VitaDeploy/raw/main/screenshots/install_os_pup.png)

## Notes
- I wrote a small [guide](https://hackmyvita.gitbook.io/start) to hacking the vita from scratch using VD
- If you do not have network access you can [download](https://mega.nz/folder/egoijADB#aBS8os-NEToqbLcrysjwiw) the packages manually
  - Put the zip in "ur0:vd-udl.zip", VD will skip OS and tai download
- "Format Storage"/"Clean the update partition" does not work on 3.73
  - This will probably be fixed in future updates
- It is recommended to disable all plugins before using VitaDeploy

## Included tools
- [VitaShell](https://github.com/TheOfficialFloW/VitaShell)
- [tiny modoru](https://github.com/SKGleba/modoru/tree/tiny)
- [universal enso](https://github.com/SKGleba/enso)
- [IMCUnlock](https://github.com/SKGleba/IMCUnlock)
- [YAMT](https://github.com/SKGleba/yamt-vita)
- [Vita-NoAutoAvls](https://github.com/SKGleba/VITA-NoAutoAvls)

## Credits
- Team Molecule for taiHENkaku and enso
- theflow0 for modoru
- xerpi for vita2d
- all vitasdk contributors

### Donation
- Via [ko-fi](https://ko-fi.com/skgleba), thanks!
