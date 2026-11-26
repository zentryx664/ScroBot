#include "Record/Recorder.hpp"
#include <Geode/Geode.hpp>

#ifdef GEODE_IS_ANDROID
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <Geode/cocos/platform/android/jni/JniHelper.h>
#include <jni.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <fcntl.h>
#include <filesystem>
#include <thread>
#include <unistd.h>
#include <vector>
using namespace geode::prelude;

namespace scro::rec {
namespace {

// ---- the few NDK media types we need; the library is loaded at runtime so the build needs no extra setup
struct AMediaCodec; struct AMediaFormat; struct AMediaMuxer; struct ANativeWindow;
struct BufInfo { int32_t offset; int32_t size; int64_t pts; uint32_t flags; };
constexpr uint32_t kEos = 4, kConfigBuf = 2;
constexpr ssize_t kTryAgain = -1, kFormatChanged = -2;
constexpr int32_t kColorSurface = 0x7F000789;

struct Ndk {
    AMediaFormat* (*fmtNew)();
    int (*fmtDelete)(AMediaFormat*);
    void (*fmtSetString)(AMediaFormat*, char const*, char const*);
    void (*fmtSetInt32)(AMediaFormat*, char const*, int32_t);
    AMediaCodec* (*createEncoder)(char const*);
    int (*configure)(AMediaCodec*, AMediaFormat const*, ANativeWindow*, void*, uint32_t);
    int (*codecStart)(AMediaCodec*);
    int (*codecStop)(AMediaCodec*);
    int (*codecDelete)(AMediaCodec*);
    ssize_t (*dequeueIn)(AMediaCodec*, int64_t);
    uint8_t* (*getIn)(AMediaCodec*, size_t, size_t*);
    int (*queueIn)(AMediaCodec*, size_t, off_t, size_t, uint64_t, uint32_t);
    ssize_t (*dequeueOut)(AMediaCodec*, BufInfo*, int64_t);
    uint8_t* (*getOut)(AMediaCodec*, size_t, size_t*);
    int (*releaseOut)(AMediaCodec*, size_t, bool);
    AMediaFormat* (*getOutFormat)(AMediaCodec*);
    int (*createSurface)(AMediaCodec*, ANativeWindow**);
    int (*signalEos)(AMediaCodec*);
    AMediaMuxer* (*muxNew)(int, int);
    ssize_t (*muxAddTrack)(AMediaMuxer*, AMediaFormat const*);
    int (*muxStart)(AMediaMuxer*);
    int (*muxWrite)(AMediaMuxer*, size_t, uint8_t const*, BufInfo const*);
    int (*muxStop)(AMediaMuxer*);
    int (*muxDelete)(AMediaMuxer*);
    void (*windowRelease)(ANativeWindow*);
    bool ok = false;
};
Ndk ndk;

template <class F> bool sym(void* lib, char const* name, F& out) {
    out = reinterpret_cast<F>(dlsym(lib, name));
    return out != nullptr;
}
bool loadNdk() {
    if (ndk.ok) return true;
    void* m = dlopen("libmediandk.so", RTLD_NOW);
    void* a = dlopen("libandroid.so", RTLD_NOW);
    if (!m || !a) return false;
    bool ok = sym(m, "AMediaFormat_new", ndk.fmtNew) && sym(m, "AMediaFormat_delete", ndk.fmtDelete)
        && sym(m, "AMediaFormat_setString", ndk.fmtSetString) && sym(m, "AMediaFormat_setInt32", ndk.fmtSetInt32)
        && sym(m, "AMediaCodec_createEncoderByType", ndk.createEncoder) && sym(m, "AMediaCodec_configure", ndk.configure)
        && sym(m, "AMediaCodec_start", ndk.codecStart) && sym(m, "AMediaCodec_stop", ndk.codecStop)
        && sym(m, "AMediaCodec_delete", ndk.codecDelete) && sym(m, "AMediaCodec_dequeueInputBuffer", ndk.dequeueIn)
        && sym(m, "AMediaCodec_getInputBuffer", ndk.getIn) && sym(m, "AMediaCodec_queueInputBuffer", ndk.queueIn)
        && sym(m, "AMediaCodec_dequeueOutputBuffer", ndk.dequeueOut) && sym(m, "AMediaCodec_getOutputBuffer", ndk.getOut)
        && sym(m, "AMediaCodec_releaseOutputBuffer", ndk.releaseOut) && sym(m, "AMediaCodec_getOutputFormat", ndk.getOutFormat)
        && sym(m, "AMediaCodec_createInputSurface", ndk.createSurface) && sym(m, "AMediaCodec_signalEndOfInputStream", ndk.signalEos)
        && sym(m, "AMediaMuxer_new", ndk.muxNew) && sym(m, "AMediaMuxer_addTrack", ndk.muxAddTrack)
        && sym(m, "AMediaMuxer_start", ndk.muxStart) && sym(m, "AMediaMuxer_writeSampleData", ndk.muxWrite)
        && sym(m, "AMediaMuxer_stop", ndk.muxStop) && sym(m, "AMediaMuxer_delete", ndk.muxDelete)
        && sym(a, "ANativeWindow_release", ndk.windowRelease);
    ndk.ok = ok;
    return ok;
}

int64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ---- game sound: a pass-through FMOD DSP on the master group copies the final mix into a ring buffer
constexpr size_t kRingSize = 1 << 19;           // floats (about 5 s of stereo audio)
std::vector<float> g_ring;
std::atomic<uint64_t> g_wr{0}, g_rd{0};

FMOD_RESULT F_CALL dspRead(FMOD_DSP_STATE*, float* in, float* out, unsigned int len, int inCh, int* outCh) {
    if (in != out) std::memcpy(out, in, sizeof(float) * len * inCh);
    *outCh = inCh;
    uint64_t w = g_wr.load(std::memory_order_relaxed), r = g_rd.load(std::memory_order_acquire);
    if (kRingSize - (w - r) < static_cast<uint64_t>(len) * 2) return FMOD_OK;     // consumer too slow: drop
    for (unsigned i = 0; i < len; i++) {
        g_ring[(w++) & (kRingSize - 1)] = in[i * inCh];
        g_ring[(w++) & (kRingSize - 1)] = inCh > 1 ? in[i * inCh + 1] : in[i * inCh];
    }
    g_wr.store(w, std::memory_order_release);
    return FMOD_OK;
}

struct Pending { bool video; std::vector<uint8_t> data; BufInfo info; };

struct State {
    bool active = false, lost = false;
    Config cfg;
    // graphics
    EGLDisplay dpy = EGL_NO_DISPLAY;
    EGLContext gdCtx = EGL_NO_CONTEXT, recCtx = EGL_NO_CONTEXT;
    EGLSurface gdDraw = EGL_NO_SURFACE, gdRead = EGL_NO_SURFACE, encSurf = EGL_NO_SURFACE;
    GLuint tex = 0, prog = 0;
    int srcW = 0, srcH = 0, outW = 0, outH = 0;
    ANativeWindow* win = nullptr;
    PFNEGLPRESENTATIONTIMEANDROIDPROC setPts = nullptr;
    // encoders and file
    AMediaCodec* vc = nullptr; AMediaCodec* ac = nullptr; AMediaMuxer* mux = nullptr;
    ssize_t vTrack = -1, aTrack = -1;
    bool muxStarted = false, hasAudio = false, vEos = false, aEos = false;
    std::vector<Pending> pending;
    FMOD::DSP* dsp = nullptr;
    int rate = 48000;
    uint64_t audioFrames = 0;
    int64_t startNs = 0, nextNs = 0, intervalNs = 0;
    // output
    int fd = -1;
    jobject uri = nullptr;       // gallery entry (Android 10+)
    std::string savedTo;
};
State s;

// ---- saving: gallery entry through MediaStore (no permission needed), plain file as a fallback
struct JniScope {
    JniMethodInfo t{};
    JNIEnv* env = nullptr;
    jobject app = nullptr;
    bool ok() {
        if (!JniHelper::getStaticMethodInfo(t, "android/app/ActivityThread", "currentApplication", "()Landroid/app/Application;")) return false;
        env = t.env;
        app = env->CallStaticObjectMethod(t.classID, t.methodID);
        return fine() && app;
    }
    bool fine() {
        if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
        return true;
    }
    jobject resolver() {
        auto cls = env->GetObjectClass(app);
        auto m = env->GetMethodID(cls, "getContentResolver", "()Landroid/content/ContentResolver;");
        auto r = env->CallObjectMethod(app, m);
        return fine() ? r : nullptr;
    }
    jobject values() {
        auto cls = env->FindClass("android/content/ContentValues");
        return env->NewObject(cls, env->GetMethodID(cls, "<init>", "()V"));
    }
    void putStr(jobject cv, char const* k, char const* v) {
        auto cls = env->GetObjectClass(cv);
        auto m = env->GetMethodID(cls, "put", "(Ljava/lang/String;Ljava/lang/String;)V");
        env->CallVoidMethod(cv, m, env->NewStringUTF(k), env->NewStringUTF(v));
    }
    void putInt(jobject cv, char const* k, int v) {
        auto ic = env->FindClass("java/lang/Integer");
        auto box = env->CallStaticObjectMethod(ic, env->GetStaticMethodID(ic, "valueOf", "(I)Ljava/lang/Integer;"), v);
        auto cls = env->GetObjectClass(cv);
        auto m = env->GetMethodID(cls, "put", "(Ljava/lang/String;Ljava/lang/Integer;)V");
        env->CallVoidMethod(cv, m, env->NewStringUTF(k), box);
    }
};

std::string stamp() {
    char b[48];
    std::time_t t = std::time(nullptr);
    std::strftime(b, sizeof b, "%Y%m%d-%H%M%S", std::localtime(&t));
    return b;
}

int openGallery(std::string const& name) {
    JniScope j;
    if (!j.ok()) return -1;
    auto vc = j.env->FindClass("android/os/Build$VERSION");
    if (j.env->GetStaticIntField(vc, j.env->GetStaticFieldID(vc, "SDK_INT", "I")) < 29) return -1;
    auto cr = j.resolver();
    if (!cr) return -1;
    auto cv = j.values();
    j.putStr(cv, "_display_name", name.c_str());
    j.putStr(cv, "mime_type", "video/mp4");
    j.putStr(cv, "relative_path", "Movies/ScroBot");
    j.putInt(cv, "is_pending", 1);
    auto mc = j.env->FindClass("android/provider/MediaStore$Video$Media");
    auto coll = j.env->GetStaticObjectField(mc, j.env->GetStaticFieldID(mc, "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;"));
    auto crc = j.env->GetObjectClass(cr);
    auto uri = j.env->CallObjectMethod(cr, j.env->GetMethodID(crc, "insert",
        "(Landroid/net/Uri;Landroid/content/ContentValues;)Landroid/net/Uri;"), coll, cv);
    if (!j.fine() || !uri) return -1;
    auto pfd = j.env->CallObjectMethod(cr, j.env->GetMethodID(crc, "openFileDescriptor",
        "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;"), uri, j.env->NewStringUTF("w"));
    if (!j.fine() || !pfd) return -1;
    auto pc = j.env->GetObjectClass(pfd);
    int fd = j.env->CallIntMethod(pfd, j.env->GetMethodID(pc, "detachFd", "()I"));
    if (!j.fine() || fd < 0) return -1;
    s.uri = j.env->NewGlobalRef(uri);
    s.savedTo = "Movies/ScroBot/" + name;
    return fd;
}

int openFile(std::string const& name) {
    auto dir = std::string(Mod::get()->getSaveDir().string()) + "/recordings";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    auto path = dir + "/" + name;
    s.savedTo = path;
    return open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
}

void finishGallery(bool keep) {
    if (!s.uri) return;
    JniScope j;
    if (j.ok()) {
        if (auto cr = j.resolver()) {
            auto crc = j.env->GetObjectClass(cr);
            if (keep) {
                auto cv = j.values();
                j.putInt(cv, "is_pending", 0);
                j.env->CallIntMethod(cr, j.env->GetMethodID(crc, "update",
                    "(Landroid/net/Uri;Landroid/content/ContentValues;Ljava/lang/String;[Ljava/lang/String;)I"), s.uri, cv, nullptr, nullptr);
            } else {
                j.env->CallIntMethod(cr, j.env->GetMethodID(crc, "delete",
                    "(Landroid/net/Uri;Ljava/lang/String;[Ljava/lang/String;)I"), s.uri, nullptr, nullptr);
            }
            j.fine();
        }
        j.env->DeleteGlobalRef(s.uri);
    }
    s.uri = nullptr;
}

// ---- muxing / encoder output
void writeSample(bool video, uint8_t const* data, BufInfo const& info) {
    ndk.muxWrite(s.mux, static_cast<size_t>(video ? s.vTrack : s.aTrack), data, &info);
}

void tryStartMux() {
    if (s.muxStarted || s.vTrack < 0 || (s.hasAudio && s.aTrack < 0)) return;
    if (ndk.muxStart(s.mux) != 0) return;
    s.muxStarted = true;
    for (auto& p : s.pending) writeSample(p.video, p.data.data(), p.info);
    s.pending.clear();
}

void drain(bool video) {
    auto codec = video ? s.vc : s.ac;
    if (!codec) return;
    for (int n = 0; n < 24; n++) {
        BufInfo info{};
        ssize_t i = ndk.dequeueOut(codec, &info, 0);
        if (i == kTryAgain) break;
        if (i == kFormatChanged) {
            if (!s.muxStarted) {
                auto f = ndk.getOutFormat(codec);
                (video ? s.vTrack : s.aTrack) = ndk.muxAddTrack(s.mux, f);
                ndk.fmtDelete(f);
                tryStartMux();
            }
            continue;
        }
        if (i < 0) continue;
        size_t sz = 0;
        uint8_t* buf = ndk.getOut(codec, static_cast<size_t>(i), &sz);
        if (buf && !(info.flags & kConfigBuf) && info.size > 0) {
            if (s.muxStarted) writeSample(video, buf + info.offset, info);
            else if (s.pending.size() < 900) s.pending.push_back({video, std::vector<uint8_t>(buf + info.offset, buf + info.offset + info.size), info});
        }
        ndk.releaseOut(codec, static_cast<size_t>(i), false);
        if (info.flags & kEos) (video ? s.vEos : s.aEos) = true;
    }
}

void feedAudio() {
    if (!s.hasAudio || s.aEos) return;
    for (;;) {
        uint64_t w = g_wr.load(std::memory_order_acquire), r = g_rd.load(std::memory_order_relaxed);
        uint64_t frames = (w - r) / 2;
        if (!frames) return;
        ssize_t idx = ndk.dequeueIn(s.ac, 0);
        if (idx < 0) return;
        size_t cap = 0;
        auto in = reinterpret_cast<int16_t*>(ndk.getIn(s.ac, static_cast<size_t>(idx), &cap));
        if (!in) return;
        uint64_t n = std::min<uint64_t>(frames, cap / 4);
        for (uint64_t k = 0; k < n * 2; k++) {
            float v = g_ring[(r + k) & (kRingSize - 1)];
            in[k] = static_cast<int16_t>(std::clamp(v, -1.f, 1.f) * 32767.f);
        }
        uint64_t pts = s.audioFrames * 1000000ULL / static_cast<uint64_t>(s.rate);
        ndk.queueIn(s.ac, static_cast<size_t>(idx), 0, static_cast<size_t>(n * 4), pts, 0);
        s.audioFrames += n;
        g_rd.store(r + n * 2, std::memory_order_release);
    }
}

void pump() {
    feedAudio();
    drain(true);
    drain(false);
}

// ---- graphics helpers
GLuint makeProgram() {
    auto compile = [](GLenum type, char const* src) {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        return sh;
    };
    GLuint vs = compile(GL_VERTEX_SHADER,
        "attribute vec2 p; varying vec2 uv; void main(){ uv = p * 0.5 + 0.5; gl_Position = vec4(p, 0.0, 1.0); }");
    GLuint fs = compile(GL_FRAGMENT_SHADER,
        "precision mediump float; varying vec2 uv; uniform sampler2D t; void main(){ gl_FragColor = texture2D(t, uv); }");
    GLuint pr = glCreateProgram();
    glAttachShader(pr, vs);
    glAttachShader(pr, fs);
    glBindAttribLocation(pr, 0, "p");
    glLinkProgram(pr);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(pr, GL_LINK_STATUS, &ok);
    if (!ok) { glDeleteProgram(pr); return 0; }
    return pr;
}

void releaseMedia() {
    if (s.dsp) {
        if (auto eng = FMODAudioEngine::sharedEngine(); eng && eng->m_system) {
            FMOD::ChannelGroup* master = nullptr;
            if (eng->m_system->getMasterChannelGroup(&master) == FMOD_OK && master) master->removeDSP(s.dsp);
        }
        s.dsp->release();
        s.dsp = nullptr;
    }
    if (s.mux) { ndk.muxDelete(s.mux); s.mux = nullptr; }
    for (auto* c : {&s.vc, &s.ac}) {
        if (*c) { ndk.codecStop(*c); ndk.codecDelete(*c); *c = nullptr; }
    }
    if (s.fd >= 0) { close(s.fd); s.fd = -1; }
}

void releaseGraphics(bool glValid) {
    if (glValid && s.recCtx != EGL_NO_CONTEXT && s.encSurf != EGL_NO_SURFACE && s.prog) {
        eglMakeCurrent(s.dpy, s.encSurf, s.encSurf, s.recCtx);
        glDeleteProgram(s.prog);
        eglMakeCurrent(s.dpy, s.gdDraw, s.gdRead, s.gdCtx);
    }
    if (glValid && s.tex) glDeleteTextures(1, &s.tex);
    if (glValid && s.encSurf != EGL_NO_SURFACE) eglDestroySurface(s.dpy, s.encSurf);
    if (glValid && s.recCtx != EGL_NO_CONTEXT) eglDestroyContext(s.dpy, s.recCtx);
    if (s.win) ndk.windowRelease(s.win);
    s.win = nullptr; s.encSurf = EGL_NO_SURFACE; s.recCtx = EGL_NO_CONTEXT; s.tex = 0; s.prog = 0;
}

void reset() { std::string keep = s.savedTo; s = State(); s.savedTo = keep; }

bool fail(std::string& error, char const* why, bool glValid = true) {
    error = why;
    releaseMedia();
    releaseGraphics(glValid);
    finishGallery(false);
    reset();
    return false;
}

void setFormat(AMediaFormat* f, char const* mime) { ndk.fmtSetString(f, "mime", mime); }

} // namespace

bool active() { return s.active; }

bool start(Config const& cfg, std::string& error) {
    if (s.active) return true;
    reset();
    s.savedTo.clear();
    s.cfg = cfg;
    if (!loadNdk()) { error = "device has no media encoder (Android 8+ needed)"; return false; }

    // current game graphics state
    s.dpy = eglGetCurrentDisplay();
    s.gdCtx = eglGetCurrentContext();
    s.gdDraw = eglGetCurrentSurface(EGL_DRAW);
    s.gdRead = eglGetCurrentSurface(EGL_READ);
    EGLint w = 0, h = 0;
    if (s.dpy == EGL_NO_DISPLAY || s.gdCtx == EGL_NO_CONTEXT || s.gdDraw == EGL_NO_SURFACE
        || !eglQuerySurface(s.dpy, s.gdDraw, EGL_WIDTH, &w) || !eglQuerySurface(s.dpy, s.gdDraw, EGL_HEIGHT, &h) || w < 64 || h < 64)
        return fail(error, "graphics context unavailable", false);
    s.srcW = w; s.srcH = h;
    s.outH = std::max(144, std::min(cfg.height, h) / 16 * 16);
    s.outW = std::max(144, static_cast<int>(static_cast<int64_t>(w) * s.outH / h) / 16 * 16);
    s.setPts = reinterpret_cast<PFNEGLPRESENTATIONTIMEANDROIDPROC>(eglGetProcAddress("eglPresentationTimeANDROID"));
    if (!s.setPts) return fail(error, "EGL timestamps unsupported", false);

    // output file
    std::string name = "ScroBot-" + stamp() + ".mp4";
    s.fd = openGallery(name);
    if (s.fd < 0) s.fd = openFile(name);
    if (s.fd < 0) return fail(error, "cannot create the video file", false);

    // video encoder with a GPU input surface
    s.vc = ndk.createEncoder("video/avc");
    if (!s.vc) return fail(error, "no H.264 encoder", false);
    {
        auto f = ndk.fmtNew();
        setFormat(f, "video/avc");
        ndk.fmtSetInt32(f, "width", s.outW);
        ndk.fmtSetInt32(f, "height", s.outH);
        ndk.fmtSetInt32(f, "color-format", kColorSurface);
        ndk.fmtSetInt32(f, "bitrate", std::min<int>(20000000, s.outW * s.outH * cfg.fps / 4));
        ndk.fmtSetInt32(f, "frame-rate", cfg.fps);
        ndk.fmtSetInt32(f, "i-frame-interval", 1);
        int st = ndk.configure(s.vc, f, nullptr, nullptr, 1);
        ndk.fmtDelete(f);
        if (st != 0 || ndk.createSurface(s.vc, &s.win) != 0 || !s.win || ndk.codecStart(s.vc) != 0)
            return fail(error, "video encoder refused the settings", false);
    }

    // sound (optional: the video is still saved without it)
    if (auto eng = FMODAudioEngine::sharedEngine(); eng && eng->m_system) {
        FMOD_SPEAKERMODE mode; int raw = 0;
        FMOD::ChannelGroup* master = nullptr;
        if (eng->m_system->getSoftwareFormat(&s.rate, &mode, &raw) == FMOD_OK
            && eng->m_system->getMasterChannelGroup(&master) == FMOD_OK && master) {
            s.ac = ndk.createEncoder("audio/mp4a-latm");
            if (s.ac) {
                auto f = ndk.fmtNew();
                setFormat(f, "audio/mp4a-latm");
                ndk.fmtSetInt32(f, "sample-rate", s.rate);
                ndk.fmtSetInt32(f, "channel-count", 2);
                ndk.fmtSetInt32(f, "bitrate", 128000);
                ndk.fmtSetInt32(f, "aac-profile", 2);
                ndk.fmtSetInt32(f, "max-input-size", 65536);
                int st = ndk.configure(s.ac, f, nullptr, nullptr, 1);
                ndk.fmtDelete(f);
                if (st == 0 && ndk.codecStart(s.ac) == 0) {
                    g_ring.assign(kRingSize, 0.f);
                    g_wr = 0; g_rd = 0;
                    FMOD_DSP_DESCRIPTION d;
                    std::memset(&d, 0, sizeof d);
                    d.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
                    std::strncpy(d.name, "ScroRec", sizeof d.name - 1);
                    d.version = 1;
                    d.numinputbuffers = 1;
                    d.numoutputbuffers = 1;
                    d.read = dspRead;
                    if (eng->m_system->createDSP(&d, &s.dsp) == FMOD_OK && master->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, s.dsp) == FMOD_OK)
                        s.hasAudio = true;
                }
                if (!s.hasAudio) {
                    if (s.dsp) { s.dsp->release(); s.dsp = nullptr; }
                    ndk.codecStop(s.ac); ndk.codecDelete(s.ac); s.ac = nullptr;
                }
            }
        }
    }

