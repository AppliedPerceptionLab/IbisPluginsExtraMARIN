# IbisPluginsExtraMARIN
These plugins are complementary to the [MARIN app](https://github.com/AppliedPerceptionLab/MARIN) to integrate with the [IbisNeuronav platform](https://github.com/IbisNeuronav/Ibis) and enable in situ AR guidance on a mobile device. They were developed by [Étienne Léger](https://ap-lab.ca/people/etienneleger/), at the [Applied Perception Lab](https://ap-lab.ca/).

If you use MARIN or these plugins in your research, cite: 
> Léger, É., Reyes, J., Drouin, S., Popa, T., Hall, J. A., Collins, D. L., Kersten-Oertel, M., "MARIN: an Open Source Mobile Augmented Reality Interactive Neuronavigation System", International Journal of Computer Assisted Radiology and Surgery (2020). https://doi.org/10.1007/s11548-020-02155-6

DISCLAIMER: MARIN and these plugins are a research tool: It is not intended for normal clinical use, and is not FDA nor CE approved.

> ⚠ **OpenIGTLSender on this branch only works with the Unity version of the tablet app ([ARSpine_Ipad](https://github.com/Bigadin/ARSpine_Ipad)).**
> It no longer streams OpenIGTLink H.264 video: frames are sent as JPEG images split into raw UDP datagrams with a custom header (see below). The original MARIN iOS app, 3D Slicer, PLUS or any other generic OpenIGTLink client will **not** be able to read this stream.

## Build Instructions:  

This code should simply be checked out inside Ibis' root directory (Ibis picks up every `IbisPluginsExtra*/*/declare-plugin.cmake`). The plugins are then enabled with `IBIS_PLUGINS_BUILD_CommandsProcessing` and `IBIS_PLUGINS_BUILD_OpenIGTLSender`.

### Build OpenIGTLSender with current Ibis

Tested on Ubuntu with Qt 6, against [Bigadin/Ibis](https://codeberg.org/Bigadin/Ibis) (official Ibis main + the AR_SPINE fixes, see its `AR_SPINE.md`).

1. System packages (Ubuntu): `build-essential cmake ninja-build git nasm qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev qt6-declarative-dev libgl-dev libglu1-mesa-dev libxkbcommon-dev ocl-icd-opencl-dev opencl-headers`, plus the OpenCL driver of the GPU. Qt 6 only, not Qt 5. `nasm` is needed by OpenH264.
2. Sources:
   ```bash
   git clone https://codeberg.org/Bigadin/Ibis.git src
   cd src && git clone -b openigtl-jpeg-udp https://github.com/Bigadin/IbisPluginsExtraMARIN.git && cd ..
   ```
3. Superbuild (builds VTK 9.6.2, ITK 5.4.6, OpenIGTLink `fixes` with H.264, OpenIGTLinkIO `UDP_support`, Elastix, OpenCV, then Ibis):
   ```bash
   mkdir superbuild && cd superbuild
   cmake -G Ninja ../src/IbisSuperBuild -DIBIS_BUILD_IGTL_HARDWARE_SUPPORT=ON -DIBIS_BUILD_OIGTL_WITH_H264=ON \
         -DIBIS_BUILD_ELASTIX=ON -DIBIS_BUILD_OPENCV=ON
   CMAKE_BUILD_PARALLEL_LEVEL=2 ninja -j1     # limits RAM use; 16 GB is not enough for a fully parallel build
   ```
4. The superbuild does not forward plugin options. Enable them in the inner Ibis build, then rebuild and install:
   ```bash
   cd Ibis
   cmake . -DIBIS_PLUGINS_BUILD_GPU_RigidRegistration=ON -DIBIS_PLUGINS_BUILD_GPU_VolumeReconstruction=ON \
           -DIBIS_PLUGINS_BUILD_PedicleScrewNavigation=ON -DIBIS_PLUGINS_BUILD_OpenIGTLSender=ON
   ninja -j2 && ninja install      # -> ../Ibis-install/ibis
   ```
   GPU_RigidRegistration needs Elastix (`IBIS_BUILD_ELASTIX=ON` above), PedicleScrewNavigation needs both GPU plugins, and OpenIGTLSender needs PedicleScrewNavigation. None of these dependencies is enabled automatically.

### Dependencies  
* [OpenIGTLink](https://github.com/IbisNeuronav/OpenIGTLink/tree/fixes) built with H.264 (`IBIS_BUILD_OIGTL_WITH_H264=ON`): the sender still includes the H.264 encoder headers and reads `OH264Config.cfg` at start-up, although frames are now sent as JPEG.
* [OpenIGTLinkIO](https://github.com/IbisNeuronav/OpenIGTLinkIO/tree/UDP_support) (built by the Ibis superbuild).
* The Ibis plugin **PedicleScrewNavigation**.
* libyuv is **no longer** required.

## OpenIGTLSender: streaming protocol (Unity tablet app only)

Everything goes over UDP to the client address, on the video port. Each encoded image is split into datagrams of at most 20 + 1184 bytes (kept under a 1500 byte MTU). Every datagram starts with a 20 byte little-endian header:

| Offset | Type   | Field |
|-------:|--------|-------|
| 0      | uint32 | frameId (counted separately for each stream) |
| 4      | uint16 | chunkIndex |
| 6      | uint16 | chunkCount |
| 8      | uint64 | timestamp, ms since epoch |
| 16     | uint8  | streamId: `0` = AR overlay, `1` = navigation panels |
| 17     | uint8  | panelsActive (1 when the pointer is close enough to show the panels) |
| 18     | uint16 | panelDistanceMm (pointer tip to target bounding box, `0xFFFF` = unknown) |

- **Stream 0, AR overlay**: JPEG of 2 × 1280 × 720. Left half is the colour render, right half is its alpha channel as greyscale (JPEG has no alpha, so the tablet rebuilds transparency from this mask). The capture always takes the full window height and a fixed centred crop, so resizing the Ibis window does not change the field of view sent to the tablet.
- **Stream 1, navigation panels**: JPEG of 2 × 320 × 320, the axial and sagittal views of the PedicleScrewNavigation plugin side by side, sent every 100 ms while that plugin is navigating and the pointer tip is within 5 mm of the target (they switch off past 15 mm). The target is the reference volume, or the object named in the *Proximity object* field of the plugin panel.
- The status channel (TCP) is unchanged.

### Hardcoded values (`OpenIGTLSender/openigtlsenderplugininterface.h`)

| Define | Value | Meaning |
|--------|-------|---------|
| `CLIENT_ADDRESS` | `192.168.3.174` | Default tablet IP, editable in the plugin panel |
| `VIDEO_PORT` | `18946` | UDP port of both image streams |
| `STATUS_PORT` | `18949` | TCP status port |
| `MARIN_CAMERA_NAME_IN_CONFIG` | `iPad` | Name of the tracked camera in the Ibis hardware configuration |
| `DEVICE_NAME` | `IBIS` | OpenIGTLink device name |
| `SEND_WIDTH` × `SEND_HEIGHT` | `1280 × 720` | Size of each half of the AR frame |
| `JPEG_QUALITY` | `75` | JPEG quality of both streams |
| `PANEL_WIDTH` × `PANEL_HEIGHT` | `320 × 320` | Size of each navigation panel |
| `PANEL_PERIOD_MS` | `100` | Panel refresh period |
| `PANEL_NEAR_MM` / `PANEL_FAR_MM` | `5` / `15` | Panel show / hide distances (hysteresis) |
| `CHUNK_PAYLOAD_SIZE` | `1184` | Payload bytes per datagram |
| `CONFIG_FILE` | `OH264Config.cfg` | Still read from the Ibis config directory at startup: the H.264 encoder is initialised even though it no longer sends anything |

The client address field in the plugin panel defaults to `192.168.0.3` in the `.ui` file and is overwritten by `CLIENT_ADDRESS` at runtime.

### Additional requirements

- The **PedicleScrewNavigation** plugin must be enabled in the Ibis build: OpenIGTLSender now links against it to capture its views.
- The Qt **jpeg image format plugin** (`imageformats/qjpeg`) must be deployed next to the Ibis executable. Without it JPEG encoding fails and nothing is sent (an error is printed on stderr).
