public class For {
    public static int EntryPoint() {
        int x = 5;
        for(int i = 0; i < 7; i += 2) {
            x = x + i;
        }

        return x;
    }
}