    s.mux = ndk.muxNew(s.fd, 0);                    // 0 = mp4
    if (!s.mux) return fail(error, "cannot create the mp4 muxer", false);

    // copy target in the game context + our own context sharing it, drawing into the encoder surface
    glGenTextures(1, &s.tex);
    glBindTexture(GL_TEXTURE_2D, s.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s.srcW, s.srcH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);                 // GD re-binds what it needs

    EGLint cfgAttr[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, 0x3142 /* EGL_RECORDABLE_ANDROID */, 1, EGL_NONE};
    EGLConfig eglCfg = nullptr;
    EGLint count = 0;
    if (!eglChooseConfig(s.dpy, cfgAttr, &eglCfg, 1, &count) || count < 1) return fail(error, "no recordable EGL config");
    EGLint ver = 2;
    eglQueryContext(s.dpy, s.gdCtx, EGL_CONTEXT_CLIENT_VERSION, &ver);
    EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, ver, EGL_NONE};
    s.recCtx = eglCreateContext(s.dpy, eglCfg, s.gdCtx, ctxAttr);
    s.encSurf = eglCreateWindowSurface(s.dpy, eglCfg, reinterpret_cast<EGLNativeWindowType>(s.win), nullptr);
    if (s.recCtx == EGL_NO_CONTEXT || s.encSurf == EGL_NO_SURFACE) return fail(error, "cannot create the encoder surface");
    if (!eglMakeCurrent(s.dpy, s.encSurf, s.encSurf, s.recCtx)) return fail(error, "cannot use the encoder surface");
    s.prog = makeProgram();
    eglMakeCurrent(s.dpy, s.gdDraw, s.gdRead, s.gdCtx);
    if (!s.prog) return fail(error, "shader failed");

    s.intervalNs = 1000000000LL / cfg.fps;
    s.startNs = nowNs();
    s.nextNs = s.startNs;
    s.active = true;
    return true;
}

