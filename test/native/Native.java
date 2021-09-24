public class Native {
    static native int doStuff();

    public static int EntryPoint() {
        return doStuff();
    }
}