public class TestCaseFinalField {

    public static int EntryPoint() {
        TestCaseFinalField field = new TestCaseFinalField(5);

        return field.getConstant();
    }

    private final int constant;

    public TestCaseFinalField(int constant) {
        this.constant = constant;
    }

    public int getConstant() {
        return constant;
    }
}
