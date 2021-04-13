public class TestCaseInterface implements IOperation {

    public static int EntryPoint() {
        IOperation operation1 = new TestCaseInterface();
        IOperation operation2 = new TestCaseInterface();

        return operation1.op(operation2.op(5, 10), 20);
    }

    @Override
    public int op(int a, int b) {
        return (a + b) * (b - a);
    }
}