static void abortRec() {
    // game graphics changed under us (app paused / context lost): stop without touching GL
    s.lost = true;
    s.active = false;
    releaseMedia();
    releaseGraphics(false);
    finishGallery(false);
    reset();
}

void frame() {
    if (!s.active) return;
    if (eglGetCurrentContext() != s.gdCtx || eglGetCurrentDisplay() != s.dpy) { abortRec(); return; }
    int64_t now = nowNs();
    if (now < s.nextNs) { pump(); return; }
    s.nextNs += s.intervalNs;
    if (s.nextNs < now) s.nextNs = now + s.intervalNs;

    // 1. game context: copy the finished frame into the shared texture (all on the GPU)
    GLint prevFbo = 0, prevTex = 0, prevUnit = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevUnit);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, s.tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, s.srcW, s.srcH);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    glActiveTexture(static_cast<GLenum>(prevUnit));
    glFlush();

    // 2. our context: draw it scaled into the encoder
    if (!eglMakeCurrent(s.dpy, s.encSurf, s.encSurf, s.recCtx)) { abortRec(); return; }
    static GLfloat const quad[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
    glViewport(0, 0, s.outW, s.outH);
    glUseProgram(s.prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s.tex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    s.setPts(s.dpy, s.encSurf, static_cast<EGLnsecsANDROID>(now - s.startNs));
    eglSwapBuffers(s.dpy, s.encSurf);

    // 3. back to the game
    if (!eglMakeCurrent(s.dpy, s.gdDraw, s.gdRead, s.gdCtx)) { s.lost = true; }
    pump();

    // no sound track after 3 s: save the video alone instead of waiting forever
    if (!s.muxStarted && s.hasAudio && s.vTrack >= 0 && now - s.startNs > 3000000000LL) {
        s.hasAudio = false;
        tryStartMux();
    }
}

std::string stop() {
    if (!s.active) return "";
    s.active = false;
    bool glValid = !s.lost;
    // the sound tap goes first so nothing is added while we finish
    if (s.dsp) {
        if (auto eng = FMODAudioEngine::sharedEngine(); eng && eng->m_system) {
            FMOD::ChannelGroup* master = nullptr;
            if (eng->m_system->getMasterChannelGroup(&master) == FMOD_OK && master) master->removeDSP(s.dsp);
        }
        s.dsp->release();
        s.dsp = nullptr;
    }
    feedAudio();
    ndk.signalEos(s.vc);
    if (s.hasAudio && s.ac) {
        for (int i = 0; i < 20; i++) {
            ssize_t idx = ndk.dequeueIn(s.ac, 10000);
            if (idx >= 0) {
                uint64_t pts = s.audioFrames * 1000000ULL / static_cast<uint64_t>(s.rate);
                ndk.queueIn(s.ac, static_cast<size_t>(idx), 0, 0, pts, kEos);
                break;
            }
            drain(false);
        }
    }
    for (int i = 0; i < 150 && !(s.vEos && (s.aEos || !s.hasAudio || !s.ac)); i++) {   // at most about 1.5 s
        drain(true);
        drain(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    bool wrote = s.muxStarted;
    if (!s.muxStarted && s.vTrack >= 0) { s.hasAudio = false; tryStartMux(); wrote = s.muxStarted; }
    if (s.muxStarted) ndk.muxStop(s.mux);
    releaseMedia();
    releaseGraphics(glValid);
    finishGallery(wrote);
    std::string where = wrote ? s.savedTo : "";
    reset();
    return where;
}
}

#else
namespace scro::rec {
bool start(Config const&, std::string& error) { error = "Screen Record is mobile only"; return false; }
void frame() {}
std::string stop() { return ""; }
bool active() { return false; }
}
#endif
