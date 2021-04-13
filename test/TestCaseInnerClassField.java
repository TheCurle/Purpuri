public class TestCaseInnerClassField {

    public static int EntryPoint() {
        return new InnerClassField(40).constant;
    }

    static class InnerClassField {

        private final int constant;

        public InnerClassField(int constant) {
            this.constant = constant;
        }

    }

}
