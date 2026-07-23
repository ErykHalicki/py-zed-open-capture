# py-zed-open-capture

Python bindings for [zed-open-capture](https://github.com/stereolabs/zed-open-capture)

Allows raw video capture for the ZED, ZED-Mini, ZED-2 and ZED-2i stereo cameras without the official ZED SDK.

## Why this exists

ZED cameras can send corrupt frames when opened as a plain UVC device (e.g. via
`cv2.VideoCapture`). Fixing it requires a vendor-specific register write over a UVC
extension unit that isn't exposed by standard V4L2/OpenCV. `zed-open-capture`
implements that fix; this package wraps it so it can be called from
Python without the full ZED SDK, and applies it automatically when you open the camera.

## Install

```bash
pip install py-zed-open-capture
```

Prebuilt wheels are published for `manylinux_x86_64` and `manylinux_aarch64`, Python
>=3.12, via the stable ABI (one wheel per arch covers any 3.12+ interpreter). To build
from source instead (e.g. for a different architecture):

```bash
sudo apt install build-essential cmake libusb-1.0-0-dev pkg-config
git clone --recurse-submodules https://github.com/ErykHalicki/py-zed-open-capture.git
pip install ./py-zed-open-capture
```

## Usage

The API mirrors zed-open-capture's C++ `VideoCapture`/`VideoParams`/`Frame` directly
(see `third_party/zed-open-capture/include/videocapture.hpp`), with snake_case names:

```python
from py_zed_open_capture import VideoCapture, VideoParams, Resolution, Fps

params = VideoParams()
params.res = Resolution.HD720
params.fps = Fps.FPS_30

cap = VideoCapture(params)
cap.initialize_video(-1)         # -1 = auto-detect device

frame = cap.get_last_frame(500)  # timeout_msec; None on timeout
frame.data                       # numpy uint8 array, raw YUV 4:2:2, side-by-side stereo
frame.width, frame.height, frame.channels
frame.frame_id, frame.timestamp
```

Two standalone helpers are provided for the common post-processing steps (not part of
the mirrored C++ API):

```python
from py_zed_open_capture import split_stereo, to_bgr, to_rgb

left, right = split_stereo(frame)  # raw YUV422 halves
bgr = to_bgr(frame)                # combined side-by-side BGR image, requires opencv
rgb = to_rgb(frame)                # combined side-by-side RGB image, requires opencv
```

`initialize_video()` calls the C++ `initializeVideo()`, which resets the camera's
AEC/AGC ISP registers, this is the step that avoids the corruption artifact. If it
reappears mid-session, call `cap.reset_aec_agc()`.
