"""Python bindings for zed-open-capture: raw video capture for the ZED, ZED-Mini,
ZED-2 and ZED-2i stereo cameras, without the CUDA-only ZED SDK.

This module mirrors the C++ API 1:1 (see zed-open-capture's videocapture.hpp):

    from py_zed_open_capture import VideoCapture, VideoParams, Resolution, Fps

    params = VideoParams()
    params.res = Resolution.HD720
    params.fps = Fps.FPS_30

    cap = VideoCapture(params)
    cap.initialize_video(-1)          # -1 = auto-detect device; also resets the
                                       # AEC/AGC ISP registers (fixes the frame
                                       # corruption artifact seen over plain UVC/cv2)
    frame = cap.get_last_frame(500)   # timeout_msec; None on timeout
    frame.data                        # numpy uint8 array, shape (height, width*channels)
    frame.width, frame.height, frame.channels
    frame.frame_id, frame.timestamp
"""

import numpy as np

from ._py_zed_open_capture_ext import (
    CamSensPos,
    Fps,
    Frame,
    Resolution,
    VideoCapture,
    VideoParams,
)

__all__ = [
    "CamSensPos",
    "Fps",
    "Frame",
    "Resolution",
    "VideoCapture",
    "VideoParams",
    "split_stereo",
    "to_bgr",
    "to_rgb",
]


def split_stereo(frame: Frame):
    """Split a Frame's combined side-by-side raw data into (left, right) halves."""
    half_row_bytes = (frame.width // 2) * frame.channels
    return frame.data[:, :half_row_bytes], frame.data[:, half_row_bytes:]


def _require_cv2(fn_name: str):
    try:
        import cv2
    except ImportError as e:
        raise ImportError(
            f"{fn_name}() requires opencv. Install with: pip install py-zed-open-capture[cv2]"
        ) from e
    return cv2


def to_bgr(frame: Frame) -> np.ndarray:
    """Convert a Frame's raw YUV 4:2:2 (YUYV) data to a BGR image.
    Requires opencv: pip install py-zed-open-capture[cv2]"""
    cv2 = _require_cv2("to_bgr")
    arr = frame.data.reshape(frame.height, frame.width, frame.channels)
    return cv2.cvtColor(arr, cv2.COLOR_YUV2BGR_YUYV)


def to_rgb(frame: Frame) -> np.ndarray:
    """Convert a Frame's raw YUV 4:2:2 (YUYV) data to an RGB image.
    Requires opencv: pip install py-zed-open-capture[cv2]"""
    cv2 = _require_cv2("to_rgb")
    arr = frame.data.reshape(frame.height, frame.width, frame.channels)
    return cv2.cvtColor(arr, cv2.COLOR_YUV2RGB_YUYV)
