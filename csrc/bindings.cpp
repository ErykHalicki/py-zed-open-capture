// nanobind wrapper around zed-open-capture's video-only API (sl_oc::video::VideoCapture).
// Mirrors the C++ API directly (same classes, same methods, snake_case names).
//
// Frame buffers are copied out immediately because the underlying C++ Frame::data
// pointer refers to an internal double buffer that the capture thread can overwrite
// between calls.

#define VIDEO_MOD_AVAILABLE

#include <cstring>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>

#include "videocapture.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace sl_oc::video;

namespace {

// Python-facing mirror of sl_oc::video::Frame, with `data` as an owned numpy
// array instead of a raw pointer into the capture thread's internal buffer.
struct PyFrame {
    nb::ndarray<nb::numpy, uint8_t> data;
    uint16_t width;
    uint16_t height;
    uint8_t channels;
    uint64_t frame_id;
    uint64_t timestamp;
};

} // namespace

NB_MODULE(_py_zed_open_capture_ext, m) {
    nb::enum_<RESOLUTION>(m, "Resolution")
        .value("HD2K", RESOLUTION::HD2K)
        .value("HD1080", RESOLUTION::HD1080)
        .value("HD720", RESOLUTION::HD720)
        .value("VGA", RESOLUTION::VGA);

    nb::enum_<FPS>(m, "Fps")
        .value("FPS_15", FPS::FPS_15)
        .value("FPS_30", FPS::FPS_30)
        .value("FPS_60", FPS::FPS_60)
        .value("FPS_100", FPS::FPS_100);

    nb::enum_<CAM_SENS_POS>(m, "CamSensPos")
        .value("LEFT", CAM_SENS_POS::LEFT)
        .value("RIGHT", CAM_SENS_POS::RIGHT);

    nb::class_<VideoParams>(m, "VideoParams")
        .def(nb::init<>())
        .def_rw("res", &VideoParams::res)
        .def_rw("fps", &VideoParams::fps)
        .def_rw("verbose", &VideoParams::verbose);

    nb::class_<PyFrame>(m, "Frame")
        .def_ro("data", &PyFrame::data, nb::rv_policy::automatic)
        .def_ro("width", &PyFrame::width)
        .def_ro("height", &PyFrame::height)
        .def_ro("channels", &PyFrame::channels)
        .def_ro("frame_id", &PyFrame::frame_id)
        .def_ro("timestamp", &PyFrame::timestamp);

    nb::class_<VideoCapture>(m, "VideoCapture")
        .def(nb::init<VideoParams>(), "params"_a = VideoParams())

        .def("initialize_video", &VideoCapture::initializeVideo, "dev_id"_a = -1)

        .def("get_frame_size",
             [](VideoCapture &self) {
                 int w = 0, h = 0;
                 self.getFrameSize(w, h);
                 return std::make_pair(w, h);
             })

        .def(
            "get_last_frame",
            [](VideoCapture &self, uint64_t timeout_msec) -> nb::object {
                const Frame &frame = self.getLastFrame(timeout_msec);
                if (frame.data == nullptr || frame.width == 0 || frame.height == 0)
                    return nb::none();

                size_t nbytes = (size_t)frame.width * frame.height * frame.channels;
                uint8_t *buf = new uint8_t[nbytes];
                std::memcpy(buf, frame.data, nbytes);
                nb::capsule owner(buf, [](void *p) noexcept { delete[] static_cast<uint8_t *>(p); });

                PyFrame pf;
                pf.data = nb::ndarray<nb::numpy, uint8_t>(
                    buf, {(size_t)frame.height, (size_t)frame.width * frame.channels}, owner);
                pf.width = frame.width;
                pf.height = frame.height;
                pf.channels = frame.channels;
                pf.frame_id = frame.frame_id;
                pf.timestamp = frame.timestamp;
                return nb::cast(pf);
            },
            "timeout_msec"_a = 1000)

        // ---- LED ----
        .def("set_led_status", &VideoCapture::setLEDstatus, "status"_a)
        .def("get_led_status",
             [](VideoCapture &self) {
                 bool status = false;
                 int rc = self.getLEDstatus(&status);
                 return std::make_pair(rc, status);
             })
        .def("toggle_led",
             [](VideoCapture &self) {
                 bool value = false;
                 int rc = self.toggleLED(&value);
                 return std::make_pair(rc, value);
             })

        // ---- Image controls ----
        .def("set_brightness", &VideoCapture::setBrightness, "brightness"_a)
        .def("get_brightness", &VideoCapture::getBrightness)
        .def("reset_brightness", &VideoCapture::resetBrightness)

        .def("set_sharpness", &VideoCapture::setSharpness, "sharpness"_a)
        .def("get_sharpness", &VideoCapture::getSharpness)
        .def("reset_sharpness", &VideoCapture::resetSharpness)

        .def("set_contrast", &VideoCapture::setContrast, "contrast"_a)
        .def("get_contrast", &VideoCapture::getContrast)
        .def("reset_contrast", &VideoCapture::resetContrast)

        .def("set_hue", &VideoCapture::setHue, "hue"_a)
        .def("get_hue", &VideoCapture::getHue)
        .def("reset_hue", &VideoCapture::resetHue)

        .def("set_saturation", &VideoCapture::setSaturation, "saturation"_a)
        .def("get_saturation", &VideoCapture::getSaturation)
        .def("reset_saturation", &VideoCapture::resetSaturation)

        .def("set_white_balance", &VideoCapture::setWhiteBalance, "wb"_a)
        .def("get_white_balance", &VideoCapture::getWhiteBalance)
        .def("set_auto_white_balance", &VideoCapture::setAutoWhiteBalance, "active"_a)
        .def("get_auto_white_balance", &VideoCapture::getAutoWhiteBalance)
        .def("reset_auto_white_balance", &VideoCapture::resetAutoWhiteBalance)

        .def("set_gamma", &VideoCapture::setGamma, "gamma"_a)
        .def("get_gamma", &VideoCapture::getGamma)
        .def("reset_gamma", &VideoCapture::resetGamma)

        // ---- Auto exposure / auto gain control (reset_aec_agc is what fixes the
        // ZED-Mini frame-corruption artifact: it rewrites the ISP registers that
        // boot into a bad state) ----
        .def("set_aec_agc", &VideoCapture::setAECAGC, "active"_a)
        .def("get_aec_agc", &VideoCapture::getAECAGC)
        .def("reset_aec_agc", &VideoCapture::resetAECAGC)

        .def("set_roi_for_aec_agc", &VideoCapture::setROIforAECAGC, "side"_a, "x"_a, "y"_a, "w"_a, "h"_a)
        .def("reset_roi_for_aec_agc", &VideoCapture::resetROIforAECAGC, "side"_a)
        .def("get_roi_for_aec_agc",
             [](VideoCapture &self, CAM_SENS_POS side) {
                 uint16_t x = 0, y = 0, w = 0, h = 0;
                 bool ok = self.getROIforAECAGC(side, x, y, w, h);
                 return nb::make_tuple(ok, x, y, w, h);
             },
             "side"_a)

        .def("set_gain", &VideoCapture::setGain, "cam"_a, "gain"_a)
        .def("get_gain", &VideoCapture::getGain, "cam"_a)
        .def("set_exposure", &VideoCapture::setExposure, "cam"_a, "exposure"_a)
        .def("get_exposure", &VideoCapture::getExposure, "cam"_a)

        // ---- Misc ----
        // Note: setColorBars() and resetAGCAECregisters() are declared in
        // videocapture.hpp but their definitions are compiled only under the
        // upstream SENSOR_LOG_AVAILABLE debug-logging flag (an upstream
        // header/impl mismatch), which this build doesn't enable, so they're
        // not bound here.
        .def("get_serial_number", &VideoCapture::getSerialNumber)
        .def("get_device_name", &VideoCapture::getDeviceName)
        .def("get_device_id", &VideoCapture::getDeviceId);
}
