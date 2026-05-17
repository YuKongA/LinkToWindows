package ltw;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

public final class LtwFetcherHandler implements InvocationHandler {

    private static volatile Object sCachedManager;

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        String name = method.getName();
        Class<?> returnType = method.getReturnType();

        if ((name.equals("getService") || name.equals("getOrCreateService"))
                && !returnType.isPrimitive() && returnType != Void.TYPE) {
            return obtainManager();
        }

        if (name.equals("toString") && method.getParameterCount() == 0) {
            return "LtwInjectedCrossDeviceFetcher";
        }
        if (name.equals("hashCode") && method.getParameterCount() == 0) {
            return System.identityHashCode(proxy);
        }
        if (name.equals("equals") && method.getParameterCount() == 1) {
            return proxy == args[0];
        }
        return null;
    }

    private static Object obtainManager() throws Throwable {
        Object cached = sCachedManager;
        if (cached != null) return cached;
        synchronized (LtwFetcherHandler.class) {
            cached = sCachedManager;
            if (cached == null) {
                Class<?> cls = Class.forName("android.app.CrossDeviceManager", true,
                        LtwFetcherHandler.class.getClassLoader());
                cached = cls.getDeclaredConstructor().newInstance();
                sCachedManager = cached;
            }
            return cached;
        }
    }
}
