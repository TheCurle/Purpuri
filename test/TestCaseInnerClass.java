public class TestCaseInnerClass {

    public static int EntryPoint() {
        return new InnerClassField().get();
    }

    static class InnerClassField {

        private int get() {
            return 50;
        }

    }

}
