public class ArrayMain {
    char[] chars = new char[3];

    public static int EntryPoint() {
        int[] arr = new int[5];
        arr[2] = (int) 'P';
        arr[1] = 581231;
        arr[3] = arr[1] + arr[2];

        ArrayMain array = new ArrayMain();

        array.chars[0] = 5;
        array.chars[1] = 4;
        array.chars[2] = 3;
        return arr[3] + array.chars[1];
    }
}