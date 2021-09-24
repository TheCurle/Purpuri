public class Other {
    private int seed;

    public Other() {
        this.seed = 1519;
    }

    public int process(int number) {
        return number + seed;
    }
}