public class MemTest {
    // until gc is implemented, this will cause a memory leak
    // that is its purpose
    public static int EntryPoint() {
        while (true) {
            int i = 0;
            i += 5;
            i *= 2;
            int i2 = -10;
            i2 *= 0.5;
            i2 += 60;
            i += i2 * i - (i2 / i);
            Object o = new Object();
        }
        return -1;
    }
}