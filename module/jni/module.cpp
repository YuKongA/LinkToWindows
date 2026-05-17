#include <android/log.h>
#include <jni.h>
#include <string.h>
#include <unistd.h>
#include "zygisk.hpp"
#include "fetcher_dex.h"

static constexpr auto TAG = "LtwFlagInjector";

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

class LtwFlagInjector : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *pApi, JNIEnv *pEnv) override {
        api = pApi;
        env = pEnv;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        shouldInject = false;
        if (args == nullptr || args->nice_name == nullptr) return;

        const char *name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (name == nullptr) {
            env->ExceptionClear();
            return;
        }
        if (matchesTarget(name)) {
            strncpy(processName, name, sizeof(processName) - 1);
            processName[sizeof(processName) - 1] = '\0';
            shouldInject = true;
        }
        env->ReleaseStringUTFChars(args->nice_name, name);
    }

    static bool matchesTarget(const char *name) {
        static constexpr const char *kPrefixes[] = {
                "com.microsoft.",                 // appmanager, deviceintegrationservice
                "com.microsoftsdk.",              // crossdeviceservicebroker
        };
        for (const char *p: kPrefixes) {
            if (strncmp(name, p, strlen(p)) == 0) return true;
        }
        return false;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (shouldInject) enableLtwFlag();
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override {
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        strcpy(processName, "system_server");
        shouldInject = true;
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs *args) override {
        if (shouldInject) enableLtwFlag();

    }

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool shouldInject = false;
    char processName[128] = {0};

    bool drainException(const char *where) {
        if (!env->ExceptionCheck()) return false;
        env->ExceptionClear();
        LOGE("[%s] exception at %s", processName, where);
        return true;
    }

    void enableLtwFlag() {
        if (env->PushLocalFrame(16) < 0) {
            env->ExceptionClear();
            LOGE("[%s] PushLocalFrame failed", processName);
            return;
        }

        jclass flagsCls = env->FindClass("android/os/microsoft/flags/Flags");
        if (flagsCls == nullptr) {
            env->ExceptionClear();
            env->PopLocalFrame(nullptr);
            return;
        }

        jclass fakeCls = env->FindClass("android/os/microsoft/flags/FakeFeatureFlagsImpl");
        if (fakeCls == nullptr) {
            env->ExceptionClear();
            LOGE("[%s] FakeFeatureFlagsImpl missing", processName);
            env->PopLocalFrame(nullptr);
            return;
        }

        jmethodID fakeCtor = env->GetMethodID(fakeCls, "<init>", "()V");
        if (fakeCtor == nullptr) {
            drainException("GetMethodID(<init>)");
            env->PopLocalFrame(nullptr);
            return;
        }

        jobject fake = env->NewObject(fakeCls, fakeCtor);
        if (fake == nullptr || drainException("NewObject")) {
            env->PopLocalFrame(nullptr);
            return;
        }

        jmethodID setFlagMid = env->GetMethodID(fakeCls, "setFlag", "(Ljava/lang/String;Z)V");
        if (setFlagMid == nullptr) {
            drainException("GetMethodID(setFlag)");
            env->PopLocalFrame(nullptr);
            return;
        }

        jstring flagName = env->NewStringUTF("android.os.microsoft.flags.ltw_enabled");
        if (flagName == nullptr) {
            drainException("NewStringUTF");
            env->PopLocalFrame(nullptr);
            return;
        }

        env->CallVoidMethod(fake, setFlagMid, flagName, JNI_TRUE);
        if (drainException("setFlag")) {
            env->PopLocalFrame(nullptr);
            return;
        }

        jfieldID field = env->GetStaticFieldID(flagsCls, "FEATURE_FLAGS",
                                               "Landroid/os/microsoft/flags/FeatureFlags;");
        if (field == nullptr) {
            drainException("GetStaticFieldID(FEATURE_FLAGS)");
            env->PopLocalFrame(nullptr);
            return;
        }
        env->SetStaticObjectField(flagsCls, field, fake);
        if (drainException("SetStaticObjectField")) {
            env->PopLocalFrame(nullptr);
            return;
        }

        ensureCrossDeviceFetcher();

        env->PopLocalFrame(nullptr);
    }

    void ensureCrossDeviceFetcher() {
        if (env->PushLocalFrame(64) < 0) {
            env->ExceptionClear();
            LOGE("[%s] ensureCrossDeviceFetcher PushLocalFrame failed", processName);
            return;
        }
        doCrossDeviceFetcherWork();
        env->PopLocalFrame(nullptr);
    }

    void doCrossDeviceFetcherWork() {
        jclass contextCls = env->FindClass("android/content/Context");
        if (contextCls == nullptr) {
            env->ExceptionClear();
            return;
        }
        jfieldID nameField = env->GetStaticFieldID(
                contextCls, "CROSS_DEVICE_SERVICE", "Ljava/lang/String;");
        if (nameField == nullptr) {
            env->ExceptionClear();
            LOGW("[%s] Context.CROSS_DEVICE_SERVICE field missing", processName);
            return;
        }
        jstring serviceName = (jstring) env->GetStaticObjectField(contextCls, nameField);
        if (serviceName == nullptr) return;

        jclass ssrCls = env->FindClass("android/app/SystemServiceRegistry");
        if (ssrCls == nullptr) {
            env->ExceptionClear();
            LOGW("[%s] SystemServiceRegistry not loaded yet", processName);
            return;
        }
        jfieldID fetchersFid = env->GetStaticFieldID(ssrCls, "SYSTEM_SERVICE_FETCHERS",
                                                     "Ljava/util/Map;");
        jfieldID namesFid = env->GetStaticFieldID(ssrCls, "SYSTEM_SERVICE_NAMES",
                                                  "Ljava/util/Map;");
        jfieldID classNamesFid = env->GetStaticFieldID(ssrCls, "SYSTEM_SERVICE_CLASS_NAMES",
                                                       "Ljava/util/Map;");
        if (fetchersFid == nullptr || namesFid == nullptr || classNamesFid == nullptr) {
            env->ExceptionClear();
            LOGW("[%s] SystemServiceRegistry maps hidden", processName);
            return;
        }
        jobject fetchersMap = env->GetStaticObjectField(ssrCls, fetchersFid);
        jobject namesMap = env->GetStaticObjectField(ssrCls, namesFid);
        jobject classNamesMap = env->GetStaticObjectField(ssrCls, classNamesFid);
        if (fetchersMap == nullptr || namesMap == nullptr || classNamesMap == nullptr) return;

        jclass mapCls = env->FindClass("java/util/Map");
        jmethodID mapGet = env->GetMethodID(mapCls, "get",
                                            "(Ljava/lang/Object;)Ljava/lang/Object;");
        jmethodID mapPut = env->GetMethodID(mapCls, "put",
                                            "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        jobject existing = env->CallObjectMethod(fetchersMap, mapGet, serviceName);
        if (drainException("fetchers.get")) return;
        if (existing != nullptr) return;

        jclass managerCls = env->FindClass("android/app/CrossDeviceManager");
        if (managerCls == nullptr) {
            env->ExceptionClear();
            LOGW("[%s] CrossDeviceManager not on classpath", processName);
            return;
        }

        jclass classCls = env->FindClass("java/lang/Class");
        jmethodID classGetCL = env->GetMethodID(classCls, "getClassLoader",
                                                "()Ljava/lang/ClassLoader;");
        jobject bootCL = env->CallObjectMethod(managerCls, classGetCL);
        if (drainException("Class.getClassLoader")) return;

        jbyteArray dexBytes = env->NewByteArray((jsize) kFetcherDexLen);
        if (dexBytes == nullptr) {
            env->ExceptionClear();
            return;
        }
        env->SetByteArrayRegion(dexBytes, 0, (jsize) kFetcherDexLen,
                                reinterpret_cast<const jbyte *>(kFetcherDex));
        jclass byteBufferCls = env->FindClass("java/nio/ByteBuffer");
        jmethodID bbWrap = env->GetStaticMethodID(byteBufferCls, "wrap",
                                                  "([B)Ljava/nio/ByteBuffer;");
        jobject dexBuffer = env->CallStaticObjectMethod(byteBufferCls, bbWrap, dexBytes);
        if (drainException("ByteBuffer.wrap")) return;

        jclass imdclCls = env->FindClass("dalvik/system/InMemoryDexClassLoader");
        if (imdclCls == nullptr) {
            env->ExceptionClear();
            LOGE("[%s] InMemoryDexClassLoader missing", processName);
            return;
        }
        jmethodID imdclCtor = env->GetMethodID(
                imdclCls, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
        jobject helperLoader = env->NewObject(imdclCls, imdclCtor, dexBuffer, bootCL);
        if (drainException("InMemoryDexClassLoader.<init>") || helperLoader == nullptr) return;

        jclass clCls = env->FindClass("java/lang/ClassLoader");
        jmethodID clLoadCls = env->GetMethodID(clCls, "loadClass",
                                               "(Ljava/lang/String;)Ljava/lang/Class;");
        jstring handlerName = env->NewStringUTF("ltw.LtwFetcherHandler");
        jclass handlerCls = (jclass) env->CallObjectMethod(helperLoader, clLoadCls, handlerName);
        if (drainException("loadClass(ltw.LtwFetcherHandler)") || handlerCls == nullptr) return;

        jmethodID handlerCtor = env->GetMethodID(handlerCls, "<init>", "()V");
        jobject handler = env->NewObject(handlerCls, handlerCtor);
        if (drainException("LtwFetcherHandler.<init>") || handler == nullptr) return;

        jclass fetcherIface = env->FindClass("android/app/SystemServiceRegistry$ServiceFetcher");
        if (fetcherIface == nullptr) {
            env->ExceptionClear();
            LOGE("[%s] ServiceFetcher interface missing", processName);
            return;
        }

        jobject proxyCL = bootCL;
        if (proxyCL == nullptr) {
            jmethodID getSysCL = env->GetStaticMethodID(
                    clCls, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
            proxyCL = env->CallStaticObjectMethod(clCls, getSysCL);
            if (drainException("getSystemClassLoader")) return;
        }
        jobjectArray ifaceArr = env->NewObjectArray(1, classCls, fetcherIface);
        jclass proxyCls = env->FindClass("java/lang/reflect/Proxy");
        jmethodID newProxy = env->GetStaticMethodID(proxyCls, "newProxyInstance",
                                                    "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");
        jobject proxy = env->CallStaticObjectMethod(proxyCls, newProxy, proxyCL, ifaceArr, handler);
        if (drainException("Proxy.newProxyInstance") || proxy == nullptr) return;

        env->CallObjectMethod(fetchersMap, mapPut, serviceName, proxy);
        if (drainException("fetchers.put")) return;
        env->CallObjectMethod(namesMap, mapPut, managerCls, serviceName);
        if (drainException("names.put")) return;
        jstring simpleName = env->NewStringUTF("CrossDeviceManager");
        env->CallObjectMethod(classNamesMap, mapPut, serviceName, simpleName);
        if (drainException("classNames.put")) return;

        jobject verify = env->CallObjectMethod(fetchersMap, mapGet, serviceName);
        if (drainException("verify get") || verify == nullptr) {
            LOGE("[%s] cross_device fetcher verify failed", processName);
            return;
        }
        LOGI("[%s] ready", processName);
    }
};

REGISTER_ZYGISK_MODULE(LtwFlagInjector)
