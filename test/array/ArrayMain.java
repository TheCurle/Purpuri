class ArrayMain {

    public static int EntryPoint() {
        int[] arr = new int[5];
        arr[2] = (int) 'P';
        arr[1] = 581231;
        arr[3] = arr[1] + arr[2];
        return arr[3];
    }
}