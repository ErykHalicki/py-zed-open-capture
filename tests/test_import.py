"""Smoke test: the compiled extension imports and exposes the expected API.
No camera is required (and none is assumed to be present in CI)."""

import py_zed_open_capture as zoc


def test_public_api():
    for name in ["CamSensPos", "Fps", "Frame", "Resolution", "VideoCapture", "VideoParams"]:
        assert hasattr(zoc, name)


def test_video_params_defaults():
    params = zoc.VideoParams()
    params.res = zoc.Resolution.HD720
    params.fps = zoc.Fps.FPS_30
    assert params.res == zoc.Resolution.HD720
    assert params.fps == zoc.Fps.FPS_30


def test_video_capture_constructs():
    params = zoc.VideoParams()
    cap = zoc.VideoCapture(params)
    assert cap is not None
