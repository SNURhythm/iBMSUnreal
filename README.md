# iBMSUnreal

A crossplatform BMS player

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

## Caveats

### Android

Due to Android's scope storage policy, iBMSUnreal cannot read arbitrary BMS folders. So there's two ways you can import BMS files:
- Open zip file with iBMSUnreal: the app will unzip the file to its data folder.
- Connect the phone to your PC
  - If you want to import unzipped files, copy files into Android/data/com.YourCompany.iBMSUnreal/files/BMS (this method is super slow if you have a large number of files)
  - If you want to import zip files, copy files into Android/data/com.YourCompany.iBMSUnreal/files/imported
