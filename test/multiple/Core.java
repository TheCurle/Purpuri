public class Core {
    public static int EntryPoint() {
        int x = 31;

        Other other = new Other();
        x = other.process(x);

        return x;
    }
}