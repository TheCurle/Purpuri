public class TestCaseFinalFieldInterface implements IOperation {

    public static int EntryPoint() {
        IOperation operation1 = new TestCaseFinalFieldInterface(5);
        IOperation operation2 = new TestCaseFinalFieldInterface(2);

        return operation1.op(operation2.op(5, 10), 20);
    }

    private final int constant;

    public TestCaseFinalFieldInterface(int constant) {
        this.constant = constant;
    }

    @Override
    public int op(int a, int b) {
        return a + b + constant;
    }
}
