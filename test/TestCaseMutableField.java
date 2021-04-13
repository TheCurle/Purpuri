public class TestCaseMutableField {

    public static int EntryPoint() {
        TestCaseMutableField field = new TestCaseMutableField(5);
        field.setMutable(10);
        return field.getMutable();
    }

    private int mutable;

    public TestCaseMutableField(int mutable) {
        this.mutable = mutable;
    }

    public int getMutable() {
        return mutable;
    }

    public void setMutable(int mutable) {
        this.mutable = mutable;
    }

}
