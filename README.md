# iBMSUnreal

A crossplatform BMS player, powered by Unreal Engine 5

## Supported platforms

- [x] Android
- [x] iOS
- [x] Windows
- [x] macOS
- [ ] Linux (coming soon)

## Capabilities
- Super-fast multithreaded BMS parsing
- BGA playback for all platforms (via ffmpeg transcoding)
- Touch input on mobile devices
- Independent input thread (currently, only for macOS and Windows)

## TODOs
- Parser
  - #SCROLL
  - BMSON support
  - PMS support
- Gameplay
  - Groove Gauge
  - S-RANDOM, HALF-RANDOM, ...
  - Play options (HI-SPEED, LIFT, SUDDEN, LNMODE, etc.)
  - Result screen 
  - Key bomb effect
  - Classic 2D VSRG gear for desktop
  - Course play
- Additional features
  - Replay
  - In-game chart viewer
  - Custom skin support
  - ASIO support
  - Add free audio library (OpenAL?)
  - Controller support
- Miscellaneous
  - Consider implementing vlc-for-unreal to play BGA without transcoding

## How to import BMS

### Android

Due to Android's scope storage policy, iBMSUnreal cannot read arbitrary BMS folders. So there's two ways you can import BMS files:
- Open zip file with iBMSUnreal: the app will unzip the file to its data folder.
- Connect the phone to your PC
  - If you want to import unzipped files, copy files into Android/data/com.YourCompany.iBMSUnreal/files/BMS (this method is super slow if you have a large number of files)
  - If you want to import zip files, copy files into Android/data/com.YourCompany.iBMSUnreal/files/imported

### iOS
1. Open Files app
2. Put your BMS folder under iBMSUnreal/BMS

### macOS, Windows

Simply run the game and select your BMS folder in the dialog.

## License 

Although iBMSUnreal is licensed under GPLv3.0, it includes non-free GPL-incompatible libraries like fmod. We are planning to address this issue by supporting free audio library as an alternative.
